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

#include <AmrParams.h>

namespace android {

namespace telephony {

namespace imsmedia {

AmrParams::AmrParams() {
    amrMode = 0;
    octetAligned = false;
    maxRedundancyMillis = 0;
}

AmrParams::AmrParams(AmrParams& param) {
    this->amrMode = param.amrMode;
    this->octetAligned = param.octetAligned;
    this->maxRedundancyMillis = param.maxRedundancyMillis;
}

AmrParams::~AmrParams() {

}

status_t AmrParams::writeToParcel(Parcel* parcel) const {
    (void)parcel;
    return NO_ERROR;
}

status_t AmrParams::readFromParcel(const Parcel* in) {
    status_t err;
    err = in->readInt32(&amrMode);
    if (err != NO_ERROR) {
        return err;
    }

    int32_t value = 0;
    err = in->readInt32(&value);
    if (err != NO_ERROR) {
        return err;
    }

    value == 0 ? octetAligned = false : octetAligned = true;

    err = in->readInt32(&maxRedundancyMillis);
    if (err != NO_ERROR) {
        return err;
    }

    return NO_ERROR;
}

int32_t AmrParams::getAmrMode() {
    return amrMode;
}

bool AmrParams::getOctetAligned() {
    return octetAligned;
}

int32_t AmrParams::getMaxRedundancyMillis() {
    return maxRedundancyMillis;
}

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android
