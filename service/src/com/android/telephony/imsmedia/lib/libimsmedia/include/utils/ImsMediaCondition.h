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

#ifndef IMS_MEDIA_CONDITION_H
#define IMS_MEDIA_CONDITION_H

#include <stdint.h>

/**
 *    @class        ImsMediaCondition
 *    @brief        Event class
 */
class ImsMediaCondition
{
private:
    void* mCond;
    void* mMutex;
    uint32_t mWaitFlag;
    uint32_t mSignalFlag;
    uint32_t mWaitCount;
    uint32_t mSignalCount;

private:
    ImsMediaCondition(const ImsMediaCondition& objRHS);
    ImsMediaCondition& operator=(const ImsMediaCondition& objRHS);

public:
    ImsMediaCondition();
    ~ImsMediaCondition();
    void wait();
    /*
     *@param nRelativeTime : unit msec
     */
    bool wait_timeout(int64_t nRelativeTime);
    void signal();
    void reset();

private:
    void IncCount(uint32_t* pnCount);
};

#endif  // IMS_MEDIA_CONDITION_H
