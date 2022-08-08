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

#include <TextRendererNode.h>
#include <TextConfig.h>
#include <ImsMediaTimer.h>
#include <ImsMediaTrace.h>

TextRendererNode::TextRendererNode(BaseSessionCallback* callback) :
        JitterBufferControlNode(callback, IMS_MEDIA_TEXT)
{
    mCodecType = TextConfig::TEXT_CODEC_NONE;
    mBOMReceived = false;
}

TextRendererNode::~TextRendererNode() {}

kBaseNodeId TextRendererNode::GetNodeId()
{
    return kNodeIdTextRenderer;
}

ImsMediaResult TextRendererNode::Start()
{
    IMLOGD1("[Start] codec[%d], redCount[%d]", mCodecType);
    mBOMReceived = false;

    if (mCodecType == TextConfig::TEXT_CODEC_NONE)
    {
        return RESULT_INVALID_PARAM;
    }

    mNodeState = kNodeStateRunning;
    return RESULT_SUCCESS;
}

void TextRendererNode::Stop()
{
    IMLOGD0("[Stop]");
    mNodeState = kNodeStateStopped;
}

bool TextRendererNode::IsRunTime()
{
    return false;
}

bool TextRendererNode::IsSourceNode()
{
    return false;
}

void TextRendererNode::SetConfig(void* config)
{
    if (config == NULL)
    {
        return;
    }

    TextConfig* pConfig = reinterpret_cast<TextConfig*>(config);
    mCodecType = pConfig->getCodecType();
    mRedundantRevel = pConfig->getRedundantLevel();
}

bool TextRendererNode::IsSameConfig(void* config)
{
    if (config == NULL)
    {
        return true;
    }

    TextConfig* pConfig = reinterpret_cast<TextConfig*>(config);

    return (mCodecType == pConfig->getCodecType() &&
            mRedundantRevel == pConfig->getRedundantLevel());
}

void TextRendererNode::ProcessData()
{
    static const char* CHAR_REPLACEMENT = "\xEf\xbf\xbd";
    uint8_t* pData;
    uint32_t nDataSize;
    uint32_t nTimeStamp;
    uint32_t nSeqNum;
    bool bMark;
    ImsMediaSubType subtype;
    ImsMediaSubType dataType;
    uint32_t nCurTime = 0;

    while (GetData(&subtype, &pData, &nDataSize, &nTimeStamp, &bMark, &nSeqNum, &dataType))
    {
        if (pData == NULL)
            break;

        IMLOGD4("[ProcessData] Size[%u],TS[%u],Mark[%u],Seq[%u]", nDataSize, nTimeStamp, bMark,
                nSeqNum);

        if (mLastPlayedSeq != -1)
        {
            // detect lost packet
            if (nSeqNum - mLastPlayedSeq > 1)
            {
                // to wait 1000 sec - RFC4103 - 5.4 - Compensation for Packets Out of Order
                nCurTime = ImsMediaTimer::GetTimeInMilliSeconds();

                if (mLossWaitTime == 0)
                {
                    mLossWaitTime = nCurTime;
                }

                if (nCurTime - mLossWaitTime <= 1000)
                {
                    IMLOGD1("[ProcessData] Lost wait[%u]", nCurTime - mLossWaitTime);
                    break;
                }

                // lost count should consider redundant level, maximum lost count = lost packet
                // count - (redundancy level - 1)
                uint32_t nLostCount = 0;
                mRedundantRevel == 0 ? nLostCount = nSeqNum - mLastPlayedSeq - 1
                                     : nLostCount = nSeqNum - mLastPlayedSeq - mRedundantRevel;

                IMLOGD1("[ProcessData] nLostCount[%d]", nLostCount);

                // Send a lost T140 as question mark
                for (uint32_t nIndex = 0; nIndex < nLostCount; nIndex++)
                {
                    // send replacement character in case of lost packet detected
                    android::String8* text = new android::String8(CHAR_REPLACEMENT);
                    mCallback->SendEvent(
                            kImsMediaEventNotifyRttReceived, reinterpret_cast<uint64_t>(text), 0);
                    IMLOGD_PACKET2(IM_PACKET_LOG_TEXT, "[ProcessData] LostSeq[%u], text[%s]",
                            nIndex + mLastPlayedSeq + 1, text->string());
                }
            }

            mLossWaitTime = 0;  // reset loss wait
        }

        // send event to notify to transfer rtt data received
        uint32_t nTransSize = 0;
        uint32_t nOffset = nDataSize;

        while (nOffset > 0 && pData != NULL &&
                (nSeqNum > (uint32_t)mLastPlayedSeq || mLastPlayedSeq == -1))
        {
            // remain last null data
            nOffset > MAX_RTT_LEN - 1 ? nTransSize = MAX_RTT_LEN - 1 : nTransSize = nOffset;

            if (mBOMReceived == false && pData[0] == 0xef && pData[1] == 0xbb && pData[2] == 0xbf)
            {
                // it is BOM - optional packet
                IMLOGD0("[ProcessData] got BOM");
                mBOMReceived = true;
                pData += 3;
                nTransSize -= 3;
                nOffset -= 3;
            }

            if (nTransSize > 0 && pData != NULL)
            {
                android::String8* text = new android::String8(reinterpret_cast<char*>(pData));
                mCallback->SendEvent(
                        kImsMediaEventNotifyRttReceived, reinterpret_cast<uint64_t>(text), 0);
                IMLOGD_PACKET2(IM_PACKET_LOG_TEXT, "[ProcessData] data[%s], text[%s]", pData,
                        text->string());
            }

            pData += nTransSize;
            nOffset -= nTransSize;
        }

        mLastPlayedSeq = (uint32_t)nSeqNum;
        DeleteData();
    }
}