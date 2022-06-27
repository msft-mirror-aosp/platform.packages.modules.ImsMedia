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

#include <VideoStreamGraphRtcp.h>
#include <RtcpEncoderNode.h>
#include <RtcpDecoderNode.h>
#include <SocketReaderNode.h>
#include <SocketWriterNode.h>
#include <ImsMediaNetworkUtil.h>
#include <ImsMediaTrace.h>
#include <VideoConfig.h>

VideoStreamGraphRtcp::VideoStreamGraphRtcp(BaseSessionCallback* callback, int localFd) :
        BaseStreamGraph(callback, localFd)
{
    mConfig = NULL;
}

VideoStreamGraphRtcp::~VideoStreamGraphRtcp()
{
    if (mConfig != NULL)
    {
        delete mConfig;
        mConfig = NULL;
    }
}

ImsMediaResult VideoStreamGraphRtcp::create(void* config)
{
    IMLOGD0("[createGraph]");
    mConfig = new VideoConfig(reinterpret_cast<VideoConfig*>(config));
    BaseNode* pNodeRtcpEncoder = BaseNode::Load(BaseNodeID::NODEID_RTCPENCODER, mCallback);

    if (pNodeRtcpEncoder == NULL)
    {
        return RESULT_NOT_READY;
    }

    pNodeRtcpEncoder->SetMediaType(IMS_MEDIA_VIDEO);
    char localIp[MAX_IP_LEN];
    uint32_t localPort = 0;
    ImsMediaNetworkUtil::GetLocalIPPortFromSocketFD(mLocalFd, localIp, MAX_IP_LEN, localPort);
    RtpAddress localAddress(localIp, localPort - 1);
    ((RtcpEncoderNode*)pNodeRtcpEncoder)->SetLocalAddress(localAddress);
    pNodeRtcpEncoder->SetConfig(config);
    AddNode(pNodeRtcpEncoder);

    BaseNode* pNodeSocketWriter = BaseNode::Load(BaseNodeID::NODEID_SOCKETWRITER, mCallback);

    if (pNodeSocketWriter == NULL)
    {
        return RESULT_NOT_READY;
    }

    pNodeSocketWriter->SetMediaType(IMS_MEDIA_VIDEO);
    ((SocketWriterNode*)pNodeSocketWriter)->SetLocalFd(mLocalFd);
    ((SocketWriterNode*)pNodeSocketWriter)->SetLocalAddress(RtpAddress(localIp, localPort));
    ((SocketWriterNode*)pNodeSocketWriter)->SetProtocolType(RTCP);
    pNodeSocketWriter->SetConfig(config);
    AddNode(pNodeSocketWriter);
    pNodeRtcpEncoder->ConnectRearNode(pNodeSocketWriter);

    BaseNode* pNodeSocketReader = BaseNode::Load(BaseNodeID::NODEID_SOCKETREADER, mCallback);

    if (pNodeSocketReader == NULL)
    {
        return RESULT_NOT_READY;
    }

    pNodeSocketReader->SetMediaType(IMS_MEDIA_VIDEO);
    ((SocketReaderNode*)pNodeSocketReader)->SetLocalFd(mLocalFd);
    ((SocketReaderNode*)pNodeSocketReader)->SetLocalAddress(RtpAddress(localIp, localPort));
    ((SocketReaderNode*)pNodeSocketReader)->SetProtocolType(RTCP);
    pNodeSocketReader->SetConfig(config);
    AddNode(pNodeSocketReader);

    BaseNode* pNodeRtcpDecoder = BaseNode::Load(BaseNodeID::NODEID_RTCPDECODER, mCallback);

    if (pNodeRtcpDecoder == NULL)
    {
        return RESULT_NOT_READY;
    }

    pNodeRtcpDecoder->SetMediaType(IMS_MEDIA_VIDEO);
    ((RtcpDecoderNode*)pNodeRtcpDecoder)->SetLocalAddress(localAddress);
    pNodeRtcpDecoder->SetConfig(config);
    AddNode(pNodeRtcpDecoder);
    pNodeSocketReader->ConnectRearNode(pNodeRtcpDecoder);
    setState(StreamState::kStreamStateCreated);
    return ImsMediaResult::RESULT_SUCCESS;
}

ImsMediaResult VideoStreamGraphRtcp::update(void* config)
{
    IMLOGD0("[update]");
    if (config == NULL)
        return RESULT_INVALID_PARAM;

    VideoConfig* pConfig = reinterpret_cast<VideoConfig*>(config);

    if (*reinterpret_cast<VideoConfig*>(mConfig) == *pConfig)
    {
        IMLOGD0("[update] no update");
        return RESULT_SUCCESS;
    }

    if (mConfig != NULL)
    {
        delete mConfig;
    }

    mConfig = new VideoConfig(pConfig);

    if (mConfig->getRtcpConfig().getIntervalSec() == 0)
    {
        IMLOGD0("[update] pause RTCP");
        return stop();
    }

    ImsMediaResult ret = ImsMediaResult::RESULT_NOT_READY;
    // stop scheduler
    if (mGraphState == kStreamStateRunning)
    {
        mScheduler->Stop();
    }

    for (auto& node : mListNodeStarted)
    {
        if (node != NULL)
        {
            IMLOGD1("[update] update node[%s]", node->GetNodeName());
            ret = node->UpdateConfig(pConfig);
            if (ret != RESULT_SUCCESS)
            {
                IMLOGE2("[update] error in update node[%s], ret[%d]", node->GetNodeName(), ret);
            }
        }
    }

    if (mGraphState == kStreamStateCreated && mConfig->getRtcpConfig().getIntervalSec() != 0)
    {
        IMLOGD0("[update] resume RTCP");
        return start();
    }

    // restart scheduler
    if (mGraphState == kStreamStateRunning)
    {
        mScheduler->Start();
    }

    return ret;
}

void VideoStreamGraphRtcp::setMediaQualityThreshold(MediaQualityThreshold* threshold)
{
    if (threshold == NULL)
        return;

    bool found = false;
    for (auto& node : mListNodeToStart)
    {
        if (node != NULL && node->GetNodeID() == BaseNodeID::NODEID_RTCPDECODER)
        {
            RtcpDecoderNode* pNode = reinterpret_cast<RtcpDecoderNode*>(node);
            pNode->SetInactivityTimerSec(threshold->getRtcpInactivityTimerMillis() / 1000);
            found = true;
            break;
        }
    }

    if (found == false)
    {
        for (auto& node : mListNodeStarted)
        {
            if (node != NULL && node->GetNodeID() == NODEID_RTCPDECODER)
            {
                RtcpDecoderNode* pNode = reinterpret_cast<RtcpDecoderNode*>(node);
                pNode->SetInactivityTimerSec(threshold->getRtcpInactivityTimerMillis() / 1000);
                break;
            }
        }
    }
}