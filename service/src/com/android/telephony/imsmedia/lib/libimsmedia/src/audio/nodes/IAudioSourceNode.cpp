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
#include <ImsMediaAudioFmt.h>
#include <string.h>
#include <AudioConfig.h>
#include <RtpConfig.h>
#include <EvsParams.h>

IAudioSourceNode::IAudioSourceNode()
{
    std::unique_ptr<ImsMediaAudioSource> recoder(new ImsMediaAudioSource());
    mAudioSource = std::move(recoder);
    mCodecType = 0;
    mMode = 0;
    mFirstFrame = false;
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
        mAudioSource->SetSamplingRate(mSamplingRate * 1000);

        if (mCodecType == kAudioCodecEvs)
        {
            mAudioSource->SetEvsBandwidth(mEvsBandwidth);
            mAudioSource->SetEvsChAwOffset(mEvsChAwOffset);
        }

        if (mAudioSource->Start())
        {
            mNodeState = kNodeStateRunning;
            mFirstFrame = false;
            return RESULT_SUCCESS;
        }
    }
    else
    {
        IMLOGE0("[IAudioSourceNode] Not able to start AudioSource");
    }

    return RESULT_NOT_READY;
}

void IAudioSourceNode::Stop()
{
    IMLOGD0("[Stop]");
    if (mAudioSource)
    {
        mAudioSource->Stop();
    }
    mNodeState = kNodeStateStopped;
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
    {
        return;
    }

    AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);
    mCodecType = ImsMediaAudioFmt::ConvertCodecType(pConfig->getCodecType());
    if (mCodecType == kAudioCodecAmr || mCodecType == kAudioCodecAmrWb)
    {
        mMode = pConfig->getAmrParams().getAmrMode();
    }
    else if (mCodecType == kAudioCodecEvs)
    {
        mMode = pConfig->getEvsParams().getEvsMode();
        mEvsBandwidth = (kEvsBandwidth)pConfig->getEvsParams().getEvsBandwidth();
        mEvsChAwOffset = pConfig->getEvsParams().getChannelAwareMode();
    }
    mSamplingRate = pConfig->getSamplingRateKHz();
    mPtime = pConfig->getPtimeMillis();
}

bool IAudioSourceNode::IsSameConfig(void* config)
{
    if (config == NULL)
    {
        return true;
    }

    AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);

    if (mCodecType == ImsMediaAudioFmt::ConvertCodecType(pConfig->getCodecType()))
    {
        if (mCodecType == kAudioCodecAmr || mCodecType == kAudioCodecAmrWb)
        {
            return (mMode == pConfig->getAmrParams().getAmrMode() &&
                    mSamplingRate == pConfig->getSamplingRateKHz());
        }
        else if (mCodecType == kAudioCodecEvs)
        {
            return (mMode == pConfig->getEvsParams().getEvsMode() &&
                    mEvsBandwidth == (kEvsBandwidth)pConfig->getEvsParams().getEvsBandwidth() &&
                    mEvsChAwOffset == pConfig->getEvsParams().getChannelAwareMode() &&
                    mSamplingRate == pConfig->getSamplingRateKHz());
        }
    }

    return false;
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
                !client->mFirstFrame, MEDIASUBTYPE_UNDEFINED);
        if (!client->mFirstFrame)
        {
            client->mFirstFrame = true;
        }
    }
}
