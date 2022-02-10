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
    : mDirection(0),
    mRemoteAddress(),
    mRemotePort(0) {

}

RtpConfig::~RtpConfig() {

}

RtpConfig::RtpConfig(RtpConfig& config) {
    mDirection = config.mDirection;
    mRemoteAddress = config.mRemoteAddress;
    mRemotePort = config.mRemotePort;
}

RtpConfig::RtpConfig(int direction, String16& remoteAddress, int remotePort)
    : mDirection(direction),
    mRemoteAddress(remoteAddress),
    mRemotePort(remotePort) {
}

int RtpConfig::getMediaDirection() {
    return mDirection;
}

void RtpConfig::setMediaDirection(int direction) {
    mDirection = direction;
}

String16 RtpConfig::getRemoteAddress() {
    return mRemoteAddress;
}

int RtpConfig::getRemotePort() {
    return mRemotePort;
}

status_t RtpConfig::writeToParcel(Parcel* out) const {
    status_t err;
    err = out->writeInt32(mDirection);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeString16(mRemoteAddress);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeInt32(mRemotePort);
    if (err != NO_ERROR) {
        return err;
    }

    return NO_ERROR;
}

status_t RtpConfig::readFromParcel(const Parcel* in) {
    status_t err;

    err = in->readInt32(&mDirection);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readString16(&mRemoteAddress);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readInt32(&mRemotePort);
    if (err != NO_ERROR) {
        return err;
    }

    return NO_ERROR;
}

}
}
}