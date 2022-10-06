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

#include <VideoSession.h>
#include <ImsMediaTrace.h>
#include <ImsMediaEventHandler.h>
#include <VideoConfig.h>
#include <string>
#include <sys/socket.h>

VideoSession::VideoSession()
{
    IMLOGD0("[VideoSession]");
    mGraphRtpTx = NULL;
    mGraphRtpRx = NULL;
    mGraphRtcp = NULL;
    mPreviewSurface = NULL;
    mDisplaySurface = NULL;
}

VideoSession::~VideoSession()
{
    IMLOGD0("[~VideoSession]");

    if (mGraphRtpTx != NULL)
    {
        if (mGraphRtpTx->getState() == kStreamStateRunning)
        {
            mGraphRtpTx->stop();
        }

        delete mGraphRtpTx;
    }

    if (mGraphRtpRx != NULL)
    {
        if (mGraphRtpRx->getState() == kStreamStateRunning)
        {
            mGraphRtpRx->stop();
        }

        delete mGraphRtpRx;
    }

    if (mGraphRtcp != NULL)
    {
        if (mGraphRtcp->getState() == kStreamStateRunning)
        {
            mGraphRtcp->stop();
        }

        delete mGraphRtcp;
    }
}

SessionState VideoSession::getState()
{
    SessionState state = kSessionStateOpened;

    if ((mGraphRtpTx != NULL && mGraphRtpTx->getState() == kStreamStateWaitSurface) ||
            (mGraphRtpRx != NULL && mGraphRtpRx->getState() == kStreamStateWaitSurface))
    {
        return kSessionStateSuspended;
    }
    else if ((mGraphRtpTx != NULL && mGraphRtpTx->getState() == kStreamStateRunning) ||
            (mGraphRtpRx != NULL && mGraphRtpRx->getState() == kStreamStateRunning))
    {
        return kSessionStateActive;
    }

    if (mGraphRtcp != NULL && mGraphRtcp->getState() == kStreamStateRunning)
    {
        return kSessionStateSuspended;
    }

    return state;
}

ImsMediaResult VideoSession::startGraph(RtpConfig* config)
{
    IMLOGD0("[startGraph]");

    if (config == NULL)
    {
        return RESULT_INVALID_PARAM;
    }

    VideoConfig* pConfig = reinterpret_cast<VideoConfig*>(config);
    ImsMediaResult ret = RESULT_NOT_READY;

    if (mGraphRtpTx != NULL)
    {
        ret = mGraphRtpTx->update(config);

        if (ret != RESULT_SUCCESS)
        {
            IMLOGE1("[startGraph] update error[%d]", ret);
            return ret;
        }

        if (mPreviewSurface != NULL)
        {
            mGraphRtpTx->setSurface(mPreviewSurface);
        }
    }
    else
    {
        mGraphRtpTx = new VideoStreamGraphRtpTx(this, mRtpFd);
        ret = mGraphRtpTx->create(config);

        if (ret == RESULT_SUCCESS)
        {
            if (pConfig->getVideoMode() == VideoConfig::VIDEO_MODE_PREVIEW)
            {
                ret = mGraphRtpTx->start();
            }
            else
            {
                if (pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_SEND_ONLY ||
                        pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_SEND_RECEIVE)
                {
                    ret = mGraphRtpTx->start();
                }
            }

            if (ret != RESULT_SUCCESS)
            {
                IMLOGE1("[startGraph] start error[%d]", ret);
                return ret;
            }

            if (mPreviewSurface != NULL)
            {
                mGraphRtpTx->setSurface(mPreviewSurface);
            }
        }
    }

    if (pConfig->getVideoMode() == VideoConfig::VIDEO_MODE_PREVIEW &&
            std::strcmp(pConfig->getRemoteAddress().c_str(), "") == 0)
    {
        return RESULT_SUCCESS;
    }

    if (mGraphRtpRx != NULL)
    {
        mGraphRtpRx->setMediaQualityThreshold(&mThreshold);
        ret = mGraphRtpRx->update(config);

        if (ret != RESULT_SUCCESS)
        {
            IMLOGE1("[startGraph] update error[%d]", ret);
            return ret;
        }

        if (mDisplaySurface != NULL)
        {
            mGraphRtpRx->setSurface(mDisplaySurface);
        }
    }
    else
    {
        mGraphRtpRx = new VideoStreamGraphRtpRx(this, mRtpFd);
        ret = mGraphRtpRx->create(config);

        if (ret == RESULT_SUCCESS &&
                (pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_RECEIVE_ONLY ||
                        pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_SEND_RECEIVE))
        {
            mGraphRtpRx->setMediaQualityThreshold(&mThreshold);
            ret = mGraphRtpRx->start();

            if (ret != RESULT_SUCCESS)
            {
                IMLOGE1("[startGraph] start error[%d]", ret);
                return ret;
            }

            if (mDisplaySurface != NULL)
            {
                mGraphRtpRx->setSurface(mDisplaySurface);
            }
        }
    }

    if (mGraphRtcp != NULL)
    {
        mGraphRtcp->setMediaQualityThreshold(&mThreshold);
        ret = mGraphRtcp->update(config);

        if (ret != RESULT_SUCCESS)
        {
            IMLOGE1("[startGraph] update error[%d]", ret);
            return ret;
        }
    }
    else
    {
        mGraphRtcp = new VideoStreamGraphRtcp(this, mRtcpFd);
        ret = mGraphRtcp->create(config);

        if (ret == RESULT_SUCCESS)
        {
            mGraphRtcp->setMediaQualityThreshold(&mThreshold);
            ret = mGraphRtcp->start();
            if (ret != RESULT_SUCCESS)
            {
                IMLOGE1("[startGraph] start error[%d]", ret);
                return ret;
            }
        }
    }

    return ret;
}

void VideoSession::onEvent(int32_t type, uint64_t param1, uint64_t param2)
{
    IMLOGD3("[onEvent] type[%d], param1[%d], param2[%d]", type, param1, param2);

    switch (type)
    {
        case kImsMediaEventNotifyError:
            /** TODO: need to add to send error to the client */
            break;
        case kImsMediaEventStateChanged:
            if (mState != getState())
            {
                mState = getState();
            }
            break;
        case kImsMediaEventFirstPacketReceived:
            ImsMediaEventHandler::SendEvent(
                    "VIDEO_RESPONSE_EVENT", kVideoFirstMediaPacketInd, param1, param2);
            break;
        case kImsMediaEventResolutionChanged:
            ImsMediaEventHandler::SendEvent(
                    "VIDEO_RESPONSE_EVENT", kVideoPeerDimensionChanged, mSessionId, param1, param2);
            break;
        case kImsMediaEventHeaderExtensionReceived:
            ImsMediaEventHandler::SendEvent("VIDEO_RESPONSE_EVENT", kVideoRtpHeaderExtensionInd,
                    mSessionId, param1, param2);
            break;
        case kImsMediaEventMediaInactivity:
            ImsMediaEventHandler::SendEvent(
                    "VIDEO_RESPONSE_EVENT", kVideoMediaInactivityInd, mSessionId, param1, param2);
            break;
        case kImsMediaEventPacketLoss:
            ImsMediaEventHandler::SendEvent(
                    "VIDEO_RESPONSE_EVENT", kVideoPacketLossInd, mSessionId, param1, param2);
            break;
        case kImsMediaEventNotifyVideoDataUsage:
            ImsMediaEventHandler::SendEvent(
                    "VIDEO_RESPONSE_EVENT", kVideoDataUsageInd, mSessionId, param1, param2);
            break;
        case kRequestVideoCvoUpdate:
        case kRequestVideoBitrateChange:
        case kRequestVideoIdrFrame:
        case kRequestVideoSendNack:
        case kRequestVideoSendPictureLost:
        case kRequestVideoSendTmmbr:
        case kRequestVideoSendTmmbn:
        case kRequestRoundTripTimeDelayUpdate:
            ImsMediaEventHandler::SendEvent(
                    "VIDEO_REQUEST_EVENT", type, mSessionId, param1, param2);
            break;
        default:
            break;
    }
}

ImsMediaResult VideoSession::setPreviewSurface(ANativeWindow* surface)
{
    if (surface == NULL)
    {
        return RESULT_INVALID_PARAM;
    }

    mPreviewSurface = surface;

    if (mGraphRtpTx != NULL)
    {
        mGraphRtpTx->setSurface(surface);
    }

    return RESULT_SUCCESS;
}

ImsMediaResult VideoSession::setDisplaySurface(ANativeWindow* surface)
{
    if (surface == NULL)
    {
        return RESULT_INVALID_PARAM;
    }

    mDisplaySurface = surface;

    if (mGraphRtpRx != NULL)
    {
        mGraphRtpRx->setSurface(surface);
    }

    return RESULT_SUCCESS;
}

void VideoSession::SendInternalEvent(int32_t type, uint64_t param1, uint64_t param2)
{
    IMLOGD3("[SendInternalEvent] type[%d], param1[%d], param2[%d]", type, param1, param2);

    switch (type)
    {
        case kRequestVideoCvoUpdate:
        case kRequestVideoBitrateChange:
        case kRequestVideoIdrFrame:
            if (mGraphRtpTx != NULL)
            {
                if (!mGraphRtpTx->OnEvent(type, param1, param2))
                {
                    IMLOGE0("[SendInternalEvent] fail to send event");
                }
            }
            break;
        case kRequestVideoSendNack:
        case kRequestVideoSendPictureLost:
        case kRequestVideoSendTmmbr:
        case kRequestVideoSendTmmbn:
            if (mGraphRtcp != NULL)
            {
                if (!mGraphRtcp->OnEvent(type, param1, param2))
                {
                    IMLOGE0("[SendInternalEvent] fail to send event");
                }
            }
            break;
        case kRequestRoundTripTimeDelayUpdate:
            if (mGraphRtpRx != NULL)
            {
                if (!mGraphRtpRx->OnEvent(type, param1, param2))
                {
                    IMLOGE0("[SendInternalEvent] fail to send event");
                }
            }
            break;
        default:
            break;
    }
}