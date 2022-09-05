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

#include <AudioManager.h>
#include <ImsMediaTrace.h>
#include <ImsMediaNetworkUtil.h>

using namespace android;

AudioManager* AudioManager::sManager = NULL;

AudioManager::AudioManager() {}

AudioManager::~AudioManager() {}

AudioManager* AudioManager::getInstance()
{
    if (sManager == NULL)
    {
        sManager = new AudioManager();
    }

    return sManager;
}

int AudioManager::getState(int sessionId)
{
    auto session = mSessions.find(sessionId);
    if (session != mSessions.end())
    {
        return (session->second)->getState();
    }
    else
    {
        return kSessionStateClosed;
    }
}

ImsMediaResult AudioManager::openSession(int sessionId, int rtpFd, int rtcpFd, AudioConfig* config)
{
    IMLOGD1("[openSession] sessionId[%d]", sessionId);

    // set debug log
    ImsMediaTrace::IMSetDebugLog(ImsMediaTrace::IMGetDebugLog() | IM_PACKET_LOG_RTPSTACK);

    if (rtpFd == -1 || rtcpFd == -1)
    {
        return RESULT_INVALID_PARAM;
    }

    if (!mSessions.count(sessionId))
    {
        AudioSession* session = new AudioSession();
        session->setSessionId(sessionId);
        session->setLocalEndPoint(rtpFd, rtcpFd);
        mSessions.insert(std::make_pair(sessionId, std::move(session)));
        ImsMediaResult ret = session->startGraph(config);

        if (ret != RESULT_SUCCESS)
        {
            IMLOGD1("[openSession] startGraph failed[%d]", ret);
        }
    }
    else
    {
        return RESULT_INVALID_PARAM;
    }

    return RESULT_SUCCESS;
}

ImsMediaResult AudioManager::closeSession(int sessionId)
{
    IMLOGD1("closeSession() - sessionId[%d]", sessionId);
    if (mSessions.count(sessionId))
    {
        mSessions.erase(sessionId);
        return RESULT_SUCCESS;
    }
    return RESULT_INVALID_PARAM;
}

ImsMediaResult AudioManager::modifySession(int sessionId, AudioConfig* config)
{
    auto session = mSessions.find(sessionId);
    IMLOGD1("modifySession() - sessionId[%d]", sessionId);
    if (session != mSessions.end())
    {
        return (session->second)->startGraph(config);
    }
    else
    {
        IMLOGE1("modifySession() - no session id[%d]", sessionId);
        return RESULT_INVALID_PARAM;
    }
}

ImsMediaResult AudioManager::addConfig(int sessionId, AudioConfig* config)
{
    auto session = mSessions.find(sessionId);
    IMLOGD1("addConfig() - sessionId[%d]", sessionId);
    if (session != mSessions.end())
    {
        return (session->second)->addGraph(config);
    }
    else
    {
        IMLOGE1("addConfig() - no session id[%d]", sessionId);
        return RESULT_INVALID_PARAM;
    }
}

ImsMediaResult AudioManager::deleteConfig(int sessionId, AudioConfig* config)
{
    auto session = mSessions.find(sessionId);
    IMLOGD1("deleteConfig() - sessionId[%d]", sessionId);
    if (session != mSessions.end())
    {
        return (session->second)->deleteGraph(config);
    }
    else
    {
        IMLOGE1("deleteConfig() - no session id[%d]", sessionId);
        return RESULT_INVALID_PARAM;
    }
}

ImsMediaResult AudioManager::confirmConfig(int sessionId, AudioConfig* config)
{
    auto session = mSessions.find(sessionId);
    IMLOGD1("confirmConfig() - sessionId[%d]", sessionId);
    if (session != mSessions.end())
    {
        return (session->second)->confirmGraph(config);
    }
    else
    {
        IMLOGE1("confirmConfig() - no session id[%d]", sessionId);
        return RESULT_INVALID_PARAM;
    }
}

void AudioManager::sendDtmf(int sessionId, char dtmfDigit, int duration)
{
    auto session = mSessions.find(sessionId);
    IMLOGD1("sendDtmf() - sessionId[%d]", sessionId);
    if (session != mSessions.end())
    {
        (session->second)->sendDtmf(dtmfDigit, duration);
    }
    else
    {
        IMLOGE1("sendDtmf() - no session id[%d]", sessionId);
    }
}

/*void AudioManager::sendHeaderExtension(int sessionId, RtpHeaderExtension* data) {
    (void)sessionId;
    (void)data;
}*/

void AudioManager::setMediaQualityThreshold(int sessionId, MediaQualityThreshold* threshold)
{
    auto session = mSessions.find(sessionId);
    IMLOGD1("setMediaQualityThreshold() - sessionId[%d]", sessionId);
    if (session != mSessions.end())
    {
        (session->second)->setMediaQualityThreshold(*threshold);
    }
    else
    {
        IMLOGE1("setMediaQualityThreshold() - no session id[%d]", sessionId);
    }
}

void AudioManager::sendMessage(const int sessionId, const android::Parcel& parcel)
{
    int nMsg = parcel.readInt32();
    status_t err = NO_ERROR;
    switch (nMsg)
    {
        case kAudioOpenSession:
        {
            int rtpFd = parcel.readInt32();
            int rtcpFd = parcel.readInt32();
            AudioConfig* config = new AudioConfig();
            err = config->readFromParcel(&parcel);

            if (err != NO_ERROR && err != -ENODATA)
            {
                IMLOGE1("sendMessage() - error readFromParcel[%d]", err);
            }
            EventParamOpenSession* param = new EventParamOpenSession(rtpFd, rtcpFd, config);
            ImsMediaEventHandler::SendEvent(
                    "AUDIO_REQUEST_EVENT", nMsg, sessionId, reinterpret_cast<uint64_t>(param));
        }
        break;
        case kAudioCloseSession:
            ImsMediaEventHandler::SendEvent("AUDIO_REQUEST_EVENT", nMsg, sessionId);
            break;
        case kAudioModifySession:
        case kAudioAddConfig:
        case kAudioConfirmConfig:
        case kAudioDeleteConfig:
        {
            AudioConfig* config = new AudioConfig();
            config->readFromParcel(&parcel);
            if (err != NO_ERROR)
            {
                IMLOGE1("sendMessage() - error readFromParcel[%d]", err);
            }
            ImsMediaEventHandler::SendEvent(
                    "AUDIO_REQUEST_EVENT", nMsg, sessionId, reinterpret_cast<uint64_t>(config));
        }
        break;
        case kAudioSendDtmf:
        {
            EventParamDtmf* param = new EventParamDtmf(parcel.readByte(), parcel.readInt32());
            ImsMediaEventHandler::SendEvent(
                    "AUDIO_REQUEST_EVENT", nMsg, sessionId, reinterpret_cast<uint64_t>(param));
        }
        break;
        case kAudioSendHeaderExtension:
            // TO DO
            break;
        case kAudioSetMediaQualityThreshold:
        {
            MediaQualityThreshold* threshold = new MediaQualityThreshold();
            threshold->readFromParcel(&parcel);
            ImsMediaEventHandler::SendEvent(
                    "AUDIO_REQUEST_EVENT", nMsg, sessionId, reinterpret_cast<uint64_t>(threshold));
        }
        break;
        default:
            break;
    }
}

void AudioManager::SendInternalEvent(
        uint32_t event, uint64_t sessionId, uint64_t paramA, uint64_t paramB)
{
    auto session = mSessions.find(sessionId);
    IMLOGD1("[SendInternalEvent] sessionId[%d]", sessionId);

    if (session != mSessions.end())
    {
        (session->second)->SendInternalEvent(event, paramA, paramB);
    }
    else
    {
        IMLOGE1("[SendInternalEvent] no session id[%d]", sessionId);
    }
}

AudioManager::RequestHandler::RequestHandler() :
        ImsMediaEventHandler("AUDIO_REQUEST_EVENT")
{
}

AudioManager::RequestHandler::~RequestHandler() {}

void AudioManager::RequestHandler::processEvent(
        uint32_t event, uint64_t sessionId, uint64_t paramA, uint64_t paramB)
{
    IMLOGD4("[processEvent] event[%d], sessionId[%d], paramA[%d], paramB[%d]", event, sessionId,
            paramA, paramB);
    ImsMediaResult result = RESULT_SUCCESS;
    switch (event)
    {
        case kAudioOpenSession:
        {
            EventParamOpenSession* param = reinterpret_cast<EventParamOpenSession*>(paramA);
            if (param != NULL)
            {
                AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(param->mConfig);
                result = AudioManager::getInstance()->openSession(
                        static_cast<int>(sessionId), param->rtpFd, param->rtcpFd, pConfig);

                if (result == RESULT_SUCCESS)
                {
                    ImsMediaEventHandler::SendEvent(
                            "AUDIO_RESPONSE_EVENT", kAudioOpenSessionSuccess, sessionId);
                }
                else
                {
                    ImsMediaEventHandler::SendEvent(
                            "AUDIO_RESPONSE_EVENT", kAudioOpenSessionFailure, sessionId, result);
                }

                delete param;

                if (pConfig != NULL)
                {
                    delete pConfig;
                }
            }
            else
            {
                ImsMediaEventHandler::SendEvent("AUDIO_RESPONSE_EVENT", kAudioOpenSessionFailure,
                        sessionId, RESULT_INVALID_PARAM);
            }
        }
        break;
        case kAudioCloseSession:
            AudioManager::getInstance()->closeSession(static_cast<int>(sessionId));
            ImsMediaEventHandler::SendEvent(
                    "AUDIO_RESPONSE_EVENT", kAudioSessionClosed, sessionId, 0, 0);
            break;
        case kAudioModifySession:
        {
            AudioConfig* config = reinterpret_cast<AudioConfig*>(paramA);
            result =
                    AudioManager::getInstance()->modifySession(static_cast<int>(sessionId), config);
            ImsMediaEventHandler::SendEvent(
                    "AUDIO_RESPONSE_EVENT", kAudioModifySessionResponse, sessionId, result, paramA);
        }
        break;
        case kAudioAddConfig:
        {
            AudioConfig* config = reinterpret_cast<AudioConfig*>(paramA);
            result = AudioManager::getInstance()->addConfig(static_cast<int>(sessionId), config);
            ImsMediaEventHandler::SendEvent(
                    "AUDIO_RESPONSE_EVENT", kAudioAddConfigResponse, sessionId, result, paramA);
        }
        break;
        case kAudioConfirmConfig:
        {
            AudioConfig* config = reinterpret_cast<AudioConfig*>(paramA);
            result =
                    AudioManager::getInstance()->confirmConfig(static_cast<int>(sessionId), config);
            ImsMediaEventHandler::SendEvent(
                    "AUDIO_RESPONSE_EVENT", kAudioConfirmConfigResponse, sessionId, result, paramA);
        }
        break;
        case kAudioDeleteConfig:
        {
            AudioConfig* config = reinterpret_cast<AudioConfig*>(paramA);
            if (config != NULL)
            {
                AudioManager::getInstance()->deleteConfig(static_cast<int>(sessionId), config);
                delete config;
            }
        }
        break;
        case kAudioSendDtmf:
        {
            EventParamDtmf* param = reinterpret_cast<EventParamDtmf*>(paramA);
            if (param != NULL)
            {
                AudioManager::getInstance()->sendDtmf(
                        static_cast<int>(sessionId), param->digit, param->duration);
                delete param;
            }
        }
        break;
        case kAudioSendHeaderExtension:
            // TO DO : add implementation
            break;
        case kAudioSetMediaQualityThreshold:
        {
            MediaQualityThreshold* threshold = reinterpret_cast<MediaQualityThreshold*>(paramA);
            if (threshold != NULL)
            {
                AudioManager::getInstance()->setMediaQualityThreshold(
                        static_cast<int>(sessionId), threshold);
                delete threshold;
            }
        }
        break;
        case kRequestAudioCmr:
            AudioManager::getInstance()->SendInternalEvent(
                    kRequestAudioCmr, static_cast<int>(sessionId), paramA, paramB);
            break;
        default:
            break;
    }
}

AudioManager::ResponseHandler::ResponseHandler() :
        ImsMediaEventHandler("AUDIO_RESPONSE_EVENT")
{
}

AudioManager::ResponseHandler::~ResponseHandler() {}

void AudioManager::ResponseHandler::processEvent(
        uint32_t event, uint64_t sessionId, uint64_t paramA, uint64_t paramB)
{
    IMLOGD4("[processEvent] event[%d], sessionId[%d], paramA[%d], paramB[%d]", event, sessionId,
            paramA, paramB);
    android::Parcel parcel;
    switch (event)
    {
        case kAudioOpenSessionSuccess:
        case kAudioOpenSessionFailure:
            parcel.writeInt32(event);
            parcel.writeInt32(static_cast<int>(sessionId));
            if (event == kAudioOpenSessionFailure)
            {
                // fail reason
                parcel.writeInt32(static_cast<int>(paramA));
            }
            AudioManager::getInstance()->sendResponse(
                    reinterpret_cast<uint64_t>(AudioManager::getInstance()), parcel);
            break;
        case kAudioModifySessionResponse:  // fall through
        case kAudioAddConfigResponse:      // fall through
        case kAudioConfirmConfigResponse:
        {
            parcel.writeInt32(event);
            parcel.writeInt32(paramA);  // result
            AudioConfig* config = reinterpret_cast<AudioConfig*>(paramB);
            if (config != NULL)
            {
                config->writeToParcel(&parcel);

                AudioManager::getInstance()->sendResponse(
                        reinterpret_cast<uint64_t>(AudioManager::getInstance()), parcel);
                delete config;
            }
        }
        break;
        case kAudioSessionChangedInd:
            parcel.writeInt32(event);
            parcel.writeInt32(static_cast<int>(sessionId));
            parcel.writeInt32(paramA);  // state
            AudioManager::getInstance()->sendResponse(
                    reinterpret_cast<uint64_t>(AudioManager::getInstance()), parcel);
            break;
        case kAudioFirstMediaPacketInd:
            parcel.writeInt32(event);
            AudioManager::getInstance()->sendResponse(
                    reinterpret_cast<uint64_t>(AudioManager::getInstance()), parcel);
            break;
        case kAudioRtpHeaderExtensionInd:
            // TODO : add implementation
            break;
        case kAudioMediaInactivityInd:
            parcel.writeInt32(event);
            parcel.writeInt32(static_cast<int>(paramA));  // type
            parcel.writeInt32(static_cast<int>(paramB));  // duration
            AudioManager::getInstance()->sendResponse(
                    reinterpret_cast<uint64_t>(AudioManager::getInstance()), parcel);
            break;
        case kAudioPacketLossInd:
        case kAudioJitterInd:
        case kAudioTriggerAnbrQueryInd:
        case kAudioDtmfReceivedInd:
        case kAudioCallQualityChangedInd:
            /** TODO: add implementation */
            break;
        case kAudioSessionClosed:
            parcel.writeInt32(event);
            parcel.writeInt32(static_cast<int>(sessionId));
            AudioManager::getInstance()->sendResponse(
                    reinterpret_cast<uint64_t>(AudioManager::getInstance()), parcel);
            break;
        default:
            break;
    }
}
