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

#ifndef AUDIO_TRACK_H_INCLUDED
#define AUDIO_TRACK_H_INCLUDED

#include <ImsMediaAudioDefine.h>
#include <media/AudioTrack.h>
#include <media/stagefright/MediaCodec.h>

using android::sp;
using android::AudioTrack;
using android::MediaCodec;

class ImsMediaVoiceRenderer {
private:
    sp<AudioTrack> m_AudioTrack;
    sp<MediaCodec> m_MediaCodec;
    eAudioCodecType m_nCodecType;
    uint32_t m_amr_mode;
#ifdef DEBUG_PCM_DUMP
    FILE    *fPCMDump; // file pointer for PCM Dump
    uint8_t bPCMDump[PCM_DUMP_SIZE];  // buffer of PCM Dump
#endif

public:
    ImsMediaVoiceRenderer();
    ~ImsMediaVoiceRenderer();
    void SetCodec(eAudioCodecType eCodecType);
    void SetCodecMode(uint32_t mode);
    bool Start();
    void Stop();
    bool onDataFrame(uint8_t* buffer, uint32_t size);
};

#endif
