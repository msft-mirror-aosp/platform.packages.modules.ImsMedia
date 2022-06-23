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

#include <AudioStreamGraphRtpRx.h>
#include <ImsMediaNodeList.h>
#include <ImsMediaAudioNodeList.h>
#include <ImsMediaTrace.h>
#include <ImsMediaNetworkUtil.h>
#include <AudioConfig.h>

AudioStreamGraphRtpRx::AudioStreamGraphRtpRx(BaseSessionCallback* callback, int localFd) :
        BaseStreamGraph(callback, localFd)
{
}

AudioStreamGraphRtpRx::~AudioStreamGraphRtpRx() {}

ImsMediaResult AudioStreamGraphRtpRx::create(void* config)
{
    IMLOGD0("[create]");
    mConfig = new AudioConfig(reinterpret_cast<AudioConfig*>(config));

    BaseNode* pNodeSocketReader = BaseNode::Load(BaseNodeID::NODEID_SOCKETREADER, mCallback);
    if (pNodeSocketReader == NULL)
        return RESULT_NOT_READY;
    pNodeSocketReader->SetMediaType(IMS_MEDIA_AUDIO);
    char localIp[MAX_IP_LEN];
    uint32_t localPort = 0;
    ImsMediaNetworkUtil::GetLocalIPPortFromSocketFD(mLocalFd, localIp, MAX_IP_LEN, localPort);
    RtpAddress localAddress(localIp, localPort);
    ((SocketReaderNode*)pNodeSocketReader)->SetLocalFd(mLocalFd);
    ((SocketReaderNode*)pNodeSocketReader)->SetLocalAddress(localAddress);
    ((SocketReaderNode*)pNodeSocketReader)->SetProtocolType(RTP);
    pNodeSocketReader->SetConfig(config);
    AddNode(pNodeSocketReader);

    BaseNode* pNodeRtpDecoder = BaseNode::Load(BaseNodeID::NODEID_RTPDECODER, mCallback);
    if (pNodeRtpDecoder == NULL)
        return RESULT_NOT_READY;
    pNodeRtpDecoder->SetMediaType(IMS_MEDIA_AUDIO);
    pNodeRtpDecoder->SetConfig(mConfig);
    ((RtpDecoderNode*)pNodeRtpDecoder)->SetLocalAddress(localAddress);
    AddNode(pNodeRtpDecoder);
    pNodeSocketReader->ConnectRearNode(pNodeRtpDecoder);

    BaseNode* pNodeRtpPayloadDecoder =
            BaseNode::Load(BaseNodeID::NODEID_RTPPAYLOAD_DECODER_AUDIO, mCallback);
    if (pNodeRtpPayloadDecoder == NULL)
        return RESULT_NOT_READY;
    pNodeRtpPayloadDecoder->SetMediaType(IMS_MEDIA_AUDIO);
    pNodeRtpPayloadDecoder->SetConfig(mConfig);
    AddNode(pNodeRtpPayloadDecoder);
    pNodeRtpDecoder->ConnectRearNode(pNodeRtpPayloadDecoder);

    BaseNode* pNodeRenderer = BaseNode::Load(BaseNodeID::NODEID_AUDIOPLAYER, mCallback);
    if (pNodeRenderer == NULL)
        return RESULT_NOT_READY;
    pNodeRenderer->SetMediaType(IMS_MEDIA_AUDIO);
    pNodeRenderer->SetConfig(mConfig);
    AddNode(pNodeRenderer);
    pNodeRtpPayloadDecoder->ConnectRearNode(pNodeRenderer);
    setState(StreamState::kStreamStateCreated);
    return RESULT_SUCCESS;
}

ImsMediaResult AudioStreamGraphRtpRx::update(void* config)
{
    IMLOGD0("[update]");
    if (config == NULL)
        return RESULT_INVALID_PARAM;

    AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);

    if (*mConfig == *pConfig)
    {
        IMLOGD0("[update] no update");
        return RESULT_SUCCESS;
    }

    if (mConfig != NULL)
    {
        delete mConfig;
    }

    mConfig = new AudioConfig(pConfig);

    if (mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_NO_FLOW ||
            mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_SEND_ONLY ||
            mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_INACTIVE)
    {
        IMLOGD0("[update] pause RX");
        return stop();
    }
    ImsMediaResult ret = RESULT_NOT_READY;

    if (mGraphState == kStreamStateRunning)
    {
        mScheduler->Stop();
        for (auto& node : mListNodeStarted)
        {
            IMLOGD1("[update] update node[%s]", node->GetNodeName());
            ret = node->UpdateConfig(mConfig);
            if (ret != RESULT_SUCCESS)
            {
                IMLOGE2("[update] error in update node[%s], ret[%d]", node->GetNodeName(), ret);
            }
        }
        mScheduler->Start();
    }
    else if (mGraphState == kStreamStateCreated)
    {
        for (auto& node : mListNodeToStart)
        {
            IMLOGD1("[update] update node[%s]", node->GetNodeName());
            ret = node->UpdateConfig(mConfig);
            if (ret != RESULT_SUCCESS)
            {
                IMLOGE2("[update] error in update node[%s], ret[%d]", node->GetNodeName(), ret);
            }
        }
    }

    if (mGraphState == kStreamStateCreated &&
            (pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_RECEIVE_ONLY ||
                    pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_SEND_RECEIVE))
    {
        IMLOGD0("[update] resume RX");
        return start();
    }

    return ret;
}

void AudioStreamGraphRtpRx::setMediaQualityThreshold(MediaQualityThreshold* threshold)
{
    if (threshold == NULL)
        return;

    bool found = false;
    for (auto& i : mListNodeToStart)
    {
        if (i->GetNodeID() == BaseNodeID::NODEID_RTPDECODER)
        {
            RtpDecoderNode* pNode = reinterpret_cast<RtpDecoderNode*>(i);
            pNode->SetInactivityTimerSec(threshold->getRtpInactivityTimerMillis() / 1000);
            found = true;
            break;
        }
    }

    if (found == false)
    {
        for (auto& node : mListNodeStarted)
        {
            if (node != NULL && node->GetNodeID() == NODEID_RTPDECODER)
            {
                RtpDecoderNode* pNode = reinterpret_cast<RtpDecoderNode*>(node);
                pNode->SetInactivityTimerSec(threshold->getRtpInactivityTimerMillis() / 1000);
                break;
            }
        }
    }
}