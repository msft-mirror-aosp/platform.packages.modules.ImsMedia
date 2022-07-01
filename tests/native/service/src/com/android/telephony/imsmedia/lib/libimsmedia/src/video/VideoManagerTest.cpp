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

#include <VideoConfig.h>
#include <VideoManager.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <ImsMediaNetworkUtil.h>
#include <media/NdkImageReader.h>
#include <ImsMediaCondition.h>
#include <ImsMediaTrace.h>

using namespace android::telephony::imsmedia;
// RtpConfig
const int32_t kMediaDirection = RtpConfig::MEDIA_DIRECTION_NO_FLOW;
const android::String8 kRemoteAddress("0.0.0.0");
const int32_t kRemotePort = 1000;
const int32_t kMtu = 1500;
const int8_t kDscp = 0;
const int8_t kRxPayload = 102;
const int8_t kTxPayload = 102;
const int8_t kSamplingRate = 90;

// RtcpConfig
const android::String8 kCanonicalName("name");
const int32_t kTransmitPort = 1001;
const int32_t kIntervalSec = 1500;
const int32_t kRtcpXrBlockTypes = RtcpConfig::FLAG_RTCPXR_STATISTICS_SUMMARY_REPORT_BLOCK |
        RtcpConfig::FLAG_RTCPXR_VOIP_METRICS_REPORT_BLOCK;

// VideoConfig
const int32_t kVideoMode = VideoConfig::VIDEO_MODE_PREVIEW;
const int32_t kCodecType = VideoConfig::CODEC_AVC;
const int32_t kFramerate = DEFAULT_FRAMERATE;
const int32_t kBitrate = DEFAULT_BITRATE;
const int32_t kCodecProfile = VideoConfig::AVC_PROFILE_BASELINE;
const int32_t kCodecLevel = VideoConfig::AVC_LEVEL_12;
const int32_t kIntraFrameIntervalSec = 1;
const int32_t kPacketizationMode = VideoConfig::MODE_NON_INTERLEAVED;
const int32_t kCameraId = 0;
const int32_t kCameraZoom = 10;
const int32_t kResolutionWidth = DEFAULT_RESOLUTION_WIDTH;
const int32_t kResolutionHeight = DEFAULT_RESOLUTION_HEIGHT;
const android::String8 kPauseImagePath("data/user_de/0/com.android.telephony.imsmedia/test.jpg");
const int32_t kDeviceOrientationDegree = 0;
const int32_t kCvoValue = 1;
const int32_t kRtcpFbTypes = VideoConfig::RTP_FB_NONE;

using namespace android::telephony::imsmedia;

class VideoManagerTest : public ::testing::Test
{
public:
    VideoManagerTest() {}
    ~VideoManagerTest() {}

    AImageReader* previewReader;
    AImageReader* displayReader;
    ANativeWindow* previewSurface;
    ANativeWindow* displaySurface;
    RtcpConfig rtcp;
    VideoConfig config;
    VideoManager* manager;

protected:
    virtual void SetUp() override
    {
        manager = VideoManager::getInstance();

        auto status = AImageReader_new(
                kResolutionWidth, kResolutionHeight, AIMAGE_FORMAT_YUV_420_888, 1, &previewReader);
        if (status != AMEDIA_OK)
        {
            IMLOGE0("[SetUp] error");
        }
        AImageReader_getWindow(previewReader, &previewSurface);

        status = AImageReader_new(
                kResolutionWidth, kResolutionHeight, AIMAGE_FORMAT_YUV_420_888, 1, &displayReader);
        if (status != AMEDIA_OK)
        {
            IMLOGE0("[SetUp] error");
        }
        AImageReader_getWindow(displayReader, &displaySurface);

        rtcp.setCanonicalName(kCanonicalName);
        rtcp.setTransmitPort(kTransmitPort);
        rtcp.setIntervalSec(kIntervalSec);
        rtcp.setRtcpXrBlockTypes(kRtcpXrBlockTypes);
        config.setMediaDirection(kMediaDirection);
        config.setRemoteAddress(kRemoteAddress);
        config.setRemotePort(kRemotePort);
        config.setRtcpConfig(rtcp);
        config.setMaxMtuBytes(kMtu);
        config.setDscp(kDscp);
        config.setRxPayloadTypeNumber(kRxPayload);
        config.setTxPayloadTypeNumber(kTxPayload);
        config.setSamplingRateKHz(kSamplingRate);
        config.setVideoMode(kVideoMode);
        config.setCodecType(kCodecType);
        config.setFramerate(kFramerate);
        config.setBitrate(kBitrate);
        config.setCodecProfile(kCodecProfile);
        config.setCodecLevel(kCodecLevel);
        config.setIntraFrameInterval(kIntraFrameIntervalSec);
        config.setPacketizationMode(kPacketizationMode);
        config.setCameraId(kCameraId);
        config.setCameraZoom(kCameraZoom);
        config.setResolutionWidth(kResolutionWidth);
        config.setResolutionHeight(kResolutionHeight);
        config.setPauseImagePath(kPauseImagePath);
        config.setDeviceOrientationDegree(kDeviceOrientationDegree);
        config.setCvoValue(kCvoValue);
        config.setRtcpFbType(kRtcpFbTypes);
    }

    virtual void TearDown() override
    {
        if (previewReader != NULL)
        {
            AImageReader_delete(previewReader);
        }
        if (displayReader != NULL)
        {
            AImageReader_delete(displayReader);
        }
    }
};

TEST_F(VideoManagerTest, TestVideoPreview)
{
    ImsMediaCondition condition;
    config.setVideoMode(VideoConfig::VIDEO_MODE_PREVIEW);
    int sessionId = 0;

    const char testIp[] = "127.0.0.1";
    unsigned int testPort = 12340;
    int socketRtpFd = ImsMediaNetworkUtil::createSocketFD(testIp, testPort, AF_INET);
    int socketRtcpFd = ImsMediaNetworkUtil::createSocketFD(testIp, testPort + 1, AF_INET);
    EXPECT_NE(socketRtpFd, -1);
    EXPECT_NE(socketRtcpFd, -1);

    android::Parcel parcel;
    parcel.writeInt32(kVideoOpenSession);
    parcel.writeInt32(socketRtpFd);
    parcel.writeInt32(socketRtcpFd);
    config.writeToParcel(&parcel);
    parcel.setDataPosition(0);

    manager->sendMessage(sessionId, parcel);
    condition.wait_timeout(500);
    EXPECT_EQ(manager->getState(sessionId), kSessionStateSuspended);

    manager->setPreviewSurface(sessionId, previewSurface);
    condition.wait_timeout(500);
    EXPECT_EQ(manager->getState(sessionId), kSessionStateActive);
    condition.wait_timeout(500);

    android::Parcel parcel2;
    parcel2.writeInt32(kVideoCloseSession);
    parcel2.writeInt32(sessionId);
    parcel2.setDataPosition(0);

    manager->sendMessage(sessionId, parcel2);
    condition.wait_timeout(500);
    EXPECT_EQ(manager->getState(sessionId), kSessionStateClosed);

    ImsMediaNetworkUtil::closeSocketFD(socketRtpFd);
    ImsMediaNetworkUtil::closeSocketFD(socketRtcpFd);
}
