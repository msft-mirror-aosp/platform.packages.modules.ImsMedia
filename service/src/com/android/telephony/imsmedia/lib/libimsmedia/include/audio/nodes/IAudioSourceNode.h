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

#ifndef IAUDIO_SOURCE_NODE_H_INCLUDED
#define IAUDIO_SOURCE_NODE_H_INCLUDED

#include <BaseNode.h>
#include <ImsMediaAudioSource.h>

/**
 * @brief This class is interface between audio device and ims media packetization node
 */
class IAudioSourceNode : public BaseNode
{
private:
    IAudioSourceNode();
    ~IAudioSourceNode();

public:
    static BaseNode* GetInstance();
    static void ReleaseInstance(BaseNode* pNode);
    virtual BaseNodeID GetNodeID();
    virtual ImsMediaResult Start();
    virtual void Stop();
    virtual bool IsRunTime();
    virtual bool IsSourceNode();
    virtual void SetConfig(void* config);
    virtual bool IsSameConfig(void* config);
    void SetCodec(int32_t type);
    void SetCodecMode(uint32_t mode);
    static void CB_AudioUplink(
            void* pClient, uint8_t* pBitstream, uint32_t pnSize, int64_t pstUsec, uint32_t flag);

public:
    uint64_t m_uiTimestamp;
    uint64_t m_uiPrevTimestamp;
    bool m_bFirstFrame;
    std::unique_ptr<ImsMediaAudioSource> mAudioSource;
    int32_t mCodecType;
    uint32_t mMode;
    uint32_t mPtime;
};

#endif