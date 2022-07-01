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

static uint32_t sTimeout = 100;

VideoStreamGraphRtpTx::VideoStreamGraphRtpTx(BaseSessionCallback* callback, int localFd) :
        BaseStreamGraph(callback, localFd)
{
    mConfig = NULL;
    mSurface = NULL;
    mVideoMode = -1;
    mClosed = false;
}

VideoStreamGraphRtpTx::~VideoStreamGraphRtpTx()
{
    if (mConfig)
    {
        delete mConfig;
    }

    mClosed = true;
    mCondition.signal();
    mConditionExit.wait_timeout(sTimeout);
}

ImsMediaResult VideoStreamGraphRtpTx::create(void* config)
{
    IMLOGD0("[create]");
    if (config == NULL)
    {
        return RESULT_INVALID_PARAM;
    }

    VideoConfig* pConfig = reinterpret_cast<VideoConfig*>(config);

    if (pConfig->getVideoMode() == VideoConfig::VIDEO_MODE_PREVIEW)
    {
        return createPreviewMode(pConfig);
    }

    if (mConfig != NULL)
    {
        delete mConfig;
        mConfig = NULL;
    }

    mConfig = new VideoConfig(pConfig);

    char localIp[MAX_IP_LEN];
    uint32_t localPort = 0;
    ImsMediaNetworkUtil::GetLocalIPPortFromSocketFD(mLocalFd, localIp, MAX_IP_LEN, localPort);
    RtpAddress localAddress(localIp, localPort);

    BaseNode* pNodeSource = BaseNode::Load(BaseNodeID::NODEID_VIDEOSOURCE, mCallback);

    if (pNodeSource == NULL)
    {
        return RESULT_NOT_READY;
    }

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
    mVideoMode = pConfig->getVideoMode();

    return RESULT_SUCCESS;
}

ImsMediaResult VideoStreamGraphRtpTx::update(void* config)
{
    IMLOGD1("[update] mode[%d]", mVideoMode);
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

    ImsMediaResult ret = RESULT_NOT_READY;

    if (pConfig->getVideoMode() != mVideoMode &&
            (mVideoMode == VideoConfig::VIDEO_MODE_PREVIEW ||
                    pConfig->getVideoMode() == VideoConfig::VIDEO_MODE_PREVIEW))
    {
        ret = stop();
        if (ret != RESULT_SUCCESS)
        {
            return ret;
        }

        /** delete nodes */
        deleteNodes();
        mSurface = NULL;

        /** create nodes */
        ret = create(pConfig);
        if (ret != RESULT_SUCCESS)
        {
            return ret;
        }

        return start();
    }

    mConfig = new VideoConfig(pConfig);

    if (mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_NO_FLOW ||
            mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_RECEIVE_ONLY ||
            mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_INACTIVE)
    {
        IMLOGD0("[update] pause TX");
        return stop();
    }

    ret = RESULT_NOT_READY;

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
            (pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_SEND_ONLY ||
                    pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_SEND_RECEIVE))
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
        if (node != NULL && node->GetNodeID() == NODEID_VIDEOSOURCE)
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
            if (node != NULL && node->GetNodeID() == NODEID_VIDEOSOURCE)
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

ImsMediaResult VideoStreamGraphRtpTx::createPreviewMode(void* config)
{
    if (config == NULL)
    {
        return RESULT_INVALID_PARAM;
    }

    if (mConfig != NULL)
    {
        delete mConfig;
        mConfig = NULL;
    }

    IMLOGD0("[createPreviewMode]");

    mConfig = new VideoConfig(reinterpret_cast<VideoConfig*>(config));

    BaseNode* pNodeSource = BaseNode::Load(BaseNodeID::NODEID_VIDEOSOURCE, mCallback);

    if (pNodeSource == NULL)
    {
        return RESULT_NOT_READY;
    }

    pNodeSource->SetMediaType(IMS_MEDIA_VIDEO);
    pNodeSource->SetConfig(mConfig);
    AddNode(pNodeSource);
    setState(StreamState::kStreamStateCreated);
    mVideoMode = VideoConfig::VIDEO_MODE_PREVIEW;

    return RESULT_SUCCESS;
}

void VideoStreamGraphRtpTx::processStart()
{
    if (mConfig == NULL)
    {
        return;
    }

    IMLOGD0("[processStart]");
    mMutex.lock();
    mCondition.reset();

    VideoConfig* pConfig = reinterpret_cast<VideoConfig*>(mConfig);

    if (pConfig->getVideoMode() != VideoConfig::VIDEO_MODE_PREVIEW &&
            (pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_NO_FLOW ||
                    pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_RECEIVE_ONLY ||
                    pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_INACTIVE))
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
        mCallback->SendEvent(kImsMediaEventNotifyError, result, kStreamModeRtpTx);
    }

    setState(StreamState::kStreamStateRunning);
    mMutex.unlock();
    IMLOGD0("[processStart] exit");
}

void VideoStreamGraphRtpTx::OnEvent(int32_t type, uint64_t param1, uint64_t param2)
{
    IMLOGD3("[OnEvent] type[%d], param1[%d], param2[%d]", type, param1, param2);

    bool found = false;
    switch (type)
    {
        case kRequestVideoCvoUpdate:
            for (auto& node : mListNodeToStart)
            {
                if (node != NULL && node->GetNodeID() == NODEID_RTPENCODER)
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
                    if (node != NULL && node->GetNodeID() == NODEID_RTPENCODER)
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