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

bool VoiceManager::openSession(int sessionId, int rtpFd, int rtcpFd, AudioConfig* config) {
    IMLOGD1("[openSession] sessionId[%d]", sessionId);

    //set debug log
    ImsMediaTrace::IMSetDebugLog(IM_PACKET_LOG_SOCKET | IM_PACKET_LOG_AUDIO | IM_PACKET_LOG_RTP |
        IM_PACKET_LOG_RTCP | IM_PACKET_LOG_PH | IM_PACKET_LOG_JITTER);

    if (!mSessions.count(sessionId)) {
        AudioSession* session = new AudioSession();
        session->setSessionId(sessionId);
        session->setLocalEndPoint(rtpFd, rtcpFd);
        mSessions.insert(std::make_pair(sessionId, std::move(session)));
        if (config != NULL) {
            ImsMediaResult ret = session->startGraph(config);
            if (ret != IMS_MEDIA_OK) {
                IMLOGD1("[openSession] startGraph failed[%d]", ret);
            }
        }
        return true;
    }

    return 0;
}

ImsMediaResult VoiceManager::closeSession(int sessionId) {
    IMLOGD1("closeSession() - sessionId[%d]", sessionId);
    if (mSessions.count(sessionId)) {
        mSessions.erase(sessionId);
        return IMS_MEDIA_OK;
    }
    return IMS_MEDIA_ERROR_UNKNOWN;
}

bool VoiceManager::modifySession(int sessionId, AudioConfig* config) {
    auto session = mSessions.find(sessionId);
    IMLOGD1("modifySession() - sessionId[%d]", sessionId);
    if (session != mSessions.end()) {
        (session->second)->startGraph(config);
        return true;
    } else {
        IMLOGE1("modifySession() - no session id[%d]", sessionId);
        return false;
    }
}

bool VoiceManager::addConfig(int sessionId, AudioConfig* config) {
    (void)sessionId;
    (void)config;
    //TODO : add implementation
    return false;
}

bool VoiceManager::deleteConfig(int sessionId, AudioConfig* config) {
    auto session = mSessions.find(sessionId);
    IMLOGD1("deleteConfig() - sessionId[%d]", sessionId);
    if (session != mSessions.end()) {
        if ((session->second)->deleteGraph(config) == IMS_MEDIA_OK) {
            return true;
        }
    } else {
        IMLOGE1("deleteConfig() - no session id[%d]", sessionId);
        return false;
    }
    return false;
}

bool VoiceManager::confirmConfig(int sessionId, AudioConfig* config) {
    (void)sessionId;
    (void)config;
    //TODO : add implementation
    return false;
}

void VoiceManager::startDtmf(int sessionId, char dtmfDigit, int volume, int duration) {
    auto session = mSessions.find(sessionId);
    IMLOGD1("startDtmf() - sessionId[%d]", sessionId);
    if (session != mSessions.end()) {
        (session->second)->startDtmf(dtmfDigit, volume, duration);
    } else {
        IMLOGE1("startDtmf() - no session id[%d]", sessionId);
    }
}

void VoiceManager::stopDtmf(int sessionId) {
    auto session = mSessions.find(sessionId);
    IMLOGD1("stopDtmf() - sessionId[%d]", sessionId);
    if (session != mSessions.end()) {
        (session->second)->stopDtmf();
    } else {
        IMLOGE1("stopDtmf() - no session id[%d]", sessionId);
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
                delete config;
                return;
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
                delete config;
                return;
            }
            ImsMediaEventHandler::SendEvent("VOICE_REQUEST_EVENT", nMsg,
                sessionId, reinterpret_cast<uint64_t>(config));
        }
            break;
        case START_DTMF:
        {
            EventParamDtmf* param = new EventParamDtmf(parcel.readByte(),
                parcel.readInt32(), parcel.readInt32());
            ImsMediaEventHandler::SendEvent("VOICE_REQUEST_EVENT", nMsg,
                sessionId, reinterpret_cast<uint64_t>(param));
        }
            break;
        case STOP_DTMF:
            ImsMediaEventHandler::SendEvent("VOICE_REQUEST_EVENT", nMsg,
                sessionId, 0);
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
    uint64_t paramA, uint64_t paramB, uint64_t paramC) {
    IMLOGD4("[processEvent] event[%d], paramA[%d], paramB[%d], paramC[%d]",
        event, paramA, paramB, paramC);
    bool res = false;
    switch (event) {
        case OPEN_SESSION:
        {
            EventParamOpenSession* param = reinterpret_cast<EventParamOpenSession*>(paramB);
            if (param != NULL) {
                res = VoiceManager::getInstance()->openSession(static_cast<int>(paramA),
                    param->rtpFd, param->rtcpFd, param->mConfig);
                ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT",
                    res == true ? OPEN_SUCCESS : OPEN_FAILURE, paramA, 0);
                delete param;
            } else {
                ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT", OPEN_FAILURE, paramA, 0);
            }
        }
            break;
        case CLOSE_SESSION:
            VoiceManager::getInstance()->closeSession(static_cast<int>(paramA));
            break;
        case MODIFY_SESSION:
        {
            AudioConfig* param = reinterpret_cast<AudioConfig*>(paramB);
            res = VoiceManager::getInstance()->modifySession(static_cast<int>(paramA), param);
            ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT", MODIFY_SESSION_RESPONSE,
                paramA, res == true ? RESPONSE_SUCCESS : RESPONSE_FAIL, paramB);
        }
            break;
        case ADD_CONFIG:
        {
            AudioConfig* param = reinterpret_cast<AudioConfig*>(paramB);
            res = VoiceManager::getInstance()->addConfig(static_cast<int>(paramA), param);
            ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT", ADD_CONFIG_RESPONSE,
                paramA, res == true ? RESPONSE_SUCCESS : RESPONSE_FAIL, paramB);
        }
            break;
        case CONFIRM_CONFIG:
        {
            AudioConfig* param = reinterpret_cast<AudioConfig*>(paramB);
            res = VoiceManager::getInstance()->confirmConfig(static_cast<int>(paramA), param);
            ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT", CONFIRM_CONFIG_RESPONSE,
                paramA, res == true ? RESPONSE_SUCCESS : RESPONSE_FAIL, paramB);
        }
            break;
        case DELETE_CONFIG:
        {
            AudioConfig* param = reinterpret_cast<AudioConfig*>(paramB);
            if (param != NULL) {
                VoiceManager::getInstance()->deleteConfig(static_cast<int>(paramA), param);
                delete param;
            }
        }
            break;
        case START_DTMF:
        {
            EventParamDtmf* param = reinterpret_cast<EventParamDtmf*>(paramB);
            if (param != NULL) {
                VoiceManager::getInstance()->startDtmf(static_cast<int>(paramA), param->digit,
                    param->volume, param->duration);
                delete param;
            }
        }
            break;
        case STOP_DTMF:
            VoiceManager::getInstance()->stopDtmf(static_cast<int>(paramA));
            break;
        case SEND_HEADER_EXTENSION:
            //TO DO : add implementation
            break;
        case SET_MEDIA_QUALITY_THRESHOLD:
        {
            MediaQualityThreshold* param = reinterpret_cast<MediaQualityThreshold*>(paramB);
            if (param != NULL) {
                VoiceManager::getInstance()->setMediaQualityThreshold(
                    static_cast<int>(paramA), param);
                delete param;
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
    uint64_t paramA, uint64_t paramB, uint64_t paramC) {
    IMLOGD4("[processEvent] event[%d], paramA[%d], paramB[%d], paramC[%d]",
        event, paramA, paramB, paramC);
    android::Parcel parcel;
    switch (event) {
        case OPEN_SUCCESS:
        case OPEN_FAILURE:
            parcel.writeInt32(event);
            parcel.writeInt32(static_cast<int>(paramA));   //session id
            if (event == OPEN_FAILURE) {
                //add fail reason
                parcel.writeInt32(static_cast<int>(paramB));
            }
            VoiceManager::getInstance()->getCallback()(
                reinterpret_cast<uint64_t>(VoiceManager::getInstance()), parcel);
            break;
        case MODIFY_SESSION_RESPONSE:   //fall through
        case ADD_CONFIG_RESPONSE:       //fall through
        case CONFIRM_CONFIG_RESPONSE:
        {
            parcel.writeInt32(event);
            parcel.writeInt32(paramB);
            AudioConfig* config = reinterpret_cast<AudioConfig*>(paramC);
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
            parcel.writeInt32(static_cast<int>(paramA));
            parcel.writeInt32(static_cast<int>(paramB));
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
