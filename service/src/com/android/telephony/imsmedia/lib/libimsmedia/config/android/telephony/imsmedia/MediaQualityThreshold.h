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

namespace android {

namespace telephony {

namespace imsmedia {

class MediaQualityThreshold : public Parcelable {
private:
    /** Timer in milliseconds for monitoring RTP inactivity */
    int mRtpInactivityTimerMillis;
    /** Timer in milliseconds for monitoring RTCP inactivity */
    int mRtcpInactivityTimerMillis;
    /** Duration in milliseconds for monitoring the RTP packet loss rate */
    int mRtpPacketLossDurationMillis;
    /**
     * Packet loss rate in percentage of (total number of packets lost) /
     * (total number of packets expected) during rtpPacketLossDurationMs
     */
    int mRtpPacketLossRate;
    /** Duration in milliseconds for monitoring the jitter for RTP traffic */
    int mJitterDurationMillis;
    /** RTP jitter threshold in milliseconds */
    int mRtpJitterMillis;

public:
    MediaQualityThreshold();
    virtual ~MediaQualityThreshold();
    MediaQualityThreshold(Parcel& in);\
    MediaQualityThreshold(int rtpInactivityTimerMillis, int rtcpInactivityTimerMillis,
            int rtpPacketLossDurationMillis, int rtpPacketLossRate,
            int jitterDurationMillis, int rtpJitterMillis);
    virtual status_t writeToParcel(Parcel* out) const;
    virtual status_t readFromParcel(const Parcel* in);
};

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android

#endif