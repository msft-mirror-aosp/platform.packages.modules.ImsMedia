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

#include <VideoStreamGraphRtpRx.h>
#include <ImsMediaNodeList.h>
#include <ImsMediaVideoNodeList.h>
#include <ImsMediaTrace.h>
#include <ImsMediaNetworkUtil.h>
#include <VideoConfig.h>
#include <thread>

static uint32_t sTimeout = 100;

VideoStreamGraphRtpRx::VideoStreamGraphRtpRx(BaseSessionCallback* callback, int localFd) :
        BaseStreamGraph(callback, localFd)
{
    mConfig = NULL;
    mSurface = NULL;
    mClosed = false;
}

VideoStreamGraphRtpRx::~VideoStreamGraphRtpRx()
{
    if (mConfig)
    {
        delete mConfig;
    }

    mClosed = true;
    mCondition.signal();
    mConditionExit.wait_timeout(sTimeout);
}

ImsMediaResult VideoStreamGraphRtpRx::create(void* config)
{
    IMLOGD0("[create]");
    mConfig = new VideoConfig(reinterpret_cast<VideoConfig*>(config));

    char localIp[MAX_IP_LEN];
    uint32_t localPort = 0;
    ImsMediaNetworkUtil::GetLocalIPPortFromSocketFD(mLocalFd, localIp, MAX_IP_LEN, localPort);
    RtpAddress localAddress(localIp, localPort);

    BaseNode* pNodeSocketReader = BaseNode::Load(BaseNodeID::NODEID_SOCKETREADER, mCallback);

    if (pNodeSocketReader == NULL)
    {
        return RESULT_NOT_READY;
    }

    pNodeSocketReader->SetMediaType(IMS_MEDIA_VIDEO);
    ((SocketReaderNode*)pNodeSocketReader)->SetLocalFd(mLocalFd);
    ((SocketReaderNode*)pNodeSocketReader)->SetLocalAddress(localAddress);
    ((SocketReaderNode*)pNodeSocketReader)->SetProtocolType(RTP);
    pNodeSocketReader->SetConfig(config);
    AddNode(pNodeSocketReader);

    BaseNode* pNodeRtpDecoder = BaseNode::Load(BaseNodeID::NODEID_RTPDECODER, mCallback);

    if (pNodeRtpDecoder == NULL)
    {
        return RESULT_NOT_READY;
    }

    pNodeRtpDecoder->SetMediaType(IMS_MEDIA_VIDEO);
    pNodeRtpDecoder->SetConfig(mConfig);
    ((RtpDecoderNode*)pNodeRtpDecoder)->SetLocalAddress(localAddress);
    AddNode(pNodeRtpDecoder);
    pNodeSocketReader->ConnectRearNode(pNodeRtpDecoder);

    BaseNode* pNodeRtpPayloadDecoder =
            BaseNode::Load(BaseNodeID::NODEID_RTPPAYLOAD_DECODER_VIDEO, mCallback);

    if (pNodeRtpPayloadDecoder == NULL)
    {
        return RESULT_NOT_READY;
    }

    pNodeRtpPayloadDecoder->SetMediaType(IMS_MEDIA_VIDEO);
    pNodeRtpPayloadDecoder->SetConfig(mConfig);
    AddNode(pNodeRtpPayloadDecoder);
    pNodeRtpDecoder->ConnectRearNode(pNodeRtpPayloadDecoder);

    BaseNode* pNodeRenderer = BaseNode::Load(BaseNodeID::NODEID_VIDEORENDERER, mCallback);

    if (pNodeRenderer == NULL)
    {
        return RESULT_NOT_READY;
    }
    pNodeRenderer->SetMediaType(IMS_MEDIA_VIDEO);
    pNodeRenderer->SetConfig(mConfig);
    AddNode(pNodeRenderer);
    pNodeRtpPayloadDecoder->ConnectRearNode(pNodeRenderer);
    setState(StreamState::kStreamStateCreated);
    return RESULT_SUCCESS;
}

ImsMediaResult VideoStreamGraphRtpRx::update(void* config)
{
    IMLOGD0("[update]");
    if (config == NULL)
    {
        return RESULT_INVALID_PARAM;
    }

    VideoConfig* pConfig = reinterpret_cast<VideoConfig*>(config);

    if (*reinterpret_cast<VideoConfig*>(mConfig) == *pConfig)
    {
        IMLOGD0("[update] no update");
        return RESULT_SUCCESS;
    }

    if (mConfig != NULL)
    {
        delete mConfig;
        mConfig = NULL;
    }

    mConfig = new VideoConfig(pConfig);

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
            if (node != NULL)
            {
                IMLOGD1("[update] update node[%s]", node->GetNodeName());
                ret = node->UpdateConfig(mConfig);
                if (ret != RESULT_SUCCESS)
                {
                    IMLOGE2("[update] error in update node[%s], ret[%d]", node->GetNodeName(), ret);
                }
            }
        }
        mScheduler->Start();
    }
    else if (mGraphState == kStreamStateCreated)
    {
        for (auto& node : mListNodeToStart)
        {
            if (node != NULL)
            {
                IMLOGD1("[update] update node[%s]", node->GetNodeName());
                ret = node->UpdateConfig(mConfig);
                if (ret != RESULT_SUCCESS)
                {
                    IMLOGE2("[update] error in update node[%s], ret[%d]", node->GetNodeName(), ret);
                }
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

ImsMediaResult VideoStreamGraphRtpRx::start()
{
    IMLOGD0("[start]");
    // start encoder output thread
    setState(StreamState::kStreamStateWaitSurface);
    std::thread t1(&VideoStreamGraphRtpRx::processStart, this);
    t1.detach();
    return RESULT_SUCCESS;
}

void VideoStreamGraphRtpRx::setMediaQualityThreshold(MediaQualityThreshold* threshold)
{
    if (threshold == NULL)
    {
        return;
    }

    bool found = false;
    for (auto& node : mListNodeToStart)
    {
        if (node != NULL && node->GetNodeID() == BaseNodeID::NODEID_RTPDECODER)
        {
            RtpDecoderNode* pNode = reinterpret_cast<RtpDecoderNode*>(node);
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

void VideoStreamGraphRtpRx::setSurface(ANativeWindow* surface)
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
        if (node != NULL && node->GetNodeID() == NODEID_VIDEORENDERER)
        {
            IVideoRendererNode* pNode = reinterpret_cast<IVideoRendererNode*>(node);
            pNode->UpdateSurface(surface);
            found = true;
            break;
        }
    }

    if (found == false)
    {
        for (auto& node : mListNodeStarted)
        {
            if (node != NULL && node->GetNodeID() == NODEID_VIDEORENDERER)
            {
                IVideoRendererNode* pNode = reinterpret_cast<IVideoRendererNode*>(node);
                pNode->UpdateSurface(surface);
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

void VideoStreamGraphRtpRx::processStart()
{
    if (mConfig == NULL)
    {
        return;
    }

    IMLOGD0("[processStart]");
    mMutex.lock();
    mCondition.reset();
    VideoConfig* pConfig = reinterpret_cast<VideoConfig*>(mConfig);

    if (pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_NO_FLOW ||
            pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_SEND_ONLY ||
            mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_INACTIVE)
    {
        IMLOGD1("[processStart] direction[%d] no need to start", pConfig->getMediaDirection());
        mMutex.unlock();
        return;
    }

    if (mSurface == NULL)
    {
        IMLOGD2("[processStart] direction[%d], mode[%d], surface is not ready, wait",
                pConfig->getMediaDirection(), pConfig->getVideoMode());
        setState(StreamState::kStreamStateWaitSurface);
        mMutex.unlock();
        mCondition.wait();

        if (mClosed)
        {
            IMLOGD0("[processStart] exit");
            mConditionExit.signal();
            return;
        }
    }

    ImsMediaResult result = startNodes();
    if (result != RESULT_SUCCESS)
    {
        setState(StreamState::kStreamStateCreated);
        mCallback->SendEvent(kImsMediaEventNotifyError, result, kStreamModeRtpRx);
        mMutex.unlock();
        return;
    }

    setState(StreamState::kStreamStateRunning);
    mMutex.unlock();
    IMLOGD0("[processStart] exit");
}