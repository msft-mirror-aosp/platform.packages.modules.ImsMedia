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

#include <pthread.h>
#include <errno.h>
#include <IImsMediaThread.h>
#include <ImsMediaTrace.h>

/**
 *    IImsMediaThread
 */
IImsMediaThread::IImsMediaThread() {
    mThread = NULL;
    mStopped = false;
}

IImsMediaThread::~IImsMediaThread() {
}

bool IImsMediaThread::StartThread() {
    IMLOGD0("[IImsMediaThread::StartThread]");
    pthread_t thr;
    pthread_attr_t attr;
    mStopped = false;

    if (pthread_attr_init(&attr) < 0) {
        IMLOGD0("[IImsMediaThread::StartThread] pthread_attr_init error");
        mStopped = true;
        return false;
    }

    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) < 0) {
        IMLOGD0("[IImsMediaThread::StartThread] pthread_attr_setdetachstate error");
        pthread_attr_destroy(&attr);
        mStopped = true;
        return false;
    }

    if (pthread_create(&thr, &attr, thread_fn, (void*)this) < 0) {
        IMLOGD0("[IImsMediaThread::StartThread] pthread_create error");
        pthread_attr_destroy(&attr);
        mStopped = true;
        return false;
    }

    pthread_attr_destroy(&attr);
    mThread = (void*)thr;
    return true;
}

void IImsMediaThread::StopThread() {
    mStopped = true;
}

bool IImsMediaThread::IsMyThread() {
    pthread_t tid = reinterpret_cast<pthread_t>(mThread);
    return (tid == pthread_self()) ? true : false;
}

bool IImsMediaThread::IsThreadStopped() {
    return mStopped;
}

void* IImsMediaThread::thread_fn(void* arg) {
    IImsMediaThread* thread = (IImsMediaThread*)arg;
#if 1
    void* exitCode = thread->run_base();
    pthread_exit(NULL);
    return exitCode;
#else
    return thread->run_base();
#endif
}

void* IImsMediaThread::run_base() {
    return run();
}
