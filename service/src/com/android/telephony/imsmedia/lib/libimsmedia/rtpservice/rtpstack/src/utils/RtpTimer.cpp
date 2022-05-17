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

#include <RtpTimer.h>
#include <rtp_trace.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <chrono>
#include <thread>
#include <utils/Atomic.h>

struct TimerInstance
{
    fn_TimerCb m_pTimerCb;
    uint32_t m_nDuration;
    bool m_bRepeat;
    void* m_pUserData;
    void* m_pThread;
    bool m_bTerminateThread;
    uint32_t m_nStartTimeSec;
    uint32_t m_nStartTimeMSec;
};

typedef struct IMTimerListNode* PIMTimerListNode;
struct IMTimerListNode
{
    TimerInstance* pInstance;
    PIMTimerListNode pNext;
};

static IMTimerListNode* pTimerListHead = NULL;
static IMTimerListNode* pTimerListTail = NULL;
static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t timer_list_mutex = PTHREAD_MUTEX_INITIALIZER;

static void AddTimerToList(TimerInstance* pInstance)
{
    IMTimerListNode* node = (IMTimerListNode*)malloc(sizeof(IMTimerListNode));
    if (node == NULL)
    {
        RTP_TRACE_ERROR("[AddTimerToList] Error can't alloc memory", 0, 0);
        return;
    }

    node->pInstance = pInstance;
    node->pNext = NULL;

    pthread_mutex_lock(&timer_list_mutex);

    if (pTimerListTail == NULL)
    {
        pTimerListHead = node;
        pTimerListTail = node;
    }
    else
    {
        pTimerListTail->pNext = node;
        pTimerListTail = node;
    }

    pthread_mutex_unlock(&timer_list_mutex);
}

static void DeleteTimerFromList(TimerInstance* pInstance)
{
    IMTimerListNode* pre_node;
    IMTimerListNode* node;

    pthread_mutex_lock(&timer_list_mutex);

    if (pTimerListHead == NULL)
    {
        goto DeleteTimerFromList_Exit;
    }

    if (pTimerListHead->pInstance == pInstance)
    {
        node = pTimerListHead;

        pTimerListHead = node->pNext;
        if (pTimerListHead == NULL)
            pTimerListTail = NULL;

        free(node);
        goto DeleteTimerFromList_Exit;
    }

    for (pre_node = pTimerListHead, node = pTimerListHead->pNext; node;
            pre_node = node, node = node->pNext)
    {
        if (node->pInstance == pInstance)
            break;
    }
    if (node == NULL)
    {
        goto DeleteTimerFromList_Exit;
    }

    pre_node->pNext = node->pNext;
    if (pTimerListTail == node)
        pTimerListTail = pre_node;

    free(node);

DeleteTimerFromList_Exit:
    pthread_mutex_unlock(&timer_list_mutex);
}

static bool IsValidTimer(TimerInstance* pInstance)
{
    IMTimerListNode* node;
    pthread_mutex_lock(&timer_list_mutex);

    for (node = pTimerListHead; node; node = node->pNext)
    {
        if (node->pInstance == pInstance)
            break;
    }

    pthread_mutex_unlock(&timer_list_mutex);

    if (node == NULL)
        return false;
    else
        return true;
}

static int32_t RtpTimer_GetMilliSecDiff(uint32_t m_nStartTimeSec, uint32_t m_nStartTimeMSec,
        uint32_t m_nCurrTimeSec, uint32_t m_nCurrTimeMSec)
{
    uint32_t nDiffSec;
    uint32_t nDiffMSec;
    nDiffSec = m_nCurrTimeSec - m_nStartTimeSec;
    m_nCurrTimeMSec += (nDiffSec * 1000);
    nDiffMSec = m_nCurrTimeMSec - m_nStartTimeMSec;
    return nDiffMSec;
}

static void* RtpTimer_run(void* arg)
{
    TimerInstance* pInstance = (TimerInstance*)arg;
    uint32_t nSleepTime;

    if (pInstance == NULL)
        return NULL;
    if (pInstance->m_nDuration < 100)
        nSleepTime = 10;
    else if (pInstance->m_nDuration < 1000)
        nSleepTime = pInstance->m_nDuration / 10;
    else
        nSleepTime = 100;

    for (;;)
    {
        struct timeval tp;

        if (pInstance->m_bTerminateThread)
            break;

        std::this_thread::sleep_for(std::chrono::milliseconds(nSleepTime));

        if (pInstance->m_bTerminateThread)
            break;

        if (gettimeofday(&tp, NULL) != -1)
        {
            uint32_t nCurrTimeSec, nCurrTimeMSec;
            uint32_t nTimeDiff;
            nCurrTimeSec = tp.tv_sec;
            nCurrTimeMSec = tp.tv_usec / 1000;
            nTimeDiff = RtpTimer_GetMilliSecDiff(pInstance->m_nStartTimeSec,
                    pInstance->m_nStartTimeMSec, nCurrTimeSec, nCurrTimeMSec);

            if (nTimeDiff >= pInstance->m_nDuration)
            {
                if (pInstance->m_bRepeat == true)
                {
                    pInstance->m_nStartTimeSec = nCurrTimeSec;
                    pInstance->m_nStartTimeMSec = nCurrTimeMSec;
                }

                pthread_mutex_lock(&timer_mutex);

                if (pInstance->m_bTerminateThread)
                {
                    pthread_mutex_unlock(&timer_mutex);
                    break;
                }

                if (pInstance->m_pTimerCb)
                {
                    pInstance->m_pTimerCb(pInstance, pInstance->m_pUserData);
                }

                pthread_mutex_unlock(&timer_mutex);

                if (pInstance->m_bRepeat == false)
                {
                    break;
                }
            }
        }
    }

    DeleteTimerFromList(pInstance);

    if (pInstance != NULL)
    {
        free(pInstance);
        pInstance = NULL;
    }

    return NULL;
}

hTimerHandler RtpTimer::TimerStart(
        uint32_t nDuration, bool bRepeat, fn_TimerCb pTimerCb, void* pUserData)
{
    pthread_t thr;
    pthread_attr_t attr;
    struct timeval tp;
    TimerInstance* pInstance;

    pInstance = (TimerInstance*)malloc(sizeof(TimerInstance));

    if (pInstance == NULL)
        return NULL;

    pInstance->m_pTimerCb = pTimerCb;
    pInstance->m_nDuration = nDuration;
    pInstance->m_bRepeat = bRepeat;
    pInstance->m_pUserData = pUserData;
    pInstance->m_bTerminateThread = false;

    RTP_TRACE_NORMAL("[TimerStart] Duratation[%u], bRepeat[%d]", pInstance->m_nDuration, bRepeat);

    if (gettimeofday(&tp, NULL) != -1)
    {
        pInstance->m_nStartTimeSec = tp.tv_sec;
        pInstance->m_nStartTimeMSec = tp.tv_usec / 1000;
    }
    else
    {
        free(pInstance);
        return NULL;
    }

    AddTimerToList(pInstance);

    if (pthread_attr_init(&attr) != 0)
    {
        RTP_TRACE_ERROR("[TimerStart] pthread_attr_init() FAILED", 0, 0);
        DeleteTimerFromList(pInstance);
        free(pInstance);
        return NULL;
    }

    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
    {
        pthread_attr_destroy(&attr);
        RTP_TRACE_ERROR("[TimerStart] pthread_attr_setdetachstate() FAILED", 0, 0);
        DeleteTimerFromList(pInstance);
        free(pInstance);
        return NULL;
    }

    if (pthread_create(&thr, &attr, RtpTimer_run, (void*)pInstance) != 0)
    {
        pthread_attr_destroy(&attr);
        RTP_TRACE_ERROR("[TimerStart] pthread_create() FAILED", 0, 0);
        DeleteTimerFromList(pInstance);
        free(pInstance);
        return NULL;
    }

    if (pthread_attr_destroy(&attr) != 0)
    {
        RTP_TRACE_ERROR("[TimerStart] pthread_attr_destroy() FAILED", 0, 0);
        DeleteTimerFromList(pInstance);
        free(pInstance);
        return NULL;
    }

    pInstance->m_pThread = (void*)thr;
    return (hTimerHandler)pInstance;
}

bool RtpTimer::TimerStop(hTimerHandler hTimer, void** ppUserData)
{
    TimerInstance* pInstance = (TimerInstance*)hTimer;

    if (pInstance == NULL)
        return false;
    if (IsValidTimer(pInstance) == false)
        return false;

    pthread_mutex_lock(&timer_mutex);  // just wait until timer callback returns...
    pInstance->m_bTerminateThread = true;
    if (ppUserData)
        *ppUserData = pInstance->m_pUserData;
    pthread_mutex_unlock(&timer_mutex);
    return true;
}
