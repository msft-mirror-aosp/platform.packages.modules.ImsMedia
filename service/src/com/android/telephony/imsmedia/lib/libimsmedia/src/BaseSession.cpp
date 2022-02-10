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

#include <BaseSession.h>
#include <ImsMediaTrace.h>
#include <ImsMediaEventHandler.h>
#include <string.h>

BaseSession::BaseSession() : mRtpFd(0), mRtcpFd(0) {
}

BaseSession::~BaseSession() {
}

void BaseSession::setSessionId(int sessionid) {
    mSessionId = sessionid;
}

void BaseSession::setLocalEndPoint(int rtpFd, int rtcpFd) {
    IMLOGD2("setLocalEndPoint() - rtpFd[%d], rtcpFd[%d]", rtpFd, rtcpFd);
    mRtpFd = rtpFd;
    mRtcpFd = rtcpFd;
}

int BaseSession::getLocalRtpFd() {
    return mRtpFd;
}

int BaseSession::getLocalRtcpFd() {
    return mRtcpFd;
}

void BaseSession::onEvent(ImsMediaEventType type, uint64_t param1, uint64_t param2) {
    IMLOGD1("[onEvent] type[%d]", type);
    (void)param2;
    switch (type) {
        case EVENT_NOTIFY_ERROR:
            break;
        case EVENT_NOTIFY_FIRST_MEDIA_PACKET_RECEIVED:
            ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT",
                FIRST_MEDIA_PACKET_RECEIVED, 0, 0);
            break;
        case EVENT_NOTIFY_HEADER_EXTENSION_RECEIVED:
            ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT",
                HEADER_EXTENSION_RECEIVED, 0, 0);
            break;
        case EVENT_NOTIFY_MEDIA_INACITIVITY:
            ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT",
                NOTIFY_MEDIA_INACITIVITY, param1, 0);
            break;
        case EVENT_NOTIFY_PACKET_LOSS:
            ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT",
                NOTIFY_PACKET_LOSS, param1, 0);
            break;
        case EVENT_NOTIFY_JITTER:
            ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT",
                NOTIFY_JITTER, param1, 0);
            break;
        default:
            break;
    }
}