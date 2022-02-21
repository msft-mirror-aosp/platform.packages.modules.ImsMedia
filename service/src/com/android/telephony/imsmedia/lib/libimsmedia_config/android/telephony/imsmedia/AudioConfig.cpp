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

AudioConfig& AudioConfig::operator=(const AudioConfig& config) {
    pTimeMillis = config.pTimeMillis;
    maxPtimeMillis = config.maxPtimeMillis;
    txCodecModeRequest = config.txCodecModeRequest;
    dtxEnabled = config.dtxEnabled;
    codecType = config.codecType;
    evsBandwidth = config.evsBandwidth;
    dtmfPayloadTypeNumber = config.dtmfPayloadTypeNumber;
    dtmfsamplingRateKHz = config.dtmfsamplingRateKHz;
    return *this;
}

bool AudioConfig::operator==(const AudioConfig &config) const {
    return (this->direction == config.direction
        && this->accessNetwork == config.accessNetwork
        && this->remoteAddress == config.remoteAddress
        && this->remotePort == config.remotePort
        && this->rtcpConfig == config.rtcpConfig
        && this->maxMtuBytes == config.maxMtuBytes
        && this->dscp == config.dscp
        && this->rxPayloadTypeNumber == config.rxPayloadTypeNumber
        && this->txPayloadTypeNumber == config.txPayloadTypeNumber
        && this->samplingRateKHz == config.samplingRateKHz
        && this->pTimeMillis == config.pTimeMillis
        && this->maxPtimeMillis == config.maxPtimeMillis
        && this->txCodecModeRequest == config.txCodecModeRequest
        && this->dtxEnabled == config.dtxEnabled
        && this->codecType == config.codecType
        && this->evsBandwidth == config.evsBandwidth
        && this->dtmfPayloadTypeNumber == config.dtmfPayloadTypeNumber
        && this->dtmfsamplingRateKHz == config.dtmfsamplingRateKHz);
}

bool AudioConfig::operator!=(const AudioConfig &config) const {
    return (this->direction != config.direction
        || this->accessNetwork != config.accessNetwork
        || this->remoteAddress != config.remoteAddress
        || this->remotePort != config.remotePort
        || this->rtcpConfig != config.rtcpConfig
        || this->maxMtuBytes != config.maxMtuBytes
        || this->dscp != config.dscp
        || this->rxPayloadTypeNumber != config.rxPayloadTypeNumber
        || this->txPayloadTypeNumber != config.txPayloadTypeNumber
        || this->samplingRateKHz != config.samplingRateKHz
        || this->pTimeMillis != config.pTimeMillis
        || this->maxPtimeMillis != config.maxPtimeMillis
        || this->txCodecModeRequest != config.txCodecModeRequest
        || this->dtxEnabled != config.dtxEnabled
        || this->codecType != config.codecType
        || this->evsBandwidth != config.evsBandwidth
        || this->dtmfPayloadTypeNumber != config.dtmfPayloadTypeNumber
        || this->dtmfsamplingRateKHz != config.dtmfsamplingRateKHz);
}

status_t AudioConfig::writeToParcel(Parcel* out) const {
    status_t err;
    err = RtpConfig::writeToParcel(out);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeByte(pTimeMillis);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeByte(maxPtimeMillis);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeByte(txCodecModeRequest);
    if (err != NO_ERROR) {
        return err;
    }

    int32_t value = 0;
    dtxEnabled ? value = 1 : value = 0;
    err = out->writeInt32(value);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeInt32(codecType);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeInt32(evsBandwidth);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeInt32(dtmfPayloadTypeNumber);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeInt32(dtmfsamplingRateKHz);
    if (err != NO_ERROR) {
        return err;
    }

    err = amrParams.writeToParcel(out);
    if (err != NO_ERROR) {
        return err;
    }

    err = evsParams.writeToParcel(out);
    if (err != NO_ERROR) {
        return err;
    }

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

void AudioConfig::setPtimeMillis(int8_t ptime) {
    pTimeMillis = ptime;
}

int8_t AudioConfig::getPtimeMillis() {
    return pTimeMillis;
}

void AudioConfig::setMaxPtimeMillis(int8_t maxPtime) {
    maxPtimeMillis = maxPtime;
}

int8_t AudioConfig::getMaxPtimeMillis() {
    return maxPtimeMillis;
}

void AudioConfig::setTxCodecModeRequest(int8_t cmr) {
    txCodecModeRequest = cmr;
}

int8_t AudioConfig::getTxCodecModeRequest() {
    return txCodecModeRequest;
}

void AudioConfig::setDtxEnabled(bool enable) {
    dtxEnabled = enable;
}

bool AudioConfig::getDtxEnabled() {
    return dtxEnabled;
}

void AudioConfig::setCodecType(int32_t type) {
    codecType = type;
}

int32_t AudioConfig::getCodecType() {
    return codecType;
}

void AudioConfig::setEvsBandwidth(int32_t bandwidth) {
    evsBandwidth = bandwidth;
}

int32_t AudioConfig::getEvsBandwidth() {
    return evsBandwidth;
}

void AudioConfig::setDtmfPayloadTypeNumber(int32_t num) {
    dtmfPayloadTypeNumber = num;
}

int32_t AudioConfig::getDtmfPayloadTypeNumber() {
    return dtmfPayloadTypeNumber;
}

void AudioConfig::setDtmfsamplingRateKHz(int32_t sampling) {
    dtmfsamplingRateKHz = sampling;
}

int32_t AudioConfig::getDtmfsamplingRateKHz() {
    return dtmfsamplingRateKHz;
}

void AudioConfig::setAmrParams(const AmrParams& param) {
    amrParams = param;
}

AmrParams AudioConfig::getAmrParams() {
    return amrParams;
}

void AudioConfig::setEvsParams(const EvsParams& param) {
    evsParams = param;
}

EvsParams AudioConfig::getEvsParams() {
    return evsParams;
}

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android
