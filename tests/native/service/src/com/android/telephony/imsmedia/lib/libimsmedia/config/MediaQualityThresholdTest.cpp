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

class MediaQualityThresholdTest : public ::testing::Test
{
public:
    MediaQualityThreshold threshold1;
    MediaQualityThreshold threshold2;
    MediaQualityThreshold threshold3;

protected:
    virtual void SetUp() override
    {
        threshold1.setRtpInactivityTimerMillis(kRtpInactivityTimerMillis);
        threshold1.setRtcpInactivityTimerMillis(kRtcpInactivityTimerMillis);
        threshold1.setRtpPacketLossDurationMillis(kRtpPacketLossDurationMillis);
        threshold1.setRtpPacketLossRate(kRtpPacketLossRate);
        threshold1.setJitterDurationMillis(kJitterDurationMillis);
        threshold1.setRtpJitterMillis(kRtpJitterMillis);
    }

    virtual void TearDown() override {}
};

TEST_F(MediaQualityThresholdTest, TestGetterSetter)
{
    EXPECT_EQ(threshold1.getRtpInactivityTimerMillis(), kRtpInactivityTimerMillis);
    EXPECT_EQ(threshold1.getRtcpInactivityTimerMillis(), kRtcpInactivityTimerMillis);
    EXPECT_EQ(threshold1.getRtpPacketLossDurationMillis(), kRtpPacketLossDurationMillis);
    EXPECT_EQ(threshold1.getRtpPacketLossRate(), kRtpPacketLossRate);
    EXPECT_EQ(threshold1.getJitterDurationMillis(), kJitterDurationMillis);
    EXPECT_EQ(threshold1.getRtpJitterMillis(), kRtpJitterMillis);
}

TEST_F(MediaQualityThresholdTest, TestParcel)
{
    android::Parcel parcel;
    threshold1.writeToParcel(&parcel);
    parcel.setDataPosition(0);

    MediaQualityThreshold testThreshold;
    testThreshold.readFromParcel(&parcel);
    EXPECT_EQ(testThreshold, threshold1);
}

TEST_F(MediaQualityThresholdTest, TestAssign)
{
    MediaQualityThreshold threshold2 = threshold1;
    EXPECT_EQ(threshold1, threshold2);
}

TEST_F(MediaQualityThresholdTest, TestEqual)
{
    threshold2.setRtpInactivityTimerMillis(kRtpInactivityTimerMillis);
    threshold2.setRtcpInactivityTimerMillis(kRtcpInactivityTimerMillis);
    threshold2.setRtpPacketLossDurationMillis(kRtpPacketLossDurationMillis);
    threshold2.setRtpPacketLossRate(kRtpPacketLossRate);
    threshold2.setJitterDurationMillis(kJitterDurationMillis);
    threshold2.setRtpJitterMillis(kRtpJitterMillis);

    EXPECT_EQ(threshold1, threshold2);
}

TEST_F(MediaQualityThresholdTest, TestNotEqual)
{
    threshold2.setRtpInactivityTimerMillis(5000);
    threshold2.setRtcpInactivityTimerMillis(kRtcpInactivityTimerMillis);
    threshold2.setRtpPacketLossDurationMillis(kRtpPacketLossDurationMillis);
    threshold2.setRtpPacketLossRate(kRtpPacketLossRate);
    threshold2.setJitterDurationMillis(kJitterDurationMillis);
    threshold2.setRtpJitterMillis(kRtpJitterMillis);

    threshold3.setRtpInactivityTimerMillis(kRtpInactivityTimerMillis);
    threshold3.setRtcpInactivityTimerMillis(kRtcpInactivityTimerMillis);
    threshold3.setRtpPacketLossDurationMillis(kRtpPacketLossDurationMillis);
    threshold3.setRtpPacketLossRate(1);
    threshold3.setJitterDurationMillis(kJitterDurationMillis);
    threshold3.setRtpJitterMillis(kRtpJitterMillis);

    EXPECT_NE(threshold1, threshold2);
    EXPECT_NE(threshold1, threshold3);
}