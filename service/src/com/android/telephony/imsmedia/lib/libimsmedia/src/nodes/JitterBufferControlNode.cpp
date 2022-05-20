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

#include <JitterBufferControlNode.h>
#include <AudioJitterBuffer.h>
#include <ImsMediaTrace.h>

JitterBufferControlNode::JitterBufferControlNode(ImsMediaType type) :
        mMediaType(type)
{
    if (mMediaType == IMS_MEDIA_AUDIO)
    {
        mJitterBuffer = new AudioJitterBuffer();
    }
}

JitterBufferControlNode::~JitterBufferControlNode()
{
    if (mJitterBuffer != NULL)
    {
        delete mJitterBuffer;
    }
}

void JitterBufferControlNode::SetJitterBufferSize(uint32_t nInit, uint32_t nMin, uint32_t nMax)
{
    IMLOGD3("[SetJitterBufferSize] init[%d], min[%d], max[%d]", nInit, nMin, nMax);
    if (mJitterBuffer)
    {
        mJitterBuffer->SetJitterBufferSize(nInit, nMin, nMax);
    }
}

void JitterBufferControlNode::SetJitterOptions(
        uint32_t nReduceTH, uint32_t nStepSize, double zValue, bool bIgnoreSID, bool bImprovement)
{
    IMLOGD5("[SetJitterOptions] nReduceTH[%d], nStepSize[%d], zValue[%lf], bSID[%d], bImprove[%d]",
            nReduceTH, nStepSize, zValue, bIgnoreSID, bImprovement);
    if (mJitterBuffer)
    {
        mJitterBuffer->SetJitterOptions(nReduceTH, nStepSize, zValue, bIgnoreSID, bImprovement);
    }
}

void JitterBufferControlNode::Reset()
{
    IMLOGD0("[Reset]");
    if (mJitterBuffer)
    {
        mJitterBuffer->Reset();
    }
}

uint32_t JitterBufferControlNode::GetDataCount()
{
    if (mJitterBuffer)
    {
        return mJitterBuffer->GetCount();
    }
    return 0;
}

void JitterBufferControlNode::OnDataFromFrontNode(ImsMediaSubType subtype, uint8_t* pData,
        uint32_t nDataSize, uint32_t nTimestamp, bool bMark, uint32_t nSeqNum,
        ImsMediaSubType nDataType)
{
    if (mJitterBuffer)
    {
        mJitterBuffer->Add(subtype, pData, nDataSize, nTimestamp, bMark, nSeqNum, nDataType);
    }
}

bool JitterBufferControlNode::GetData(ImsMediaSubType* pSubtype, uint8_t** ppData,
        uint32_t* pnDataSize, uint32_t* pnTimestamp, bool* pbMark, uint32_t* pnSeqNum,
        ImsMediaSubType* pnDataType)
{
    (void)pnDataType;
    if (mJitterBuffer)
    {
        return mJitterBuffer->Get(pSubtype, ppData, pnDataSize, pnTimestamp, pbMark, pnSeqNum);
    }
    return false;
}

void JitterBufferControlNode::DeleteData()
{
    if (mJitterBuffer)
    {
        mJitterBuffer->Delete();
    }
}