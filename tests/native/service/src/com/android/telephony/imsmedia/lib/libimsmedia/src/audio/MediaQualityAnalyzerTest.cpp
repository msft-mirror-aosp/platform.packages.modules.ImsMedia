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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AudioConfig.h>

#include <ImsMediaAudioUtil.h>
#include <MediaQualityAnalyzer.h>
#include <ImsMediaCondition.h>

#include <MockBaseSessionCallback.h>

using namespace android::telephony::imsmedia;
using ::testing::_;

// RtpConfig
const int32_t kMediaDirection = RtpConfig::MEDIA_DIRECTION_INACTIVE;
const android::String8 kRemoteAddress("127.0.0.1");
const int32_t kRemotePort = 10000;
const int8_t kDscp = 0;
const int8_t kRxPayload = 96;
const int8_t kTxPayload = 96;
const int8_t kSamplingRate = 16;

// RtcpConfig
const android::String8 kCanonicalName("name");
const int32_t kTransmitPort = 1001;
const int32_t kIntervalSec = 3;
const int32_t kRtcpXrBlockTypes = RtcpConfig::FLAG_RTCPXR_STATISTICS_SUMMARY_REPORT_BLOCK |
        RtcpConfig::FLAG_RTCPXR_VOIP_METRICS_REPORT_BLOCK;

// AudioConfig
const int8_t kPTimeMillis = 20;
const int32_t kMaxPtimeMillis = 100;
const int8_t kcodecModeRequest = 15;
const bool kDtxEnabled = true;
const int32_t kCodecType = AudioConfig::CODEC_AMR_WB;
const int8_t kDtmfPayloadTypeNumber = 100;
const int8_t kDtmfsamplingRateKHz = 16;

// AmrParam
const int32_t kAmrMode = 8;
const bool kOctetAligned = false;
const int32_t kMaxRedundancyMillis = 240;

// EvsParam
const int32_t kEvsBandwidth = EvsParams::EVS_BAND_NONE;
const int32_t kEvsMode = 8;
const int8_t kChannelAwareMode = 3;
const bool kUseHeaderFullOnly = false;

class MediaQualityAnalyzerTest : public ::testing::Test
{
public:
    MediaQualityAnalyzerTest() {}
    virtual ~MediaQualityAnalyzerTest() {}

protected:
    MediaQualityAnalyzer* analyzer;
    AudioConfig config;
    RtcpConfig rtcp;
    AmrParams amr;
    EvsParams evs;
    MockBaseSessionCallback callback;
    ImsMediaCondition condition;

    virtual void SetUp() override
    {
        analyzer = new MediaQualityAnalyzer();
        rtcp.setCanonicalName(kCanonicalName);
        rtcp.setTransmitPort(kTransmitPort);
        rtcp.setIntervalSec(kIntervalSec);
        rtcp.setRtcpXrBlockTypes(kRtcpXrBlockTypes);

        amr.setAmrMode(kAmrMode);
        amr.setOctetAligned(kOctetAligned);
        amr.setMaxRedundancyMillis(kMaxRedundancyMillis);

        evs.setEvsBandwidth(kEvsBandwidth);
        evs.setEvsMode(kEvsMode);
        evs.setChannelAwareMode(kChannelAwareMode);
        evs.setUseHeaderFullOnly(kUseHeaderFullOnly);
        evs.setCodecModeRequest(kcodecModeRequest);

        config.setMediaDirection(kMediaDirection);
        config.setRemoteAddress(kRemoteAddress);
        config.setRemotePort(kRemotePort);
        config.setRtcpConfig(rtcp);
        config.setDscp(kDscp);
        config.setRxPayloadTypeNumber(kRxPayload);
        config.setTxPayloadTypeNumber(kTxPayload);
        config.setSamplingRateKHz(kSamplingRate);
        config.setPtimeMillis(kPTimeMillis);
        config.setMaxPtimeMillis(kMaxPtimeMillis);
        config.setDtxEnabled(kDtxEnabled);
        config.setCodecType(kCodecType);
        config.setTxDtmfPayloadTypeNumber(kDtmfPayloadTypeNumber);
        config.setRxDtmfPayloadTypeNumber(kDtmfPayloadTypeNumber);
        config.setDtmfsamplingRateKHz(kDtmfsamplingRateKHz);
        config.setAmrParams(amr);
        config.setEvsParams(evs);

        analyzer->setCallback(&callback);
        analyzer->setConfig(&config);
    }

    virtual void TearDown() override
    {
        if (analyzer != NULL)
        {
            delete analyzer;
        }
    }
};

TEST_F(MediaQualityAnalyzerTest, TestStartStop)
{
    EXPECT_CALL(callback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(1);
    analyzer->startTimer(1000);
    analyzer->stopTimer();
}

TEST_F(MediaQualityAnalyzerTest, TestRxInactivityInd)
{
    EXPECT_CALL(callback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(2);
    analyzer->startTimer(1000);
    condition.wait_timeout(6000);  // 6sec
    analyzer->stopTimer();
}

TEST_F(MediaQualityAnalyzerTest, TestCallQualityLevelChanged)
{
    EXPECT_CALL(callback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(2);
    analyzer->startTimer(1000);

    for (uint32_t i = 0; i < 10; i++)
    {
        RtpPacket* packet = new RtpPacket();

        if (i == 5)  // make 10% loss rate
        {
            continue;
        }

        packet->seqNum = i;
        packet->jitter = 10;
        analyzer->collectInfo(kStreamRtpRx, packet);
        analyzer->collectRxRtpStatus(i, kRtpStatusNormal);
    }

    analyzer->collectOptionalInfo(kReportPacketLossGap, 5, 1);

    condition.wait_timeout(6000);  // 6sec
    analyzer->stopTimer();
}

TEST_F(MediaQualityAnalyzerTest, TestJitterInd)
{
    EXPECT_CALL(callback, onEvent(kImsMediaEventNotifyJitter, _, _)).Times(1);
    EXPECT_CALL(callback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(1);
    // set 10 milliseconds jitter threshold in 1 sec interval
    analyzer->setJitterThreshold(1, 10);
    analyzer->startTimer(1000);

    for (uint32_t i = 0; i < 20; i++)
    {
        RtpPacket* packet = new RtpPacket();
        packet->seqNum = i;
        packet->jitter = 20;
        analyzer->collectInfo(kStreamRtpRx, packet);
    }

    condition.wait_timeout(1500);  // wait 1.5sec
    analyzer->stopTimer();
}

TEST_F(MediaQualityAnalyzerTest, TestPacketLossInd)
{
    EXPECT_CALL(callback, onEvent(kImsMediaEventPacketLoss, _, _)).Times(1);
    EXPECT_CALL(callback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(1);
    // 1 percent packet loss threshold in 1 sec interval
    analyzer->setPacketLossThreshold(1, 1);
    analyzer->startTimer(1000);

    for (uint32_t i = 0; i < 10; i++)
    {
        RtpPacket* packet = new RtpPacket();

        if (i == 5)  // make 10% loss rate
        {
            continue;
        }

        packet->seqNum = i;
        packet->jitter = 10;
        analyzer->collectInfo(kStreamRtpRx, packet);
    }

    analyzer->collectOptionalInfo(kReportPacketLossGap, 5, 1);

    condition.wait_timeout(1500);  // wait 1.5sec
    analyzer->stopTimer();
}