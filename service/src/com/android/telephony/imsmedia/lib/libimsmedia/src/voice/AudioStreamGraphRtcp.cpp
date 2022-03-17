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
#include <ImsMediaNetworkUtil.h>
#include <ImsMediaTrace.h>

AudioStreamGraphRtcp::AudioStreamGraphRtcp(BaseSessionCallback* callback, int localFd)
 : BaseStreamGraph(callback, localFd) {
    mConfig = NULL;
}

AudioStreamGraphRtcp::~AudioStreamGraphRtcp() {
    if (mConfig != NULL) {
        delete mConfig;
        mConfig = NULL;
    }
}

ImsMediaResult AudioStreamGraphRtcp::createGraph(void* config) {
    IMLOGD0("[createGraph]");
    mConfig = new AudioConfig(reinterpret_cast<AudioConfig*>(config));
    BaseNode* pNodeRtcpEncoder = BaseNode::Load(BaseNodeID::NODEID_RTCPENCODER, mCallback);
    if (pNodeRtcpEncoder == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeRtcpEncoder->SetMediaType(IMS_MEDIA_AUDIO);
    char localIp[128];
    uint32_t localPort = 0;
    ImsMediaNetworkUtil::GetLocalIPPortFromSocketFD(mLocalFd, localIp, 128, localPort);
    RtpAddress localAddress(localIp, localPort - 1);
    ((RtcpEncoderNode*)pNodeRtcpEncoder)->SetLocalAddress(localAddress);
    pNodeRtcpEncoder->SetConfig(config);
    AddNode(pNodeRtcpEncoder);

    BaseNode* pNodeSocketWriter = BaseNode::Load(BaseNodeID::NODEID_SOCKETWRITER, mCallback);
    if (pNodeSocketWriter == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeSocketWriter->SetMediaType(IMS_MEDIA_AUDIO);
    ((SocketWriterNode*)pNodeSocketWriter)->SetLocalFd(mLocalFd);
    ((SocketWriterNode*)pNodeSocketWriter)->SetLocalAddress(RtpAddress(localIp, localPort));
    ((SocketWriterNode*)pNodeSocketWriter)->SetProtocolType(RTCP);
    pNodeSocketWriter->SetConfig(config);
    AddNode(pNodeSocketWriter);
    pNodeRtcpEncoder->ConnectRearNode(pNodeSocketWriter);
    setState(StreamState::STATE_CREATED);

    BaseNode* pNodeSocketReader = BaseNode::Load(BaseNodeID::NODEID_SOCKETREADER, mCallback);
    if (pNodeSocketReader == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeSocketReader->SetMediaType(IMS_MEDIA_AUDIO);
    ((SocketReaderNode*)pNodeSocketReader)->SetLocalFd(mLocalFd);
    ((SocketReaderNode*)pNodeSocketReader)->SetLocalAddress(RtpAddress(localIp, localPort));
    ((SocketReaderNode*)pNodeSocketReader)->SetProtocolType(RTCP);
    pNodeSocketReader->SetConfig(config);
    AddNode(pNodeSocketReader);

    BaseNode* pNodeRtcpDecoder = BaseNode::Load(BaseNodeID::NODEID_RTCPDECODER, mCallback);
    if (pNodeRtcpDecoder == NULL) return IMS_MEDIA_ERROR_UNKNOWN;
    pNodeRtcpDecoder->SetMediaType(IMS_MEDIA_AUDIO);
    ((RtcpDecoderNode*)pNodeRtcpDecoder)->SetLocalAddress(localAddress);
    pNodeRtcpDecoder->SetConfig(config);
    ((RtcpDecoderNode*)pNodeRtcpDecoder)->SetInactivityTimerSec(
        mThreshold.getRtpInactivityTimerMillis() == 0 ? 0 :
            mThreshold.getRtpInactivityTimerMillis() / 1000);
    AddNode(pNodeRtcpDecoder);
    pNodeSocketReader->ConnectRearNode(pNodeRtcpDecoder);
    return ImsMediaResult::IMS_MEDIA_OK;
}

ImsMediaResult AudioStreamGraphRtcp::updateGraph(void* config)  {
    IMLOGD0("[updateGraph]");
    if (config == NULL) return IMS_MEDIA_ERROR_INVALID_ARGUMENT;

    AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);

    if (*mConfig == *pConfig) {
        IMLOGD0("[updateGraph] no update");
        return IMS_MEDIA_OK;
    }

    if (mConfig != NULL) {
        delete mConfig;
        mConfig = new AudioConfig(pConfig);
    }

    if (mConfig->getRtcpConfig().getIntervalSec() == 0) {
        IMLOGD0("[updateGraph] pause RTCP");
        return stopGraph();
    }

    ImsMediaResult ret = ImsMediaResult::IMS_MEDIA_ERROR_UNKNOWN;
    //stop scheduler
    if (mGraphState == STATE_RUN) {
        mScheduler->Stop();
    }

    for (auto& node:mListNodeStarted) {
        IMLOGD1("[updateGraph] update node[%s]", node->GetNodeName());
        ret = node->UpdateConfig(pConfig);
        if (ret != IMS_MEDIA_OK) {
            IMLOGE2("[updateGraph] error in update node[%s], ret[%d]", node->GetNodeName(), ret);
        }
    }

    if (mGraphState == STATE_CREATED && mConfig->getRtcpConfig().getIntervalSec() != 0) {
        IMLOGD0("[updateGraph] resume RTCP");
        return startGraph();
    }

    //restart scheduler
    if (mGraphState == STATE_RUN) {
        mScheduler->Start();
    }

    return ret;
}

bool AudioStreamGraphRtcp::isSameConfig(RtpConfig* config) {
    if (mConfig == NULL) return false;
    //check compare
    if (mConfig->getRemoteAddress().compare(config->getRemoteAddress()) != 0
        && mConfig->getRemotePort() == config->getRemotePort()) {
        return true;
    }

    return false;
}