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

/** Maximum waiting time when packet loss found */
#define TEXT_LOSS_MAX_WAITING_TIME 1000

TextRendererNode::TextRendererNode(BaseSessionCallback* callback) :
        JitterBufferControlNode(callback, IMS_MEDIA_TEXT)
{
    mCodecType = TextConfig::TEXT_CODEC_NONE;
    mBOMReceived = false;
    mLastPlayedSeq = -1;
    mLossWaitTime = 0;
}

TextRendererNode::~TextRendererNode() {}

kBaseNodeId TextRendererNode::GetNodeId()
{
    return kNodeIdTextRenderer;
}

ImsMediaResult TextRendererNode::Start()
{
    IMLOGD1("[Start] codec[%d]", mCodecType);

    if (mCodecType == TextConfig::TEXT_CODEC_NONE)
    {
        return RESULT_INVALID_PARAM;
    }

    mBOMReceived = false;
    mLastPlayedSeq = -1;
    mLossWaitTime = 0;
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
    uint8_t* data;
    uint32_t size;
    uint32_t timestamp;
    uint32_t seq;
    bool mark;
    ImsMediaSubType subtype;
    ImsMediaSubType dataType;
    uint32_t nCurTime = 0;

    while (GetData(&subtype, &data, &size, &timestamp, &mark, &seq, &dataType))
    {
        IMLOGD4("[ProcessData] Size[%u],TS[%u],Mark[%u],Seq[%u]", size, timestamp, mark, seq);

        // ignore empty t.140
        if (size == 0)
        {
            mLastPlayedSeq = (uint32_t)seq;
            DeleteData();
            break;
        }

        if (data == NULL)
        {
            IMLOGD0("[ProcessData] invalid data");
            break;
        }

        if (mLastPlayedSeq != -1)
        {
            uint32_t lostCount = seq - mLastPlayedSeq - 1;
            // detect lost packet
            if (lostCount > 0)
            {
                /* it is out of idle period, lost packet is empty t.140, ignore this */
                if (lostCount == 1 && mark == true)
                {
                    mLastPlayedSeq++;
                    break;
                }

                // to wait 1000 sec - RFC4103 - 5.4 - Compensation for Packets Out of Order
                nCurTime = ImsMediaTimer::GetTimeInMilliSeconds();

                if (mLossWaitTime == 0)
                {
                    mLossWaitTime = nCurTime;
                }

                if (nCurTime - mLossWaitTime <= TEXT_LOSS_MAX_WAITING_TIME)
                {
                    IMLOGD1("[ProcessData] Lost wait[%u]", nCurTime - mLossWaitTime);
                    break;
                }

                // lost count should consider redundant level, maximum lost count = lost packet
                // count - (redundancy level - 1)
                mRedundantRevel == 0 ? lostCount = seq - mLastPlayedSeq - 1
                                     : lostCount = seq - mLastPlayedSeq - mRedundantRevel;

                IMLOGD1("[ProcessData] lostCount[%d]", lostCount);

                // Send a lost T140 as question mark
                for (uint32_t nIndex = 0; nIndex < lostCount; nIndex++)
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
        uint32_t transSize = 0;
        uint32_t offset = size;

        while (offset > 0 && data != NULL && (seq > mLastPlayedSeq || mLastPlayedSeq == -1))
        {
            // remain last null data
            offset > MAX_RTT_LEN - 1 ? transSize = MAX_RTT_LEN - 1 : transSize = offset;

            if (mBOMReceived == false && offset >= 3 && data[0] == 0xef && data[1] == 0xbb &&
                    data[2] == 0xbf)
            {
                // it is BOM - optional packet
                IMLOGD0("[ProcessData] got BOM");
                mBOMReceived = true;
                data += 3;
                transSize -= 3;
                offset -= 3;
            }

            if (transSize > 0 && data != NULL)
            {
                memset(mBuffer, 0, MAX_RTT_LEN);
                memcpy(mBuffer, data, transSize);
                android::String8* text = new android::String8(mBuffer);
                mCallback->SendEvent(
                        kImsMediaEventNotifyRttReceived, reinterpret_cast<uint64_t>(text), 0);
                IMLOGD_PACKET1(IM_PACKET_LOG_TEXT, "[ProcessData] text[%s]", mBuffer);
            }

            data += transSize;
            offset -= transSize;
        }

        mLastPlayedSeq = (uint32_t)seq;
        DeleteData();
    }
}