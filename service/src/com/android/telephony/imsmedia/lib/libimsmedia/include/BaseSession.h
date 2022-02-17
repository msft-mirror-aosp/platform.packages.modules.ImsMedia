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

#ifndef BASE_SESSION_H
#define BASE_SESSION_H

#include <BaseSessionCallback.h>
#include <RtpConfig.h>
#include <MediaQualityThreshold.h>
#include <stdint.h>

class BaseSession : public BaseSessionCallback {
public:
    BaseSession();
    virtual ~BaseSession();
    void setSessionId(int sessionid);
    void setLocalEndPoint(int rtpFd, int rtcpFd);
    int getLocalRtpFd();
    int getLocalRtcpFd();
    // BaseSessionCallback
    virtual void onEvent(ImsMediaEventType type, uint64_t param1, uint64_t param2);

protected:
    virtual ImsMediaResult startGraph(RtpConfig* config) = 0;
    virtual ImsMediaResult addGraph(RtpConfig* config) = 0;
    virtual ImsMediaResult confirmGraph(RtpConfig* config) = 0;
    virtual ImsMediaResult deleteGraph(RtpConfig* config) = 0;
    virtual void setMediaQualityThreshold(MediaQualityThreshold* threshold) = 0;

    int mSessionId;
    int mRtpFd;
    int mRtcpFd;
};

#endif