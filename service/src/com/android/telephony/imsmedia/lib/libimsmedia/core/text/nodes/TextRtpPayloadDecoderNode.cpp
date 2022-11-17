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

#include <TextRtpPayloadDecoderNode.h>
#include <TextConfig.h>
#include <ImsMediaTrace.h>
#include <list>

TextRtpPayloadDecoderNode::TextRtpPayloadDecoderNode(BaseSessionCallback* callback) :
        BaseNode(callback)
{
    mCodecType = TextConfig::TEXT_CODEC_NONE;
}

TextRtpPayloadDecoderNode::~TextRtpPayloadDecoderNode() {}

kBaseNodeId TextRtpPayloadDecoderNode::GetNodeId()
{
    return kNodeIdTextPayloadDecoder;
}

ImsMediaResult TextRtpPayloadDecoderNode::Start()
{
    IMLOGD1("[Start] codec[%d]", mCodecType);

    if (mCodecType == TextConfig::TEXT_CODEC_NONE)
    {
        return RESULT_INVALID_PARAM;
    }

    mNodeState = kNodeStateRunning;
    return RESULT_SUCCESS;
}

void TextRtpPayloadDecoderNode::Stop()
{
    IMLOGD0("[Stop]");
    mNodeState = kNodeStateStopped;
}

bool TextRtpPayloadDecoderNode::IsRunTime()
{
    return true;
}

bool TextRtpPayloadDecoderNode::IsSourceNode()
{
    return false;
}

void TextRtpPayloadDecoderNode::OnDataFromFrontNode(ImsMediaSubType subtype, uint8_t* data,
        uint32_t size, uint32_t timestamp, bool mark, uint32_t seqNum, ImsMediaSubType dataType,
        uint32_t arrivalTime)
{
    (void)dataType;
    (void)arrivalTime;

    if (subtype == MEDIASUBTYPE_REFRESHED)
    {
        SendDataToRearNode(subtype, NULL, size, 0, 0, 0, MEDIASUBTYPE_UNDEFINED);
        return;
    }

    switch (mCodecType)
    {
        case TextConfig::TEXT_T140:
        case TextConfig::TEXT_T140_RED:
            DecodeT140(data, size, subtype, timestamp, mark, seqNum);
            break;
        default:
            IMLOGE1("[OnDataFromFrontNode invalid codec type[%u]", mCodecType);
            break;
    }
}

void TextRtpPayloadDecoderNode::SetConfig(void* config)
{
    if (config == NULL)
    {
        return;
    }

    TextConfig* pConfig = reinterpret_cast<TextConfig*>(config);
    mCodecType = pConfig->getCodecType();
}

bool TextRtpPayloadDecoderNode::IsSameConfig(void* config)
{
    if (config == NULL)
    {
        return true;
    }

    TextConfig* pConfig = reinterpret_cast<TextConfig*>(config);

    return (mCodecType == pConfig->getCodecType());
}

void TextRtpPayloadDecoderNode::DecodeT140(uint8_t* data, uint32_t size, ImsMediaSubType subtype,
        uint32_t timestamp, bool mark, uint32_t seq)
{
    IMLOGD_PACKET5(IM_PACKET_LOG_PH,
            "[DecodeT140] subtype[%u], size[%u], timestamp[%d], mark[%d], seq[%d]", subtype, size,
            timestamp, mark, seq);

    if (subtype == MEDIASUBTYPE_BITSTREAM_T140 || subtype == MEDIASUBTYPE_BITSTREAM_T140_RED)
    {
        std::list<uint32_t> lstTSOffset;
        std::list<uint32_t> lstLength;
        uint32_t nReadByte = 0;
        uint32_t nRedCount = 0;

        mBitReader.SetBuffer(data, size);

        /*
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |1|   T140 PT   |  timestamp offset of "R"  | "R" block length  |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |0|   T140 PT   | "R" T.140 encoded redundant data              |
        +-+-+-+-+-+-+-+-+                               +---------------+
        |                                               |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        */

        // Primary Data Only
        // Red header + primary header = 5 byte
        if (subtype == MEDIASUBTYPE_BITSTREAM_T140)
        {
            SendDataToRearNode(MEDIASUBTYPE_BITSTREAM_T140, data, size, timestamp, mark, seq);
            return;
        }

        // Redundant data included
        while (mBitReader.Read(1) == 1)  // Rendundancy bit
        {
            uint32_t nPT = mBitReader.Read(7);  // T140 payload type
            uint32_t nTSOffset = mBitReader.Read(14);
            uint32_t nLen = mBitReader.Read(10);

            lstTSOffset.push_back(nTSOffset);  // timestamp offset
            lstLength.push_back(nLen);         // block length

            IMLOGD_PACKET3(IM_PACKET_LOG_PH, "[DecodeT140] nPT[%u], nTSOffset[%u], nLen[%u]", nPT,
                    nTSOffset, nLen);
            nReadByte += 4;
            nRedCount++;
        }

        mBitReader.Read(7);  // T140 payload type (111)
        nReadByte += 1;

        // redundant data
        while (lstTSOffset.size() > 0)
        {
            uint32_t nRedTimestamp = 0;
            uint32_t nRedLength = 0;
            uint32_t nRedSeqNum = 0;

            nRedTimestamp = lstTSOffset.front();
            nRedLength = lstLength.front();

            if (nRedLength > 0)
            {
                // here should compare mPayload size red length
                mBitReader.ReadByteBuffer(mPayload, nRedLength * 8);
                nReadByte += nRedLength;

                if (seq < nRedCount)
                {
                    nRedSeqNum = seq + 0xffff - nRedCount;  // round trip
                }
                else
                {
                    nRedSeqNum = seq - nRedCount;
                }

                IMLOGD_PACKET3(IM_PACKET_LOG_PH,
                        "[DecodeT140] nRedTimestamp[%u], nRedLength[%u], nRedSeqNum[%u]",
                        timestamp - nRedTimestamp, nRedLength, nRedSeqNum);
                SendDataToRearNode(MEDIASUBTYPE_BITSTREAM_T140, mPayload, nRedLength,
                        timestamp - nRedTimestamp, mark, nRedSeqNum);
            }

            nRedCount--;
            lstTSOffset.pop_front();
            lstLength.pop_front();
        }

        // primary data
        if (size - nReadByte > 0)
        {
            mBitReader.ReadByteBuffer(mPayload, (size - nReadByte) * 8);
        }

        SendDataToRearNode(
                MEDIASUBTYPE_BITSTREAM_T140, mPayload, (size - nReadByte), timestamp, mark, seq);
    }
    else
    {
        IMLOGW1("[DecodeT140] INVALID media sub type[%u]", subtype);
    }
}
