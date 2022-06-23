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

#include <IVideoSourceNode.h>
#include <ImsMediaVideoSource.h>
#include <ImsMediaTrace.h>
#include <VideoConfig.h>
#include <string.h>

using namespace android::telephony::imsmedia;

#define DEFAULT_UNDEFINE -1

IVideoSourceNode::IVideoSourceNode()
{
    std::unique_ptr<ImsMediaVideoSource> source(new ImsMediaVideoSource());
    mVideoSource = std::move(source);
    mVideoSource->SetListener(this);
    mCodecType = DEFAULT_UNDEFINE;
    mVideoMode = DEFAULT_UNDEFINE;
    mCodecProfile = VideoConfig::CODEC_PROFILE_NONE;
    mCodecLevel = VideoConfig::CODEC_LEVEL_NONE;
    mCameraId = 0;
    mCameraZoom = 0;
    mWidth = 0;
    mHeight = 0;
    mFramerate = DEFAULT_FRAMERATE;
    mBitrate = DEFAULT_BITRATE;
    mSamplingRate = 0;
    mIntraInterval = 1;
    mImagePath = "";
    mDeviceOrientation = 0;
    mWindow = NULL;
}

IVideoSourceNode::~IVideoSourceNode() {}

BaseNode* IVideoSourceNode::GetInstance()
{
    return new IVideoSourceNode();
}

void IVideoSourceNode::ReleaseInstance(BaseNode* pNode)
{
    delete (IVideoSourceNode*)pNode;
}

BaseNodeID IVideoSourceNode::GetNodeID()
{
    return BaseNodeID::NODEID_VIDEOSOURCE;
}

ImsMediaResult IVideoSourceNode::Start()
{
    IMLOGD2("[Start] codec[%d], mode[%d]", mCodecType, mVideoMode);
    std::lock_guard<std::mutex> guard(mMutex);
    if (mVideoSource)
    {
        mVideoSource->SetCodecConfig(
                mCodecType, mCodecProfile, mCodecLevel, mBitrate, mFramerate, mIntraInterval);
        mVideoSource->SetVideoMode(mVideoMode);
        mVideoSource->SetResolution(mWidth, mHeight);
        if (mVideoMode == VideoConfig::VIDEO_MODE_PREVIEW ||
                mVideoMode == VideoConfig::VIDEO_MODE_RECORDING)
        {
            mVideoSource->SetCameraConfig(mCameraId, mCameraZoom);

            if (mWindow == NULL)
            {
                IMLOGE0("[Start] surface is not ready");
                return RESULT_NOT_READY;
            }
        }
        else
        {
            mVideoSource->SetImagePath(mImagePath);
        }
        mVideoSource->SetSurface(mWindow);
        if (mVideoSource->Start() == false)
        {
            return RESULT_NOT_READY;
        }
        mVideoSource->SetDeviceOrientation(mDeviceOrientation);
    }
    mNodeState = kNodeStateRunning;
    return RESULT_SUCCESS;
}

void IVideoSourceNode::Stop()
{
    IMLOGD0("[Stop]");
    std::lock_guard<std::mutex> guard(mMutex);
    if (mVideoSource)
    {
        mVideoSource->Stop();
    }
    mNodeState = kNodeStateStopped;
}

bool IVideoSourceNode::IsRunTime()
{
    return true;
}

bool IVideoSourceNode::IsSourceNode()
{
    return true;
}

void IVideoSourceNode::SetConfig(void* config)
{
    if (config == NULL)
        return;
    VideoConfig* pConfig = reinterpret_cast<VideoConfig*>(config);
    mCodecType = ImsMediaVideoUtil::ConvertCodecType(pConfig->getCodecType());
    mVideoMode = pConfig->getVideoMode();
    mSamplingRate = pConfig->getSamplingRateKHz();
    mCodecProfile = pConfig->getCodecProfile();
    mCodecLevel = pConfig->getCodecLevel();
    mFramerate = pConfig->getFramerate();
    mBitrate = pConfig->getBitrate();
    mWidth = pConfig->getResolutionWidth();
    mHeight = pConfig->getResolutionHeight();
    mIntraInterval = pConfig->getIntraFrameInterval();
    if (mVideoMode == VideoConfig::VIDEO_MODE_PREVIEW ||
            mVideoMode == VideoConfig::VIDEO_MODE_RECORDING)
    {
        mCameraId = pConfig->getCameraId();
        mCameraZoom = pConfig->getCameraZoom();
    }
    else
    {
        mImagePath = pConfig->getPauseImagePath();
    }

    mDeviceOrientation = pConfig->getDeviceOrientationDegree();
}

bool IVideoSourceNode::IsSameConfig(void* config)
{
    if (config == NULL)
    {
        return true;
    }

    VideoConfig* pConfig = reinterpret_cast<VideoConfig*>(config);

    return (mCodecType == ImsMediaVideoUtil::ConvertCodecType(pConfig->getCodecType()) &&
            mVideoMode == pConfig->getVideoMode() &&
            mSamplingRate == pConfig->getSamplingRateKHz() &&
            mCodecProfile == pConfig->getCodecProfile() && mCodecLevel == pConfig->getVideoMode() &&
            mFramerate == pConfig->getFramerate() && mBitrate == pConfig->getBitrate() &&
            mCameraId == pConfig->getCameraId() && mCameraZoom == pConfig->getCameraZoom() &&
            mWidth == pConfig->getResolutionWidth() && mHeight == pConfig->getResolutionHeight() &&
            mDeviceOrientation == pConfig->getDeviceOrientationDegree());
}

ImsMediaResult IVideoSourceNode::UpdateConfig(void* config)
{
    IMLOGD0("[UpdateConfig]");

    if (config == NULL)
    {
        return RESULT_INVALID_PARAM;
    }

    bool isRestart = false;
    if (IsSameConfig(config))
    {
        IMLOGD0("[UpdateConfig] no update");
        return RESULT_SUCCESS;
    }

    VideoConfig* pConfig = reinterpret_cast<VideoConfig*>(config);

    if (mCodecType != pConfig->getCodecType() || mCodecProfile != pConfig->getCodecProfile() ||
            mCodecLevel != pConfig->getCodecLevel() || mFramerate != pConfig->getFramerate() ||
            mCameraId != pConfig->getCameraId() || mWidth != pConfig->getResolutionWidth() ||
            mHeight != pConfig->getResolutionHeight())
    {
        isRestart = true;
    }
    else
    {
        if (mBitrate != pConfig->getBitrate())
        {
            // TODO : bitrate change
        }

        if (mDeviceOrientation != pConfig->getDeviceOrientationDegree())
        {
            mDeviceOrientation = pConfig->getDeviceOrientationDegree();
            mVideoSource->SetDeviceOrientation(mDeviceOrientation);
        }

        return RESULT_SUCCESS;
    }

    if (isRestart)
    {
        kBaseNodeState prevState = mNodeState;
        if (mNodeState == kNodeStateRunning)
        {
            Stop();
        }

        // reset the parameters
        SetConfig(config);

        if (prevState == kNodeStateRunning)
        {
            return Start();
        }
    }

    return RESULT_SUCCESS;
}

void IVideoSourceNode::UpdateSurface(ANativeWindow* window)
{
    IMLOGD1("[UpdateSurface] surface[%p]", window);
    mWindow = window;
}

void IVideoSourceNode::OnUplinkEvent(
        uint8_t* buffer, uint32_t size, int64_t timestamp, uint32_t flag)
{
    (void)flag;
    IMLOGD_PACKET2(
            IM_PACKET_LOG_VIDEO, "[OnUplinkEvent] size[%zu], timestamp[%ld]", size, timestamp);
    SendDataToRearNode(
            MEDIASUBTYPE_UNDEFINED, buffer, size, timestamp, true, MEDIASUBTYPE_UNDEFINED);
}

void IVideoSourceNode::OnEvent(int32_t type, int32_t param1, int32_t param2)
{
    IMLOGD1("[OnEvent] type[%d]", type);

    if (mCallback == NULL)
    {
        IMLOGE0("[OnEvent] callback is null");
        return;
    }

    switch (type)
    {
        case kVideoSourceEventUpdateCamera:
            mCallback->SendEvent(kRequestVideoCvoUpdate, param1, param2);
            break;
        case kVideoSourceEventCameraError:
            mCallback->SendEvent(kImsMediaEventNotifyError, param1, param2);
            break;
        default:
            break;
    }
}