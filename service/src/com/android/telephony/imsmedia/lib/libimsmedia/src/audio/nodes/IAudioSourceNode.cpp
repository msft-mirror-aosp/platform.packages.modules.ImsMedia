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

#include <IAudioSourceNode.h>
#include <ImsMediaAudioSource.h>
#include <ImsMediaTrace.h>
#include <string.h>
#include <AudioConfig.h>

IAudioSourceNode::IAudioSourceNode()
{
    std::unique_ptr<ImsMediaAudioSource> recoder(new ImsMediaAudioSource());
    mAudioSource = std::move(recoder);
    mCodecType = 0;
    mMode = 0;
    m_bFirstFrame = false;
}

IAudioSourceNode::~IAudioSourceNode() {}

BaseNode* IAudioSourceNode::GetInstance()
{
    return new IAudioSourceNode();
}

void IAudioSourceNode::ReleaseInstance(BaseNode* pNode)
{
    delete (IAudioSourceNode*)pNode;
}

BaseNodeID IAudioSourceNode::GetNodeID()
{
    return BaseNodeID::NODEID_AUDIOSOURCE;
}

ImsMediaResult IAudioSourceNode::Start()
{
    IMLOGD2("[Start] codec[%d], mode[%d]", mCodecType, mMode);
    if (mAudioSource)
    {
        mAudioSource->SetUplinkCallback(this, IAudioSourceNode::CB_AudioUplink);
        mAudioSource->SetCodec(mCodecType);
        mAudioSource->SetCodecMode(mMode);
        mAudioSource->SetPtime(mPtime);
        if (mAudioSource->Start() == true)
        {
            m_bFirstFrame = false;
        }
    }

    mNodeState = NODESTATE_RUNNING;
    return RESULT_SUCCESS;
}

void IAudioSourceNode::Stop()
{
    IMLOGD0("[Stop]");
    if (mAudioSource)
    {
        mAudioSource->Stop();
    }
    mNodeState = NODESTATE_STOPPED;
}

bool IAudioSourceNode::IsRunTime()
{
    return true;
}

bool IAudioSourceNode::IsSourceNode()
{
    return true;
}

void IAudioSourceNode::SetConfig(void* config)
{
    if (config == NULL)
        return;
    AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);
    SetCodec(pConfig->getCodecType());
    if (mCodecType == AUDIO_AMR || mCodecType == AUDIO_AMR_WB)
    {
        SetCodecMode(pConfig->getAmrParams().getAmrMode());
    }
    mPtime = pConfig->getPtimeMillis();
}

bool IAudioSourceNode::IsSameConfig(void* config)
{
    if (config == NULL)
        return true;
    AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);

    if (mCodecType == pConfig->getCodecType())
    {
        if (mCodecType == AUDIO_AMR || mCodecType == AUDIO_AMR_WB)
        {
            if (mMode == pConfig->getAmrParams().getAmrMode())
            {
                return true;
            }
        }
        else if (mCodecType == AUDIO_EVS)
        {
            if (mMode == pConfig->getEvsParams().getEvsMode())
            {
                return true;
            }
        }
    }

    return false;
}

void IAudioSourceNode::SetCodec(int32_t type)
{
    switch (type)
    {
        case AudioConfig::CODEC_AMR:
            mCodecType = AUDIO_AMR;
            break;
        case AudioConfig::CODEC_AMR_WB:
            mCodecType = AUDIO_AMR_WB;
            break;
        case AudioConfig::CODEC_EVS:
            mCodecType = AUDIO_EVS;
            break;
        case AudioConfig::CODEC_PCMA:
            mCodecType = AUDIO_G711_PCMA;
            break;
        case AudioConfig::CODEC_PCMU:
            mCodecType = AUDIO_G711_PCMU;
            break;
        default:
            break;
    }
}

void IAudioSourceNode::SetCodecMode(uint32_t mode)
{
    mMode = mode;
}

void IAudioSourceNode::CB_AudioUplink(
        void* pClient, uint8_t* pBitstream, uint32_t pnSize, int64_t pstUsec, uint32_t flag)
{
    (void)flag;
    IAudioSourceNode* client = reinterpret_cast<IAudioSourceNode*>(pClient);
    if (client != NULL)
    {
        IMLOGD_PACKET2(IM_PACKET_LOG_AUDIO, "[CB_AudioUplink] size[%zu], pts=%ld", pnSize, pstUsec);
        client->SendDataToRearNode(MEDIASUBTYPE_UNDEFINED, pBitstream, pnSize, pstUsec,
                !client->m_bFirstFrame, MEDIASUBTYPE_UNDEFINED);
        if (!client->m_bFirstFrame)
        {
            client->m_bFirstFrame = true;
        }
    }
}