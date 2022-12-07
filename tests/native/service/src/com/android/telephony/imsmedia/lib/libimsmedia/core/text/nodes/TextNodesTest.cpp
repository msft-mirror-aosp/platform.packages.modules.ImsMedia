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
#include <TextConfig.h>

#include <TextRtpPayloadEncoderNode.h>
#include <TextRtpPayloadDecoderNode.h>
#include <TextSourceNode.h>
#include <TextRendererNode.h>
#include <MockBaseSessionCallback.h>

using namespace android::telephony::imsmedia;

const int32_t kMediaDirection = RtpConfig::MEDIA_DIRECTION_SEND_ONLY;
const android::String8 kRemoteAddress("127.0.0.1");
const int32_t kRemotePort = 10000;
const int8_t kDscp = 0;
const int8_t kRxPayload = 96;
const int8_t kTxPayload = 96;
const int8_t kSamplingRate = 16;

// RtcpConfig
const android::String8 kCanonicalName("name");
const int32_t kTransmitPort = 1001;
const int32_t kIntervalSec = 5;
const int32_t kRtcpXrBlockTypes = RtcpConfig::FLAG_RTCPXR_STATISTICS_SUMMARY_REPORT_BLOCK |
        RtcpConfig::FLAG_RTCPXR_VOIP_METRICS_REPORT_BLOCK;

// TextConfig
const int32_t kCodecType = TextConfig::TEXT_T140_RED;
const int32_t kBitrate = 100;
const int8_t kRedundantPayload = 102;
const int8_t kRedundantLevel = 3;
const bool kKeepRedundantLevel = true;
const android::String8 kTestText("hello");

using ::testing::_;
using ::testing::NotNull;
using ::testing::Return;

class FakeBaseSessionCallback : public BaseSessionCallback
{
public:
    FakeBaseSessionCallback() {}
    virtual ~FakeBaseSessionCallback() {}

    virtual void SendEvent(int32_t type, uint64_t param1, uint64_t param2)
    {
        (void)param2;
        EXPECT_EQ(type, kNodeIdTextRenderer);
        android::String8* text = reinterpret_cast<android::String8*>(param1);
        ASSERT_TRUE(text != nullptr);
        EXPECT_EQ(*text, kTestText);
        delete text;
    }

    virtual void onEvent(int32_t type, uint64_t param1, uint64_t param2)
    {
        (void)type;
        (void)param1;
        (void)param2;
    }
};

class TextNodesTest : public ::testing::Test
{
public:
    TextNodesTest() {}
    virtual ~TextNodesTest() {}

protected:
    TextConfig mConfig;
    RtcpConfig mRtcp;
    std::list<BaseNode*> mNodes;
    FakeBaseSessionCallback mFakeCallback;
    MockBaseSessionCallback mCallback;

    virtual void SetUp() override
    {
        mCallback.SetDelegate(&mFakeCallback);
        mCallback.DelegateToFake();

        mRtcp.setCanonicalName(kCanonicalName);
        mRtcp.setTransmitPort(kTransmitPort);
        mRtcp.setIntervalSec(kIntervalSec);
        mRtcp.setRtcpXrBlockTypes(kRtcpXrBlockTypes);

        mConfig.setMediaDirection(kMediaDirection);
        mConfig.setRemoteAddress(kRemoteAddress);
        mConfig.setRemotePort(kRemotePort);
        mConfig.setRtcpConfig(mRtcp);
        mConfig.setDscp(kDscp);
        mConfig.setRxPayloadTypeNumber(kRxPayload);
        mConfig.setTxPayloadTypeNumber(kTxPayload);
        mConfig.setSamplingRateKHz(kSamplingRate);
        mConfig.setCodecType(kCodecType);
        mConfig.setBitrate(kBitrate);
        mConfig.setRedundantPayload(kRedundantPayload);
        mConfig.setRedundantLevel(kRedundantLevel);
        mConfig.setKeepRedundantLevel(kKeepRedundantLevel);

        BaseNode* source = new TextSourceNode(&mCallback);
        source->SetMediaType(IMS_MEDIA_TEXT);
        source->SetConfig(&mConfig);
        mNodes.push_back(source);

        BaseNode* rtpPayloadEncoder = new TextRtpPayloadEncoderNode(&mCallback);
        rtpPayloadEncoder->SetMediaType(IMS_MEDIA_TEXT);
        rtpPayloadEncoder->SetConfig(&mConfig);
        mNodes.push_back(rtpPayloadEncoder);
        source->ConnectRearNode(rtpPayloadEncoder);

        BaseNode* rtpPayloadDecoder = new TextRtpPayloadDecoderNode(&mCallback);
        rtpPayloadDecoder->SetMediaType(IMS_MEDIA_TEXT);
        rtpPayloadDecoder->SetConfig(&mConfig);
        mNodes.push_back(rtpPayloadDecoder);
        rtpPayloadEncoder->ConnectRearNode(rtpPayloadDecoder);

        BaseNode* renderer = new TextRendererNode(&mCallback);
        renderer->SetMediaType(IMS_MEDIA_TEXT);
        renderer->SetConfig(&mConfig);
        mNodes.push_back(renderer);
        rtpPayloadDecoder->ConnectRearNode(renderer);
    }

    virtual void TearDown() override
    {
        while (mNodes.size() > 0)
        {
            BaseNode* node = mNodes.front();
            delete node;
            mNodes.pop_front();
        }
    }
};

TEST_F(TextNodesTest, startFail)
{
    mConfig.setCodecType(TextConfig::TEXT_CODEC_NONE);

    for (auto& nodes : mNodes)
    {
        nodes->SetConfig(&mConfig);
    }

    for (auto& nodes : mNodes)
    {
        EXPECT_EQ(nodes->Start(), RESULT_INVALID_PARAM);
    }
}

TEST_F(TextNodesTest, sendRtt)
{
    for (auto& nodes : mNodes)
    {
        EXPECT_EQ(nodes->Start(), RESULT_SUCCESS);
    }

    TextSourceNode* node = reinterpret_cast<TextSourceNode*>(mNodes.front());

    EXPECT_NE(node, nullptr);
    node->SendRtt(&kTestText);

    for (auto& nodes : mNodes)
    {
        if (nodes->GetNodeId() == kNodeIdTextRenderer)
        {
            nodes->ProcessData();
        }
    }
}