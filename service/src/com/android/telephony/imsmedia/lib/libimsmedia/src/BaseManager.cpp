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

#include <BaseManager.h>
#include <ImsMediaTrace.h>

std::function<int(long, const android::Parcel&)> BaseManager::mCallback;

BaseManager::BaseManager()
{
    mCallback = NULL;
}

BaseManager::~BaseManager() {}

void BaseManager::setCallback(CBManager pfnCallback)
{
    if (mCallback == NULL)
    {
        IMLOGD0("[setCallback]");
        mCallback = std::bind(pfnCallback, std::placeholders::_1, std::placeholders::_2);
    }
}

int BaseManager::sendResponse(long obj, const android::Parcel& parcel)
{
    IMLOGD0("[sendResponse]");

    if (mCallback != NULL)
    {
        IMLOGD0("[sendResponse] 1");
        return mCallback(obj, parcel);
    }

    return -1;
}