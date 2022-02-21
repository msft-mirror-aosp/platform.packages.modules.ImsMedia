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
    //pNodeSource->SetRtpSessionParams(&mConfig->sessionParams);
    //testing
    ((IVoiceSourceNode*)pNodeSource)->SetAttributeSource(VoiceManager::getAttributeSource());
    ((IVoiceSourceNode*)pNodeSource)->SetCodec(AUDIO_AMR_WB);
    ((IVoiceSourceNode*)pNodeSource)->SetCodecMode(8);
    AddNode(pNodeSource);

    BaseNode* pNodeRtpPayloadEncoder = BaseNode::Load(BaseNodeID::NODEID_RTPPAYLOAD_ENCODER_AUDIO,
        mCallback);
    if (pNodeRtpPayloadEncoder == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeRtpPayloadEncoder->SetMediaType(IMS_MEDIA_AUDIO);
    //pNodeRtpPayloadEncoder->SetRtpSessionParams(&mConfig->sessionParams);
    //test parameters
    ((AudioRtpPayloadEncoderNode*)pNodeRtpPayloadEncoder)->SetCodec(AUDIO_AMR_WB);
    ((AudioRtpPayloadEncoderNode*)pNodeRtpPayloadEncoder)->SetPayloadMode(
        RTPPAYLOADHEADER_MODE_AMR_OCTETALIGNED);
    ((AudioRtpPayloadEncoderNode*)pNodeRtpPayloadEncoder)->SetPtime(20);
    AddNode(pNodeRtpPayloadEncoder);
    pNodeSource->ConnectRearNode(pNodeRtpPayloadEncoder);

    BaseNode* pNodeRtpEncoder = BaseNode::Load(BaseNodeID::NODEID_RTPENCODER, mCallback);
    if (pNodeRtpEncoder == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeRtpEncoder->SetMediaType(IMS_MEDIA_AUDIO);
    //test parameters
    RtpAddress localAddrss("0.0.0.0", 61002);
    RtpAddress peerAddrss("0.0.0.0", 61002);
    ((RtpEncoderNode*)pNodeRtpEncoder)->SetLocalAddress(localAddrss);
    ((RtpEncoderNode*)pNodeRtpEncoder)->SetPeerAddress(peerAddrss);
    //pNodeRtpEncoder->SetRtpSessionParams(&mConfig->sessionParams);
    AddNode(pNodeRtpEncoder);
    pNodeRtpPayloadEncoder->ConnectRearNode(pNodeRtpEncoder);

    BaseNode* pNodeSocketWriter = BaseNode::Load(BaseNodeID::NODEID_SOCKETWRITER, mCallback);
    if (pNodeSocketWriter == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeSocketWriter->SetMediaType(IMS_MEDIA_AUDIO);
    //test parameters
    ((SocketWriterNode*)pNodeSocketWriter)->SetLocalFd(mLocalFd);
    ((SocketWriterNode*)pNodeSocketWriter)->SetLocalEndpoint(localAddrss.ipAddress,
        localAddrss.port);
    ((SocketWriterNode*)pNodeSocketWriter)->SetPeerEndpoint(peerAddrss.ipAddress,
        peerAddrss.port);

    //need to set socket id here
    AddNode(pNodeSocketWriter);
    pNodeRtpEncoder->ConnectRearNode(pNodeSocketWriter);
    setState(StreamState::STATE_CREATED);

    bool bEnableDTMF = false;
    if (bEnableDTMF) {
        BaseNode* pDtmfEncoderNode = BaseNode::Load(BaseNodeID::NODEID_DTMFENCODER, mCallback);
        BaseNode* pDtmfSenderNode = BaseNode::Load(BaseNodeID::NODEID_DTMFSENDER, mCallback);

        if (pDtmfEncoderNode != NULL && pDtmfSenderNode != NULL) {
            AddNode(pDtmfEncoderNode);
            mListDtmfNodes.push_back(pDtmfEncoderNode);
            pDtmfEncoderNode->SetMediaType(IMS_MEDIA_AUDIO);
            //test parameters
            ((DtmfEncoderNode*)pDtmfEncoderNode)->SetSamplingRate(16000);
            ((DtmfEncoderNode*)pDtmfEncoderNode)->SetDuration(10, 2);
            ((DtmfEncoderNode*)pDtmfEncoderNode)->SetVolume(10);

            AddNode(pDtmfSenderNode);
            mListDtmfNodes.push_back(pDtmfSenderNode);
            pDtmfSenderNode->SetMediaType(IMS_MEDIA_AUDIO);
            //test parameters
            ((DtmfSenderNode*)pDtmfSenderNode)->SetInterval(3);
            pDtmfEncoderNode->ConnectRearNode(pDtmfSenderNode);
            pDtmfSenderNode->ConnectRearNode(pNodeRtpEncoder);
        }
    }
    return ImsMediaResult::IMS_MEDIA_OK;
}

ImsMediaResult AudioStreamGraphRtpTx::updateGraph(void* config)  {
    IMLOGD0("[updateGraph]");
    (void)config;
    return ImsMediaResult::IMS_MEDIA_OK;
}

bool AudioStreamGraphRtpTx::isSameConfig(RtpConfig* config) {
    if (mConfig == NULL) return false;
    //check compare
    if (mConfig->getRemoteAddress().compare(config->getRemoteAddress()) != 0
        && mConfig->getRemotePort() == config->getRemotePort()) {
        return true;
    }

    return false;
}

void AudioStreamGraphRtpTx::startDtmf(char digit, int volume, int duration) {
    IMLOGD0("[startDtmf]");
    (void)digit;
    (void)volume;
    (void)duration;
}

void AudioStreamGraphRtpTx::stopDtmf() {
    IMLOGD0("[stopDtmf]");

}