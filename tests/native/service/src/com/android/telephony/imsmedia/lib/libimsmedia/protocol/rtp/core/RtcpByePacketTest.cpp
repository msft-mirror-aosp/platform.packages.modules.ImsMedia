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

#include <RtcpByePacket.h>
#include <gtest/gtest.h>

#include <memory>

namespace android
{

class RtcpByePacketTest : public ::testing::Test
{
public:
    RtcpByePacket* testRtcpByePacket;

protected:
    virtual void SetUp() override
    {
        testRtcpByePacket = new RtcpByePacket();
        ASSERT_TRUE(testRtcpByePacket != nullptr);
    }

    virtual void TearDown() override
    {
        if (testRtcpByePacket)
        {
            delete testRtcpByePacket;
            testRtcpByePacket = nullptr;
        }
    }
};

/** Successful Test scenario with multiple SSRC */
TEST_F(RtcpByePacketTest, decodeByePacketMultipleSSRCTest)
{
    std::unique_ptr<RtpDt_UChar[]> pucByeBuf(new RtpDt_UChar[25]);
    ASSERT_TRUE(pucByeBuf != nullptr);

    /* pucByeBuf injected with multiple ssrc number */
    memcpy(pucByeBuf.get(),
            (RtpDt_UChar[]){0x82, 0xCB, 0x00, 0x08, 0x19, 0x6D, 0x27, 0xC5, 0xE2, 0xA5, 0x19, 0x01,
                    0x08, 0x74, 0x65, 0x61, 0x72, 0x64, 0x6F, 0x77, 0x6E},
            22);

    EXPECT_TRUE(testRtcpByePacket->decodeByePacket(pucByeBuf.get(), 22));
}

/** Successful Test scenario single SSRC without Optional reason and length */
TEST_F(RtcpByePacketTest, decodeByePacketSingleSSRCTest)
{
    std::unique_ptr<RtpDt_UChar[]> pucByeBuf(new RtpDt_UChar[13]);
    ASSERT_TRUE(pucByeBuf != nullptr);

    /* pucByeBuf contains session RTCP Header, Optional reason and length excluded */
    memcpy(pucByeBuf.get(),
            (RtpDt_UChar[]){0x82, 0xCB, 0x00, 0x08, 0x19, 0x6D, 0x27, 0xC5, 0xE2, 0xA5, 0x19, 0x01},
            12);

    EXPECT_TRUE(testRtcpByePacket->decodeByePacket(pucByeBuf.get(), 12));
}

/** Successful Test scenario with multiple SSRC with optional data included. */
TEST_F(RtcpByePacketTest, decodeByePacketMultipleSSRCOptionalDataTest)
{
    std::unique_ptr<RtpDt_UChar[]> pucByeBuf(new RtpDt_UChar[28]);
    ASSERT_TRUE(pucByeBuf != nullptr);

    /* pucByeBuf contains RTCP header, 2 SSRC and optional Length and Reason. */
    memcpy(pucByeBuf.get(),
            (RtpDt_UChar[]){0x83, 0xCB, 0x00, 0x08, 0x19, 0x6D, 0x27, 0xC5, 0xE2, 0xA5, 0x19, 0x01,
                    0x07, 0xFF, 0xF4, 0xAA, 0x08, 0x74, 0x65, 0x61, 0x72, 0x64, 0x6F, 0x77, 0x6E},
            26);

    EXPECT_TRUE(testRtcpByePacket->decodeByePacket(pucByeBuf.get(), 26));
}

/** Unexpected more length of Optional reason test. */
TEST_F(RtcpByePacketTest, decodeByePacketReasonLengthOverflowTest)
{
    std::unique_ptr<RtpDt_UChar[]> pucByeBuf(new RtpDt_UChar[28]);
    ASSERT_TRUE(pucByeBuf != nullptr);

    /* pucByeBuf injected with unexpected Optiona length value, how ever
     * optional message having less length */
    memcpy(pucByeBuf.get(),
            (RtpDt_UChar[]){0x83, 0xCB, 0x00, 0x08, 0x19, 0x6D, 0x27, 0xC5, 0xE2, 0xA5, 0x19, 0x01,
                    0x07, 0xFF, 0xF4, 0xAA, 0x0C, 0x74, 0x65, 0x61, 0x72, 0x64, 0x6F, 0x77, 0x6E},
            26);

    EXPECT_TRUE(testRtcpByePacket->decodeByePacket(pucByeBuf.get(), 26));
}

/** Unexpected less length of Optional reason test. */
TEST_F(RtcpByePacketTest, decodeByePacketReasonLengthUnderflowTest)
{
    std::unique_ptr<RtpDt_UChar[]> pucByeBuf(new RtpDt_UChar[28]);
    ASSERT_TRUE(pucByeBuf != nullptr);

    /* pucByeBuf injected with unexpected Optiona length value, how ever
     * optional message having more length */
    memcpy(pucByeBuf.get(),
            (RtpDt_UChar[]){0x83, 0xCB, 0x00, 0x08, 0x19, 0x6D, 0x27, 0xC5, 0xE2, 0xA5, 0x19, 0x01,
                    0x07, 0xFF, 0xF4, 0xAA, 0x04, 0x74, 0x65, 0x61, 0x72, 0x64, 0x6F, 0x77, 0x6E},
            26);

    EXPECT_TRUE(testRtcpByePacket->decodeByePacket(pucByeBuf.get(), 26));
}
/** Forming RtcpBye packet without Optional Bye reason */
TEST_F(RtcpByePacketTest, formByePacketSuccessTest)
{
    std::unique_ptr<RtpBuffer> testBuf(new RtpBuffer());
    ASSERT_TRUE(testBuf != nullptr);

    std::unique_ptr<RtpDt_UChar[]> pcBuff(new RtpDt_UChar[RTP_DEF_MTU_SIZE]);

    ASSERT_TRUE(pcBuff != nullptr);

    testBuf->setBufferInfo(RTP_DEF_MTU_SIZE, pcBuff.release());
    testBuf->setLength(RTP_ZERO);

    EXPECT_EQ(RTP_ZERO, testBuf->getLength());

    EXPECT_TRUE(testRtcpByePacket->formByePacket(testBuf.get()));
}

/** Forming RtcpBye packet with Optional Bye reason */
TEST_F(RtcpByePacketTest, formByePacketSuccessTestWithByeReason)
{
    std::unique_ptr<RtpBuffer> testBuf(new RtpBuffer());
    ASSERT_TRUE(testBuf != nullptr);

    std::unique_ptr<RtpDt_UChar[]> pcBuff(new RtpDt_UChar[RTP_DEF_MTU_SIZE]);

    ASSERT_TRUE(pcBuff != nullptr);

    testBuf->setBufferInfo(RTP_DEF_MTU_SIZE, pcBuff.release());
    testBuf->setLength(RTP_ZERO);

    EXPECT_EQ(RTP_ZERO, testBuf->getLength());

    std::unique_ptr<RtpBuffer> testReasonBuf(new RtpBuffer());

    std::unique_ptr<RtpDt_UChar[]> byeReasonBuf(new RtpDt_UChar[10]);
    memcpy(byeReasonBuf.get(),
            (RtpDt_UChar[]){0x08, 0x74, 0x65, 0x61, 0x72, 0x64, 0x6F, 0x77, 0x6E}, 9);

    testReasonBuf->setBufferInfo(9, byeReasonBuf.release());

    testRtcpByePacket->setReason(testReasonBuf.release());

    EXPECT_TRUE(testRtcpByePacket->formByePacket(testBuf.get()));
}

}  // namespace android
