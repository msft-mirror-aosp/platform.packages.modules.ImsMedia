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

#include <ImsMediaEventHandler.h>
#include <ImsMediaTrace.h>
#include <string.h>
#include <string>

std::list<ImsMediaEventHandler*> ImsMediaEventHandler::gListEventHandler;
std::mutex ImsMediaEventHandler::mMutex;

ImsMediaEventHandler::ImsMediaEventHandler(const char* strName)
{
    strncpy(mName, strName, MAX_EVENTHANDLER_NAME);
    mbTerminate = false;
    gListEventHandler.push_back(this);
    IMLOGD1("[ImsMediaEventHandler] %s", mName);
    StartThread();
}

ImsMediaEventHandler::~ImsMediaEventHandler()
{
    IMLOGD1("[~ImsMediaEventHandler] %s", mName);
    gListEventHandler.remove(this);
    mListevent.clear();
    mListParamA.clear();
    mListParamB.clear();
    mListParamC.clear();
    mbTerminate = true;
    mCondition.signal();
    mConditionExit.wait();

    if (IsMyThread())
    {
        IMLOGD0("[~ImsMediaEventHandler] On my thread");
    }
}

void ImsMediaEventHandler::SendEvent(const char* strEventHandlerName, uint32_t event,
        uint64_t paramA, uint64_t paramB, uint64_t paramC)
{
    if (strEventHandlerName == NULL)
    {
        IMLOGE0("[SendEvent] strEventHandlerName is NULL");
        return;
    }

    std::lock_guard<std::mutex> guard(mMutex);
    IMLOGD5("[SendEvent] Name[%s], event[%d], paramA[%p], paramB[%p], paramC[%p]",
            strEventHandlerName, event, paramA, paramB, paramC);

    for (auto& i : gListEventHandler)
    {
        if (i != NULL && strcmp(i->getName(), strEventHandlerName) == 0)
        {
            i->AddEvent(event, paramA, paramB, paramC);
        }
    }
}

char* ImsMediaEventHandler::getName()
{
    return mName;
}

void ImsMediaEventHandler::AddEvent(
        uint32_t event, uint64_t paramA, uint64_t paramB, uint64_t paramC)
{
    // lock
    IMLOGD2("[AddEvent] event[%d], size[%d]", event, mListevent.size());
    mMutexEvent.lock();
    mListevent.push_back(event);
    mListParamA.push_back(paramA);
    mListParamB.push_back(paramB);
    mListParamC.push_back(paramC);
    // unlock
    mMutexEvent.unlock();
    IMLOGD0("[AddEvent] signal");
    mCondition.signal();
    IMLOGD0("[AddEvent] exit");
}

void* ImsMediaEventHandler::run()
{
    IMLOGD1("[run] enter, %p", this);
    for (;;)
    {
        IMLOGD0("[run] wait");
        mCondition.wait();
        for (;;)
        {
            // lock
            mMutexEvent.lock();
            if (mbTerminate == true || mListevent.size() == 0)
            {
                mMutexEvent.unlock();
                break;
            }
            mMutexEvent.unlock();
            processEvent(mListevent.front(), mListParamA.front(), mListParamB.front(),
                    mListParamC.front());
            mMutexEvent.lock();
            mListevent.pop_front();
            mListParamA.pop_front();
            mListParamB.pop_front();
            mListParamC.pop_front();
            mMutexEvent.unlock();
            // Only when thread needs to be destroyed inside of processEvent,
            // this method returns "true".
            if (IsThreadStopped())
            {
                mbTerminate = true;
                break;
            }
        }
        if (mbTerminate)
            break;
    }

    bool bSelfDestroy = IsThreadStopped();
    IMLOGD1("[run] exit, %p", this);
    mConditionExit.signal();

    if (bSelfDestroy)
    {
        IMLOGD0("[run] self-destroy");
        delete this;
    }

    return NULL;
}
