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

RtpConfig::RtpConfig()
    : direction(0),
    remoteAddress(),
    remotePort(0) {

}

RtpConfig::~RtpConfig() {

}

RtpConfig::RtpConfig(RtpConfig& config) {
    direction = config.direction;
    accessNetwork = config.accessNetwork;
    remoteAddress = config.remoteAddress;
    remotePort = config.remotePort;
    rtcpConfig = config.rtcpConfig;
    //QosSessionAttributes qos;
    maxMtuBytes = config.maxMtuBytes;
    dscp = config.dscp;
    rxPayloadTypeNumber = config.rxPayloadTypeNumber;
    txPayloadTypeNumber = config.txPayloadTypeNumber;
    samplingRateKHz = config.samplingRateKHz;
}

status_t RtpConfig::writeToParcel(Parcel* parcel) const {
    (void)parcel;
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

    err = in->readString16(&remoteAddress);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readInt32(&remotePort);
    if (err != NO_ERROR) {
        return err;
    }

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


int32_t RtpConfig::getMediaDirection() {
    return direction;
}

String16 RtpConfig::getRemoteAddress() {
    return remoteAddress;
}

int32_t RtpConfig::getRemotePort() {
    return remotePort;
}

RtcpConfig RtpConfig::getRtcpConfig() {
    return rtcpConfig;
}

int32_t RtpConfig::getmaxMtuBytes() {
    return maxMtuBytes;
}

int32_t RtpConfig::getDscp() {
    return dscp;
}

int32_t RtpConfig::getRxPayloadTypeNumber() {
    return rxPayloadTypeNumber;
}

int32_t RtpConfig::getTxPayloadTypeNumber() {
    return txPayloadTypeNumber;
}

int8_t RtpConfig::getSamplingRateKHz() {
    return samplingRateKHz;
}

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android
