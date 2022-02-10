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
#include <ImsMediaVoiceNodeList.h>

AudioStreamGraphRtpRx::AudioStreamGraphRtpRx(BaseSessionCallback* callback, int localFd)
    : BaseStreamGraph(callback, localFd) {
}

AudioStreamGraphRtpRx::~AudioStreamGraphRtpRx() {
}

ImsMediaResult AudioStreamGraphRtpRx::createGraph(RtpConfig* config) {
    //copy the config
    if (config != NULL) {
        mConfig = std::make_shared<RtpConfig>(*config);
    }

    BaseNode* pNodeSocketReader = BaseNode::Load(BaseNodeID::NODEID_SOCKETREADER, mCallback);
    if (pNodeSocketReader == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeSocketReader->SetMediaType(IMS_MEDIA_AUDIO);
    //need to set socket id here
    //test parameters
    RtpAddress localAddrss("0.0.0.0", 61002);
    RtpAddress peerAddrss("0.0.0.0", 61002);
    ((SocketReaderNode*)pNodeSocketReader)->SetLocalFd(mLocalFd);
    ((SocketReaderNode*)pNodeSocketReader)->SetLocalEndpoint(localAddrss.ipAddress,
        localAddrss.port);
    ((SocketReaderNode*)pNodeSocketReader)->SetPeerEndpoint(peerAddrss.ipAddress,
        peerAddrss.port);
    AddNode(pNodeSocketReader);

    BaseNode* pNodeRtpDecoder = BaseNode::Load(BaseNodeID::NODEID_RTPDECODER, mCallback);
    if (pNodeRtpDecoder == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeRtpDecoder->SetMediaType(IMS_MEDIA_AUDIO);
    //pNodeRtpDecoder->SetRtpSessionParams(&mConfig->sessionParams);
    //test parameters
    ((RtpDecoderNode*)pNodeRtpDecoder)->SetLocalAddress(localAddrss);
    ((RtpDecoderNode*)pNodeRtpDecoder)->SetPeerAddress(peerAddrss);
    ((RtpDecoderNode*)pNodeRtpDecoder)->SetSamplingRate(16000);
    AddNode(pNodeRtpDecoder);
    pNodeSocketReader->ConnectRearNode(pNodeRtpDecoder);

    BaseNode* pNodeRtpPayloadDecoder =
        BaseNode::Load(BaseNodeID::NODEID_RTPPAYLOAD_DECODER_AUDIO, mCallback);
    if (pNodeRtpPayloadDecoder == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeRtpPayloadDecoder->SetMediaType(IMS_MEDIA_AUDIO);
    //pNodeRtpPayloadDecoder->SetRtpSessionParams(&mConfig->sessionParams);
    //test parameters
    ((AudioRtpPayloadDecoderNode*)pNodeRtpPayloadDecoder)->SetCodec(AUDIO_AMR_WB);
    ((AudioRtpPayloadDecoderNode*)pNodeRtpPayloadDecoder)->SetPayloadMode(
        RTPPAYLOADHEADER_MODE_AMR_OCTETALIGNED);
    AddNode(pNodeRtpPayloadDecoder);
    pNodeRtpDecoder->ConnectRearNode(pNodeRtpPayloadDecoder);

    BaseNode* pNodeRenderer = BaseNode::Load(BaseNodeID::NODEID_VOICERENDERER, mCallback);
    if (pNodeRenderer == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeRenderer->SetMediaType(IMS_MEDIA_AUDIO);
    //pNodeRenderer->SetRtpSessionParams(&mConfig->sessionParams);
    //testing parameters
    ((IVoiceRendererNode*)pNodeRenderer)->SetCodec(AUDIO_AMR_WB);
    ((IVoiceRendererNode*)pNodeRenderer)->SetCodecMode(8);
    ((IVoiceRendererNode*)pNodeRenderer)->SetJitterBufferSize(4, 4, 9);
    ((IVoiceRendererNode*)pNodeRenderer)->SetJitterOptions(80, 1, (double)25 / 10, true, true);
    AddNode(pNodeRenderer);
    pNodeRtpPayloadDecoder->ConnectRearNode(pNodeRenderer);
    setState(StreamState::STATE_CREATED);
    return ImsMediaResult::IMS_MEDIA_OK;
}

ImsMediaResult AudioStreamGraphRtpRx::updateGraph(ImsMediaHal::RtpConfig* config) {
    (void)config;
    return ImsMediaResult::IMS_MEDIA_OK;
}