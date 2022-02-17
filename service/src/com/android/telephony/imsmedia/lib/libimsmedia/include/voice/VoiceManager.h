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

#ifndef VOICE_MANAGER_H
#define VOICE_MANAGER_H

#include <ImsMediaDefine.h>
#include <ImsMediaHal.h>
#include <BaseManager.h>
#include <AudioSession.h>
#include <RtpConfig.h>
#include <MediaQualityThreshold.h>
#include <unordered_map>
#include <android/content/AttributionSourceState.h>

using namespace std;
using namespace android::telephony::imsmedia;

class VoiceManager : public BaseManager {
public:
    /**
     * @class   RequestHandler
     * @brief   request serialization
     */
    class RequestHandler : public ImsMediaEventHandler {
    public:
        RequestHandler();
        virtual ~RequestHandler();
    protected:
        virtual void processEvent(uint32_t event, uint64_t pParam, uint64_t lParam);
    };

    /**
     * @class   ResponseHandler
     * @brief   ResponseHandler
     *                  has its own thread and sent the response to the client in its own thread
     */
    class ResponseHandler : public ImsMediaEventHandler {
    public:
        ResponseHandler();
        virtual ~ResponseHandler();
    protected:
        virtual void processEvent(uint32_t event, uint64_t pParam, uint64_t lParam);
    };

    static VoiceManager* getInstance();
    static void setAttributeSource(const android::content::AttributionSourceState& client);
    static android::content::AttributionSourceState& getAttributeSource();
    virtual void sendMessage(const int sessionid, const android::Parcel& parcel);

private:
    VoiceManager();
    virtual ~VoiceManager();
    bool openSession(int sessionid, int rtpFd, int rtcpFd, RtpConfig* config);
    ImsMediaResult closeSession(int sessionid);
    bool modifySession(int sessionid, RtpConfig* config);
    void addConfig(int sessionid, RtpConfig* config);
    bool deleteConfig(int sessionid, RtpConfig* config);
    void confirmConfig(int sessionid, RtpConfig* config);
    void startDtmf(int sessionid, char dtmfDigit, int volume, int duration);
    void stopDtmf(int sessionid);
    //void sendHeaderExtension(int sessionid, RtpHeaderExtension* data);
    void setMediaQualityThreshold(int sessionid, MediaQualityThreshold* threshold);

    static VoiceManager* sManager;
    static android::content::AttributionSourceState mAttributionSource;
    std::unordered_map<int, std::unique_ptr<AudioSession>> mSessions;
    RequestHandler mRequestHandler;
    ResponseHandler mResponseHandler;
};

#endif