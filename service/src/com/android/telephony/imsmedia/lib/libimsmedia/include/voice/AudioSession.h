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

#ifndef AUDIO_SESSION_H
#define AUDIO_SESSION_H

#include <ImsMediaDefine.h>
#include <BaseSession.h>
#include <AudioStreamGraphRtpTx.h>
#include <AudioStreamGraphRtpRx.h>
#include <AudioStreamGraphRtcp.h>
#include <RtpConfig.h>
#include <list>

class AudioSession : public BaseSession
{
public:
    AudioSession();
    virtual ~AudioSession();
    virtual ImsMediaResult startGraph(RtpConfig* config);
    virtual ImsMediaResult addGraph(RtpConfig* config);
    virtual ImsMediaResult confirmGraph(RtpConfig* config);
    virtual ImsMediaResult deleteGraph(RtpConfig* config);
    virtual void setMediaQualityThreshold(MediaQualityThreshold* threshold);
    // BaseSessionCallback
    virtual void onEvent(ImsMediaEventType type, uint64_t param1, uint64_t param2);
    // dtmf method
    void startDtmf(char digit, int volume, int duration);
    void stopDtmf();

private:
    std::list<AudioStreamGraphRtpTx*> mListGraphRtpTx;
    std::list<AudioStreamGraphRtpRx*> mListGraphRtpRx;
    std::list<AudioStreamGraphRtcp*> mListGraphRtcp;
};

#endif