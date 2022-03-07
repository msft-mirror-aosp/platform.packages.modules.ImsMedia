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
#include <gtest/gtest.h>

using namespace android::telephony::imsmedia;

const int32_t kRtpInactivityTimerMillis = 10000;
const int32_t kRtcpInactivityTimerMillis = 20000;
const int32_t kRtpPacketLossDurationMillis = 5000;
const int32_t kRtpPacketLossRate = 5;
const int32_t kJitterDurationMillis = 5000;
const int32_t kRtpJitterMillis = 300;

TEST(MediaQualityThresholdTest, TestGetterSetter) {
    MediaQualityThreshold* threshold = new MediaQualityThreshold();
    threshold->setRtpInactivityTimerMillis(kRtpInactivityTimerMillis);
    threshold->setRtcpInactivityTimerMillis(kRtcpInactivityTimerMillis);
    threshold->setRtpPacketLossDurationMillis(kRtpPacketLossDurationMillis);
    threshold->setRtpPacketLossRate(kRtpPacketLossRate);
    threshold->setJitterDurationMillis(kJitterDurationMillis);
    threshold->setRtpJitterMillis(kRtpJitterMillis);
    EXPECT_EQ(threshold->getRtpInactivityTimerMillis(), kRtpInactivityTimerMillis);
    EXPECT_EQ(threshold->getRtcpInactivityTimerMillis(), kRtcpInactivityTimerMillis);
    EXPECT_EQ(threshold->getRtpPacketLossDurationMillis(), kRtpPacketLossDurationMillis);
    EXPECT_EQ(threshold->getRtpPacketLossRate(), kRtpPacketLossRate);
    EXPECT_EQ(threshold->getJitterDurationMillis(), kJitterDurationMillis);
    EXPECT_EQ(threshold->getRtpJitterMillis(), kRtpJitterMillis);
    delete threshold;
}

TEST(MediaQualityThresholdTest, TestParcel) {
    MediaQualityThreshold* threshold = new MediaQualityThreshold();
    threshold->setRtpInactivityTimerMillis(kRtpInactivityTimerMillis);
    threshold->setRtcpInactivityTimerMillis(kRtcpInactivityTimerMillis);
    threshold->setRtpPacketLossDurationMillis(kRtpPacketLossDurationMillis);
    threshold->setRtpPacketLossRate(kRtpPacketLossRate);
    threshold->setJitterDurationMillis(kJitterDurationMillis);
    threshold->setRtpJitterMillis(kRtpJitterMillis);

    android::Parcel parcel;
    threshold->writeToParcel(&parcel);
    parcel.setDataPosition(0);

    MediaQualityThreshold* threshold2 = new MediaQualityThreshold();
    threshold2->readFromParcel(&parcel);
    EXPECT_EQ(*threshold2, *threshold);

    delete threshold;
    delete threshold2;
}

TEST(MediaQualityThresholdTest, TestAssign) {
    MediaQualityThreshold threshold;
    threshold.setRtpInactivityTimerMillis(kRtpInactivityTimerMillis);
    threshold.setRtcpInactivityTimerMillis(kRtcpInactivityTimerMillis);
    threshold.setRtpPacketLossDurationMillis(kRtpPacketLossDurationMillis);
    threshold.setRtpPacketLossRate(kRtpPacketLossRate);
    threshold.setJitterDurationMillis(kJitterDurationMillis);
    threshold.setRtpJitterMillis(kRtpJitterMillis);

    MediaQualityThreshold threshold2;
    threshold2 = threshold;
    EXPECT_EQ(threshold, threshold2);
}

TEST(MediaQualityThresholdTest, TestEqual) {
    MediaQualityThreshold* threshold = new MediaQualityThreshold();
    threshold->setRtpInactivityTimerMillis(kRtpInactivityTimerMillis);
    threshold->setRtcpInactivityTimerMillis(kRtcpInactivityTimerMillis);
    threshold->setRtpPacketLossDurationMillis(kRtpPacketLossDurationMillis);
    threshold->setRtpPacketLossRate(kRtpPacketLossRate);
    threshold->setJitterDurationMillis(kJitterDurationMillis);
    threshold->setRtpJitterMillis(kRtpJitterMillis);

    MediaQualityThreshold* threshold2 = new MediaQualityThreshold();
    threshold2->setRtpInactivityTimerMillis(kRtpInactivityTimerMillis);
    threshold2->setRtcpInactivityTimerMillis(kRtcpInactivityTimerMillis);
    threshold2->setRtpPacketLossDurationMillis(kRtpPacketLossDurationMillis);
    threshold2->setRtpPacketLossRate(kRtpPacketLossRate);
    threshold2->setJitterDurationMillis(kJitterDurationMillis);
    threshold2->setRtpJitterMillis(kRtpJitterMillis);

    EXPECT_EQ(*threshold, *threshold2);
    delete threshold;
    delete threshold2;
}

TEST(MediaQualityThresholdTest, TestNotEqual) {
    MediaQualityThreshold* threshold = new MediaQualityThreshold();
    threshold->setRtpInactivityTimerMillis(kRtpInactivityTimerMillis);
    threshold->setRtcpInactivityTimerMillis(kRtcpInactivityTimerMillis);
    threshold->setRtpPacketLossDurationMillis(kRtpPacketLossDurationMillis);
    threshold->setRtpPacketLossRate(kRtpPacketLossRate);
    threshold->setJitterDurationMillis(kJitterDurationMillis);
    threshold->setRtpJitterMillis(kRtpJitterMillis);

    MediaQualityThreshold* threshold2 = new MediaQualityThreshold();
    threshold->setRtpInactivityTimerMillis(5000);
    threshold->setRtcpInactivityTimerMillis(kRtcpInactivityTimerMillis);
    threshold->setRtpPacketLossDurationMillis(kRtpPacketLossDurationMillis);
    threshold->setRtpPacketLossRate(kRtpPacketLossRate);
    threshold->setJitterDurationMillis(kJitterDurationMillis);
    threshold->setRtpJitterMillis(kRtpJitterMillis);

    MediaQualityThreshold* threshold3 = new MediaQualityThreshold();
    threshold->setRtpInactivityTimerMillis(kRtpInactivityTimerMillis);
    threshold->setRtcpInactivityTimerMillis(kRtcpInactivityTimerMillis);
    threshold->setRtpPacketLossDurationMillis(kRtpPacketLossDurationMillis);
    threshold->setRtpPacketLossRate(1);
    threshold->setJitterDurationMillis(kJitterDurationMillis);
    threshold->setRtpJitterMillis(kRtpJitterMillis);

    EXPECT_NE(*threshold, *threshold2);
    EXPECT_NE(*threshold, *threshold3);
    delete threshold;
    delete threshold2;
    delete threshold3;
}