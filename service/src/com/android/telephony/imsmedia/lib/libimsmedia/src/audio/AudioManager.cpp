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

AudioManager::AudioManager() {
}

AudioManager::~AudioManager() {
}

AudioManager* AudioManager::getInstance() {
    if (sManager == NULL) {
        sManager = new AudioManager();
    }
    return sManager;
}

ImsMediaResult AudioManager::openSession(int sessionId, int rtpFd, int rtcpFd,
    AudioConfig* config) {
    IMLOGD1("[openSession] sessionId[%d]", sessionId);

    //set debug log
    ImsMediaTrace::IMSetDebugLog(IM_PACKET_LOG_SOCKET | IM_PACKET_LOG_AUDIO | IM_PACKET_LOG_RTP |
        IM_PACKET_LOG_RTCP | IM_PACKET_LOG_PH | IM_PACKET_LOG_JITTER);

    if (rtpFd == -1 || rtcpFd == -1) return RESULT_INVALID_PARAM;

    if (!mSessions.count(sessionId)) {
        AudioSession* session = new AudioSession();
        session->setSessionId(sessionId);
        session->setLocalEndPoint(rtpFd, rtcpFd);
        mSessions.insert(std::make_pair(sessionId, std::move(session)));
        if (config != NULL) {
            ImsMediaResult ret = session->startGraph(config);
            if (ret != RESULT_SUCCESS) {
                IMLOGD1("[openSession] startGraph failed[%d]", ret);
            }
        }
    } else {
        return RESULT_INVALID_PARAM;
    }

    return RESULT_SUCCESS;
}

ImsMediaResult AudioManager::closeSession(int sessionId) {
    IMLOGD1("closeSession() - sessionId[%d]", sessionId);
    if (mSessions.count(sessionId)) {
        mSessions.erase(sessionId);
        return RESULT_SUCCESS;
    }
    return RESULT_INVALID_PARAM;
}

ImsMediaResult AudioManager::modifySession(int sessionId, AudioConfig* config) {
    auto session = mSessions.find(sessionId);
    IMLOGD1("modifySession() - sessionId[%d]", sessionId);
    if (session != mSessions.end()) {
        return (session->second)->startGraph(config);
    } else {
        IMLOGE1("modifySession() - no session id[%d]", sessionId);
        return RESULT_INVALID_PARAM;
    }
}

ImsMediaResult AudioManager::addConfig(int sessionId, AudioConfig* config) {
    auto session = mSessions.find(sessionId);
    IMLOGD1("addConfig() - sessionId[%d]", sessionId);
    if (session != mSessions.end()) {
        return (session->second)->addGraph(config);
    } else {
        IMLOGE1("addConfig() - no session id[%d]", sessionId);
        return RESULT_INVALID_PARAM;
    }
}

ImsMediaResult AudioManager::deleteConfig(int sessionId, AudioConfig* config) {
    auto session = mSessions.find(sessionId);
    IMLOGD1("deleteConfig() - sessionId[%d]", sessionId);
    if (session != mSessions.end()) {
        return (session->second)->deleteGraph(config);
    } else {
        IMLOGE1("deleteConfig() - no session id[%d]", sessionId);
        return RESULT_INVALID_PARAM;
    }
}

ImsMediaResult AudioManager::confirmConfig(int sessionId, AudioConfig* config) {
    auto session = mSessions.find(sessionId);
    IMLOGD1("confirmConfig() - sessionId[%d]", sessionId);
    if (session != mSessions.end()) {
        return (session->second)->confirmGraph(config);
    } else {
        IMLOGE1("confirmConfig() - no session id[%d]", sessionId);
        return RESULT_INVALID_PARAM;
    }
}

void AudioManager::sendDtmf(int sessionId, char dtmfDigit, int duration) {
    auto session = mSessions.find(sessionId);
    IMLOGD1("sendDtmf() - sessionId[%d]", sessionId);
    if (session != mSessions.end()) {
        (session->second)->sendDtmf(dtmfDigit, duration);
    } else {
        IMLOGE1("sendDtmf() - no session id[%d]", sessionId);
    }
}

/*void AudioManager::sendHeaderExtension(int sessionId, RtpHeaderExtension* data) {
    (void)sessionId;
    (void)data;
}*/

void AudioManager::setMediaQualityThreshold(int sessionId,
    MediaQualityThreshold* threshold) {
    auto session = mSessions.find(sessionId);
    IMLOGD1("setMediaQualityThreshold() - sessionId[%d]", sessionId);
    if (session != mSessions.end()) {
        (session->second)->setMediaQualityThreshold(threshold);
    } else {
        IMLOGE1("setMediaQualityThreshold() - no session id[%d]", sessionId);
    }
}

void AudioManager::sendMessage(const int sessionId, const android::Parcel& parcel) {
    int nMsg = parcel.readInt32();
    status_t err = NO_ERROR;
    switch (nMsg) {
        case OPEN_SESSION:
        {
            int rtpFd = parcel.readInt32();
            int rtcpFd = parcel.readInt32();
            AudioConfig* config = new AudioConfig();
            err = config->readFromParcel(&parcel);
            if (err != NO_ERROR) {
                IMLOGE1("sendMessage() - error readFromParcel[%d]", err);
            }
            EventParamOpenSession* param = new EventParamOpenSession(rtpFd, rtcpFd, config);
            ImsMediaEventHandler::SendEvent("AUDIO_REQUEST_EVENT", nMsg,
                sessionId, reinterpret_cast<uint64_t>(param));
        }
            break;
        case CLOSE_SESSION:
            ImsMediaEventHandler::SendEvent("AUDIO_REQUEST_EVENT", nMsg, sessionId);
            break;
        case MODIFY_SESSION:
        case ADD_CONFIG:
        case CONFIRM_CONFIG:
        case DELETE_CONFIG:
        {
            AudioConfig* config = new AudioConfig();
            config->readFromParcel(&parcel);
            if (err != NO_ERROR) {
                IMLOGE1("sendMessage() - error readFromParcel[%d]", err);
            }
            ImsMediaEventHandler::SendEvent("AUDIO_REQUEST_EVENT", nMsg,
                sessionId, reinterpret_cast<uint64_t>(config));
        }
            break;
        case SEND_DTMF:
        {
            EventParamDtmf* param = new EventParamDtmf(parcel.readByte(), parcel.readInt32());
            ImsMediaEventHandler::SendEvent("AUDIO_REQUEST_EVENT", nMsg,
                sessionId, reinterpret_cast<uint64_t>(param));
        }
            break;
        case SEND_HEADER_EXTENSION:
            //TO DO
            break;
        case SET_MEDIA_QUALITY_THRESHOLD:
        {
            MediaQualityThreshold* threshold = new MediaQualityThreshold();
            threshold->readFromParcel(&parcel);
            ImsMediaEventHandler::SendEvent("AUDIO_REQUEST_EVENT", nMsg,
                sessionId, reinterpret_cast<uint64_t>(threshold));
        }
            break;
        default:
            break;
    }
}

AudioManager::RequestHandler::RequestHandler()
    : ImsMediaEventHandler("AUDIO_REQUEST_EVENT") {
}

AudioManager::RequestHandler::~RequestHandler() {
}

void AudioManager::RequestHandler::processEvent(uint32_t event,
    uint64_t sessionId, uint64_t paramA, uint64_t paramB) {
    IMLOGD4("[processEvent] event[%d], sessionId[%d], paramA[%d], paramB[%d]",
        event, sessionId, paramA, paramB);
    ImsMediaResult result = RESULT_SUCCESS;
    switch (event) {
        case OPEN_SESSION:
        {
            EventParamOpenSession* param = reinterpret_cast<EventParamOpenSession*>(paramA);
            if (param != NULL) {
                result = AudioManager::getInstance()->openSession(static_cast<int>(sessionId),
                    param->rtpFd, param->rtcpFd, param->mConfig);
                if (result == RESULT_SUCCESS) {
                    ImsMediaEventHandler::SendEvent("AUDIO_RESPONSE_EVENT", OPEN_SESSION_SUCCESS,
                        sessionId);
                } else {
                    ImsMediaEventHandler::SendEvent("AUDIO_RESPONSE_EVENT", OPEN_SESSION_FAILURE,
                        sessionId, result);
                }
                delete param;
            } else {
                ImsMediaEventHandler::SendEvent("AUDIO_RESPONSE_EVENT", OPEN_SESSION_FAILURE,
                    sessionId, RESULT_INVALID_PARAM);
            }
        }
            break;
        case CLOSE_SESSION:
            AudioManager::getInstance()->closeSession(static_cast<int>(sessionId));
            break;
        case MODIFY_SESSION:
        {
            AudioConfig* config = reinterpret_cast<AudioConfig*>(paramA);
            result = AudioManager::getInstance()->modifySession(static_cast<int>(sessionId),
                config);
            ImsMediaEventHandler::SendEvent("AUDIO_RESPONSE_EVENT", MODIFY_SESSION_RESPONSE,
                sessionId, result, paramA);
        }
            break;
        case ADD_CONFIG:
        {
            AudioConfig* config = reinterpret_cast<AudioConfig*>(paramA);
            result = AudioManager::getInstance()->addConfig(static_cast<int>(sessionId), config);
            ImsMediaEventHandler::SendEvent("AUDIO_RESPONSE_EVENT", ADD_CONFIG_RESPONSE,
                sessionId, result, paramA);
        }
            break;
        case CONFIRM_CONFIG:
        {
            AudioConfig* config = reinterpret_cast<AudioConfig*>(paramA);
            result = AudioManager::getInstance()->confirmConfig(static_cast<int>(sessionId),
                config);
            ImsMediaEventHandler::SendEvent("AUDIO_RESPONSE_EVENT", CONFIRM_CONFIG_RESPONSE,
                sessionId, result, paramA);
        }
            break;
        case DELETE_CONFIG:
        {
            AudioConfig* config = reinterpret_cast<AudioConfig*>(paramA);
            if (config != NULL) {
                AudioManager::getInstance()->deleteConfig(static_cast<int>(sessionId), config);
                delete config;
            }
        }
            break;
        case SEND_DTMF:
        {
            EventParamDtmf* param = reinterpret_cast<EventParamDtmf*>(paramA);
            if (param != NULL) {
                AudioManager::getInstance()->sendDtmf(static_cast<int>(sessionId), param->digit,
                    param->duration);
                delete param;
            }
        }
            break;
        case SEND_HEADER_EXTENSION:
            //TO DO : add implementation
            break;
        case SET_MEDIA_QUALITY_THRESHOLD:
        {
            MediaQualityThreshold* threshold = reinterpret_cast<MediaQualityThreshold*>(paramA);
            if (threshold != NULL) {
                AudioManager::getInstance()->setMediaQualityThreshold(
                    static_cast<int>(sessionId), threshold);
                delete threshold;
            }
        }
            break;
        default:
            break;
    }
}

AudioManager::ResponseHandler::ResponseHandler()
: ImsMediaEventHandler("AUDIO_RESPONSE_EVENT") {
}

AudioManager::ResponseHandler::~ResponseHandler() {
}

void AudioManager::ResponseHandler::processEvent(uint32_t event,
    uint64_t sessionId, uint64_t paramA, uint64_t paramB) {
    IMLOGD4("[processEvent] event[%d], sessionId[%d], paramA[%d], paramB[%d]",
        event, sessionId, paramA, paramB);
    android::Parcel parcel;
    switch (event) {
        case OPEN_SESSION_SUCCESS:
        case OPEN_SESSION_FAILURE:
            parcel.writeInt32(event);
            parcel.writeInt32(static_cast<int>(sessionId));
            if (event == OPEN_SESSION_FAILURE) {
                // fail reason
                parcel.writeInt32(static_cast<int>(paramA));
            }
            AudioManager::getInstance()->getCallback()(
                reinterpret_cast<uint64_t>(AudioManager::getInstance()), parcel);
            break;
        case MODIFY_SESSION_RESPONSE:   //fall through
        case ADD_CONFIG_RESPONSE:       //fall through
        case CONFIRM_CONFIG_RESPONSE:
        {
            parcel.writeInt32(event);
            parcel.writeInt32(paramA);  // result
            AudioConfig* config = reinterpret_cast<AudioConfig*>(paramB);
            if (config != NULL) {
                config->writeToParcel(&parcel);
            }
            AudioManager::getInstance()->getCallback()(
                reinterpret_cast<uint64_t>(AudioManager::getInstance()), parcel);
        }
            break;
        case SESSION_CHANGED_IND:
            //TODO : add implementation
            break;
        case FIRST_MEDIA_PACKET_IND:
            parcel.writeInt32(event);
            AudioManager::getInstance()->getCallback()(
                reinterpret_cast<uint64_t>(AudioManager::getInstance()), parcel);
            break;
        case RTP_HEADER_EXTENSION_IND:
            //TODO : add implementation
            break;
        case MEDIA_INACITIVITY_IND:
            parcel.writeInt32(event);
            parcel.writeInt32(static_cast<int>(paramA));    //type
            parcel.writeInt32(static_cast<int>(paramB));    //duration
            AudioManager::getInstance()->getCallback()(
                reinterpret_cast<uint64_t>(AudioManager::getInstance()), parcel);
            break;
        case PACKET_LOSS_IND:
        case JITTER_IND:
            //TODO : add implementation
            break;
        default:
            break;
    }
}
