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

#include <MediaQualityThreshold.h>

namespace android {
namespace telephony {
namespace imsmedia {

MediaQualityThreshold::MediaQualityThreshold() {

}

MediaQualityThreshold::~MediaQualityThreshold() {

}

MediaQualityThreshold::MediaQualityThreshold(Parcel& in) {
    mRtpInactivityTimerMillis = in.readInt32();
    mRtcpInactivityTimerMillis = in.readInt32();
    mRtpPacketLossDurationMillis = in.readInt32();
    mRtpPacketLossRate = in.readInt32();
    mJitterDurationMillis = in.readInt32();
    mRtpJitterMillis = in.readInt32();
}

MediaQualityThreshold::MediaQualityThreshold(int rtpInactivityTimerMillis,
        int rtcpInactivityTimerMillis, int rtpPacketLossDurationMillis, int rtpPacketLossRate,
        int jitterDurationMillis, int rtpJitterMillis) {
    mRtpInactivityTimerMillis = rtpInactivityTimerMillis;
    mRtcpInactivityTimerMillis = rtcpInactivityTimerMillis;
    mRtpPacketLossDurationMillis = rtpPacketLossDurationMillis;
    mRtpPacketLossRate = rtpPacketLossRate;
    mJitterDurationMillis = jitterDurationMillis;
    mRtpJitterMillis = rtpJitterMillis;
}

status_t MediaQualityThreshold::writeToParcel(Parcel* out) const {
    status_t err;
    err = out->writeInt32(mRtpInactivityTimerMillis);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeInt32(mRtcpInactivityTimerMillis);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeInt32(mRtpPacketLossDurationMillis);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeInt32(mRtpPacketLossRate);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeInt32(mJitterDurationMillis);
    if (err != NO_ERROR) {
        return err;
    }

    err = out->writeInt32(mRtpJitterMillis);
    if (err != NO_ERROR) {
        return err;
    }
    return NO_ERROR;
}

status_t MediaQualityThreshold::readFromParcel(const Parcel* in) {
    status_t err;
    err = in->readInt32(&mRtpInactivityTimerMillis);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readInt32(&mRtcpInactivityTimerMillis);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readInt32(&mRtpPacketLossDurationMillis);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readInt32(&mRtpPacketLossRate);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readInt32(&mJitterDurationMillis);
    if (err != NO_ERROR) {
        return err;
    }

    err = in->readInt32(&mRtpJitterMillis);
    if (err != NO_ERROR) {
        return err;
    }

    return NO_ERROR;
}

}
}
}