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
    mRedCount = 0;
    mTimeLastSent = 0;
    mBomEnabled = false;
    mSentBOM = false;
}

TextSourceNode::~TextSourceNode()
{
    while (mListTextSource.size() > 0)
    {
        android::String8* text = mListTextSource.front();
        delete text;
        mListTextSource.pop_front();
    }
}

kBaseNodeId TextSourceNode::GetNodeId()
{
    return kNodeIdTextSource;
}

ImsMediaResult TextSourceNode::Start()
{
    IMLOGD2("[Start] codec[%d], redCount[%d]", mCodecType, mRedCount);

    if (mCodecType == TextConfig::TEXT_CODEC_NONE)
    {
        return RESULT_INVALID_PARAM;
    }

    mRedCount = 0;
    mTimeLastSent = 0;
    mSentBOM = false;

    return RESULT_SUCCESS;
}

void TextSourceNode::Stop()
{
    IMLOGD0("[Stop]");
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
    mRedCount = pConfig->getRedundantLevel();
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

    return (mCodecType == pConfig->getCodecType() && mRedCount == pConfig->getRedundantLevel() &&
            mBitrate == pConfig->getBitrate() && mBomEnabled == pConfig->getKeepRedundantLevel());
}

void TextSourceNode::ProcessData()
{
    // RFC 4103 recommended T.140/RED interval is 300ms
    if (mTimeLastSent != 0 &&
            ImsMediaTimer::GetTimeInMilliSeconds() - mTimeLastSent < T140_BUFFERING_TIME)
    {
        return;
    }

    std::lock_guard<std::mutex> guard(mMutex);

    if (mBomEnabled == true && mSentBOM == false)
    {
        uint8_t pBOMData[3];
        // send BOM - RFC4103 the BOM character shall be transmitted as 0xEFBBBF
        pBOMData[0] = 0xef;
        pBOMData[1] = 0xbb;
        pBOMData[2] = 0xbf;

        SendDataToRearNode(MEDIASUBTYPE_BITSTREAM_T140, pBOMData, 3,
                ImsMediaTimer::GetTimeInMilliSeconds(), false, 0);
        mSentBOM = true;
        mTimeLastSent = ImsMediaTimer::GetTimeInMilliSeconds();
        return;
    }

    int32_t redCount = mRedCount;

    for (int32_t i = 0; i < mListTextSource.size(); i++)
    {
        // get first node data
        android::String8* str = mListTextSource.front();

        mTimeLastSent = ImsMediaTimer::GetTimeInMilliSeconds();
        SendDataToRearNode(MEDIASUBTYPE_BITSTREAM_T140, (uint8_t*)str->string(), str->length(),
                mTimeLastSent, false, 0);

        if (mRedCount == 0)
        {
            redCount = 1;
        }

        delete str;
        mListTextSource.pop_front();
    }

    if (mListTextSource.empty())
    {
        if (redCount > 0)
        {
            IMLOGD1("[ProcessData] send empty, nRedCount[%d]", redCount);
            // send default if there is no data to send
            mTimeLastSent = ImsMediaTimer::GetTimeInMilliSeconds();
            SendDataToRearNode(MEDIASUBTYPE_BITSTREAM_T140, NULL, 0, mTimeLastSent, false, 0);
            redCount--;
        }
    }
}

void TextSourceNode::SendRtt(const android::String8* text)
{
    if (text == NULL || text->length() <= 0)
        return;

    IMLOGD2("[SendRtt] size[%u], listSize[%d]", text->length(), mListTextSource.size());
    std::lock_guard<std::mutex> guard(mMutex);

    mListTextSource.push_back(new android::String8(*text));
}