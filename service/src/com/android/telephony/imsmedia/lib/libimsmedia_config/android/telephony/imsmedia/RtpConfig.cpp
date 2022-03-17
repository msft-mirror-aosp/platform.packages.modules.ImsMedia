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

#include <RtpConfig.h>

namespace android {

namespace telephony {

namespace imsmedia {

const android::String8 kClassNameRtcpConfig("android.telephony.imsmedia.RtcpConfig");

RtpConfig::RtpConfig()
    : direction(0),
    accessNetwork(0),
    remoteAddress(""),
    remotePort(0),
    maxMtuBytes(0),
    dscp(0),
    rxPayloadTypeNumber(0),
    txPayloadTypeNumber(0),
    samplingRateKHz(0) {
}

RtpConfig::~RtpConfig() {

}

RtpConfig::RtpConfig(RtpConfig& config) {
    direction = config.direction;
    accessNetwork = config.accessNetwork;
    remoteAddress = config.remoteAddress;
    remotePort = config.remotePort;
    rtcpConfig = config.rtcpConfig;
    maxMtuBytes = config.maxMtuBytes;
    dscp = config.dscp;
    rxPayloadTypeNumber = config.rxPayloadTypeNumber;
    txPayloadTypeNumber = config.txPayloadTypeNumber;
    samplingRateKHz = config.samplingRateKHz;
}

RtpConfig& RtpConfig::operator=(const RtpConfig& config) {
    direction = config.direction;
    accessNetwork = config.accessNetwork;
    remoteAddress = config.remoteAddress;
    remotePort = config.remotePort;
    rtcpConfig = config.rtcpConfig;
    maxMtuBytes = config.maxMtuBytes;
    dscp = config.dscp;
    rxPayloadTypeNumber = config.rxPayloadTypeNumber;
    txPayloadTypeNumber = config.txPayloadTypeNumber;
    samplingRateKHz = config.samplingRateKHz;
    return *this;
}

bool RtpConfig::operator==(const RtpConfig &config) const{
    return (this->direction == config.direction
        && this->accessNetwork == config.accessNetwork
        && this->remoteAddress == config.remoteAddress
        && this->remotePort == config.remotePort
        && this->rtcpConfig == config.rtcpConfig
        && this->maxMtuBytes == config.maxMtuBytes
        && this->dscp == config.dscp
        && this->rxPayloadTypeNumber == config.rxPayloadTypeNumber
        && this->txPayloadTypeNumber == config.txPayloadTypeNumber
        && this->samplingRateKHz == config.samplingRateKHz);
}

bool RtpConfig::operator!=(const RtpConfig &config) const{
    return (this->direction != config.direction
        || this->accessNetwork != config.accessNetwork
        || this->remoteAddress != config.remoteAddress
        || this->remotePort != config.remotePort
        || this->rtcpConfig != config.rtcpConfig
        || this->maxMtuBytes != config.maxMtuBytes
        || this->dscp != config.dscp
        || this->rxPayloadTypeNumber != config.rxPayloadTypeNumber
        || this->txPayloadTypeNumber != config.txPayloadTypeNumber
        || this->samplingRateKHz != config.samplingRateKHz);
}

status_t RtpConfig::writeToParcel(Parcel* out) const {
    status_t err;

    err = out->writeInt32(direction);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeInt32(accessNetwork);
    if (err != NO_ERROR) {
        return err;
    }

    String16 address(remoteAddress);
    err = out->writeString16(address);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeInt32(remotePort);
    if (err != NO_ERROR) {
        return err;
    }

    String16 className(kClassNameRtcpConfig);
    err = out->writeString16(className);
    if (err != NO_ERROR) {
        return err;
    }

    err = rtcpConfig.writeToParcel(out);
    //err = out->writeParcelable(rtcpConfig);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeInt32(maxMtuBytes);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeInt32(dscp);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeInt32(rxPayloadTypeNumber);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeInt32(txPayloadTypeNumber);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeByte(samplingRateKHz);
    if (err != NO_ERROR) {
        return err;
    }

    return NO_ERROR;
}

status_t RtpConfig::readFromParcel(const Parcel* in) {
    status_t err;

    err = in->readInt32(&direction);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readInt32(&accessNetwork);
    if (err != NO_ERROR) {
        return err;
    }

    String16 address;
    err = in->readString16(&address);
    if (err != NO_ERROR) {
        return err;
    }

    remoteAddress = String8(address.string());

    err = in->readInt32(&remotePort);
    if (err != NO_ERROR) {
        return err;
    }

    String16 className;
    err = in->readString16(&className);
    if (err != NO_ERROR) {
        return err;
    }

    //read RtcpConfig
    String8 className2 = String8(className.string());
    err = rtcpConfig.readFromParcel(in);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readInt32(&maxMtuBytes);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readInt32(&dscp);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readInt32(&rxPayloadTypeNumber);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readInt32(&txPayloadTypeNumber);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readByte(&samplingRateKHz);
    if (err != NO_ERROR) {
        return err;
    }

    return NO_ERROR;
}

void RtpConfig::setMediaDirection(int32_t direction) {
    this->direction = direction;
}

int32_t RtpConfig::getMediaDirection() {
    return direction;
}

void RtpConfig::setRemoteAddress(String8 address) {
    this->remoteAddress = address;
}

String8 RtpConfig::getRemoteAddress() {
    return remoteAddress;
}

void RtpConfig::setRemotePort(int32_t port) {
    this->remotePort = port;
}

int32_t RtpConfig::getRemotePort() {
    return remotePort;
}

void RtpConfig::setRtcpConfig(const RtcpConfig& config) {
    this->rtcpConfig = config;
}

RtcpConfig RtpConfig::getRtcpConfig() {
    return rtcpConfig;
}

void RtpConfig::setMaxMtuBytes(int32_t mtu) {
    this->maxMtuBytes = mtu;
}

int32_t RtpConfig::getmaxMtuBytes() {
    return maxMtuBytes;
}

void RtpConfig::setDscp(int dscp) {
    this->dscp = dscp;
}

int32_t RtpConfig::getDscp() {
    return dscp;
}

void RtpConfig::setRxPayloadTypeNumber(int32_t num) {
    this->rxPayloadTypeNumber = num;
}

int32_t RtpConfig::getRxPayloadTypeNumber() {
    return rxPayloadTypeNumber;
}

void RtpConfig::setTxPayloadTypeNumber(int32_t num) {
    this->txPayloadTypeNumber = num;
}

int32_t RtpConfig::getTxPayloadTypeNumber() {
    return txPayloadTypeNumber;
}

void RtpConfig::setSamplingRateKHz(int8_t sample) {
    this->samplingRateKHz = sample;
}

int8_t RtpConfig::getSamplingRateKHz() {
    return samplingRateKHz;
}

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android
