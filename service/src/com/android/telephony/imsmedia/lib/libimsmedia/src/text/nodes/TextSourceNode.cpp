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

#include <TextSourceNode.h>
#include <TextConfig.h>
#include <ImsMediaTimer.h>
#include <ImsMediaTrace.h>

TextSourceNode::TextSourceNode(BaseSessionCallback* callback) :
        BaseNode(callback)
{
    mCodecType = TextConfig::TEXT_CODEC_NONE;
    mRedundantLevel = 0;
    mRedundantCount = 0;
    mTimeLastSent = 0;
    mBomEnabled = false;
    mSentBOM = false;
}

TextSourceNode::~TextSourceNode()
{
    while (mListTextSource.size() > 0)
    {
        uint8_t* text = mListTextSource.front();
        delete text;
        mListTextSource.pop_front();
    }

    mListTextSourceSize.clear();
}

kBaseNodeId TextSourceNode::GetNodeId()
{
    return kNodeIdTextSource;
}

ImsMediaResult TextSourceNode::Start()
{
    IMLOGD2("[Start] codec[%d], Redundant Level[%d]", mCodecType, mRedundantLevel);

    if (mCodecType == TextConfig::TEXT_CODEC_NONE)
    {
        return RESULT_INVALID_PARAM;
    }

    mRedundantCount = 0;
    mTimeLastSent = 0;
    mSentBOM = false;
    mNodeState = kNodeStateRunning;
    return RESULT_SUCCESS;
}

void TextSourceNode::Stop()
{
    IMLOGD0("[Stop]");
    ClearDataQueue();
    mNodeState = kNodeStateStopped;
}

bool TextSourceNode::IsRunTime()
{
    return false;
}

bool TextSourceNode::IsSourceNode()
{
    return true;
}

void TextSourceNode::SetConfig(void* config)
{
    if (config == NULL)
    {
        return;
    }

    TextConfig* pConfig = reinterpret_cast<TextConfig*>(config);
    mCodecType = pConfig->getCodecType();
    mRedundantLevel = pConfig->getRedundantLevel();
    mBitrate = pConfig->getBitrate();
    mBomEnabled = pConfig->getKeepRedundantLevel();
}

bool TextSourceNode::IsSameConfig(void* config)
{
    if (config == NULL)
    {
        return true;
    }

    TextConfig* pConfig = reinterpret_cast<TextConfig*>(config);

    return (mCodecType == pConfig->getCodecType() &&
            mRedundantLevel == pConfig->getRedundantLevel() && mBitrate == pConfig->getBitrate() &&
            mBomEnabled == pConfig->getKeepRedundantLevel());
}

void TextSourceNode::ProcessData()
{
    // RFC 4103 recommended T.140 buffering time is 300ms
    if (mTimeLastSent != 0 &&
            ImsMediaTimer::GetTimeInMilliSeconds() - mTimeLastSent < T140_BUFFERING_TIME)
    {
        return;
    }

    std::lock_guard<std::mutex> guard(mMutex);

    if (mBomEnabled == true && mSentBOM == false)
    {
        SendBOM();
        mSentBOM = true;
    }

    uint32_t nLoopCount = 1;

    if (mListTextSource.size() >= T140_MAX_CHUNK)
    {
        nLoopCount = T140_MAX_CHUNK;
    }

    uint32_t nSendingDataSize = 0;
    memset(mTextToSend, 0, sizeof(MAX_RTT_LEN));

    for (uint32_t i = 0; i < nLoopCount; i++)
    {
        // get first node data
        uint8_t* pData = mListTextSource.front();
        uint32_t nDataSize = mListTextSourceSize.front();

        if (pData == NULL || nDataSize == 0)
        {
            continue;
        }

        memcpy(mTextToSend + nSendingDataSize, pData, nDataSize);
        nSendingDataSize += nDataSize;

        free(pData);
        mListTextSourceSize.pop_front();
        mListTextSource.pop_front();
    }

    if (nSendingDataSize > 0)
    {
        // send it one char
        IMLOGD_PACKET1(IM_PACKET_LOG_TEXT, "[ProcessData] send[%s]", mTextToSend);

        mTimeLastSent = ImsMediaTimer::GetTimeInMilliSeconds();
        SendDataToRearNode(MEDIASUBTYPE_BITSTREAM_T140, mTextToSend, nSendingDataSize,
                mTimeLastSent, false, 0);

        mRedundantCount = mRedundantLevel;

        if (mRedundantCount == 0)
        {
            mRedundantCount = 1;
        }
    }
    else if (mRedundantCount > 0)
    {
        /** RFC 4103. 5.2
         * When valid T.140 data has been sent and no new T.140 data is available for transmission
         * after the selected buffering time, an empty T140block SHOULD be transmitted.  This
         * situation is regarded as the beginning of an idle period.
         */
        IMLOGD1("[ProcessData] send empty, nRedCount[%d]", mRedundantCount);
        // send default if there is no data to send
        SendDataToRearNode(MEDIASUBTYPE_BITSTREAM_T140, NULL, 0,
                ImsMediaTimer::GetTimeInMilliSeconds(), false, 0);
        mRedundantCount--;
    }
}

void TextSourceNode::SendRtt(const android::String8* text)
{
    if (text == NULL || text->length() <= 0)
    {
        return;
    }

    IMLOGD2("[SendRtt] size[%u], listSize[%d]", text->length(), mListTextSource.size());
    std::lock_guard<std::mutex> guard(mMutex);

    uint8_t* pData = (uint8_t*)text->string();
    uint32_t nSize = text->length();
    uint32_t nChunkSize = 0;

    while (nSize > 0)
    {
        // split by unicode unit with UTF-8
        if (pData[0] >= 0xC2 && pData[0] <= 0xDF && nSize >= 2)
        {
            nChunkSize = 2;
        }
        else if (pData[0] >= 0xE0 && pData[0] <= 0xEF && nSize >= 3)
        {
            nChunkSize = 3;
        }
        else if (pData[0] >= 0xF0 && pData[0] <= 0xFF && nSize >= 4)
        {
            nChunkSize = 4;
        }
        else  // 1byte
        {
            nChunkSize = 1;
        }

        uint8_t* buffer = (uint8_t*)malloc(nChunkSize + 1);  // add null

        if (buffer == NULL)
        {
            IMLOGE0("[SendRtt] allocated data is null");
            return;
        }

        memset(buffer, 0, sizeof(nChunkSize + 1));
        memcpy(buffer, pData, nChunkSize);
        mListTextSource.push_back(buffer);
        mListTextSourceSize.push_back(nChunkSize);
        pData += nChunkSize;
        nSize -= nChunkSize;
    }
}

void TextSourceNode::SendBOM()
{
    uint8_t* buffer = (uint8_t*)malloc(4);
    memset(buffer, 0, 4);
    // send BOM - RFC4103 the BOM character shall be transmitted as 0xEFBBBF
    buffer[0] = 0xef;
    buffer[1] = 0xbb;
    buffer[2] = 0xbf;

    IMLOGD0("[ProcessData] send BOM");
    mListTextSource.push_front(buffer);
    mListTextSourceSize.push_front(3);
}