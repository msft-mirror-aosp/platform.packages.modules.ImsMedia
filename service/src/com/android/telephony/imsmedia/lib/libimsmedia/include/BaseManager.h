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

typedef int (*CBManager)(long nNativeObj, const android::Parcel& pParcel);

class BaseManager {
public:
    BaseManager() {}
    virtual ~BaseManager() {}
    virtual void sendMessage(const int sessionId, const android::Parcel& parcel) = 0;
    virtual void setCallback(CBManager pfnCallback) {
        mfnCallback = pfnCallback;
    }
    virtual CBManager getCallback() {
        return mfnCallback;
    }

protected:
    CBManager mfnCallback;


};

#endif