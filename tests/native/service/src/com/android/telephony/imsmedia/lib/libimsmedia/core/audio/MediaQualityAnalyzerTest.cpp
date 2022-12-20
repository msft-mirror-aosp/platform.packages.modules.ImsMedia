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

class FakeMediaQualityCallback : public BaseSessionCallback
{
public:
    FakeMediaQualityCallback() {}
    virtual ~FakeMediaQualityCallback() {}

    virtual void SendEvent(int32_t type, uint64_t param1, uint64_t param2)
    {
        (void)param2;

        if (type == kAudioCallQualityChangedInd)
        {
            MediaQuality* quality = reinterpret_cast<MediaQuality*>(param1);

            if (quality != NULL)
            {
                mMediaQuality = *quality;
                delete quality;
            }
        }
    }

    virtual void onEvent(int32_t type, uint64_t param1, uint64_t param2)
    {
        (void)type;
        (void)param1;
        (void)param2;
    }

    MediaQuality getMediaQuality() { return mMediaQuality; }

private:
    MediaQuality mMediaQuality;
};

class MediaQualityAnalyzerTest : public ::testing::Test
{
public:
    MediaQualityAnalyzerTest() { mAnalyzer = NULL; }
    virtual ~MediaQualityAnalyzerTest() {}

protected:
    MediaQualityAnalyzer* mAnalyzer;
    AudioConfig mConfig;
    RtcpConfig mRtcpConfig;
    AmrParams mAmrParam;
    EvsParams mEvsParam;
    FakeMediaQualityCallback mFakeCallback;
    MockBaseSessionCallback mCallback;
    ImsMediaCondition mCondition;

    virtual void SetUp() override
    {
        mCallback.SetDelegate(&mFakeCallback);
        mCallback.DelegateToFake();

        mAnalyzer = new MediaQualityAnalyzer();
        mRtcpConfig.setCanonicalName(kCanonicalName);
        mRtcpConfig.setTransmitPort(kTransmitPort);
        mRtcpConfig.setIntervalSec(kIntervalSec);
        mRtcpConfig.setRtcpXrBlockTypes(kRtcpXrBlockTypes);

        mAmrParam.setAmrMode(kAmrMode);
        mAmrParam.setOctetAligned(kOctetAligned);
        mAmrParam.setMaxRedundancyMillis(kMaxRedundancyMillis);

        mEvsParam.setEvsBandwidth(kEvsBandwidth);
        mEvsParam.setEvsMode(kEvsMode);
        mEvsParam.setChannelAwareMode(kChannelAwareMode);
        mEvsParam.setUseHeaderFullOnly(kUseHeaderFullOnly);
        mEvsParam.setCodecModeRequest(kcodecModeRequest);

        mConfig.setMediaDirection(kMediaDirection);
        mConfig.setRemoteAddress(kRemoteAddress);
        mConfig.setRemotePort(kRemotePort);
        mConfig.setRtcpConfig(mRtcpConfig);
        mConfig.setDscp(kDscp);
        mConfig.setRxPayloadTypeNumber(kRxPayload);
        mConfig.setTxPayloadTypeNumber(kTxPayload);
        mConfig.setSamplingRateKHz(kSamplingRate);
        mConfig.setPtimeMillis(kPTimeMillis);
        mConfig.setMaxPtimeMillis(kMaxPtimeMillis);
        mConfig.setDtxEnabled(kDtxEnabled);
        mConfig.setCodecType(kCodecType);
        mConfig.setTxDtmfPayloadTypeNumber(kDtmfPayloadTypeNumber);
        mConfig.setRxDtmfPayloadTypeNumber(kDtmfPayloadTypeNumber);
        mConfig.setDtmfsamplingRateKHz(kDtmfsamplingRateKHz);
        mConfig.setAmrParams(mAmrParam);
        mConfig.setEvsParams(mEvsParam);

        mAnalyzer->setCallback(&mCallback);
        mAnalyzer->setConfig(&mConfig);
        mCondition.reset();
    }

    virtual void TearDown() override
    {
        if (mAnalyzer != NULL)
        {
            delete mAnalyzer;
        }
    }
};

TEST_F(MediaQualityAnalyzerTest, TestStartStop)
{
    EXPECT_CALL(mCallback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(1);
    mAnalyzer->startTimer(1000);

    MediaQuality quality = mAnalyzer->getMediaQuality();
    mAnalyzer->stopTimer();

    EXPECT_EQ(mFakeCallback.getMediaQuality(), quality);
}

TEST_F(MediaQualityAnalyzerTest, TestCollectTxPackets)
{
    EXPECT_CALL(mCallback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(1);
    mAnalyzer->startTimer(1000);

    const int32_t numPackets = 10;

    for (int32_t i = 0; i < numPackets; i++)
    {
        RtpPacket* packet = new RtpPacket();
        mAnalyzer->collectInfo(kStreamRtpTx, packet);
    }

    EXPECT_EQ(mAnalyzer->getTxPacketSize(), numPackets);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 0);

    MediaQuality quality = mAnalyzer->getMediaQuality();
    mAnalyzer->stopTimer();

    EXPECT_EQ(mAnalyzer->getTxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 0);

    // Check MediaQuality value
    MediaQuality quality2 = mFakeCallback.getMediaQuality();
    EXPECT_EQ(quality2, quality);
    EXPECT_EQ(quality2.getNumRtpPacketsTransmitted(), numPackets);
}

TEST_F(MediaQualityAnalyzerTest, TestRxInactivityInd)
{
    EXPECT_CALL(mCallback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(2);
    mAnalyzer->startTimer(1000);
    mCondition.wait_timeout(6000);  // 6sec

    MediaQuality quality = mAnalyzer->getMediaQuality();
    mAnalyzer->stopTimer();

    // Check MediaQuality value
    MediaQuality quality2 = mFakeCallback.getMediaQuality();
    EXPECT_EQ(quality2, quality);
    EXPECT_TRUE(quality2.getRtpInactivityDetected());
}

TEST_F(MediaQualityAnalyzerTest, TestCallQualityLevelChanged)
{
    EXPECT_CALL(mCallback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(2);
    mAnalyzer->startTimer(1000);

    const int32_t numPackets = 10;
    const int32_t jitter = 10;

    for (int32_t i = 0; i < numPackets; i++)
    {
        RtpPacket* packet = new RtpPacket();

        if (i == 5)  // make 10% loss rate
        {
            continue;
        }

        packet->seqNum = i;
        packet->jitter = jitter;
        mAnalyzer->collectInfo(kStreamRtpRx, packet);
        mAnalyzer->collectRxRtpStatus(i, kRtpStatusNormal);
    }

    EXPECT_EQ(mAnalyzer->getTxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), numPackets - 1);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 0);
    mAnalyzer->collectOptionalInfo(kReportPacketLossGap, 5, 1);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 1);

    mCondition.wait_timeout(6000);  // 6sec

    MediaQuality quality = mAnalyzer->getMediaQuality();
    mAnalyzer->stopTimer();

    EXPECT_EQ(mAnalyzer->getTxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 0);

    // Check MediaQuality value
    MediaQuality quality2 = mFakeCallback.getMediaQuality();
    EXPECT_EQ(quality2, quality);
    EXPECT_EQ(quality2.getNumRtpPacketsReceived(), numPackets - 1);
    EXPECT_EQ(quality2.getDownlinkCallQualityLevel(), MediaQuality::kCallQualityBad);
}

TEST_F(MediaQualityAnalyzerTest, TestJitterInd)
{
    EXPECT_CALL(mCallback, onEvent(kImsMediaEventNotifyJitter, _, _)).Times(1);
    EXPECT_CALL(mCallback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(1);
    // set 10 milliseconds jitter threshold in 1 sec interval
    mAnalyzer->setJitterThreshold(1, 10);
    mAnalyzer->startTimer(1000);

    const int32_t numPackets = 20;
    const int32_t jitter = 20;
    const uint32_t ssrc = 10000;

    for (int32_t i = 0; i < numPackets; i++)
    {
        RtpPacket* packet = new RtpPacket();
        packet->seqNum = i;
        packet->jitter = jitter;
        packet->ssrc = ssrc;
        mAnalyzer->collectInfo(kStreamRtpRx, packet);
    }

    EXPECT_EQ(mAnalyzer->getTxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), numPackets);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 0);

    mCondition.wait_timeout(1500);  // wait 1.5sec

    MediaQuality quality = mAnalyzer->getMediaQuality();
    mAnalyzer->stopTimer();

    EXPECT_EQ(mAnalyzer->getTxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 0);

    MediaQuality quality2 = mFakeCallback.getMediaQuality();
    EXPECT_EQ(quality2, quality);
    EXPECT_EQ(quality2.getNumRtpPacketsReceived(), numPackets);
    EXPECT_EQ(quality2.getAverageRelativeJitter(), jitter);
}

TEST_F(MediaQualityAnalyzerTest, TestSsrcChange)
{
    mAnalyzer->startTimer(1000);

    const int32_t numPackets = 20;
    const int32_t jitter = 20;
    const uint32_t ssrc1 = 10000;
    const uint32_t ssrc2 = 20000;

    for (int32_t i = 0; i < numPackets; i++)
    {
        RtpPacket* packet = new RtpPacket();
        packet->seqNum = i;
        packet->jitter = jitter;
        packet->ssrc = ssrc1;

        if (i >= 5)
        {
            packet->ssrc = ssrc2;
        }

        mAnalyzer->collectInfo(kStreamRtpRx, packet);
    }

    EXPECT_EQ(mAnalyzer->getTxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), numPackets - 5);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 0);
}

TEST_F(MediaQualityAnalyzerTest, TestPacketLossInd)
{
    EXPECT_CALL(mCallback, onEvent(kImsMediaEventPacketLoss, _, _)).Times(1);
    EXPECT_CALL(mCallback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(1);
    // 1 percent packet loss threshold in 1 sec interval
    mAnalyzer->setPacketLossThreshold(1, 1);
    mAnalyzer->startTimer(1000);

    const int32_t numPackets = 10;

    for (int32_t i = 0; i < numPackets; i++)
    {
        RtpPacket* packet = new RtpPacket();

        if (i == 5)  // make 10% loss rate
        {
            continue;
        }

        packet->seqNum = i;
        packet->jitter = 10;
        mAnalyzer->collectInfo(kStreamRtpRx, packet);
    }

    EXPECT_EQ(mAnalyzer->getTxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), numPackets - 1);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 0);

    mAnalyzer->collectOptionalInfo(kReportPacketLossGap, 5, 1);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 1);

    mCondition.wait_timeout(1500);  // wait 1.5sec

    MediaQuality quality = mAnalyzer->getMediaQuality();
    mAnalyzer->stopTimer();

    EXPECT_EQ(mAnalyzer->getTxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 0);

    MediaQuality quality2 = mFakeCallback.getMediaQuality();
    EXPECT_EQ(quality2, quality);
    EXPECT_EQ(quality2.getNumRtpPacketsNotReceived(), 1);
}