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
#include <gtest/gtest.h>

using namespace android::telephony::imsmedia;

const int32_t kMediaDirection = RtpConfig::MEDIA_DIRECTION_NO_FLOW;
const android::String8 kRemoteAddress("0.0.0.0");
const int32_t kRemotePort = 1000;
const int32_t kMtu = 1500;
const int32_t kDscp = 1500;
const int32_t kRxPayload = 96;
const int32_t kTxPayload = 96;
const int8_t kSamplingRate = 8;

TEST(RtpConfigTest, TestGetterSetter) {
    RtpConfig* config = new RtpConfig();
    config->setMediaDirection(kMediaDirection);
    config->setRemoteAddress(kRemoteAddress);
    config->setRemotePort(kRemotePort);
    RtcpConfig rtcp;
    config->setRtcpConfig(rtcp);
    config->setMaxMtuBytes(kMtu);
    config->setDscp(kDscp);
    config->setRxPayloadTypeNumber(kRxPayload);
    config->setTxPayloadTypeNumber(kTxPayload);
    config->setSamplingRateKHz(kSamplingRate);
    EXPECT_EQ(kMediaDirection, config->getMediaDirection());
    EXPECT_EQ(kRemoteAddress, config->getRemoteAddress());
    EXPECT_EQ(kRemotePort, config->getRemotePort());
    EXPECT_EQ(rtcp, config->getRtcpConfig());
    EXPECT_EQ(kMtu, config->getmaxMtuBytes());
    EXPECT_EQ(kDscp, config->getDscp());
    EXPECT_EQ(kRxPayload, config->getRxPayloadTypeNumber());
    EXPECT_EQ(kTxPayload, config->getTxPayloadTypeNumber());
    EXPECT_EQ(kSamplingRate, config->getSamplingRateKHz());
    delete config;
}

TEST(RtpConfigTest, TestParcel) {
    RtpConfig* config = new RtpConfig();
    config->setMediaDirection(kMediaDirection);
    config->setRemoteAddress(kRemoteAddress);
    config->setRemotePort(kRemotePort);
    RtcpConfig rtcp;
    config->setRtcpConfig(rtcp);
    config->setMaxMtuBytes(kMtu);
    config->setDscp(kDscp);
    config->setRxPayloadTypeNumber(kRxPayload);
    config->setTxPayloadTypeNumber(kTxPayload);
    config->setSamplingRateKHz(kSamplingRate);

    android::Parcel parcel;
    config->writeToParcel(&parcel);
    parcel.setDataPosition(0);

    RtpConfig* config2 = new RtpConfig();
    config2->readFromParcel(&parcel);
    EXPECT_EQ(*config2, *config);

    delete config;
    delete config2;
}

TEST(RtpConfigTest, TestEqual) {
    RtpConfig* config = new RtpConfig();
    config->setMediaDirection(kMediaDirection);
    config->setRemoteAddress(kRemoteAddress);
    config->setRemotePort(kRemotePort);
    RtcpConfig rtcp;
    config->setRtcpConfig(rtcp);
    config->setMaxMtuBytes(kMtu);
    config->setDscp(kDscp);
    config->setRxPayloadTypeNumber(kRxPayload);
    config->setTxPayloadTypeNumber(kTxPayload);
    config->setSamplingRateKHz(kSamplingRate);

    RtpConfig* config2 = new RtpConfig();
    config2->setMediaDirection(kMediaDirection);
    config2->setRemoteAddress(kRemoteAddress);
    config2->setRemotePort(kRemotePort);
    config2->setRtcpConfig(rtcp);
    config2->setMaxMtuBytes(kMtu);
    config2->setDscp(kDscp);
    config2->setRxPayloadTypeNumber(kRxPayload);
    config2->setTxPayloadTypeNumber(kTxPayload);
    config2->setSamplingRateKHz(kSamplingRate);
    EXPECT_EQ(*config2, *config);

    delete config;
    delete config2;
}

TEST(RtpConfigTest, TestNotEqual) {
    RtpConfig* config = new RtpConfig();
    config->setMediaDirection(kMediaDirection);
    config->setRemoteAddress(kRemoteAddress);
    config->setRemotePort(kRemotePort);
    RtcpConfig rtcp;
    config->setRtcpConfig(rtcp);
    config->setMaxMtuBytes(kMtu);
    config->setDscp(kDscp);
    config->setRxPayloadTypeNumber(kRxPayload);
    config->setTxPayloadTypeNumber(kTxPayload);
    config->setSamplingRateKHz(kSamplingRate);

    RtpConfig* config2 = new RtpConfig();
    config2->setMediaDirection(kMediaDirection);
    config2->setRemoteAddress(kRemoteAddress);
    config2->setRemotePort(9999);
    config2->setRtcpConfig(rtcp);
    config2->setMaxMtuBytes(kMtu);
    config2->setDscp(kDscp);
    config2->setRxPayloadTypeNumber(kRxPayload);
    config2->setTxPayloadTypeNumber(kTxPayload);
    config2->setSamplingRateKHz(kSamplingRate);

    RtpConfig* config3 = new RtpConfig();
    config3->setMediaDirection(kMediaDirection);
    android::String8 addressConfig3("1.1.1.1");
    config3->setRemoteAddress(addressConfig3);
    config3->setRemotePort(kRemotePort);
    config3->setRtcpConfig(rtcp);
    config3->setMaxMtuBytes(kMtu);
    config3->setDscp(kDscp);
    config3->setRxPayloadTypeNumber(kRxPayload);
    config3->setTxPayloadTypeNumber(kTxPayload);
    config3->setSamplingRateKHz(kSamplingRate);

    EXPECT_NE(*config2, *config);
    EXPECT_NE(*config3, *config);

    delete config;
    delete config2;
    delete config3;
}