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

#include <AudioStreamGraphRtpTx.h>
#include <ImsMediaNodeList.h>
#include <ImsMediaVoiceNodeList.h>
#include <ImsMediaTrace.h>
#include <VoiceManager.h>
#include <ImsMediaNetworkUtil.h>
#include <AudioConfig.h>
#include <NetdClient.h>
#include <stdlib.h>
#include <string.h>

AudioStreamGraphRtpTx::AudioStreamGraphRtpTx(BaseSessionCallback* callback, int localFd)
    : BaseStreamGraph(callback, localFd) {
    mConfig = NULL;
}

AudioStreamGraphRtpTx::~AudioStreamGraphRtpTx() {
    if (mConfig) {
        delete mConfig;
    }
}

ImsMediaResult AudioStreamGraphRtpTx::create(void* config){
    IMLOGD0("[create]");
    mConfig = new AudioConfig(reinterpret_cast<AudioConfig*>(config));

    BaseNode* pNodeSource = BaseNode::Load(BaseNodeID::NODEID_VOICESOURCE, mCallback);
    if (pNodeSource == NULL) return RESULT_NOT_READY;
    pNodeSource->SetMediaType(IMS_MEDIA_AUDIO);
    ((IVoiceSourceNode*)pNodeSource)->SetAttributeSource(VoiceManager::getAttributeSource());
    pNodeSource->SetConfig(mConfig);
    AddNode(pNodeSource);

    BaseNode* pNodeRtpPayloadEncoder = BaseNode::Load(BaseNodeID::NODEID_RTPPAYLOAD_ENCODER_AUDIO,
        mCallback);
    if (pNodeRtpPayloadEncoder == NULL) return RESULT_NOT_READY;
    pNodeRtpPayloadEncoder->SetMediaType(IMS_MEDIA_AUDIO);
    pNodeRtpPayloadEncoder->SetConfig(mConfig);
    AddNode(pNodeRtpPayloadEncoder);
    pNodeSource->ConnectRearNode(pNodeRtpPayloadEncoder);

    BaseNode* pNodeRtpEncoder = BaseNode::Load(BaseNodeID::NODEID_RTPENCODER, mCallback);
    if (pNodeRtpEncoder == NULL) return RESULT_NOT_READY;
    pNodeRtpEncoder->SetMediaType(IMS_MEDIA_AUDIO);
    char localIp[MAX_IP_LEN];
    uint32_t localPort = 0;
    ImsMediaNetworkUtil::GetLocalIPPortFromSocketFD(mLocalFd, localIp, MAX_IP_LEN, localPort);
    RtpAddress localAddress(localIp, localPort);
    pNodeRtpEncoder->SetConfig(mConfig);
    ((RtpEncoderNode*)pNodeRtpEncoder)->SetLocalAddress(localAddress);
    AddNode(pNodeRtpEncoder);
    pNodeRtpPayloadEncoder->ConnectRearNode(pNodeRtpEncoder);

    BaseNode* pNodeSocketWriter = BaseNode::Load(BaseNodeID::NODEID_SOCKETWRITER, mCallback);
    if (pNodeSocketWriter == NULL) return RESULT_NOT_READY;
    pNodeSocketWriter->SetMediaType(IMS_MEDIA_AUDIO);
    ((SocketWriterNode*)pNodeSocketWriter)->SetLocalFd(mLocalFd);
    ((SocketWriterNode*)pNodeSocketWriter)->SetLocalAddress(localAddress);
    ((SocketWriterNode*)pNodeSocketWriter)->SetProtocolType(RTP);
    pNodeSocketWriter->SetConfig(config);
    AddNode(pNodeSocketWriter);
    pNodeRtpEncoder->ConnectRearNode(pNodeSocketWriter);
    setState(StreamState::STATE_CREATED);

    AudioConfig* audioConfig = reinterpret_cast<AudioConfig*>(mConfig);

    if (audioConfig->getDtmfPayloadTypeNumber() != 0) {
        BaseNode* pDtmfEncoderNode = BaseNode::Load(BaseNodeID::NODEID_DTMFENCODER, mCallback);
        BaseNode* pDtmfSenderNode = BaseNode::Load(BaseNodeID::NODEID_DTMFSENDER, mCallback);

        if (pDtmfEncoderNode != NULL && pDtmfSenderNode != NULL) {
            AddNode(pDtmfEncoderNode);
            mListDtmfNodes.push_back(pDtmfEncoderNode);
            pDtmfEncoderNode->SetMediaType(IMS_MEDIA_AUDIO);
            pDtmfEncoderNode->SetConfig(mConfig);
            AddNode(pDtmfSenderNode);
            mListDtmfNodes.push_back(pDtmfSenderNode);
            pDtmfSenderNode->SetMediaType(IMS_MEDIA_AUDIO);
            pDtmfEncoderNode->ConnectRearNode(pDtmfSenderNode);
            pDtmfSenderNode->ConnectRearNode(pNodeRtpEncoder);
        }
    }
    return RESULT_SUCCESS;
}

ImsMediaResult AudioStreamGraphRtpTx::update(void* config)  {
    IMLOGD0("[update]");
    if (config == NULL) return RESULT_INVALID_PARAM;

    AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);

    if (*mConfig == *pConfig) {
        IMLOGD0("[update] no update");
        return RESULT_SUCCESS;
    }

    if (mConfig != NULL) {
        delete mConfig;
    }

    mConfig = new AudioConfig(pConfig);

    if (mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_NO_FLOW
        || mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_RECEIVE_ONLY) {
        IMLOGD0("[update] pause TX");
        return stop();
    }

    ImsMediaResult ret = RESULT_NOT_READY;

    if (mGraphState == STATE_RUN) {
        mScheduler->Stop();
        for (auto& node:mListNodeStarted) {
            IMLOGD1("[update] update node[%s]", node->GetNodeName());
            ret = node->UpdateConfig(mConfig);
            if (ret != RESULT_SUCCESS) {
                IMLOGE2("[update] error in update node[%s], ret[%d]",
                    node->GetNodeName(), ret);
            }
        }
        mScheduler->Start();
    } else if (mGraphState == STATE_CREATED) {
        for (auto& node:mListNodeToStart) {
            IMLOGD1("[update] update node[%s]", node->GetNodeName());
            ret = node->UpdateConfig(mConfig);
            if (ret != RESULT_SUCCESS) {
                IMLOGE2("[update] error in update node[%s], ret[%d]",
                    node->GetNodeName(), ret);
            }
        }
    }

    if (mGraphState == STATE_CREATED &&
        (pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_TRANSMIT_ONLY
        || pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_TRANSMIT_RECEIVE)) {
        IMLOGD0("[update] resume TX");
        return start();
    }

    return ret;
}

void AudioStreamGraphRtpTx::startDtmf(char digit, int volume, int duration) {
    IMLOGD0("[startDtmf]");
    BaseNode* pDTMFNode = mListDtmfNodes.front();
    if (pDTMFNode != NULL) {
        IMLOGD3("[startDtmf] %c, vol[%d], duration[%d]", digit, volume, duration);
        ImsMediaSubType subtype = MEDIASUBTYPE_DTMF_PAYLOAD;
        if (duration == 0) {
            subtype = MEDIASUBTYPE_DTMFSTART;
        }
        pDTMFNode->OnDataFromFrontNode(subtype, (uint8_t*)&digit, 1, volume, 0, duration);
    } else {
        IMLOGE0("[startDtmf] DTMF is not enabled");
    }
}

void AudioStreamGraphRtpTx::stopDtmf() {
    IMLOGD0("[stopDtmf]");
    BaseNode* pDTMFNode = mListDtmfNodes.front();
    if (pDTMFNode != NULL) {
        pDTMFNode->OnDataFromFrontNode(MEDIASUBTYPE_DTMFEND, 0, 0, 0, 0, 0);
    } else {
        IMLOGE0("[stopDtmf] DTMF is not enabled");
    }
}