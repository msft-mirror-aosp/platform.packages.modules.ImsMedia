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

#ifndef BASE_MANAGER_H
#define BASE_MANAGER_H

#include <BaseManagerListener.h>
#include <ImsMediaEventHandler.h>
#include <BaseSession.h>
#include <binder/Parcel.h>
#include <functional>

typedef int (*CBManager)(long nativeObj, const android::Parcel& parcel);

class BaseManager
{
public:
    BaseManager() { mCallback = NULL; }
    virtual ~BaseManager() {}

    /**
     * @brief Send message to session to operate
     *
     * @param sessionId identification of session
     * @param parcel parcel of message and parameters
     */
    virtual void sendMessage(const int sessionId, const android::Parcel& parcel) = 0;

    /**
     * @brief Set the Callback object
     *
     * @param pfnCallback
     */
    virtual void setCallback(CBManager pfnCallback)
    {
        mCallback = std::bind(pfnCallback, std::placeholders::_1, std::placeholders::_2);
    }

    /**
     * @brief Send response message to assigend callback method
     *
     * @param obj The object of the manager instance.
     * @param parcel The parcel object contains event message and parameter
     * @return int Returns -1 when it is fail invoke callback function. Returns 1 when it is
     * success.
     */
    virtual int sendResponse(long obj, const android::Parcel& parcel)
    {
        if (mCallback != NULL)
        {
            return mCallback(obj, parcel);
        }

        return -1;
    }

protected:
    virtual int getState(int sessionId) = 0;
    std::function<int(long, const android::Parcel&)> mCallback;
};

#endif