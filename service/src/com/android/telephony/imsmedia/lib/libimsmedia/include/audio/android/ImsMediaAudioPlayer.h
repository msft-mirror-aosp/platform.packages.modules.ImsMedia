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

#ifndef IMSMEDIA_AUDIO_PLAYER_INCLUDED
#define IMSMEDIA_AUDIO_PLAYER_INCLUDED

#include <ImsMediaAudioDefine.h>
#include <aaudio/AAudio.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>
#include <mutex>

using android::sp;

class ImsMediaAudioPlayer
{
public:
    ImsMediaAudioPlayer();
    ~ImsMediaAudioPlayer();
    void SetCodec(int32_t type);
    void SetCodecMode(uint32_t mode);
    void SetEvsBitRate();
    void SetSamplingRate(int32_t samplingRate);
    void SetEvsChAwOffset(int32_t offset);
    void SetEvsBandwidth(kEvsBandwidth evsBandwidth);
    bool Start();
    void Stop();
    bool onDataFrame(uint8_t* buffer, uint32_t size);
    void SetEvsPayloadHeaderMode(kRtpPyaloadHeaderMode EvsPayloadHeaderMode);

private:
    void openAudioStream();
    void restartAudioStream();
    static void audioErrorCallback(AAudioStream* stream, void* userData, aaudio_result_t error);

    AAudioStream* mAudioStream;
    AMediaCodec* mCodec;
    AMediaFormat* mFormat;
    int32_t mCodecType;
    uint32_t mCodecMode;
    int32_t mSamplingRate;
    int32_t mEvsChAwOffset;
    kEvsBandwidth mEvsBandwidth;
    uint16_t mBuffer[PCM_BUFFER_SIZE];
    std::mutex mMutex;
    int32_t mEvsBitRate;
    kRtpPyaloadHeaderMode mEvsCodecHeaderMode;
};

#endif
