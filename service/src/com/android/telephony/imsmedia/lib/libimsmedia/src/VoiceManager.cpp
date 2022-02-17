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

bool VoiceManager::openSession(int sessionid, int rtpFd, int rtcpFd, RtpConfig* config) {
    (void)config;
    IMLOGD1("openSession() - sessionId[%d]", sessionid);

    //set debug log
    ImsMediaTrace::IMSetDebugLog(IM_PACKET_LOG_SOCKET | IM_PACKET_LOG_AUDIO | IM_PACKET_LOG_RTP |
        IM_PACKET_LOG_RTCP | IM_PACKET_LOG_SCHEDULER | IM_PACKET_LOG_PH | IM_PACKET_LOG_JITTER);

    if (!mSessions.count(sessionid)) {
        AudioSession* session = new AudioSession();
        session->setSessionId(sessionid);
        session->setLocalEndPoint(rtpFd, rtcpFd);
        mSessions.insert(std::make_pair(sessionid, std::move(session)));
        return true;
    }

    return 0;
}

ImsMediaResult VoiceManager::closeSession(int sessionid) {
    IMLOGD1("closeSession() - sessionId[%d]", sessionid);
    if (mSessions.count(sessionid)) {
        mSessions.erase(sessionid);
        return IMS_MEDIA_OK;
    }
    return IMS_MEDIA_ERROR_UNKNOWN;
}

bool VoiceManager::modifySession(int sessionid, RtpConfig* config) {
    auto session = mSessions.find(sessionid);
    IMLOGD1("modifySession() - sessionId[%d]", sessionid);
    if (session != mSessions.end()) {
        (session->second)->startGraph(config);
        return true;
    } else {
        IMLOGE1("modifySession() - no session id[%d]", sessionid);
        return false;
    }
}

void VoiceManager::addConfig(int sessionid, RtpConfig* config) {
    (void)sessionid;
    (void)config;
}

bool VoiceManager::deleteConfig(int sessionid, RtpConfig* config) {
    auto session = mSessions.find(sessionid);
    IMLOGD1("deleteConfig() - sessionId[%d]", sessionid);
    if (session != mSessions.end()) {
        if ((session->second)->deleteGraph(config) == IMS_MEDIA_OK) {
            return true;
        }
    } else {
        IMLOGE1("deleteConfig() - no session id[%d]", sessionid);
        return false;
    }
    return false;
}

void VoiceManager::confirmConfig(int sessionid, RtpConfig* config) {
    (void)sessionid;
    (void)config;
}

void VoiceManager::startDtmf(int sessionid, char dtmfDigit, int volume, int duration) {
    auto session = mSessions.find(sessionid);
    IMLOGD1("startDtmf() - sessionId[%d]", sessionid);
    if (session != mSessions.end()) {
        (session->second)->startDtmf(dtmfDigit, volume, duration);
    } else {
        IMLOGE1("startDtmf() - no session id[%d]", sessionid);
    }
}

void VoiceManager::stopDtmf(int sessionid) {
    auto session = mSessions.find(sessionid);
    IMLOGD1("stopDtmf() - sessionId[%d]", sessionid);
    if (session != mSessions.end()) {
        (session->second)->stopDtmf();
    } else {
        IMLOGE1("stopDtmf() - no session id[%d]", sessionid);
    }
}

/*void VoiceManager::sendHeaderExtension(int sessionid, RtpHeaderExtension* data) {
    (void)sessionid;
    (void)data;
}*/

void VoiceManager::setMediaQualityThreshold(int sessionid,
    MediaQualityThreshold* threshold) {
    auto session = mSessions.find(sessionid);
    IMLOGD1("setMediaQualityThreshold() - sessionId[%d]", sessionid);
    if (session != mSessions.end()) {
        (session->second)->setMediaQualityThreshold(threshold);
    } else {
        IMLOGE1("setMediaQualityThreshold() - no session id[%d]", sessionid);
    }
}

void VoiceManager::sendMessage(const int sessionid, const android::Parcel& parcel) {
    int nMsg = parcel.readInt32();
    switch (nMsg) {
        case OPEN_SESSION:
        {
            int rtpFd = parcel.readInt32();
            int rtcpFd = parcel.readInt32();
            RtpConfig* config = new RtpConfig();
            config->readFromParcel(&parcel);
            EventParamOpenSession* param = new EventParamOpenSession(rtpFd, rtcpFd, config);
            ImsMediaEventHandler::SendEvent("VOICE_REQUEST_EVENT", nMsg,
                sessionid, reinterpret_cast<uint64_t>(param));
        }
            break;
        case CLOSE_SESSION:
            ImsMediaEventHandler::SendEvent("VOICE_REQUEST_EVENT", nMsg, sessionid, 0);
            break;
        case MODIFY_SESSION:
        {
            RtpConfig* config = new RtpConfig();
            config->readFromParcel(&parcel);
            ImsMediaEventHandler::SendEvent("VOICE_REQUEST_EVENT", nMsg,
                sessionid, reinterpret_cast<uint64_t>(config));
        }
            break;
        case ADD_CONFIG:
        case CONFIRM_CONFIG:
        case DELETE_CONFIG:
        {
            RtpConfig* config = new RtpConfig();
            config->readFromParcel(&parcel);
            ImsMediaEventHandler::SendEvent("VOICE_REQUEST_EVENT", nMsg,
                sessionid, reinterpret_cast<uint64_t>(config));
        }
            break;
        case START_DTMF:
        {
            EventParamDtmf* param = new EventParamDtmf(parcel.readByte(),
                parcel.readInt32(), parcel.readInt32());
            ImsMediaEventHandler::SendEvent("VOICE_REQUEST_EVENT", nMsg,
                sessionid, reinterpret_cast<uint64_t>(param));
        }
            break;
        case STOP_DTMF:
            ImsMediaEventHandler::SendEvent("VOICE_REQUEST_EVENT", nMsg,
                sessionid, 0);
            break;
        case SEND_HEADER_EXTENSION:
            break;
        case SET_MEDIA_QUALITY_THRESHOLD:
        {
            MediaQualityThreshold* threshold = new MediaQualityThreshold();
            threshold->readFromParcel(&parcel);
            ImsMediaEventHandler::SendEvent("VOICE_REQUEST_EVENT", nMsg,
                sessionid, reinterpret_cast<uint64_t>(threshold));
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

void VoiceManager::RequestHandler::processEvent(uint32_t event, uint64_t pParam, uint64_t lParam) {
    IMLOGD3("[processEvent] event[%d], pParam[%d], lParam[%d]", event, pParam, lParam);
    switch (event) {
        case OPEN_SESSION:
        {
            EventParamOpenSession* param = reinterpret_cast<EventParamOpenSession*>(lParam);
            if (param) {
                if (VoiceManager::getInstance()->openSession(static_cast<int>(pParam),
                        param->rtpFd, param->rtcpFd, param->mConfig) == true) {
                    ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT", OPEN_SUCCESS,
                        pParam, 0);
                } else {
                    ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT", OPEN_FAILURE,
                        pParam, 0);
                }
                delete param;
            }
        }
            break;
        case CLOSE_SESSION:
            VoiceManager::getInstance()->closeSession(static_cast<int>(pParam));
            break;
        case MODIFY_SESSION:
        {
            RtpConfig* param = reinterpret_cast<RtpConfig*>(lParam);
            if (param != NULL) {
                VoiceManager::getInstance()->modifySession(static_cast<int>(pParam), param);
                ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT", MODIFY_SESSION_RESPONSE,
                    pParam, 0);
                delete param;
            }
        }
            break;
        case ADD_CONFIG:
        case CONFIRM_CONFIG:
        case DELETE_CONFIG:
        {
            RtpConfig* param = reinterpret_cast<RtpConfig*>(lParam);
            if (param != NULL) {
                VoiceManager::getInstance()->deleteConfig(static_cast<int>(pParam), param);
                ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT", DELETE_CONFIG_RESPONSE,
                    pParam, 0);
                delete param;
            }
        }
            break;
        case START_DTMF:
        {
            EventParamDtmf* param = reinterpret_cast<EventParamDtmf*>(lParam);
            if (param != NULL) {
                VoiceManager::getInstance()->startDtmf(static_cast<int>(pParam), param->digit,
                    param->volume, param->duration);
                delete param;
            }
        }
            break;
        case STOP_DTMF:
            VoiceManager::getInstance()->stopDtmf(static_cast<int>(pParam));
            break;
        case SEND_HEADER_EXTENSION:
            break;
        case SET_MEDIA_QUALITY_THRESHOLD:
        {
            MediaQualityThreshold* param = reinterpret_cast<MediaQualityThreshold*>(lParam);
            if (param != NULL) {
                VoiceManager::getInstance()->setMediaQualityThreshold(
                    static_cast<int>(pParam), param);
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

void VoiceManager::ResponseHandler::processEvent(uint32_t event, uint64_t pParam, uint64_t lParam) {
    IMLOGD3("[processEvent] event[%d], pParam[%d], lParam[%d]", event, pParam, lParam);
    android::Parcel parcel;
    switch (event) {
        case OPEN_SUCCESS:
        case OPEN_FAILURE:
            parcel.writeInt32(event);
            parcel.writeInt32(static_cast<int>(pParam));   //session id
            if (event == OPEN_FAILURE) {
                //add fail reason
                parcel.writeInt32(static_cast<int>(lParam));
            }
            VoiceManager::getInstance()->getCallback()(
                reinterpret_cast<uint64_t>(VoiceManager::getInstance()), parcel);
            break;
        case MODIFY_SESSION_RESPONSE:
            parcel.writeInt32(event);
            //add param later
            VoiceManager::getInstance()->getCallback()(
                reinterpret_cast<uint64_t>(VoiceManager::getInstance()), parcel);
            break;
        case DELETE_CONFIG_RESPONSE:
            parcel.writeInt32(event);
            //add param later
            VoiceManager::getInstance()->getCallback()(
                reinterpret_cast<uint64_t>(VoiceManager::getInstance()), parcel);
            break;
        default:
            break;
    }
}
