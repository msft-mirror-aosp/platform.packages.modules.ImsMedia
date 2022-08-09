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

#ifndef TEXT_SOURCE_NODE_H_INCLUDED
#define TEXT_SOURCE_NODE_H_INCLUDED

#include <BaseNode.h>
#include <mutex>

/**
 * @brief This class describes an interface between depacketization module
 */
class TextSourceNode : public BaseNode
{
public:
    TextSourceNode(BaseSessionCallback* callback = NULL);
    virtual ~TextSourceNode();
    virtual kBaseNodeId GetNodeId();
    virtual ImsMediaResult Start();
    virtual void Stop();
    virtual bool IsRunTime();
    virtual bool IsSourceNode();
    virtual void SetConfig(void* config);
    virtual bool IsSameConfig(void* config);
    virtual void ProcessData();
    void SendRtt(const android::String8* text);

private:
    void SendBOM();

    int32_t mCodecType;
    int32_t mRedundantLevel;
    int32_t mRedundantCount;
    int32_t mTimeLastSent;
    int32_t mBitrate;
    bool mBomEnabled;
    bool mSentBOM;
    std::list<uint32_t> mListTextSourceSize;
    std::list<uint8_t*> mListTextSource;
    uint8_t mTextToSend[MAX_RTT_LEN];
    std::mutex mMutex;
    int8_t mBuffer;
};

#endif
