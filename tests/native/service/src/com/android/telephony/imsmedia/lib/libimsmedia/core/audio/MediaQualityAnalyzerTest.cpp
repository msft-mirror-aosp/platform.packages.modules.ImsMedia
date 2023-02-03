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
#include <ImsMediaTimer.h>

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

// MediaQualityThreshold
const std::vector<int32_t> kRtpInactivityTimerMillis = {2000, 4000};
const int32_t kRtcpInactivityTimerMillis = 2000;
const int32_t kRtpHysteresisTimeInMillis = 2000;
const int32_t kRtpPacketLossDurationMillis = 3000;
const std::vector<int32_t> kRtpPacketLossRate = {3, 5};
const std::vector<int32_t> kRtpJitterMillis = {10, 20};

class FakeMediaQualityCallback : public BaseSessionCallback
{
public:
    FakeMediaQualityCallback() {}
    virtual ~FakeMediaQualityCallback() {}

    virtual void SendEvent(int32_t type, uint64_t param1, uint64_t /*param2*/)
    {
        if (type == kAudioCallQualityChangedInd)
        {
            CallQuality* quality = reinterpret_cast<CallQuality*>(param1);

            if (quality != nullptr)
            {
                mCallQuality = *quality;
                delete quality;
            }
        }
        else if (type == kImsMediaEventMediaQualityStatus)
        {
            MediaQualityStatus* status = reinterpret_cast<MediaQualityStatus*>(param1);

            if (status != nullptr)
            {
                mStatus = *status;
                delete status;
            }
        }
    }

    virtual void onEvent(int32_t type, uint64_t param1, uint64_t param2)
    {
        (void)type;
        (void)param1;
        (void)param2;
    }

    CallQuality getCallQuality() { return mCallQuality; }
    MediaQualityStatus getMediaQualityStatus() { return mStatus; }

private:
    CallQuality mCallQuality;
    MediaQualityStatus mStatus;
};

class MediaQualityAnalyzerTest : public ::testing::Test
{
public:
    MediaQualityAnalyzerTest() { mAnalyzer = nullptr; }
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
        if (mAnalyzer != nullptr)
        {
            delete mAnalyzer;
        }
    }
};

TEST_F(MediaQualityAnalyzerTest, TestStartStop)
{
    EXPECT_CALL(mCallback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(1);
    mAnalyzer->start();

    CallQuality quality = mAnalyzer->getCallQuality();
    mAnalyzer->stop();

    EXPECT_EQ(mFakeCallback.getCallQuality(), quality);
}

TEST_F(MediaQualityAnalyzerTest, TestCollectTxPackets)
{
    EXPECT_CALL(mCallback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(1);
    mAnalyzer->start();

    const int32_t numPackets = 10;

    for (int32_t i = 0; i < numPackets; i++)
    {
        RtpPacket* packet = new RtpPacket();
        mAnalyzer->SendEvent(kCollectPacketInfo, kStreamRtpTx, reinterpret_cast<uint64_t>(packet));
    }

    mCondition.wait_timeout(1100);  // 1.1 sec
    EXPECT_EQ(mAnalyzer->getTxPacketSize(), numPackets);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 0);
    CallQuality quality = mAnalyzer->getCallQuality();
    mAnalyzer->stop();

    EXPECT_EQ(mAnalyzer->getTxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 0);

    // Check CallQuality value
    CallQuality quality2 = mFakeCallback.getCallQuality();
    EXPECT_EQ(quality2, quality);
    EXPECT_EQ(quality2.getNumRtpPacketsTransmitted(), numPackets);
}

TEST_F(MediaQualityAnalyzerTest, TestRtpInactivity)
{
    EXPECT_CALL(mCallback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(2);
    EXPECT_CALL(mCallback, onEvent(kImsMediaEventMediaQualityStatus, _, _)).Times(3);
    MediaQualityThreshold threshold;
    threshold.setRtpInactivityTimerMillis(kRtpInactivityTimerMillis);
    mAnalyzer->setMediaQualityThreshold(threshold);
    mAnalyzer->start();
    mCondition.wait_timeout(2100);  // 2.1 sec

    // Check MediaQualityStatus value
    MediaQualityStatus quality1 = mFakeCallback.getMediaQualityStatus();
    EXPECT_EQ(quality1.getRtpInactivityTimeMillis(), 2000);

    mCondition.wait_timeout(2100);  // 2.1 sec

    // Check MediaQualityStatus value
    MediaQualityStatus quality2 = mFakeCallback.getMediaQualityStatus();
    EXPECT_EQ(quality2.getRtpInactivityTimeMillis(), 4000);

    RtpPacket* packet = new RtpPacket();
    packet->seqNum = 0;
    mAnalyzer->SendEvent(kCollectPacketInfo, kStreamRtpRx, reinterpret_cast<uint64_t>(packet));

    mCondition.wait_timeout(3100);  // 3.1 sec

    MediaQualityStatus quality3 = mFakeCallback.getMediaQualityStatus();
    EXPECT_EQ(quality3.getRtpInactivityTimeMillis(), 2000);
    mAnalyzer->stop();
}

TEST_F(MediaQualityAnalyzerTest, TestRtcpInactivity)
{
    EXPECT_CALL(mCallback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(2);
    EXPECT_CALL(mCallback, onEvent(kImsMediaEventMediaQualityStatus, _, _)).Times(3);
    MediaQualityThreshold threshold;
    threshold.setRtcpInactivityTimerMillis(kRtcpInactivityTimerMillis);
    mAnalyzer->setMediaQualityThreshold(threshold);
    mAnalyzer->start();
    mCondition.wait_timeout(2100);  // 2.1 sec

    // Check MediaQualityStatus value
    MediaQualityStatus quality1 = mFakeCallback.getMediaQualityStatus();
    EXPECT_EQ(quality1.getRtcpInactivityTimeMillis(), 2000);

    mCondition.wait_timeout(2100);  // 2.1 sec

    // Check MediaQualityStatus value
    MediaQualityStatus quality2 = mFakeCallback.getMediaQualityStatus();
    EXPECT_EQ(quality2.getRtcpInactivityTimeMillis(), 2000);

    mAnalyzer->SendEvent(kCollectPacketInfo, kStreamRtcp);

    mCondition.wait_timeout(2100);  // 2.1 sec

    MediaQualityStatus quality3 = mFakeCallback.getMediaQualityStatus();
    EXPECT_EQ(quality3.getRtcpInactivityTimeMillis(), 2000);
    mAnalyzer->stop();
}

TEST_F(MediaQualityAnalyzerTest, TestCallQualityInactivity)
{
    EXPECT_CALL(mCallback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(2);
    mAnalyzer->start();
    mCondition.wait_timeout(4100);  // 4.1 sec

    CallQuality quality = mAnalyzer->getCallQuality();
    mAnalyzer->stop();

    // Check CallQuality value
    CallQuality quality2 = mFakeCallback.getCallQuality();
    EXPECT_EQ(quality2, quality);
    EXPECT_TRUE(quality2.getRtpInactivityDetected());
}

TEST_F(MediaQualityAnalyzerTest, TestCallQualityLevelChanged)
{
    EXPECT_CALL(mCallback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(2);
    mAnalyzer->start();

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
        mAnalyzer->SendEvent(kCollectPacketInfo, kStreamRtpRx, reinterpret_cast<uint64_t>(packet));

        SessionCallbackParameter* param = new SessionCallbackParameter(
                i, kRtpStatusNormal, ImsMediaTimer::GetTimeInMilliSeconds());
        mAnalyzer->SendEvent(kCollectRxRtpStatus, reinterpret_cast<uint64_t>(param));
    }

    SessionCallbackParameter* param = new SessionCallbackParameter(kReportPacketLossGap, 5, 1);
    mAnalyzer->SendEvent(kCollectOptionalInfo, reinterpret_cast<uint64_t>(param), 0);

    mCondition.wait_timeout(5100);  // 5.1 sec

    EXPECT_EQ(mAnalyzer->getTxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), numPackets - 1);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 1);
    CallQuality quality = mAnalyzer->getCallQuality();
    mAnalyzer->stop();

    EXPECT_EQ(mAnalyzer->getTxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 0);

    // Check CallQuality value
    CallQuality quality2 = mFakeCallback.getCallQuality();
    EXPECT_EQ(quality2, quality);
    EXPECT_EQ(quality2.getNumRtpPacketsReceived(), numPackets - 1);
    EXPECT_EQ(quality2.getDownlinkCallQualityLevel(), CallQuality::kCallQualityBad);
}

TEST_F(MediaQualityAnalyzerTest, TestJitterInd)
{
    EXPECT_CALL(mCallback, onEvent(kImsMediaEventMediaQualityStatus, _, _)).Times(1);
    EXPECT_CALL(mCallback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(1);
    MediaQualityThreshold threshold;
    threshold.setRtpHysteresisTimeInMillis(kRtpHysteresisTimeInMillis);
    threshold.setRtpJitterMillis(kRtpJitterMillis);
    mAnalyzer->setMediaQualityThreshold(threshold);
    mAnalyzer->start();

    const int32_t numPackets = 20;
    const int32_t jitter = 20;
    const uint32_t ssrc = 10000;

    for (int32_t i = 0; i < numPackets; i++)
    {
        RtpPacket* packet = new RtpPacket();
        packet->seqNum = i;
        packet->jitter = jitter;
        packet->ssrc = ssrc;
        mAnalyzer->SendEvent(kCollectPacketInfo, kStreamRtpRx, reinterpret_cast<uint64_t>(packet));
    }

    mCondition.wait_timeout(1100);  // 1.1 sec

    EXPECT_EQ(mAnalyzer->getTxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), numPackets);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 0);

    CallQuality quality = mAnalyzer->getCallQuality();
    mAnalyzer->stop();

    EXPECT_EQ(mAnalyzer->getTxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 0);

    CallQuality quality2 = mFakeCallback.getCallQuality();
    EXPECT_EQ(quality2, quality);
    EXPECT_EQ(quality2.getNumRtpPacketsReceived(), numPackets);
    EXPECT_EQ(quality2.getAverageRelativeJitter(), jitter);

    MediaQualityStatus status = mFakeCallback.getMediaQualityStatus();
    EXPECT_EQ(status.getRtpJitterMillis(), jitter);
}

TEST_F(MediaQualityAnalyzerTest, TestSsrcChange)
{
    mAnalyzer->start();

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

        mAnalyzer->SendEvent(kCollectPacketInfo, kStreamRtpRx, reinterpret_cast<uint64_t>(packet));
    }

    mCondition.wait_timeout(1100);  // 1.1 sec
    EXPECT_EQ(mAnalyzer->getTxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), numPackets);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 0);
    mAnalyzer->stop();

    EXPECT_EQ(mAnalyzer->getTxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 0);
}

TEST_F(MediaQualityAnalyzerTest, TestPacketLossInd)
{
    EXPECT_CALL(mCallback, onEvent(kImsMediaEventMediaQualityStatus, _, _)).Times(1);
    EXPECT_CALL(mCallback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(1);
    MediaQualityThreshold threshold;
    threshold.setRtpHysteresisTimeInMillis(kRtpHysteresisTimeInMillis);
    threshold.setRtpPacketLossDurationMillis(kRtpPacketLossDurationMillis);
    threshold.setRtpPacketLossRate(kRtpPacketLossRate);
    mAnalyzer->setMediaQualityThreshold(threshold);
    mAnalyzer->start();

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
        packet->arrival = ImsMediaTimer::GetTimeInMilliSeconds();
        mAnalyzer->SendEvent(kCollectPacketInfo, kStreamRtpRx, reinterpret_cast<uint64_t>(packet));
    }

    SessionCallbackParameter* param = new SessionCallbackParameter(kReportPacketLossGap, 5, 1);
    mAnalyzer->SendEvent(kCollectOptionalInfo, reinterpret_cast<uint64_t>(param), 0);

    mCondition.wait_timeout(1100);  // 1.1 sec

    EXPECT_EQ(mAnalyzer->getTxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), numPackets - 1);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 1);

    CallQuality quality = mAnalyzer->getCallQuality();
    mAnalyzer->stop();

    EXPECT_EQ(mAnalyzer->getTxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getRxPacketSize(), 0);
    EXPECT_EQ(mAnalyzer->getLostPacketSize(), 0);

    CallQuality quality2 = mFakeCallback.getCallQuality();
    EXPECT_EQ(quality2, quality);
    EXPECT_EQ(quality2.getNumRtpPacketsNotReceived(), 1);

    MediaQualityStatus status = mFakeCallback.getMediaQualityStatus();
    EXPECT_EQ(status.getRtpPacketLossRate(), 10);
}

TEST_F(MediaQualityAnalyzerTest, TestNotifyMediaQualityStatus)
{
    EXPECT_CALL(mCallback, onEvent(kImsMediaEventMediaQualityStatus, _, _)).Times(1);
    EXPECT_CALL(mCallback, onEvent(kAudioCallQualityChangedInd, _, _)).Times(1);
    MediaQualityThreshold threshold;
    threshold.setNotifyCurrentStatus(true);
    mAnalyzer->setMediaQualityThreshold(threshold);
    mAnalyzer->start();

    mCondition.wait_timeout(2100);  // 2.1 sec

    mAnalyzer->stop();
}