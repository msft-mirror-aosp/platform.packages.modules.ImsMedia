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

#include <EvsParams.h>

namespace android {

namespace telephony {

namespace imsmedia {

EvsParams::EvsParams() {
    this->channelAwareMode = 0;
    this->useHeaderFullOnlyOnTx = false;
    this->useHeaderFullOnlyOnRx = false;
}

EvsParams::EvsParams(EvsParams& params) {
    this->channelAwareMode = params.channelAwareMode;
    this->useHeaderFullOnlyOnTx = params.useHeaderFullOnlyOnTx;
    this->useHeaderFullOnlyOnRx = params.useHeaderFullOnlyOnRx;
}

EvsParams::~EvsParams() {

}

status_t EvsParams::writeToParcel(Parcel* parcel) const {
    (void)parcel;
    return NO_ERROR;
}

status_t EvsParams::readFromParcel(const Parcel* in) {
    status_t err;
    err = in->readInt32(&evsMode);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readByte(&channelAwareMode);
    if (err != NO_ERROR) {
        return err;
    }

    int32_t value = 0;
    err = in->readInt32(&value);
    if (err != NO_ERROR) {
        return err;
    }

    value == 0 ? useHeaderFullOnlyOnTx = false : useHeaderFullOnlyOnTx = true;

    err = in->readInt32(&value);
    if (err != NO_ERROR) {
        return err;
    }

    value == 0 ? useHeaderFullOnlyOnRx = false : useHeaderFullOnlyOnRx = true;

    return NO_ERROR;
}

int32_t EvsParams::getEvsMode() {
    return evsMode;
}

int8_t EvsParams::getChannelAwareMode() {
    return channelAwareMode;
}

bool EvsParams::getUseHeaderFullOnlyOnTx() {
    return useHeaderFullOnlyOnTx;
}

bool EvsParams::getMaxRedundancyMillis() {
    return useHeaderFullOnlyOnRx;
}

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android
