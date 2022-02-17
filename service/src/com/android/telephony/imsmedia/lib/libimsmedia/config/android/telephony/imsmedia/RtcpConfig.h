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

#ifndef RTCPCONFIG_H
#define RTCPCONFIG_H

#include <binder/Parcel.h>
#include <binder/Parcelable.h>
#include <binder/Status.h>
#include <stdint.h>

namespace android {

namespace telephony {

namespace imsmedia {

class RtcpConfig : public Parcelable {
public:
    enum RtcpXrBlockType {
        FLAG_RTCPXR_NONE = 0,
        FLAG_RTCPXR_LOSS_RLE_REPORT_BLOCK = 1 < 0,
        FLAG_RTCPXR_DUPLICATE_RLE_REPORT_BLOCK = 1 << 1,
        FLAG_RTCPXR_PACKET_RECEIPT_TIMES_REPORT_BLOCK = 1 << 2,
        FLAG_RTCPXR_RECEIVER_REFERENCE_TIME_REPORT_BLOCK = 1 << 3,
        FLAG_RTCPXR_DLRR_REPORT_BLOCK = 1 << 4,
        FLAG_RTCPXR_STATISTICS_SUMMARY_REPORT_BLOCK = 1 << 5,
        FLAG_RTCPXR_VOIP_METRICS_REPORT_BLOCK = 1 << 6,
    };

    RtcpConfig();
    RtcpConfig(RtcpConfig& config);
    virtual ~RtcpConfig();
    RtcpConfig& operator=(const RtcpConfig& config);
    virtual status_t writeToParcel(Parcel* parcel) const;
    virtual status_t readFromParcel(const Parcel* in);
    String16 getCanonicalName();
    int32_t getTransmitPort();
    int32_t getIntervalSec();
    int32_t getRtcpXrBlockTypes();

private:
    /** Canonical name that will be sent to all session participants */
    String16 canonicalName;

    /** UDP port number for sending outgoing RTCP packets */
    int32_t transmitPort;

    /**
     * RTCP transmit interval in seconds. The value 0 indicates that RTCP
     * reports shall not be sent to the other party.
     */
    int32_t intervalSec;

    /** Bitmask of RTCP-XR blocks to enable as in RtcpXrReportBlockType */
    int32_t rtcpXrBlockTypes;
};

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android

#endif