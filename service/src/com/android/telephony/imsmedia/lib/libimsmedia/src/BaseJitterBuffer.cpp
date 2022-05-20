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

#include <BaseJitterBuffer.h>

BaseJitterBuffer::BaseJitterBuffer()
{
    mInitJitterBufferSize = 4;
    mMinJitterBufferSize = 4;
    mMaxJitterBufferSize = 9;
}

BaseJitterBuffer::~BaseJitterBuffer() {}

void BaseJitterBuffer::SetSessionCallback(BaseSessionCallback* callback)
{
    mCallback = callback;
}

void BaseJitterBuffer::SetCodecType(uint32_t type)
{
    mCodecType = type;
}

void BaseJitterBuffer::SetJitterBufferSize(uint32_t nInit, uint32_t nMin, uint32_t nMax)
{
    mInitJitterBufferSize = nInit;
    mMinJitterBufferSize = nMin;
    mMaxJitterBufferSize = nMax;
}

void BaseJitterBuffer::SetJitterOptions(
        uint32_t nReduceTH, uint32_t nStepSize, double zValue, bool bIgnoreSID, bool bImprovement)
{
    (void)nReduceTH;
    (void)nStepSize;
    (void)zValue;
    (void)bIgnoreSID;
    (void)bImprovement;
}

uint32_t BaseJitterBuffer::GetCount()
{
    return mDataQueue.GetCount();
}

void BaseJitterBuffer::Reset()
{
    // reset the analyzer
    mDataCount = 0;
    mNewInputData = false;
    mLastPlayedSeqNum = 0;
    mLastPlayedTimestamp = 0;

    while (mDataQueue.GetCount() > 0)
    {
        // CollectJitterStatus(RTP_PACKET_STATUS_OK);
        mDataQueue.Delete();
    }
}

void BaseJitterBuffer::Delete()
{
    DataEntry* pEntry;
    std::lock_guard<std::mutex> guard(mMutex);
    mDataQueue.Get(&pEntry);

    if (pEntry == NULL)
        return;

    mDataQueue.Delete();
}