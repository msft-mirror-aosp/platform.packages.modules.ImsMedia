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

#include <ImsMediaCondition.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>

ImsMediaCondition::ImsMediaCondition() {
    reset();
    mMutex = new pthread_mutex_t;
    mCond = new pthread_cond_t;

    if (mMutex != NULL) {
        pthread_mutex_init((pthread_mutex_t*)mMutex, NULL);
    }

    if (mCond != NULL) {
        pthread_cond_init((pthread_cond_t*)mCond, NULL);
    }
}

ImsMediaCondition::~ImsMediaCondition() {
    if (mCond != NULL) {
        pthread_cond_destroy((pthread_cond_t*)mCond);
    }

    if (mMutex != NULL) {
        pthread_mutex_destroy((pthread_mutex_t*)mMutex);
    }

    if (mCond != NULL) {
        delete (pthread_cond_t*) mCond;
        mCond = NULL;
    }

    if (mMutex != NULL) {
        delete (pthread_mutex_t*) mMutex;
        mMutex = NULL;
    }
}

void ImsMediaCondition::wait() {
    while (pthread_mutex_lock((pthread_mutex_t*)mMutex) == EINTR);
    if (mSignalFlag & (1<<mWaitCount)) { // signal() had been reached before wait()
        mSignalFlag = mSignalFlag ^ (1<<mWaitCount);
    } else {
        mWaitFlag = mWaitFlag | (1<<mWaitCount);
        while (pthread_cond_wait((pthread_cond_t*)mCond, (pthread_mutex_t*)mMutex) == EINTR);
    }

    IncCount(&mWaitCount);
    pthread_mutex_unlock((pthread_mutex_t*)mMutex);
}

bool ImsMediaCondition::wait_timeout(int64_t nRelativeTime) {
    // make abs time
    struct timespec ts;
    struct timeval tv;
    gettimeofday(&tv, (struct timezone *) NULL);
    uint64_t nInitTime = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
    ts.tv_sec = tv.tv_sec + (nRelativeTime / 1000);
    long addedUSec = tv.tv_usec + (nRelativeTime % 1000) * 1000L;

    if (addedUSec >= 1000000) {
        ts.tv_sec ++;
        addedUSec -= 1000000;
    }

    ts.tv_nsec = addedUSec * 1000L;
    // wait
    while (pthread_mutex_lock((pthread_mutex_t*)mMutex) == EINTR);
    if (mSignalFlag & (1 << mWaitCount)) { // signal() had been reached before wait()
        mSignalFlag = mSignalFlag ^ (1 << mWaitCount);
    } else {
        mWaitFlag = mWaitFlag | (1 << mWaitCount);
        while (pthread_cond_timedwait((pthread_cond_t*)mCond,
            (pthread_mutex_t*)mMutex, &ts) == EINTR);
    }

    IncCount(&mWaitCount);
    pthread_mutex_unlock((pthread_mutex_t*)mMutex);
    struct timeval tl;
    gettimeofday(&tl, (struct timezone *) NULL);
    uint64_t nCurrTime = (tl.tv_sec * 1000) + (tl.tv_usec / 1000);

    if (nCurrTime - nInitTime >= nRelativeTime) {
        return true;         //timeout
    } else {
        return false;        //signal
    }
}

void ImsMediaCondition::signal() {
    if (mCond == NULL || mMutex == NULL) {
        return;
    }

    while (pthread_mutex_lock((pthread_mutex_t*)mMutex) == EINTR);

    if (mWaitFlag & (1<<mSignalCount)) {
        mWaitFlag = mWaitFlag ^ (1 << mSignalCount);
    } else {
        mSignalFlag = mSignalFlag | (1 << mSignalCount);
    }

    pthread_cond_signal((pthread_cond_t*)mCond);
    IncCount(&mSignalCount);
    pthread_mutex_unlock((pthread_mutex_t*)mMutex);
}

void ImsMediaCondition::reset() {
    mWaitFlag = 0;
    mSignalFlag = 0;
    mWaitCount = 0;
    mSignalCount = 0;
}

void ImsMediaCondition::IncCount(uint32_t* pnCount) {
    uint32_t count = *pnCount;
    count++;
    if (count == 32) count = 0;
    *pnCount = count;
}
