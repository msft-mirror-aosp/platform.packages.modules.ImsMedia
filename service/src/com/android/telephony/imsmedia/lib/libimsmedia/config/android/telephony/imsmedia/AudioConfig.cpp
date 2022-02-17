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

#include <AudioConfig.h>

namespace android {

namespace telephony {

namespace imsmedia {

AudioConfig::AudioConfig() {
    pTimeMillis = 0;
    maxPtimeMillis = 0;
    txCodecModeRequest = 0;
    dtxEnabled = false;
    codecType = 0;
    evsBandwidth = 0;
    dtmfPayloadTypeNumber = 0;
    dtmfsamplingRateKHz = 0;
}

AudioConfig::AudioConfig(AudioConfig* config) {
    if (config != NULL) {
        pTimeMillis = config->pTimeMillis;
        maxPtimeMillis = config->maxPtimeMillis;
        txCodecModeRequest = config->txCodecModeRequest;
        dtxEnabled = config->dtxEnabled;
        codecType = config->codecType;
        evsBandwidth = config->evsBandwidth;
        dtmfPayloadTypeNumber = config->dtmfPayloadTypeNumber;
        dtmfsamplingRateKHz = config->dtmfsamplingRateKHz;
    }
}

AudioConfig::AudioConfig(AudioConfig& config) {
    pTimeMillis = config.pTimeMillis;
    maxPtimeMillis = config.maxPtimeMillis;
    txCodecModeRequest = config.txCodecModeRequest;
    dtxEnabled = config.dtxEnabled;
    codecType = config.codecType;
    evsBandwidth = config.evsBandwidth;
    dtmfPayloadTypeNumber = config.dtmfPayloadTypeNumber;
    dtmfsamplingRateKHz = config.dtmfsamplingRateKHz;
}

AudioConfig::~AudioConfig() {

}

status_t AudioConfig::writeToParcel(Parcel* parcel) const {
    (void)parcel;
    return NO_ERROR;
}

status_t AudioConfig::readFromParcel(const Parcel* in) {
    status_t err;
    err = RtpConfig::readFromParcel(in);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readByte(&pTimeMillis);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readByte(&maxPtimeMillis);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readByte(&txCodecModeRequest);
    if (err != NO_ERROR) {
        return err;
    }

    int32_t value = 0;
    err = in->readInt32(&value);
    if (err != NO_ERROR) {
        return err;
    }

    value == 0 ? dtxEnabled = false : dtxEnabled = true;

    err = in->readInt32(&codecType);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readInt32(&evsBandwidth);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readInt32(&dtmfPayloadTypeNumber);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readInt32(&dtmfsamplingRateKHz);
    if (err != NO_ERROR) {
        return err;
    }

    err = amrParams.readFromParcel(in);
    if (err != NO_ERROR) {
        return err;
    }

    err = evsParams.readFromParcel(in);
    if (err != NO_ERROR) {
        return err;
    }

    return NO_ERROR;
}

int8_t AudioConfig::getPtimeMillis() {
    return pTimeMillis;
}

int8_t AudioConfig::getMaxPtimeMillis() {
    return maxPtimeMillis;
}

int8_t AudioConfig::getTxCodecModeRequest() {
    return txCodecModeRequest;
}

bool AudioConfig::getDtxEnabled() {
    return dtxEnabled;
}

int32_t AudioConfig::getCodecType() {
    return codecType;
}

int32_t AudioConfig::getEvsBandwidth() {
    return evsBandwidth;
}

int32_t AudioConfig::getDtmfPayloadTypeNumber() {
    return dtmfPayloadTypeNumber;
}

int32_t AudioConfig::getDtmfsamplingRateKHz() {
    return dtmfsamplingRateKHz;
}

AmrParams AudioConfig::getAmrParams() {
    return amrParams;
}

EvsParams AudioConfig::getEvsParams() {
    return evsParams;
}

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android
