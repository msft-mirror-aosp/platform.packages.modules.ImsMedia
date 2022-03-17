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

ImsMediaResult AudioStreamGraphRtpTx::createGraph(void* config){
    IMLOGD0("[createGraph]");
    mConfig = new AudioConfig(reinterpret_cast<AudioConfig*>(config));

    BaseNode* pNodeSource = BaseNode::Load(BaseNodeID::NODEID_VOICESOURCE, mCallback);
    if (pNodeSource == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeSource->SetMediaType(IMS_MEDIA_AUDIO);
    ((IVoiceSourceNode*)pNodeSource)->SetAttributeSource(VoiceManager::getAttributeSource());
    pNodeSource->SetConfig(mConfig);
    AddNode(pNodeSource);

    BaseNode* pNodeRtpPayloadEncoder = BaseNode::Load(BaseNodeID::NODEID_RTPPAYLOAD_ENCODER_AUDIO,
        mCallback);
    if (pNodeRtpPayloadEncoder == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeRtpPayloadEncoder->SetMediaType(IMS_MEDIA_AUDIO);
    pNodeRtpPayloadEncoder->SetConfig(mConfig);
    AddNode(pNodeRtpPayloadEncoder);
    pNodeSource->ConnectRearNode(pNodeRtpPayloadEncoder);

    BaseNode* pNodeRtpEncoder = BaseNode::Load(BaseNodeID::NODEID_RTPENCODER, mCallback);
    if (pNodeRtpEncoder == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeRtpEncoder->SetMediaType(IMS_MEDIA_AUDIO);
    char localIp[128];
    uint32_t localPort = 0;
    ImsMediaNetworkUtil::GetLocalIPPortFromSocketFD(mLocalFd, localIp, 128, localPort);
    RtpAddress localAddress(localIp, localPort);
    pNodeRtpEncoder->SetConfig(mConfig);
    ((RtpEncoderNode*)pNodeRtpEncoder)->SetLocalAddress(localAddress);
    AddNode(pNodeRtpEncoder);
    pNodeRtpPayloadEncoder->ConnectRearNode(pNodeRtpEncoder);

    BaseNode* pNodeSocketWriter = BaseNode::Load(BaseNodeID::NODEID_SOCKETWRITER, mCallback);
    if (pNodeSocketWriter == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeSocketWriter->SetMediaType(IMS_MEDIA_AUDIO);
    ((SocketWriterNode*)pNodeSocketWriter)->SetLocalFd(mLocalFd);
    ((SocketWriterNode*)pNodeSocketWriter)->SetLocalAddress(localAddress);
    ((SocketWriterNode*)pNodeSocketWriter)->SetProtocolType(RTP);
    pNodeSocketWriter->SetConfig(config);
    AddNode(pNodeSocketWriter);
    pNodeRtpEncoder->ConnectRearNode(pNodeSocketWriter);
    setState(StreamState::STATE_CREATED);

    if (mConfig->getDtmfPayloadTypeNumber() != 0) {
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
            ((DtmfSenderNode*)pDtmfSenderNode)->SetInterval(3);
            pDtmfEncoderNode->ConnectRearNode(pDtmfSenderNode);
            pDtmfSenderNode->ConnectRearNode(pNodeRtpEncoder);
        }
    }
    return ImsMediaResult::IMS_MEDIA_OK;
}

ImsMediaResult AudioStreamGraphRtpTx::updateGraph(void* config)  {
    IMLOGD0("[updateGraph]");
    if (config == NULL) return IMS_MEDIA_ERROR_INVALID_ARGUMENT;

    AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);

    if (*mConfig == *pConfig) {
        IMLOGD0("[updateGraph] no update");
        return IMS_MEDIA_OK;
    }

    if (mConfig != NULL) {
        delete mConfig;
    }

    mConfig = new AudioConfig(pConfig);

    if (mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_NO_FLOW
        || mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_RECEIVE_ONLY) {
        IMLOGD0("[updateGraph] pause TX");
        return stopGraph();
    }

    ImsMediaResult ret = ImsMediaResult::IMS_MEDIA_ERROR_UNKNOWN;

    if (mGraphState == STATE_RUN) {
        mScheduler->Stop();
        for (auto& node:mListNodeStarted) {
            IMLOGD1("[updateGraph] update node[%s]", node->GetNodeName());
            ret = node->UpdateConfig(mConfig);
            if (ret != IMS_MEDIA_OK) {
                IMLOGE2("[updateGraph] error in update node[%s], ret[%d]",
                    node->GetNodeName(), ret);
            }
        }
        mScheduler->Start();
    } else if (mGraphState == STATE_CREATED) {
        for (auto& node:mListNodeToStart) {
            IMLOGD1("[updateGraph] update node[%s]", node->GetNodeName());
            ret = node->UpdateConfig(mConfig);
            if (ret != IMS_MEDIA_OK) {
                IMLOGE2("[updateGraph] error in update node[%s], ret[%d]",
                    node->GetNodeName(), ret);
            }
        }
    }

    if (mGraphState == STATE_CREATED &&
        (pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_TRANSMIT_ONLY
        || pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_TRANSMIT_RECEIVE)) {
        IMLOGD0("[updateGraph] resume TX");
        return startGraph();
    }

    return ret;
}

bool AudioStreamGraphRtpTx::isSameConfig(RtpConfig* config) {
    if (mConfig == NULL || config == NULL) return false;
    //check compare
    if (mConfig->getRemoteAddress() == config->getRemoteAddress()
        && mConfig->getRemotePort() == config->getRemotePort()) {
        return true;
    }

    return false;
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