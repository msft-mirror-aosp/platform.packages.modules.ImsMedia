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

#ifndef AUDIOCONFIG_H
#define AUDIOCONFIG_H

#include <binder/Parcel.h>
#include <binder/Parcelable.h>
#include <binder/Status.h>
#include <RtpConfig.h>
#include <EvsParams.h>
#include <AmrParams.h>

namespace android {

namespace telephony {

namespace imsmedia {

class AudioConfig : public RtpConfig {
public:
    enum CodecType {
        CODEC_AMR,
        CODEC_AMR_WB,
        CODEC_EVS,
        CODEC_PCMA,
        CODEC_PCMU,
    };
    enum EvsBandwidth {
        EVS_BAND_NONE,
        EVS_NARROW_BAND,
        EVS_WIDE_BAND,
        EVS_SUPER_WIDE_BAND,
        EVS_FULL_BAND,
    };

    AudioConfig();
    AudioConfig(AudioConfig* config);
    AudioConfig(AudioConfig& config);
    virtual ~AudioConfig();
    virtual status_t writeToParcel(Parcel* parcel) const;
    virtual status_t readFromParcel(const Parcel* in);
    int8_t getPtimeMillis();
    int8_t getMaxPtimeMillis();
    int8_t getTxCodecModeRequest();
    bool getDtxEnabled();
    int32_t getCodecType();
    int32_t getEvsBandwidth();
    int32_t getDtmfPayloadTypeNumber();
    int32_t getDtmfsamplingRateKHz();
    AmrParams getAmrParams();
    EvsParams getEvsParams();

private:
    int8_t pTimeMillis;
    int8_t maxPtimeMillis;
    int8_t txCodecModeRequest;
    bool dtxEnabled;
    int32_t codecType;
    int32_t evsBandwidth;
    int32_t dtmfPayloadTypeNumber;
    int32_t dtmfsamplingRateKHz;
    AmrParams amrParams;
    EvsParams evsParams;
};

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android

#endif