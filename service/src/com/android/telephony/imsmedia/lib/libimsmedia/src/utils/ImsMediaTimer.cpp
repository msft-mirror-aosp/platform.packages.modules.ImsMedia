/**
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ImsMediaTimer.h>
#include <ImsMediaTrace.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <chrono>
#include <thread>
#include <utils/Atomic.h>

struct TimerInstance {
    fn_TimerCb m_pTimerCb;
    uint32_t m_nDuration;
    bool m_bRepeat;
    void* m_pUserData;
    void* m_pThread;
    bool m_bTerminateThread;
    uint32_t m_nStartTimeSec;
    uint32_t m_nStartTimeMSec;
};

typedef struct IMTimerListNode *PIMTimerListNode;
struct IMTimerListNode {
    TimerInstance* pInstance;
    PIMTimerListNode pNext;
};

static IMTimerListNode* pTimerListHead = NULL;
static IMTimerListNode* pTimerListTail = NULL;
static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t timer_list_mutex = PTHREAD_MUTEX_INITIALIZER;

static void AddTimerToList(TimerInstance* pInstance) {
    IMTimerListNode* node = (IMTimerListNode*)malloc(sizeof(IMTimerListNode));
    if (node == NULL) {
        IMLOGE0("[AddTimerToList] Error can't alloc memory");
        return;
    }

    node->pInstance = pInstance;
    node->pNext = NULL;

    pthread_mutex_lock(&timer_list_mutex);

    if (pTimerListTail == NULL) {
        pTimerListHead = node;
        pTimerListTail = node;
    } else {
        pTimerListTail->pNext = node;
        pTimerListTail = node;
    }

    pthread_mutex_unlock(&timer_list_mutex);
}

static void DeleteTimerFromList(TimerInstance* pInstance) {
    IMTimerListNode* pre_node;
    IMTimerListNode* node;

    pthread_mutex_lock(&timer_list_mutex);

    if (pTimerListHead == NULL) {
        goto DeleteTimerFromList_Exit;
    }

    if (pTimerListHead->pInstance == pInstance) {
        node = pTimerListHead;

        pTimerListHead = node->pNext;
        if (pTimerListHead == NULL) pTimerListTail = NULL;

        free(node);
        goto DeleteTimerFromList_Exit;
    }

    for (pre_node = pTimerListHead, node = pTimerListHead->pNext;
        node; pre_node = node, node = node->pNext) {
        if (node->pInstance == pInstance) break;
    }
    if (node == NULL) {
        goto DeleteTimerFromList_Exit;
    }

    pre_node->pNext = node->pNext;
    if (pTimerListTail == node) pTimerListTail = pre_node;

    free(node);

DeleteTimerFromList_Exit:
    pthread_mutex_unlock(&timer_list_mutex);
}

static bool IsValidTimer(TimerInstance* pInstance) {
    IMTimerListNode* node;
    pthread_mutex_lock(&timer_list_mutex);

    for (node = pTimerListHead; node; node = node->pNext) {
        if (node->pInstance == pInstance) break;
    }

    pthread_mutex_unlock(&timer_list_mutex);

    if (node == NULL) return false;
    else return true;
}

static int32_t ImsMediaTimer_GetMilliSecDiff(uint32_t m_nStartTimeSec,
    uint32_t m_nStartTimeMSec, uint32_t m_nCurrTimeSec, uint32_t m_nCurrTimeMSec) {
    uint32_t nDiffSec;
    uint32_t nDiffMSec;
    nDiffSec = m_nCurrTimeSec - m_nStartTimeSec;
    m_nCurrTimeMSec += (nDiffSec * 1000);
    nDiffMSec = m_nCurrTimeMSec - m_nStartTimeMSec;
    return nDiffMSec;
}

static void* ImsMediaTimer_run(void* arg) {
    TimerInstance* pInstance = (TimerInstance*)arg;
    uint32_t nSleepTime;

    if (pInstance == NULL) return NULL;
    if (pInstance->m_nDuration < 100) nSleepTime = 10;
    else if (pInstance->m_nDuration < 1000) nSleepTime = pInstance->m_nDuration / 10;
    else nSleepTime = 100;

    for (;;) {
        struct timeval tp;

        if (pInstance->m_bTerminateThread) break;

        std::this_thread::sleep_for(std::chrono::milliseconds(nSleepTime));

        if (pInstance->m_bTerminateThread) break;

        if (gettimeofday(&tp, NULL) != -1) {
            uint32_t nCurrTimeSec, nCurrTimeMSec;
            uint32_t nTimeDiff;
            nCurrTimeSec = tp.tv_sec;
            nCurrTimeMSec = tp.tv_usec / 1000;
            nTimeDiff = ImsMediaTimer_GetMilliSecDiff(pInstance->m_nStartTimeSec,
                pInstance->m_nStartTimeMSec, nCurrTimeSec, nCurrTimeMSec);

            if (nTimeDiff >= pInstance->m_nDuration) {
                if (pInstance->m_bRepeat == true) {
                    pInstance->m_nStartTimeSec = nCurrTimeSec;
                    pInstance->m_nStartTimeMSec = nCurrTimeMSec;
                }

                pthread_mutex_lock(&timer_mutex);

                if (pInstance->m_bTerminateThread) {
                    pthread_mutex_unlock(&timer_mutex);
                    break;
                }

                if (pInstance->m_pTimerCb) {
                    pInstance->m_pTimerCb(pInstance, pInstance->m_pUserData);
                }

                pthread_mutex_unlock(&timer_mutex);

                if (pInstance->m_bRepeat == false) {
                    break;
                }
            }
        }
    }

    DeleteTimerFromList(pInstance);

    if (pInstance != NULL) {
        free(pInstance);
        pInstance = NULL;
    }

    return NULL;
}

hTimerHandler ImsMediaTimer::TimerStart(uint32_t nDuration,
    bool bRepeat, fn_TimerCb pTimerCb, void* pUserData) {
    pthread_t thr;
    pthread_attr_t attr;
    struct timeval tp;
    TimerInstance* pInstance;

    pInstance = (TimerInstance*)malloc(sizeof(TimerInstance));

    if (pInstance == NULL) return NULL;

    pInstance->m_pTimerCb = pTimerCb;
    pInstance->m_nDuration = nDuration;
    pInstance->m_bRepeat = bRepeat;
    pInstance->m_pUserData = pUserData;
    pInstance->m_bTerminateThread = false;

    IMLOGD3("[ImsMediaTimerStart] Duratation[%u], bRepeat[%d], pUserData[%x]",
        pInstance->m_nDuration, bRepeat, pInstance->m_pUserData);

    if (gettimeofday(&tp, NULL) != -1) {
        pInstance->m_nStartTimeSec = tp.tv_sec;
        pInstance->m_nStartTimeMSec = tp.tv_usec / 1000;
    }
    else
    {
        free(pInstance);
        return NULL;
    }

    AddTimerToList(pInstance);

    if (pthread_attr_init(&attr) != 0) {
        IMLOGE0("[ImsMediaTimerStart] pthread_attr_init() FAILED");
        DeleteTimerFromList(pInstance);
        free(pInstance);
        return NULL;
    }

    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
        pthread_attr_destroy(&attr);
        IMLOGE0("[ImsMediaTimerStart] pthread_attr_setdetachstate() FAILED");
        DeleteTimerFromList(pInstance);
        free(pInstance);
        return NULL;
    }

    if (pthread_create(&thr, &attr, ImsMediaTimer_run, (void*)pInstance) != 0) {
        pthread_attr_destroy(&attr);
        IMLOGE0("[ImsMediaTimerStart] pthread_create() FAILED");
        DeleteTimerFromList(pInstance);
        free(pInstance);
        return NULL;
    }

    if (pthread_attr_destroy(&attr) != 0) {
        IMLOGE0("[ImsMediaTimerStart] pthread_attr_destroy() FAILED");
        DeleteTimerFromList(pInstance);
        free(pInstance);
        return NULL;
    }

    pInstance->m_pThread = (void*)thr;
    return (hTimerHandler)pInstance;
}

bool ImsMediaTimer::TimerStop(hTimerHandler hTimer, void** ppUserData) {
    TimerInstance* pInstance = (TimerInstance*)hTimer;

    if (pInstance == NULL) return false;
    if (IsValidTimer(pInstance) == false) return false;

    pthread_mutex_lock(&timer_mutex);    // just wait until timer callback returns...
    pInstance->m_bTerminateThread = true;
    if (ppUserData) *ppUserData = pInstance->m_pUserData;
    pthread_mutex_unlock(&timer_mutex);
    return true;
}

void ImsMediaTimer::GetNtpTime(IMNtpTime *pNtpTime) {
    struct timeval stAndrodTp;

    if (gettimeofday(&stAndrodTp, NULL) != -1) {
         //To convert a UNIX timestamp (seconds since 1970) to NTP time, add 2,208,988,800 seconds
        pNtpTime->ntpHigh32Bits = stAndrodTp.tv_sec + 2208988800UL;
        pNtpTime->ntpLow32Bits = (unsigned int)(stAndrodTp.tv_usec * 4294UL);
    }
}

/*!
 * @brief       GetRtpTsFromNtpTs
 * @details     Transforms the current NTP time to the corresponding RTP TIme Stamp
 *              using the RTP time stamp rate for the session.
 */
uint32_t ImsMediaTimer::GetRtpTsFromNtpTs(IMNtpTime* initNtpTimestamp, uint32_t samplingRate) {
    IMNtpTime currentNtpTs;
    int32_t timeDiffHigh32Bits;
    int32_t timeDiffLow32Bits;
    uint32_t timeDiff;            /*! In Micro seconds: should always be positive */

    GetNtpTime(&currentNtpTs);

    /* SPR #1256 BEGIN */
    timeDiffHigh32Bits = currentNtpTs.ntpHigh32Bits - initNtpTimestamp->ntpHigh32Bits;
    timeDiffLow32Bits = (currentNtpTs.ntpLow32Bits / 4294UL) -
        (initNtpTimestamp->ntpLow32Bits / 4294UL);
    /*! timeDiffHigh32Bits should always be positive */
    timeDiff = (timeDiffHigh32Bits * 1000) + timeDiffLow32Bits / 1000;
    return timeDiff * (samplingRate / 1000);
}

uint32_t ImsMediaTimer::GetTimeInMilliSeconds(void) {
#if 1
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
#else
    struct timeb stTimeBuffer;
    ftime(&stTimeBuffer);
    return stTimeBuffer.time * 1000 + stTimeBuffer.millitm;
#endif
}

uint64_t ImsMediaTimer::GetTimeInMicroSeconds(void) {
#if 1
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (tp.tv_sec * 1000000) + (tp.tv_usec);
#else
    struct timeb stTimeBuffer;
    ftime(&stTimeBuffer);
    return stTimeBuffer.time * 1000 + stTimeBuffer.millitm;
#endif
}

uint32_t ImsMediaTimer::GenerateRandom(uint32_t nRange) {
#if 0
    if (0 == nRange)
        return GetTimeInMilliSeconds();
    else
        return GetTimeInMilliSeconds() % nRange;
#else
    uint32_t rand;
    struct timeval tp;

    gettimeofday(&tp, NULL);
    rand = (tp.tv_sec * 13) + (tp.tv_usec / 1000);

    if (0 == nRange) {
        return rand * 7;
    }

    return (rand * 7) % nRange;
#endif
}

int32_t ImsMediaTimer::Atomic_Inc(int32_t* v) {
    return android_atomic_inc(v);
}
int32_t ImsMediaTimer::Atomic_Dec(int32_t* v) {
    return android_atomic_dec(v);
}

void ImsMediaTimer::Sleep(unsigned int t) {
    std::this_thread::sleep_for(std::chrono::milliseconds(t));
}

void ImsMediaTimer::USleep(unsigned int t) {
    std::this_thread::sleep_for(std::chrono::microseconds(t));
}