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

#ifndef MEDIA_QUALITY_THRESHOLD_H
#define MEDIA_QUALITY_THRESHOLD_H

#include <binder/Parcel.h>
#include <binder/Parcelable.h>
#include <binder/Status.h>
#include <stdint.h>

namespace android {

namespace telephony {

namespace imsmedia {

class MediaQualityThreshold : public Parcelable {
public:
    MediaQualityThreshold();
    MediaQualityThreshold(Parcel& in);
    virtual ~MediaQualityThreshold();
    virtual status_t writeToParcel(Parcel* parcel) const;
    virtual status_t readFromParcel(const Parcel* in);
    int32_t getRtpInactivityTimerMillis();
    int32_t getRtcpInactivityTimerMillis();
    int32_t getRtpPacketLossDurationMillis();
    int32_t getRtpPacketLossRate();
    int32_t getJitterDurationMillis();
    int32_t getRtpJitterMillis();

private:
    /** Timer in milliseconds for monitoring RTP inactivity */
    int32_t mRtpInactivityTimerMillis;
    /** Timer in milliseconds for monitoring RTCP inactivity */
    int32_t mRtcpInactivityTimerMillis;
    /** Duration in milliseconds for monitoring the RTP packet loss rate */
    int32_t mRtpPacketLossDurationMillis;
    /**
     * Packet loss rate in percentage of (total number of packets lost) /
     * (total number of packets expected) during rtpPacketLossDurationMs
     */
    int32_t mRtpPacketLossRate;
    /** Duration in milliseconds for monitoring the jitter for RTP traffic */
    int32_t mJitterDurationMillis;
    /** RTP jitter threshold in milliseconds */
    int32_t mRtpJitterMillis;
};

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android

#endif