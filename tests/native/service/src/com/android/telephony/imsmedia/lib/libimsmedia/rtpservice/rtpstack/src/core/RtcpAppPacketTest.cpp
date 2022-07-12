/*
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

#include <RtcpAppPacket.h>
#include <gtest/gtest.h>

namespace android
{

class RtcpAppPacketTest : public ::testing::Test
{
public:
    RtcpAppPacket* testRtcpAppPacket;

protected:
    virtual void SetUp() override
    {
        testRtcpAppPacket = new RtcpAppPacket();
        ASSERT_TRUE(testRtcpAppPacket != nullptr);
    }

    virtual void TearDown() override
    {
        if (testRtcpAppPacket)
        {
            delete testRtcpAppPacket;
            testRtcpAppPacket = nullptr;
        }
    }
};

/** Successful Test scenario */
TEST_F(RtcpAppPacketTest, decodeAppPacketSuccess)
{
    RtpDt_UChar* pucAppBuf = new RtpDt_UChar[14];
    ASSERT_TRUE(pucAppBuf != RTP_NULL);

    memcpy(pucAppBuf,
            (RtpDt_UChar[]){0x80, 0xCC, 0x00, 0x07, 0x19, 0x6D, 0x27, 0xC5, 0x2B, 0x67, 0x01}, 13);
    EXPECT_TRUE(testRtcpAppPacket->decodeAppPacket(pucAppBuf, 13));
    delete[] pucAppBuf;
}

/** App Packet with Exact 12 Byte, Without Application dependent data */
TEST_F(RtcpAppPacketTest, decodeAppPacketBoundaryLength)
{
    RtpDt_UChar* pucAppBuf = new RtpDt_UChar[14];
    ASSERT_TRUE(pucAppBuf != RTP_NULL);

    memcpy(pucAppBuf, (RtpDt_UChar[]){0x80, 0xCC, 0x00, 0x07, 0x19, 0x6D, 0x27, 0xC5, 0x2B, 0x67},
            12);
    EXPECT_TRUE(testRtcpAppPacket->decodeAppPacket(pucAppBuf, 12));
    delete[] pucAppBuf;
}

/**  App Packet with less than expected Length, Without Application dependent data */
TEST_F(RtcpAppPacketTest, decodeAppPacketUnderBoundaryLength)
{
    RtpDt_UChar* pucAppBuf = new RtpDt_UChar[14];
    ASSERT_TRUE(pucAppBuf != RTP_NULL);

    memcpy(pucAppBuf, (RtpDt_UChar[]){0x80, 0xCC, 0x00, 0x07, 0x19, 0x6D, 0x27, 0xC5, 0x2B}, 11);
    EXPECT_TRUE(testRtcpAppPacket->decodeAppPacket(pucAppBuf, 11));
    delete[] pucAppBuf;
}

/** Successful Test scenario */
TEST_F(RtcpAppPacketTest, formAppPacketSuccessTest)
{
    RtpBuffer* testBuf;
    testBuf = new RtpBuffer();
    ASSERT_TRUE(testBuf != nullptr);

    RtpDt_UChar* pcBuff = new RtpDt_UChar[RTP_DEF_MTU_SIZE];

    ASSERT_TRUE(pcBuff != RTP_NULL);

    testBuf->setBufferInfo(RTP_DEF_MTU_SIZE, pcBuff);
    testBuf->setLength(RTP_ZERO);

    EXPECT_EQ(RTP_ZERO, testBuf->getLength());

    testRtcpAppPacket->setAppData(testBuf);

    RtpDt_UInt32 uiName = 1111;
    testRtcpAppPacket->setName(uiName);

    EXPECT_TRUE(testRtcpAppPacket->formAppPacket(testBuf));
}

/** With m_pAppData condition true */
TEST_F(RtcpAppPacketTest, formAppPacketBufferOverflowTest)
{
    RtpBuffer* testBuf;
    testBuf = new RtpBuffer();
    ASSERT_TRUE(testBuf != nullptr);

    RtpDt_UChar* pcBuff = new RtpDt_UChar[RTP_DEF_MTU_SIZE];

    ASSERT_TRUE(pcBuff != RTP_NULL);

    testBuf->setBufferInfo(RTP_DEF_MTU_SIZE, pcBuff);
    testBuf->setLength(RTP_DEF_MTU_SIZE);

    EXPECT_EQ(RTP_DEF_MTU_SIZE, testBuf->getLength());

    RtpDt_UInt32 uiName = 11111111;
    testRtcpAppPacket->setName(uiName);

    RtpBuffer* testBufAppData;
    testBufAppData = new RtpBuffer();
    ASSERT_TRUE(testBufAppData != nullptr);

    RtpDt_UChar* appBuffNew = new RtpDt_UChar[10];
    ASSERT_TRUE(appBuffNew != nullptr);

    memcpy(appBuffNew,
            (RtpDt_UChar[]){0x01, 0x07, 0x08, 0x09, 0x01, 0x02, 0x03, 0x04, 0xAA, 0xBB, 0x40, 0x20},
            10);

    testBufAppData->setBufferInfo(10, appBuffNew);
    testBufAppData->setLength(10);

    testRtcpAppPacket->setAppData(testBufAppData);
    EXPECT_TRUE(testRtcpAppPacket->formAppPacket(testBuf));

    delete testBuf;
}

}  // namespace android
