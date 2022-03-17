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

/** Native representation of android.telephony.imsmedia.AudioConfig */
class AudioConfig : public RtpConfig {
public:
    enum CodecType {
        /** Adaptive Multi-Rate */
        CODEC_AMR = 1 << 0,
        /** Adaptive Multi-Rate Wide Band */
        CODEC_AMR_WB = 1 << 1,
        /** Enhanced Voice Services */
        CODEC_EVS = 1 << 2,
        /** G.711 A-law i.e. Pulse Code Modulation using A-law */
        CODEC_PCMA = 1 << 3,
        /** G.711 μ-law i.e. Pulse Code Modulation using μ-law */
        CODEC_PCMU = 1 << 4,
    };

    AudioConfig();
    AudioConfig(AudioConfig* config);
    AudioConfig(AudioConfig& config);
    virtual ~AudioConfig();
    AudioConfig& operator=(const AudioConfig& config);
    bool operator==(const AudioConfig &config) const;
    bool operator!=(const AudioConfig &config) const;
    virtual status_t writeToParcel(Parcel* parcel) const;
    virtual status_t readFromParcel(const Parcel* in);
    void setPtimeMillis(int8_t ptime);
    int8_t getPtimeMillis();
    void setMaxPtimeMillis(int8_t maxPtime);
    int8_t getMaxPtimeMillis();
    void setTxCodecModeRequest(int8_t cmr);
    int8_t getTxCodecModeRequest();
    void setDtxEnabled(bool enable);
    bool getDtxEnabled();
    void setCodecType(int32_t type);
    int32_t getCodecType();
    void setDtmfPayloadTypeNumber(int32_t num);
    int32_t getDtmfPayloadTypeNumber();
    void setDtmfsamplingRateKHz(int32_t sampling);
    int32_t getDtmfsamplingRateKHz();
    void setAmrParams(const AmrParams& param);
    AmrParams getAmrParams();
    void setEvsParams(const EvsParams& param);
    EvsParams getEvsParams();

protected:
    int8_t pTimeMillis;
    int8_t maxPtimeMillis;
    int8_t txCodecModeRequest;
    bool dtxEnabled;
    int32_t codecType;
    int32_t dtmfPayloadTypeNumber;
    int32_t dtmfsamplingRateKHz;
    AmrParams amrParams;
    EvsParams evsParams;
};

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android

#endif