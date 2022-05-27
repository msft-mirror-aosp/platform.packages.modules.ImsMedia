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

#include <VideoStreamGraphRtpTx.h>
#include <ImsMediaNodeList.h>
#include <ImsMediaVideoNodeList.h>
#include <ImsMediaTrace.h>
#include <ImsMediaNetworkUtil.h>
#include <VideoConfig.h>
#include <stdlib.h>
#include <string.h>
#include <thread>

static uint32_t sTimeout = 1000;

VideoStreamGraphRtpTx::VideoStreamGraphRtpTx(BaseSessionCallback* callback, int localFd) :
        BaseStreamGraph(callback, localFd)
{
    mConfig = NULL;
    mSurface = NULL;
}

VideoStreamGraphRtpTx::~VideoStreamGraphRtpTx()
{
    if (mConfig)
    {
        delete mConfig;
    }

    mCondition.reset();
}

ImsMediaResult VideoStreamGraphRtpTx::create(void* config)
{
    IMLOGD0("[create]");
    mConfig = new VideoConfig(reinterpret_cast<VideoConfig*>(config));
    char localIp[MAX_IP_LEN];
    uint32_t localPort = 0;
    ImsMediaNetworkUtil::GetLocalIPPortFromSocketFD(mLocalFd, localIp, MAX_IP_LEN, localPort);
    RtpAddress localAddress(localIp, localPort);

    BaseNode* pNodeSource = BaseNode::Load(BaseNodeID::NODEID_VIDEOSOURCE, mCallback);
    if (pNodeSource == NULL)
        return RESULT_NOT_READY;
    pNodeSource->SetMediaType(IMS_MEDIA_VIDEO);
    pNodeSource->SetConfig(mConfig);
    AddNode(pNodeSource);

    BaseNode* pNodeRtpPayloadEncoder =
            BaseNode::Load(BaseNodeID::NODEID_RTPPAYLOAD_ENCODER_VIDEO, mCallback);
    if (pNodeRtpPayloadEncoder == NULL)
    {
        return RESULT_NOT_READY;
    }
    pNodeRtpPayloadEncoder->SetMediaType(IMS_MEDIA_VIDEO);
    pNodeRtpPayloadEncoder->SetConfig(mConfig);
    AddNode(pNodeRtpPayloadEncoder);
    pNodeSource->ConnectRearNode(pNodeRtpPayloadEncoder);

    BaseNode* pNodeRtpEncoder = BaseNode::Load(BaseNodeID::NODEID_RTPENCODER, mCallback);
    if (pNodeRtpEncoder == NULL)
    {
        return RESULT_NOT_READY;
    }
    pNodeRtpEncoder->SetMediaType(IMS_MEDIA_VIDEO);
    pNodeRtpEncoder->SetConfig(mConfig);
    ((RtpEncoderNode*)pNodeRtpEncoder)->SetLocalAddress(localAddress);
    AddNode(pNodeRtpEncoder);
    pNodeRtpPayloadEncoder->ConnectRearNode(pNodeRtpEncoder);

    BaseNode* pNodeSocketWriter = BaseNode::Load(BaseNodeID::NODEID_SOCKETWRITER, mCallback);
    if (pNodeSocketWriter == NULL)
    {
        return RESULT_NOT_READY;
    }
    pNodeSocketWriter->SetMediaType(IMS_MEDIA_VIDEO);
    ((SocketWriterNode*)pNodeSocketWriter)->SetLocalFd(mLocalFd);
    ((SocketWriterNode*)pNodeSocketWriter)->SetLocalAddress(localAddress);
    ((SocketWriterNode*)pNodeSocketWriter)->SetProtocolType(RTP);
    pNodeSocketWriter->SetConfig(config);
    AddNode(pNodeSocketWriter);
    pNodeRtpEncoder->ConnectRearNode(pNodeSocketWriter);
    setState(StreamState::kStreamStateCreated);
    return RESULT_SUCCESS;
}

ImsMediaResult VideoStreamGraphRtpTx::update(void* config)
{
    IMLOGD0("[update]");
    if (config == NULL)
    {
        return RESULT_INVALID_PARAM;
    }

    VideoConfig* pConfig = reinterpret_cast<VideoConfig*>(config);

    if (*mConfig == *pConfig)
    {
        IMLOGD0("[update] no update");
        return RESULT_SUCCESS;
    }

    if (mConfig != NULL)
    {
        delete mConfig;
    }

    mConfig = new VideoConfig(pConfig);

    if (mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_NO_FLOW ||
            mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_RECEIVE_ONLY)
    {
        IMLOGD0("[update] pause TX");
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
            (pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_TRANSMIT_ONLY ||
                    pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_TRANSMIT_RECEIVE))
    {
        IMLOGD0("[update] resume TX");
        return start();
    }

    return ret;
}

ImsMediaResult VideoStreamGraphRtpTx::start()
{
    IMLOGD0("[start]");
    // start encoder output thread
    std::thread t1(&VideoStreamGraphRtpTx::processStart, this);
    t1.detach();
    return RESULT_SUCCESS;
}

void VideoStreamGraphRtpTx::setSurface(ANativeWindow* surface)
{
    IMLOGD0("[setSurface]");
    std::lock_guard<std::mutex> guard(mMutex);
    if (surface == NULL)
    {
        return;
    }

    mSurface = surface;

    bool found = false;
    for (auto& node : mListNodeToStart)
    {
        if (node->GetNodeID() == NODEID_VIDEOSOURCE)
        {
            IVideoSourceNode* pNode = reinterpret_cast<IVideoSourceNode*>(node);
            pNode->UpdateSurface(surface);
            found = true;
            break;
        }
    }

    if (found == false)
    {
        for (auto& node : mListNodeStarted)
        {
            if (node->GetNodeID() == NODEID_VIDEOSOURCE)
            {
                IVideoSourceNode* pNode = reinterpret_cast<IVideoSourceNode*>(node);
                pNode->UpdateSurface(surface);
                found = true;
                break;
            }
        }
    }

    if (getState() == StreamState::kStreamStateWaitSurface)
    {
        IMLOGD0("[setSurface] signal");
        mCondition.signal();
    }
}

void VideoStreamGraphRtpTx::processStart()
{
    IMLOGD0("[processStart]");
    mMutex.lock();

    VideoConfig* pConfig = reinterpret_cast<VideoConfig*>(mConfig);

    if (pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_NO_FLOW ||
        pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_RECEIVE_ONLY)
    {
        IMLOGD1("[processStart] direction[%d] no need to start", pConfig->getMediaDirection());
        mMutex.unlock();
        return;
    }

    if (pConfig->getVideoMode() != VideoConfig::VIDEO_MODE_PAUSE_IMAGE && mSurface == NULL)
    {
        IMLOGD2("[processStart] direction[%d], mode[%d], surface is not ready, wait",
                pConfig->getMediaDirection(), pConfig->getVideoMode());
        setState(StreamState::kStreamStateWaitSurface);
        mMutex.unlock();
        if (mCondition.wait_timeout(sTimeout))
        {
            setState(StreamState::kStreamStateCreated);
            mCallback->SendEvent(kImsMediaEventNotifyError, kNotifyErrorSurfaceNotReady,
                    kStreamModeRtpTx);
            return;
        }
        mCondition.reset();
    }

    mStartResult = startNodes();
    if (mStartResult == RESULT_SUCCESS)
    {
        setState(StreamState::kStreamStateRunning);
    }
    else
    {
        setState(StreamState::kStreamStateCreated);
        mCallback->SendEvent(kImsMediaEventNotifyError, mStartResult, kStreamModeRtpTx);
    }

    mMutex.unlock();
}

void VideoStreamGraphRtpTx::OnEvent(int32_t type, uint64_t param1, uint64_t param2)
{
    bool found = false;
    switch (type)
    {
        case kRequestVideoCvoUpdate:
            for (auto& node : mListNodeToStart)
            {
                if (node->GetNodeID() == NODEID_RTPENCODER)
                {
                    RtpEncoderNode* pNode = reinterpret_cast<RtpEncoderNode*>(node);
                    pNode->SetCvoExtension(param1, param2);
                    found = true;
                    break;
                }
            }

            if (found == false)
            {
                for (auto& node : mListNodeStarted)
                {
                    if (node->GetNodeID() == NODEID_RTPENCODER)
                    {
                        RtpEncoderNode* pNode = reinterpret_cast<RtpEncoderNode*>(node);
                        pNode->SetCvoExtension(param1, param2);
                        break;
                    }
                }
            }
            break;
        case kRequestVideoBitrateChange:
            break;
        case kRequestVideoIdrFrame:
            break;
        case kRequestVideoSendNack:
            break;
        default:
            break;
    }
}