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

#include <VoiceManager.h>
#include <ImsMediaTrace.h>
#include <ImsMediaNetworkUtil.h>

using namespace android;

VoiceManager* VoiceManager::sManager = NULL;
android::content::AttributionSourceState VoiceManager::mAttributionSource;

VoiceManager::VoiceManager() {
}

VoiceManager::~VoiceManager() {
}

VoiceManager* VoiceManager::getInstance() {
    if (sManager == NULL) {
        sManager = new VoiceManager();
    }
    return sManager;
}

void VoiceManager::setAttributeSource(const android::content::AttributionSourceState& client) {
    IMLOGD1("[setAttributeSource] client[%s]", client.toString().c_str());
    mAttributionSource = client;
    IMLOGD1("[setAttributeSource] client[%s]", mAttributionSource.toString().c_str());
}

android::content::AttributionSourceState& VoiceManager::getAttributeSource() {
    return mAttributionSource;
}

ImsMediaResult VoiceManager::openSession(int sessionId, int rtpFd, int rtcpFd,
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

ImsMediaResult VoiceManager::closeSession(int sessionId) {
    IMLOGD1("closeSession() - sessionId[%d]", sessionId);
    if (mSessions.count(sessionId)) {
        mSessions.erase(sessionId);
        return RESULT_SUCCESS;
    }
    return RESULT_INVALID_PARAM;
}

ImsMediaResult VoiceManager::modifySession(int sessionId, AudioConfig* config) {
    auto session = mSessions.find(sessionId);
    IMLOGD1("modifySession() - sessionId[%d]", sessionId);
    if (session != mSessions.end()) {
        return (session->second)->startGraph(config);
    } else {
        IMLOGE1("modifySession() - no session id[%d]", sessionId);
        return RESULT_INVALID_PARAM;
    }
}

ImsMediaResult VoiceManager::addConfig(int sessionId, AudioConfig* config) {
    auto session = mSessions.find(sessionId);
    IMLOGD1("addConfig() - sessionId[%d]", sessionId);
    if (session != mSessions.end()) {
        return (session->second)->addGraph(config);
    } else {
        IMLOGE1("addConfig() - no session id[%d]", sessionId);
        return RESULT_INVALID_PARAM;
    }
}

ImsMediaResult VoiceManager::deleteConfig(int sessionId, AudioConfig* config) {
    auto session = mSessions.find(sessionId);
    IMLOGD1("deleteConfig() - sessionId[%d]", sessionId);
    if (session != mSessions.end()) {
        return (session->second)->deleteGraph(config);
    } else {
        IMLOGE1("deleteConfig() - no session id[%d]", sessionId);
        return RESULT_INVALID_PARAM;
    }
}

ImsMediaResult VoiceManager::confirmConfig(int sessionId, AudioConfig* config) {
    auto session = mSessions.find(sessionId);
    IMLOGD1("confirmConfig() - sessionId[%d]", sessionId);
    if (session != mSessions.end()) {
        return (session->second)->confirmGraph(config);
    } else {
        IMLOGE1("confirmConfig() - no session id[%d]", sessionId);
        return RESULT_INVALID_PARAM;
    }
}

void VoiceManager::sendDtmf(int sessionId, char dtmfDigit, int duration) {
    auto session = mSessions.find(sessionId);
    IMLOGD1("sendDtmf() - sessionId[%d]", sessionId);
    if (session != mSessions.end()) {
        (session->second)->sendDtmf(dtmfDigit, duration);
    } else {
        IMLOGE1("sendDtmf() - no session id[%d]", sessionId);
    }
}

/*void VoiceManager::sendHeaderExtension(int sessionId, RtpHeaderExtension* data) {
    (void)sessionId;
    (void)data;
}*/

void VoiceManager::setMediaQualityThreshold(int sessionId,
    MediaQualityThreshold* threshold) {
    auto session = mSessions.find(sessionId);
    IMLOGD1("setMediaQualityThreshold() - sessionId[%d]", sessionId);
    if (session != mSessions.end()) {
        (session->second)->setMediaQualityThreshold(threshold);
    } else {
        IMLOGE1("setMediaQualityThreshold() - no session id[%d]", sessionId);
    }
}

void VoiceManager::sendMessage(const int sessionId, const android::Parcel& parcel) {
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
            ImsMediaEventHandler::SendEvent("VOICE_REQUEST_EVENT", nMsg,
                sessionId, reinterpret_cast<uint64_t>(param));
        }
            break;
        case CLOSE_SESSION:
            ImsMediaEventHandler::SendEvent("VOICE_REQUEST_EVENT", nMsg, sessionId);
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
            ImsMediaEventHandler::SendEvent("VOICE_REQUEST_EVENT", nMsg,
                sessionId, reinterpret_cast<uint64_t>(config));
        }
            break;
        case SEND_DTMF:
        {
            EventParamDtmf* param = new EventParamDtmf(parcel.readByte(), parcel.readInt32());
            ImsMediaEventHandler::SendEvent("VOICE_REQUEST_EVENT", nMsg,
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
            ImsMediaEventHandler::SendEvent("VOICE_REQUEST_EVENT", nMsg,
                sessionId, reinterpret_cast<uint64_t>(threshold));
        }
            break;
        default:
            break;
    }
}

VoiceManager::RequestHandler::RequestHandler()
    : ImsMediaEventHandler("VOICE_REQUEST_EVENT") {
}

VoiceManager::RequestHandler::~RequestHandler() {
}

void VoiceManager::RequestHandler::processEvent(uint32_t event,
    uint64_t sessionId, uint64_t paramA, uint64_t paramB) {
    IMLOGD4("[processEvent] event[%d], sessionId[%d], paramA[%d], paramB[%d]",
        event, sessionId, paramA, paramB);
    ImsMediaResult result = RESULT_SUCCESS;
    switch (event) {
        case OPEN_SESSION:
        {
            EventParamOpenSession* param = reinterpret_cast<EventParamOpenSession*>(paramA);
            if (param != NULL) {
                result = VoiceManager::getInstance()->openSession(static_cast<int>(sessionId),
                    param->rtpFd, param->rtcpFd, param->mConfig);
                if (result == RESULT_SUCCESS) {
                    ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT", OPEN_SESSION_SUCCESS,
                        sessionId);
                } else {
                    ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT", OPEN_SESSION_FAILURE,
                        sessionId, result);
                }
                delete param;
            } else {
                ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT", OPEN_SESSION_FAILURE,
                    sessionId, RESULT_INVALID_PARAM);
            }
        }
            break;
        case CLOSE_SESSION:
            VoiceManager::getInstance()->closeSession(static_cast<int>(sessionId));
            break;
        case MODIFY_SESSION:
        {
            AudioConfig* config = reinterpret_cast<AudioConfig*>(paramA);
            result = VoiceManager::getInstance()->modifySession(static_cast<int>(sessionId),
                config);
            ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT", MODIFY_SESSION_RESPONSE,
                sessionId, result, paramA);
        }
            break;
        case ADD_CONFIG:
        {
            AudioConfig* config = reinterpret_cast<AudioConfig*>(paramA);
            result = VoiceManager::getInstance()->addConfig(static_cast<int>(sessionId), config);
            ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT", ADD_CONFIG_RESPONSE,
                sessionId, result, paramA);
        }
            break;
        case CONFIRM_CONFIG:
        {
            AudioConfig* config = reinterpret_cast<AudioConfig*>(paramA);
            result = VoiceManager::getInstance()->confirmConfig(static_cast<int>(sessionId),
                config);
            ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT", CONFIRM_CONFIG_RESPONSE,
                sessionId, result, paramA);
        }
            break;
        case DELETE_CONFIG:
        {
            AudioConfig* config = reinterpret_cast<AudioConfig*>(paramA);
            if (config != NULL) {
                VoiceManager::getInstance()->deleteConfig(static_cast<int>(sessionId), config);
                delete config;
            }
        }
            break;
        case SEND_DTMF:
        {
            EventParamDtmf* param = reinterpret_cast<EventParamDtmf*>(paramA);
            if (param != NULL) {
                VoiceManager::getInstance()->sendDtmf(static_cast<int>(sessionId), param->digit,
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
                VoiceManager::getInstance()->setMediaQualityThreshold(
                    static_cast<int>(sessionId), threshold);
                delete threshold;
            }
        }
            break;
        default:
            break;
    }
}

VoiceManager::ResponseHandler::ResponseHandler()
: ImsMediaEventHandler("VOICE_RESPONSE_EVENT") {
}

VoiceManager::ResponseHandler::~ResponseHandler() {
}

void VoiceManager::ResponseHandler::processEvent(uint32_t event,
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
            VoiceManager::getInstance()->getCallback()(
                reinterpret_cast<uint64_t>(VoiceManager::getInstance()), parcel);
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
            VoiceManager::getInstance()->getCallback()(
                reinterpret_cast<uint64_t>(VoiceManager::getInstance()), parcel);
        }
            break;
        case SESSION_CHANGED_IND:
            //TODO : add implementation
            break;
        case FIRST_MEDIA_PACKET_IND:
            parcel.writeInt32(event);
            VoiceManager::getInstance()->getCallback()(
                reinterpret_cast<uint64_t>(VoiceManager::getInstance()), parcel);
            break;
        case RTP_HEADER_EXTENSION_IND:
            //TODO : add implementation
            break;
        case MEDIA_INACITIVITY_IND:
            parcel.writeInt32(event);
            parcel.writeInt32(static_cast<int>(paramA));    //type
            parcel.writeInt32(static_cast<int>(paramB));    //duration
            VoiceManager::getInstance()->getCallback()(
                reinterpret_cast<uint64_t>(VoiceManager::getInstance()), parcel);
            break;
        case PACKET_LOSS_IND:
        case JITTER_IND:
            //TODO : add implementation
            break;
        default:
            break;
    }
}
