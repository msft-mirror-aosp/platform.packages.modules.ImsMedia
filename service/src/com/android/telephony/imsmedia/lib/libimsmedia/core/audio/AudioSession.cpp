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

#include <AudioSession.h>
#include <ImsMediaTrace.h>
#include <ImsMediaEventHandler.h>
#include <ImsMediaAudioUtil.h>

#include <AudioConfig.h>

#include <string>

AudioSession::AudioSession()
{
    IMLOGD0("[AudioSession]");
    std::unique_ptr<MediaQualityAnalyzer> analyzer(new MediaQualityAnalyzer());
    mMediaQualityAnalyzer = std::move(analyzer);
    mMediaQualityAnalyzer->setCallback(this);
}

AudioSession::~AudioSession()
{
    IMLOGD0("[~AudioSession]");

    while (mListGraphRtpTx.size() > 0)
    {
        AudioStreamGraphRtpTx* graph = mListGraphRtpTx.front();

        if (graph->getState() == kStreamStateRunning)
        {
            graph->stop();
        }

        mListGraphRtpTx.pop_front();
        delete graph;
    }

    while (mListGraphRtpRx.size() > 0)
    {
        AudioStreamGraphRtpRx* graph = mListGraphRtpRx.front();

        if (graph->getState() == kStreamStateRunning)
        {
            graph->stop();
        }

        mListGraphRtpRx.pop_front();
        delete graph;
    }

    while (mListGraphRtcp.size() > 0)
    {
        AudioStreamGraphRtcp* graph = mListGraphRtcp.front();

        if (graph->getState() == kStreamStateRunning)
        {
            graph->stop();
        }

        mListGraphRtcp.pop_front();
        delete graph;
    }

    mMediaQualityAnalyzer->stop();
}

SessionState AudioSession::getState()
{
    SessionState state = kSessionStateOpened;

    for (auto& graph : mListGraphRtpTx)
    {
        if (graph != nullptr && graph->getState() == kStreamStateRunning)
        {
            return kSessionStateActive;
        }
    }

    for (auto& graph : mListGraphRtpRx)
    {
        if (graph != nullptr && graph->getState() == kStreamStateRunning)
        {
            return kSessionStateActive;
        }
    }

    for (auto& graph : mListGraphRtcp)
    {
        if (graph != nullptr && graph->getState() == kStreamStateRunning)
        {
            return kSessionStateSuspended;
        }
    }

    return state;
}

ImsMediaResult AudioSession::startGraph(RtpConfig* config)
{
    IMLOGI0("[startGraph]");

    if (config == nullptr)
    {
        return RESULT_INVALID_PARAM;
    }

    AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);

    if (std::strcmp(pConfig->getRemoteAddress().c_str(), "") == 0)
    {
        return RESULT_INVALID_PARAM;
    }

    ImsMediaResult ret = RESULT_NOT_READY;
    IMLOGD1("[startGraph] mListGraphRtpTx size[%d]", mListGraphRtpTx.size());

    if (mListGraphRtpTx.size() != 0)
    {
        for (auto& graph : mListGraphRtpTx)
        {
            if (graph != nullptr && graph->isSameGraph(config))
            {
                ret = graph->update(config);
                break;
            }
        }

        if (ret != RESULT_SUCCESS)
        {
            IMLOGE1("[startGraph] update error[%d]", ret);
            return ret;
        }
    }
    else
    {
        mListGraphRtpTx.push_back(new AudioStreamGraphRtpTx(this, mRtpFd));
        ret = mListGraphRtpTx.back()->create(config);

        if (ret == RESULT_SUCCESS &&
                (pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_SEND_ONLY ||
                        pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_SEND_RECEIVE))
        {
            ret = mListGraphRtpTx.back()->start();
            if (ret != RESULT_SUCCESS)
            {
                IMLOGE1("[startGraph] start error[%d]", ret);
                return ret;
            }
        }
    }

    IMLOGD1("[startGraph] mListGraphRtpRx size[%d]", mListGraphRtpRx.size());

    if (mListGraphRtpRx.size() != 0)
    {
        for (auto& graph : mListGraphRtpRx)
        {
            if (graph != nullptr && graph->isSameGraph(config))
            {
                graph->setMediaQualityThreshold(&mThreshold);
                ret = graph->update(config);
                break;
            }
        }

        if (ret != RESULT_SUCCESS)
        {
            IMLOGE1("[startGraph] update error[%d]", ret);
            return ret;
        }
    }
    else
    {
        mListGraphRtpRx.push_back(new AudioStreamGraphRtpRx(this, mRtpFd));
        ret = mListGraphRtpRx.back()->create(config);

        if (ret == RESULT_SUCCESS &&
                (pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_RECEIVE_ONLY ||
                        pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_SEND_RECEIVE))
        {
            mListGraphRtpRx.back()->setMediaQualityThreshold(&mThreshold);
            ret = mListGraphRtpRx.back()->start();

            if (ret != RESULT_SUCCESS)
            {
                IMLOGE1("[startGraph] start error[%d]", ret);
                return ret;
            }
        }
    }

    IMLOGD1("[startGraph] mListGraphRtcp size[%d]", mListGraphRtcp.size());

    if (mListGraphRtcp.size() != 0)
    {
        for (auto& graph : mListGraphRtcp)
        {
            if (graph != nullptr && graph->isSameGraph(config))
            {
                graph->setMediaQualityThreshold(&mThreshold);
                ret = graph->update(config);
                break;
            }
        }

        if (ret != RESULT_SUCCESS)
        {
            IMLOGE1("[startGraph] update error[%d]", ret);
            return ret;
        }
    }
    else
    {
        mListGraphRtcp.push_back(new AudioStreamGraphRtcp(this, mRtcpFd));
        ret = mListGraphRtcp.back()->create(config);

        if (ret == RESULT_SUCCESS)
        {
            mListGraphRtcp.back()->setMediaQualityThreshold(&mThreshold);
            ret = mListGraphRtcp.back()->start();
            if (ret != RESULT_SUCCESS)
            {
                IMLOGE1("[startGraph] start error[%d]", ret);
                return ret;
            }
        }
    }

    // TODO : check that the timing is correct
    IMLOGI1("[startGraph] state[%d]", getState());

    if (mMediaQualityAnalyzer != nullptr &&
            !mMediaQualityAnalyzer->isSameConfig(reinterpret_cast<AudioConfig*>(config)))
    {
        mMediaQualityAnalyzer->stop();
        mMediaQualityAnalyzer->setConfig(reinterpret_cast<AudioConfig*>(config));
        mMediaQualityAnalyzer->start();
    }

    return ret;
}

ImsMediaResult AudioSession::addGraph(RtpConfig* config, bool enableRtcp)
{
    IMLOGD1("[addGraph], enable rtcp[%d]", enableRtcp);

    if (config == nullptr || std::strcmp(config->getRemoteAddress().c_str(), "") == 0)
    {
        return RESULT_INVALID_PARAM;
    }

    for (auto& graph : mListGraphRtpTx)
    {
        if (graph != nullptr && graph->isSameGraph(config))
        {
            IMLOGW0("[addGraph] same config is exist");
            return startGraph(config);
        }
    }

    for (auto& graph : mListGraphRtpTx)
    {
        if (graph != nullptr)
        {
            graph->stop();
        }
    }

    for (auto& graph : mListGraphRtpRx)
    {
        if (graph != nullptr)
        {
            graph->stop();
        }
    }

    for (auto& graph : mListGraphRtcp)
    {
        if (graph != nullptr && graph->getState() != kStreamStateRunning)
        {
            enableRtcp ? graph->start() : graph->stop();
        }
    }

    ImsMediaResult ret = RESULT_NOT_READY;

    mListGraphRtpTx.push_back(new AudioStreamGraphRtpTx(this, mRtpFd));
    ret = mListGraphRtpTx.back()->create(config);

    if (ret == RESULT_SUCCESS)
    {
        ret = mListGraphRtpTx.back()->start();
        if (ret != RESULT_SUCCESS)
        {
            IMLOGE1("[addGraph] start error[%d]", ret);
            return ret;
        }
    }

    IMLOGD1("[addGraph] mListGraphTx size[%d]", mListGraphRtpTx.size());

    mListGraphRtpRx.push_back(new AudioStreamGraphRtpRx(this, mRtpFd));
    ret = mListGraphRtpRx.back()->create(config);

    if (ret == RESULT_SUCCESS)
    {
        mListGraphRtpRx.back()->setMediaQualityThreshold(&mThreshold);
        ret = mListGraphRtpRx.back()->start();
        if (ret != RESULT_SUCCESS)
        {
            IMLOGE1("[addGraph] start error[%d]", ret);
            return ret;
        }
    }

    IMLOGD1("[addGraph] mListGraphRx size[%d]", mListGraphRtpRx.size());

    mListGraphRtcp.push_back(new AudioStreamGraphRtcp(this, mRtcpFd));
    ret = mListGraphRtcp.back()->create(config);

    if (ret == RESULT_SUCCESS)
    {
        mListGraphRtcp.back()->setMediaQualityThreshold(&mThreshold);
        ret = mListGraphRtcp.back()->start();
        if (ret != RESULT_SUCCESS)
        {
            IMLOGE1("[addGraph] start error[%d]", ret);
            return ret;
        }
    }

    IMLOGD1("[addGraph] mListGraphRtcp size[%d]", mListGraphRtcp.size());

    if (mMediaQualityAnalyzer != nullptr &&
            !mMediaQualityAnalyzer->isSameConfig(reinterpret_cast<AudioConfig*>(config)))
    {
        mMediaQualityAnalyzer->stop();
        mMediaQualityAnalyzer->setConfig(reinterpret_cast<AudioConfig*>(config));
        mMediaQualityAnalyzer->start();
    }

    return RESULT_SUCCESS;
}

ImsMediaResult AudioSession::confirmGraph(RtpConfig* config)
{
    if (config == nullptr || std::strcmp(config->getRemoteAddress().c_str(), "") == 0)
    {
        return RESULT_INVALID_PARAM;
    }

    ImsMediaResult ret = RESULT_NOT_READY;

    /** Stop unmatched running instances of StreamGraph. */
    for (auto& graph : mListGraphRtpTx)
    {
        if (graph != nullptr && !graph->isSameGraph(config))
        {
            graph->stop();
        }
    }

    for (auto& graph : mListGraphRtpRx)
    {
        if (graph != nullptr && !graph->isSameGraph(config))
        {
            graph->stop();
        }
    }

    for (auto& graph : mListGraphRtcp)
    {
        if (graph != nullptr && !graph->isSameGraph(config))
        {
            graph->stop();
        }
    }

    bool bFound = false;
    for (std::list<AudioStreamGraphRtpTx*>::iterator iter = mListGraphRtpTx.begin();
            iter != mListGraphRtpTx.end();)
    {
        AudioStreamGraphRtpTx* graph = *iter;

        if (graph == nullptr)
        {
            continue;
        }

        if (!graph->isSameGraph(config))
        {
            iter = mListGraphRtpTx.erase(iter);
            delete graph;
        }
        else
        {
            if (graph->getState() != kStreamStateRunning)
            {
                ret = graph->start();
                if (ret != RESULT_SUCCESS)
                {
                    IMLOGE1("[confirmGraph] start tx error[%d]", ret);
                    return ret;
                }
            }
            iter++;
            bFound = true;
        }
    }

    IMLOGD1("[confirmGraph] mListGraphTx size[%d]", mListGraphRtpTx.size());

    if (bFound == false)
    {
        IMLOGE0("[confirmGraph] no graph to confirm");
        return RESULT_INVALID_PARAM;
    }

    for (std::list<AudioStreamGraphRtpRx*>::iterator iter = mListGraphRtpRx.begin();
            iter != mListGraphRtpRx.end();)
    {
        AudioStreamGraphRtpRx* graph = *iter;

        if (graph == nullptr)
        {
            continue;
        }

        if (!graph->isSameGraph(config))
        {
            iter = mListGraphRtpRx.erase(iter);
            delete graph;
        }
        else
        {
            if (graph->getState() != kStreamStateRunning)
            {
                ret = graph->start();
                if (ret != RESULT_SUCCESS)
                {
                    IMLOGE1("[confirmGraph] start rx error[%d]", ret);
                    return ret;
                }
            }
            iter++;
        }
    }

    IMLOGD1("[confirmGraph] mListGraphRx size[%d]", mListGraphRtpRx.size());

    for (std::list<AudioStreamGraphRtcp*>::iterator iter = mListGraphRtcp.begin();
            iter != mListGraphRtcp.end();)
    {
        AudioStreamGraphRtcp* graph = *iter;

        if (graph == nullptr)
        {
            continue;
        }

        if (!graph->isSameGraph(config))
        {
            iter = mListGraphRtcp.erase(iter);
            delete graph;
        }
        else
        {
            if (graph->getState() != kStreamStateRunning)
            {
                ret = graph->start();
                if (ret != RESULT_SUCCESS)
                {
                    IMLOGE1("[confirmGraph] start rtcp error[%d]", ret);
                    return ret;
                }
            }

            iter++;
        }
    }

    IMLOGD1("[confirmGraph] mListGraphRtcp size[%d]", mListGraphRtcp.size());

    return RESULT_SUCCESS;
}

ImsMediaResult AudioSession::deleteGraph(RtpConfig* config)
{
    IMLOGI0("[deleteGraph]");
    bool bFound = false;

    for (std::list<AudioStreamGraphRtpTx*>::iterator iter = mListGraphRtpTx.begin();
            iter != mListGraphRtpTx.end();)
    {
        AudioStreamGraphRtpTx* graph = *iter;

        if (graph == nullptr)
        {
            continue;
        }

        if (graph->isSameGraph(config))
        {
            if (graph->getState() == kStreamStateRunning)
            {
                graph->stop();
            }

            iter = mListGraphRtpTx.erase(iter);
            delete graph;
            bFound = true;
            break;
        }
        else
        {
            iter++;
        }
    }

    if (bFound == false)
    {
        return RESULT_INVALID_PARAM;
    }

    IMLOGD1("[deleteGraph] mListGraphRtpTx size[%d]", mListGraphRtpTx.size());

    for (std::list<AudioStreamGraphRtpRx*>::iterator iter = mListGraphRtpRx.begin();
            iter != mListGraphRtpRx.end();)
    {
        AudioStreamGraphRtpRx* graph = *iter;

        if (graph == nullptr)
        {
            continue;
        }

        if (graph->isSameGraph(config))
        {
            if (graph->getState() == kStreamStateRunning)
            {
                graph->stop();
            }
            iter = mListGraphRtpRx.erase(iter);
            delete graph;
            break;
        }
        else
        {
            iter++;
        }
    }

    IMLOGD1("[deleteGraph] mListGraphRtpRx size[%d]", mListGraphRtpRx.size());

    for (std::list<AudioStreamGraphRtcp*>::iterator iter = mListGraphRtcp.begin();
            iter != mListGraphRtcp.end();)
    {
        AudioStreamGraphRtcp* graph = *iter;

        if (graph == nullptr)
        {
            continue;
        }

        if (graph->isSameGraph(config))
        {
            if (graph->getState() == kStreamStateRunning)
            {
                graph->stop();
            }
            iter = mListGraphRtcp.erase(iter);
            delete graph;
            break;
        }
        else
        {
            iter++;
        }
    }

    IMLOGD1("[deleteGraph] mListGraphRtcp size[%d]", mListGraphRtcp.size());
    return RESULT_SUCCESS;
}

void AudioSession::onEvent(int32_t type, uint64_t param1, uint64_t param2)
{
    switch (type)
    {
        case kImsMediaEventStateChanged:
            if (mState != getState())
            {
                mState = getState();
            }
            break;
        case kImsMediaEventNotifyError:
            break;
        case kImsMediaEventFirstPacketReceived:
            ImsMediaEventHandler::SendEvent(
                    "AUDIO_RESPONSE_EVENT", kAudioFirstMediaPacketInd, mSessionId, param1, param2);
            break;
        case kImsMediaEventHeaderExtensionReceived:
            ImsMediaEventHandler::SendEvent(
                    "AUDIO_RESPONSE_EVENT", kAudioRtpHeaderExtensionInd, 0, 0);
            break;
        case kImsMediaEventMediaInactivity:
            ImsMediaEventHandler::SendEvent(
                    "AUDIO_RESPONSE_EVENT", kAudioMediaInactivityInd, mSessionId, param1, param2);
            break;
        case kImsMediaEventPacketLoss:
            ImsMediaEventHandler::SendEvent(
                    "AUDIO_RESPONSE_EVENT", kAudioPacketLossInd, mSessionId, param1);
            break;
        case kImsMediaEventNotifyJitter:
            ImsMediaEventHandler::SendEvent(
                    "AUDIO_RESPONSE_EVENT", kAudioJitterInd, mSessionId, param1);
            break;
        case kAudioTriggerAnbrQueryInd:
        case kAudioDtmfReceivedInd:
            /** TODO: add implementation */
            break;
        case kAudioCallQualityChangedInd:
            ImsMediaEventHandler::SendEvent(
                    "AUDIO_RESPONSE_EVENT", kAudioCallQualityChangedInd, mSessionId, param1);
            break;
        case kRequestAudioCmr:
        case kRequestSendRtcpXrReport:
            ImsMediaEventHandler::SendEvent(
                    "AUDIO_REQUEST_EVENT", type, mSessionId, param1, param2);
            break;
        case kRequestRoundTripTimeDelayUpdate:
        case kCollectPacketInfo:
        case kCollectOptionalInfo:
        case kCollectRxRtpStatus:
        case kCollectJitterBufferSize:
        case kGetRtcpXrReportBlock:
            if (mMediaQualityAnalyzer != nullptr)
            {
                mMediaQualityAnalyzer->SendEvent(type, param1, param2);
            }
            break;
        default:
            break;
    }
}

void AudioSession::setMediaQualityThreshold(const MediaQualityThreshold& threshold)
{
    IMLOGI0("[setMediaQualityThreshold]");
    mThreshold = threshold;

    for (auto& graph : mListGraphRtpRx)
    {
        if (graph != nullptr && graph->getState() == kStreamStateRunning)
        {
            graph->setMediaQualityThreshold(&mThreshold);
        }
    }

    for (auto& graph : mListGraphRtcp)
    {
        if (graph != nullptr && graph->getState() == kStreamStateRunning)
        {
            graph->setMediaQualityThreshold(&mThreshold);
        }
    }

    if (mMediaQualityAnalyzer != nullptr)
    {
        mMediaQualityAnalyzer->setJitterThreshold(
                mThreshold.getJitterDurationMillis() / 1000,  // milliseconds to seconds
                mThreshold.getRtpJitterMillis());
        mMediaQualityAnalyzer->setPacketLossThreshold(
                mThreshold.getRtpPacketLossDurationMillis() / 1000,  // milliseconds to seconds
                mThreshold.getRtpPacketLossRate());
    }
}

void AudioSession::sendDtmf(char digit, int duration)
{
    for (std::list<AudioStreamGraphRtpTx*>::iterator iter = mListGraphRtpTx.begin();
            iter != mListGraphRtpTx.end(); iter++)
    {
        AudioStreamGraphRtpTx* graph = *iter;

        if (graph != nullptr && graph->getState() == kStreamStateRunning)
        {
            graph->sendDtmf(digit, duration);
        }
    }
}

bool AudioSession::IsGraphAlreadyExist(RtpConfig* config)
{
    if (mListGraphRtpTx.size() != 0)
    {
        for (auto& graph : mListGraphRtpTx)
        {
            if (graph != nullptr && graph->isSameGraph(config))
            {
                return true;
            }
        }
    }

    return false;
}

void AudioSession::SendInternalEvent(int32_t type, uint64_t param1, uint64_t param2)
{
    (void)param2;

    switch (type)
    {
        case kRequestAudioCmr:
            for (std::list<AudioStreamGraphRtpTx*>::iterator iter = mListGraphRtpTx.begin();
                    iter != mListGraphRtpTx.end(); iter++)
            {
                AudioStreamGraphRtpTx* graph = *iter;

                if (graph != nullptr && graph->getState() == kStreamStateRunning)
                {
                    graph->processCmr(static_cast<uint32_t>(param1));
                }
            }
            break;
        case kRequestSendRtcpXrReport:
            for (std::list<AudioStreamGraphRtcp*>::iterator iter = mListGraphRtcp.begin();
                    iter != mListGraphRtcp.end(); iter++)
            {
                AudioStreamGraphRtcp* graph = *iter;

                if (graph != nullptr && graph->getState() == kStreamStateRunning)
                {
                    graph->OnEvent(type, param1, param2);
                }
            }
            break;
        default:
            break;
    }
}