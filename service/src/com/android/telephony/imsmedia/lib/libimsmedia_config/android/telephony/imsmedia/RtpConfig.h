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

#ifndef RTPCONFIG_H
#define RTPCONFIG_H

#include <binder/Parcel.h>
#include <binder/Parcelable.h>
#include <binder/Status.h>
#include <RtcpConfig.h>
#include <stdint.h>

namespace android {

namespace telephony {

namespace imsmedia {

/** Native representation of android.telephony.imsmedia.RtpConfig */
class RtpConfig : public Parcelable {
public:
    enum MediaDirection {
        MEDIA_DIRECTION_NO_FLOW,
        MEDIA_DIRECTION_TRANSMIT_ONLY,
        MEDIA_DIRECTION_RECEIVE_ONLY,
        MEDIA_DIRECTION_TRANSMIT_RECEIVE,
    };

    RtpConfig();
    RtpConfig(RtpConfig& config);
    virtual ~RtpConfig();
    bool operator==(const RtpConfig &c2) const;
    bool operator!=(const RtpConfig &c2) const;
    virtual status_t writeToParcel(Parcel* parcel) const;
    virtual status_t readFromParcel(const Parcel* in);
    void setMediaDirection(int32_t direction);
    int32_t getMediaDirection();
    void setRemoteAddress(String8 address);
    String8 getRemoteAddress();
    void setRemotePort(int32_t port);
    int32_t getRemotePort();
    void setRtcpConfig(const RtcpConfig& config);
    RtcpConfig getRtcpConfig();
    //QosSessionAttributes getQos();
    void setMaxMtuBytes(int32_t mtu);
    int32_t getmaxMtuBytes();
    void setDscp(int dscp);
    int32_t getDscp();
    void setRxPayloadTypeNumber(int32_t num);
    int32_t getRxPayloadTypeNumber();
    void setTxPayloadTypeNumber(int32_t num);
    int32_t getTxPayloadTypeNumber();
    void setSamplingRateKHz(int8_t sample);
    int8_t getSamplingRateKHz();

protected:
    int32_t direction;
    int32_t accessNetwork;
    String8 remoteAddress;
    int32_t remotePort;
    RtcpConfig rtcpConfig;
    //QosSessionAttributes qos;
    int32_t maxMtuBytes;
    int32_t dscp;
    int32_t rxPayloadTypeNumber;
    int32_t txPayloadTypeNumber;
    int8_t samplingRateKHz;
};

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android

#endif