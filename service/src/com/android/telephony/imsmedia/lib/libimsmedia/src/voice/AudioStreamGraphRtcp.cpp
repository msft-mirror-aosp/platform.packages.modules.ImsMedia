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

#include <AudioStreamGraphRtcp.h>
#include <RtcpEncoderNode.h>
#include <RtcpDecoderNode.h>
#include <SocketReaderNode.h>
#include <SocketWriterNode.h>

AudioStreamGraphRtcp::AudioStreamGraphRtcp(BaseSessionCallback* callback, int localFd)
 : BaseStreamGraph(callback, localFd) {

}

AudioStreamGraphRtcp::~AudioStreamGraphRtcp() {
}

ImsMediaResult AudioStreamGraphRtcp::createGraph(RtpConfig* config) {
    (void)config;
    BaseNode* pNodeRtcpEncoder = BaseNode::Load(BaseNodeID::NODEID_RTCPENCODER, mCallback);
    if (pNodeRtcpEncoder == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeRtcpEncoder->SetMediaType(IMS_MEDIA_AUDIO);
    //test parameters
    RtpAddress localRtpAddrss("0.0.0.0", 61002);
    RtpAddress peerRtpAddrss("0.0.0.0", 61002);
    RtpAddress localRtcpAddrss("0.0.0.0", 61003);
    RtpAddress peerRtcpAddrss("0.0.0.0", 61003);
    ((RtcpEncoderNode*)pNodeRtcpEncoder)->SetLocalAddress(localRtpAddrss);
    ((RtcpEncoderNode*)pNodeRtcpEncoder)->SetPeerAddress(peerRtpAddrss);
    ((RtcpEncoderNode*)pNodeRtcpEncoder)->SetRtcpInterval(3);
    ((RtcpEncoderNode*)pNodeRtcpEncoder)->SetRtcpXrEnable(false);
    ((RtcpEncoderNode*)pNodeRtcpEncoder)->SetRtcpByeEnable(false);
    //pNodeRtcpEncoder->SetRtcpSessionParams(&mConfig->sessionParams);
    AddNode(pNodeRtcpEncoder);

    BaseNode* pNodeSocketWriter = BaseNode::Load(BaseNodeID::NODEID_SOCKETWRITER, mCallback);
    if (pNodeSocketWriter == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeSocketWriter->SetMediaType(IMS_MEDIA_AUDIO);
    //test parameters
    ((SocketWriterNode*)pNodeSocketWriter)->SetLocalFd(mLocalFd);
    ((SocketWriterNode*)pNodeSocketWriter)->SetLocalEndpoint(localRtcpAddrss.ipAddress,
        localRtcpAddrss.port);
    ((SocketWriterNode*)pNodeSocketWriter)->SetPeerEndpoint(peerRtcpAddrss.ipAddress,
        peerRtcpAddrss.port);

    //need to set socket id here
    AddNode(pNodeSocketWriter);
    pNodeRtcpEncoder->ConnectRearNode(pNodeSocketWriter);
    setState(StreamState::STATE_CREATED);

    BaseNode* pNodeSocketReader = BaseNode::Load(BaseNodeID::NODEID_SOCKETREADER, mCallback);
    if (pNodeSocketReader == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeSocketReader->SetMediaType(IMS_MEDIA_AUDIO);
    //need to set socket id here
    //test parameters
    ((SocketReaderNode*)pNodeSocketReader)->SetLocalFd(mLocalFd);
    ((SocketReaderNode*)pNodeSocketReader)->SetLocalEndpoint(localRtcpAddrss.ipAddress,
        localRtcpAddrss.port);
    ((SocketReaderNode*)pNodeSocketReader)->SetPeerEndpoint(peerRtcpAddrss.ipAddress,
        peerRtcpAddrss.port);
    AddNode(pNodeSocketReader);

    BaseNode* pNodeRtcpDecoder = BaseNode::Load(BaseNodeID::NODEID_RTCPDECODER, mCallback);
    if (pNodeRtcpDecoder == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeRtcpDecoder->SetMediaType(IMS_MEDIA_AUDIO);
    //pNodeRtcpDecoder->SetRtcpSessionParams(&mConfig->sessionParams);
    //test parameters
    ((RtcpDecoderNode*)pNodeRtcpDecoder)->SetLocalAddress(localRtpAddrss);
    ((RtcpDecoderNode*)pNodeRtcpDecoder)->SetPeerAddress(peerRtpAddrss);
    AddNode(pNodeRtcpDecoder);
    pNodeSocketReader->ConnectRearNode(pNodeRtcpDecoder);
    return ImsMediaResult::IMS_MEDIA_OK;
}

ImsMediaResult AudioStreamGraphRtcp::updateGraph(ImsMediaHal::RtpConfig* config) {
    (void)config;
    //do it later
    return ImsMediaResult::IMS_MEDIA_OK;
}