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

#include <AudioRtpPayloadEncoderNode.h>
#include <ImsMediaAudioFmt.h>
#include <ImsMediaTrace.h>
#include <AudioConfig.h>
#include <EvsParams.h>

AudioRtpPayloadEncoderNode::AudioRtpPayloadEncoderNode()
{
    mPtime = 0;
    mFirstFrame = false;
    mMaxNumOfFrame = 0;
    mCurrNumOfFrame = 0;
    mCurrFramePos = 0;
    mTotalPayloadSize = 0;
}

AudioRtpPayloadEncoderNode::~AudioRtpPayloadEncoderNode() {}

BaseNode* AudioRtpPayloadEncoderNode::GetInstance()
{
    return new AudioRtpPayloadEncoderNode();
}

void AudioRtpPayloadEncoderNode::ReleaseInstance(BaseNode* pNode)
{
    delete (AudioRtpPayloadEncoderNode*)pNode;
}

BaseNodeID AudioRtpPayloadEncoderNode::GetNodeID()
{
    return BaseNodeID::NODEID_RTPPAYLOAD_ENCODER_AUDIO;
}

ImsMediaResult AudioRtpPayloadEncoderNode::Start()
{
    IMLOGD2("[Start] codecType[%d], mode[%d]", mCodecType, mOctetAligned);
    std::lock_guard<std::mutex> guard(mMutexExit);
    mMaxNumOfFrame = mPtime / 20;

    if (mMaxNumOfFrame == 0 || mMaxNumOfFrame > MAX_FRAME_IN_PACKET)
    {
        IMLOGE1("[Start] Invalid ptime [%d]", mPtime);
        return RESULT_INVALID_PARAM;
    }

    mCurrNumOfFrame = 0;
    mCurrFramePos = 0;
    mFirstFrame = true;
    mTotalPayloadSize = 0;
    mNodeState = kNodeStateRunning;
    return RESULT_SUCCESS;
}

void AudioRtpPayloadEncoderNode::Stop()
{
    IMLOGD0("[Stop]");
    std::lock_guard<std::mutex> guard(mMutexExit);
    mNodeState = kNodeStateStopped;
}

bool AudioRtpPayloadEncoderNode::IsRunTime()
{
    return true;
}

bool AudioRtpPayloadEncoderNode::IsSourceNode()
{
    return false;
}

void AudioRtpPayloadEncoderNode::OnDataFromFrontNode(ImsMediaSubType subtype, uint8_t* pData,
        uint32_t nDataSize, uint32_t nTimestamp, bool bMark, uint32_t nSeqNum,
        ImsMediaSubType nDataType)
{
    (void)subtype;
    switch (mCodecType)
    {
        case kAudioCodecAmr:
        case kAudioCodecAmrWb:
            EncodePayloadAmr(pData, nDataSize, nTimestamp, bMark);
            break;
        case kAudioCodecPcmu:
        case kAudioCodecPcma:
            SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD, pData, nDataSize, nTimestamp, bMark,
                    nSeqNum, nDataType);
            break;
        case kAudioCodecEvs:
            EncodePayloadEvs(pData, nDataSize, nTimestamp);
            break;
        default:
            IMLOGE1("[OnDataFromFrontNode] invalid codec type[%d]", mCodecType);
            SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD, pData, nDataSize, nTimestamp, bMark,
                    nSeqNum, nDataType);
            break;
    }
}

void AudioRtpPayloadEncoderNode::SetConfig(void* config)
{
    AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);
    if (pConfig != NULL)
    {
        mCodecType = ImsMediaAudioFmt::ConvertCodecType(pConfig->getCodecType());
        if (mCodecType == kAudioCodecAmr || mCodecType == kAudioCodecAmrWb)
        {
            mOctetAligned = pConfig->getAmrParams().getOctetAligned();
        }
        else if (mCodecType == kAudioCodecEvs)
        {
            mEvsBandwidth = (kEvsBandwidth)pConfig->getEvsParams().getEvsBandwidth();
            mEvsPayloadHeaderMode =
                    (kRtpPyaloadHeaderMode)pConfig->getEvsParams().getUseHeaderFullOnlyOnTx();
            mEvsMode = (kEvsBitrate)pConfig->getEvsParams().getEvsMode();
            mEvsCodecMode = (kEvsCodecMode)ImsMediaAudioFmt::ConvertEvsCodecMode(
                    pConfig->getEvsParams().getEvsMode());
            mEvsBitRate =
                    ImsMediaAudioFmt::ConvertEVSModeToBitRate(pConfig->getEvsParams().getEvsMode());
            mEvsOffset = pConfig->getEvsParams().getChannelAwareMode();
            mSendCMR = pConfig->getCodecModeRequest();
        }

        mPtime = pConfig->getPtimeMillis();
    }
}

bool AudioRtpPayloadEncoderNode::IsSameConfig(void* config)
{
    if (config == NULL)
        return true;
    AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);

    if (mCodecType == ImsMediaAudioFmt::ConvertCodecType(pConfig->getCodecType()))
    {
        if (mCodecType == kAudioCodecAmr || mCodecType == kAudioCodecAmrWb)
        {
            return (mOctetAligned == pConfig->getAmrParams().getOctetAligned());
        }
        else if (mCodecType == kAudioCodecEvs)
        {
            return (mEvsBandwidth == (kEvsBandwidth)pConfig->getEvsParams().getEvsBandwidth() &&
                    mEvsPayloadHeaderMode ==
                            (kRtpPyaloadHeaderMode)pConfig->getEvsParams()
                                    .getUseHeaderFullOnlyOnRx() &&
                    mEvsCodecMode ==
                            ImsMediaAudioFmt::ConvertEvsCodecMode(
                                    pConfig->getEvsParams().getEvsMode()) &&
                    mEvsMode == (kEvsBitrate)pConfig->getEvsParams().getEvsMode() &&
                    mEvsBitRate ==
                            ImsMediaAudioFmt::ConvertEVSModeToBitRate(
                                    pConfig->getEvsParams().getEvsMode()) &&
                    mEvsOffset == pConfig->getEvsParams().getChannelAwareMode());
        }
    }

    return false;
}

void AudioRtpPayloadEncoderNode::EncodePayloadAmr(
        uint8_t* pData, uint32_t nDataSize, uint32_t nTimestamp, bool bMark)
{
    (void)bMark;
    uint32_t nCmr = 15;
    uint32_t f, ft, q, nDataBitSize;
    uint32_t nTotalSize;
    std::lock_guard<std::mutex> guard(mMutexExit);

#ifndef LEGACY_AUDIO_ENABLED  // for ap audio test
    pData++;
    nDataSize -= 1;
#endif
    if (nDataSize > 4)
    {
        IMLOGD_PACKET5(IM_PACKET_LOG_PH, "[EncodePayloadAmr] src = %02X %02X %02X %02X, len[%d]",
                pData[0], pData[1], pData[2], pData[3], nDataSize);
    }

    IMLOGD_PACKET2(IM_PACKET_LOG_PH, "[EncodePayloadAmr] codectype[%d], octetAligned[%d]",
            mCodecType, mOctetAligned);

    mCurrNumOfFrame++;
    f = (mCurrNumOfFrame == mMaxNumOfFrame) ? 0 : 1;

    if (mCodecType == kAudioCodecAmr)
    {
        nCmr = 0x0F;
        ft = ImsMediaAudioFmt::ConvertLenToAmrMode(nDataSize);
        nDataBitSize = ImsMediaAudioFmt::ConvertAmrModeToBitLen(ft);
    }
    else
    {
        nCmr = 0x0F;
        ft = ImsMediaAudioFmt::ConvertLenToAmrWbMode(nDataSize);
        nDataBitSize = ImsMediaAudioFmt::ConvertAmrWbModeToBitLen(ft);
    }

    q = 1;
#ifdef USE_CMR_TEST
    // cmr test code start
    // generate cmr
    static int sAMRCount = 0;
    static int bAMRUpdate = 0;
    static int nAMRNextMode = 7;
    static int nAMRInc = -1;
    sAMRCount++;
    if ((sAMRCount & 0x7F) == 0)
    {
        bAMRUpdate = 1;
    }
    nCmr = nAMRNextMode;
    if (bAMRUpdate == 1 && nDataSize > 0)
    {
        int max_mode;
        if (mCodecType == kAudioCodecAmr)
            max_mode = 7;
        else
            max_mode = 8;
        nAMRNextMode += nAMRInc;
        if (nAMRNextMode < 0)
        {
            nAMRNextMode = 1;
            nAMRInc = 1;
        }
        else if (nAMRNextMode > max_mode)
        {
            nAMRNextMode = max_mode - 1;
            nAMRInc = -1;
        }
        bAMRUpdate = 0;
    }
    // cmr test code end
#endif
    // the first paylaod
    if (mCurrNumOfFrame == 1)
    {
        memset(mPayload, 0, MAX_AUDIO_PAYLOAD_SIZE);
        mBWHeader.SetBuffer(mPayload, MAX_AUDIO_PAYLOAD_SIZE);
        mBWPayload.SetBuffer(mPayload, MAX_AUDIO_PAYLOAD_SIZE);
        mBWHeader.Write(nCmr, 4);

        if (mOctetAligned == true)
        {
            mBWHeader.Write(0, 4);
            mBWPayload.Seek(8 + mMaxNumOfFrame * 8);
        }
        else
        {
            mBWPayload.Seek(4 + mMaxNumOfFrame * 6);
        }

        mTimestamp = nTimestamp;
    }

    // Payload ToC
    mBWHeader.Write(f, 1);
    mBWHeader.Write(ft, 4);
    mBWHeader.Write(q, 1);

    if (mOctetAligned == true)
    {
        mBWHeader.AddPadding();
    }

    IMLOGD_PACKET2(IM_PACKET_LOG_PH, "[EncodePayloadAmr] nDataBitSize[%d], nDataSize[%d]",
            nDataBitSize, nDataSize);

    // Speech Frame
    mBWPayload.WriteByteBuffer(pData, nDataBitSize);

    if (mOctetAligned == true)
    {
        mBWPayload.AddPadding();
    }

    mTotalPayloadSize += nDataSize;

    if (mCurrNumOfFrame == mMaxNumOfFrame)
    {
        mBWHeader.Flush();
        mBWPayload.AddPadding();
        mBWPayload.Flush();
        nTotalSize = mBWPayload.GetBufferSize();

        IMLOGD_PACKET7(IM_PACKET_LOG_PH,
                "[EncodePayloadAmr] result = %02X %02X %02X %02X %02X %02X, len[%d]", mPayload[0],
                mPayload[1], mPayload[2], mPayload[3], mPayload[4], mPayload[5], nTotalSize);

        if (mTotalPayloadSize > 0)
        {
            SendDataToRearNode(
                    MEDIASUBTYPE_RTPPAYLOAD, mPayload, nTotalSize, mTimestamp, mFirstFrame, 0);
        }

        mCurrNumOfFrame = 0;
        mTotalPayloadSize = 0;

        if (mFirstFrame)
        {
            mFirstFrame = false;
        }
    }
}

void AudioRtpPayloadEncoderNode::EncodePayloadEvs(
        uint8_t* pData, uint32_t nDataSize, uint32_t nTimeStamp)
{
    kRtpPyaloadHeaderMode eEVSPayloadFormat = kRtpPyaloadHeaderModeEvsHeaderFull;
    kEvsCodecMode kEvsCodecMode;

    // Converting bits to bytes.
    nDataSize /= 8;

    // 0111 1111 is no request.
    uint32_t nEVSBW = 0x07;
    uint32_t nEVSBR = 0x0f;

    uint32_t nFrameType = 0;
    uint32_t nDataBitSize = 0;  // bit unit
    uint32_t nTotalSize = 0;    // byte unit

    std::lock_guard<std::mutex> guard(mMutexExit);
    if (nDataSize == 0)
        return;  // don't send 'NO DATA' packet

    eEVSPayloadFormat = mEvsPayloadHeaderMode;
    // compact or header-full format, default is compact formats
    kEvsCodecMode = mEvsCodecMode;

    // primary or amr-wb io mode, default is primary mode
    // primary or amr-wb io mode base on frameSize.
    // kEvsCodecMode = ImsMediaAudioFmt::CheckEVSCodecMode(nDataSize);
    mCurrNumOfFrame++;

    if (eEVSPayloadFormat == kRtpPyaloadHeaderModeEvsCompact)
    {
        memset(mPayload, 0, MAX_AUDIO_PAYLOAD_SIZE);
        mBWHeader.SetBuffer(mPayload, MAX_AUDIO_PAYLOAD_SIZE);
        mBWPayload.SetBuffer(mPayload, MAX_AUDIO_PAYLOAD_SIZE);

        mTimestamp = nTimeStamp;
        // exactly one coded frame without any additional EVS RTP payload header
        if (kEvsCodecMode == kEvsCodecModePrimary)
        {
            // calculate nDataBitSize from nDataSize
            nFrameType = (uint32_t)ImsMediaAudioFmt::ConvertLenToEVSAudioMode(nDataSize);
            nDataBitSize = ImsMediaAudioFmt::ConvertEVSAudioModeToBitLen(
                    (kImsAudioEvsPrimaryMode)nFrameType);

            if (nDataBitSize == 0)
            {
                return;
            }

            // special case, EVS Primary 2.8 kbps frame in Compact format
            if (nFrameType == 0)
            {
                // First data bit d(0) of the EVS Primary 2.8 kbps is always set to '0'
                pData[0] = pData[0] & 0x7f;
            }

            // write speech Frame
            mBWPayload.WriteByteBuffer(pData, nDataBitSize);
            mTotalPayloadSize += nDataSize;

            mBWHeader.AddPadding();
            mBWHeader.Flush();

            mBWPayload.AddPadding();
            mBWPayload.Flush();

            nTotalSize = mBWPayload.GetBufferSize();

            IMLOGD_PACKET7(IM_PACKET_LOG_PH, "[EncodePayloadEvs] result =\
                %02X %02X %02X %02X %02X %02X, len[%d]",
                    mPayload[0], mPayload[1], mPayload[2], mPayload[3], mPayload[4], mPayload[5],
                    nTotalSize);

            if (mTotalPayloadSize > 0)
            {
                SendDataToRearNode(
                        MEDIASUBTYPE_RTPPAYLOAD, mPayload, nTotalSize, mTimestamp, mFirstFrame, 0);
            }

            mCurrNumOfFrame = 0;
            mTotalPayloadSize = 0;
            if (mFirstFrame)
                mFirstFrame = false;
        }
        // one 3-bit CMR field, one coded frame, and zero-padding bits if necessary
        else if (kEvsCodecMode == kEvsCodecModeAmrIo)
        {
            IMLOGE0("[EncodePayloadEvs] COMPACT and AMR_WB_IO");
            // calculate nDataBitSize from nDataSize
            nFrameType = (uint32_t)ImsMediaAudioFmt::ConvertLenToAmrWbMode(nDataSize);
            nDataBitSize = ImsMediaAudioFmt::ConvertAmrWbModeToBitLen(nFrameType);

            // 0: 6.6, 1: 8.85, 2: 12.65, 3: 15.85, 4: 18.25, 5: 23.05, 6: 23.85, 7: none
            // 0111(7) is no request.
            uint32_t nCmr = 0x07;

            // write CMR except SID
            // at EVS AMR WB IO Mode, SID packet does not include cmr field...and no processing
            if (nFrameType != kImsAudioAmrWbModeSID)
            {
#ifdef USE_CMR_TEST
                uint32_t cmr_dummy = 0;
                EVSCMRGeneratorForTest();
#endif
                if (mSendCMR)
                {
                    nCmr = GenerateCMRForEVS(eEVSPayloadFormat);
                }

                mBWHeader.Write(nCmr, 3);
                mBWPayload.Seek(3);

                // append a speech data bit(0) after the last speech data bit
                uint8_t nDataBit0 = 0;
                uint32_t i = 0;
                uint32_t remain = 0;

                nDataBit0 = pData[0] >> 7;
                for (i = 0; i < (nDataSize - 1); i++)
                {
                    pData[i] = pData[i] << 1;
                    pData[i] = pData[i] + (pData[i + 1] >> 7);
                }

                // set the last speech data byte
                remain = nDataBitSize % 8;
                if (remain == 0)
                    remain = 8;
                pData[nDataSize - 1] = pData[nDataSize - 1] << 1;
                nDataBit0 = nDataBit0 << (8 - remain);
                pData[nDataSize - 1] = pData[nDataSize - 1] + nDataBit0;
            }
            else  // kImsAudioAmrWbModeSID case
            {
                // EVS amr io mode's SID is used HF format.
                // set cmr
                if (mSendCMR)
                {
                    nCmr = GenerateCMRForEVS(kRtpPyaloadHeaderModeEvsHeaderFull);
                }
                else
                {
                    nCmr = 0xff;  // no request - 0xff
                }

                mBWHeader.Write(nCmr, 8);
                mBWPayload.Seek(8);

                // set ToC
                // Header Type identification bit(1bit) - always set to 0
                uint32_t toc_h = 0;
                // (1bit - always set to 0 in compact AMR WB IO mode)
                uint32_t toc_f = 0;
                // 1 1 1001 - EVS AMR IO Mode, Q bit set 1, 1001 indicate SID packet
                uint32_t ft = 0x39;

                mBWHeader.Write(toc_h, 1);
                mBWHeader.Write(toc_f, 1);
                mBWHeader.Write(ft, 6);
                mBWPayload.Seek(8);
            }

            // write speech Frame
            mBWPayload.WriteByteBuffer(pData, nDataBitSize);
            mTotalPayloadSize += nDataSize;

            mBWHeader.Flush();

            mBWPayload.AddPadding();
            mBWPayload.Flush();

            nTotalSize = mBWPayload.GetBufferSize();

            IMLOGD_PACKET7(IM_PACKET_LOG_PH,
                    "[EncodePayloadEvs] Result = %02X %02X %02X %02X %02X %02X, len[%d]",
                    mPayload[0], mPayload[1], mPayload[2], mPayload[3], mPayload[4], mPayload[5],
                    nTotalSize);

            if (mTotalPayloadSize > 0)
            {
                SendDataToRearNode(
                        MEDIASUBTYPE_RTPPAYLOAD, mPayload, nTotalSize, mTimestamp, mFirstFrame, 0);
            }

            mCurrNumOfFrame = 0;
            mTotalPayloadSize = 0;
            if (mFirstFrame)
                mFirstFrame = false;
        }
        else
        {
            IMLOGE0("[EncodePayloadEvs] Invalid codec mode");
            return;
        }
    }
    else if (eEVSPayloadFormat == kRtpPyaloadHeaderModeEvsHeaderFull)
    {
        uint8_t nCmr = 0;

        uint32_t m_nCMR = 0;  // CMR value
        m_nCMR = mSendCMR;

        uint32_t cmr_h, cmr_t, cmr_d = 0;  // CMR byte
        // ToC byte
        uint32_t toc_h, toc_f = 0;
        uint32_t toc_ft_m, toc_ft_q, toc_ft_b = 0;  // ToC frame type index

        memset(mPayload, 0, MAX_AUDIO_PAYLOAD_SIZE);
        mBWHeader.SetBuffer(mPayload, MAX_AUDIO_PAYLOAD_SIZE);
        mBWPayload.SetBuffer(mPayload, MAX_AUDIO_PAYLOAD_SIZE);

        if (kEvsCodecMode == kEvsCodecModePrimary)
        {
            IMLOGE0("[EncodePayloadEvs] HF and PRI");
            if (nFrameType == kImsAudioEvsPrimaryModeSID || m_nCMR == 1)  // CMR value
            {
                // Header Type identification bit(1bit) - always set to 1
                cmr_h = 1;
                // Type of Request(3bits) - NB(000), IO(001), FB(100), WB(101), SWB(110)
                cmr_t = nEVSBW;
                // codec mode request(4bits)
                cmr_d = nEVSBR;
            }

            // set ToC byte
            toc_h = 0;  // Header Type identification bit(1bit) - always set to 0
            toc_f = (mCurrNumOfFrame == mMaxNumOfFrame) ? 0 : 1;  // (1bit)
            toc_ft_m = 0;  // EVS mode(1bit), Primary mode is 0
            toc_ft_q = 0;  // Q bit(1bit) - zero for kEvsCodecModePrimary
            toc_ft_b =
                    ImsMediaAudioFmt::ConvertLenToEVSAudioMode(nDataSize);  // EVS bit rate(4bits)
            nDataBitSize = ImsMediaAudioFmt::ConvertEVSAudioModeToBitLen(
                    (kImsAudioEvsPrimaryMode)toc_ft_b);

            // write CMR and seek the position of the first paylaod
            if (mCurrNumOfFrame == 1)
            {
                // set CMR byte - it's optional field...
                if (mSendCMR)
                {
                    nCmr = GenerateCMRForEVS(eEVSPayloadFormat);
                    mBWHeader.Write(nCmr, 8);
                    mBWPayload.Seek(8);
                }
                else if (nFrameType == kImsAudioEvsPrimaryModeSID || m_nCMR == 1)
                {
                    // check writing CMR or not
                    // write CMR byte
                    mBWHeader.Write(cmr_h, 1);
                    mBWHeader.Write(cmr_t, 3);
                    mBWHeader.Write(cmr_d, 4);

                    mBWPayload.Seek(8);
                }

                // ToC field.
                mBWPayload.Seek(mMaxNumOfFrame * 8);  // jump ToC bytes
                mTimestamp = nTimeStamp;              // set timestamp as the first frame
            }

            // write ToC
            mBWHeader.Write(toc_h, 1);
            mBWHeader.Write(toc_f, 1);
            mBWHeader.Write(toc_ft_m, 1);
            mBWHeader.Write(toc_ft_q, 1);
            mBWHeader.Write(toc_ft_b, 4);

            // write Speech Frame
            mBWPayload.WriteByteBuffer(pData, nDataBitSize);
            mBWPayload.AddPadding();

            mTotalPayloadSize += nDataSize;

            if (mCurrNumOfFrame == mMaxNumOfFrame)
            {
                // mBWHeader.AddPadding();
                mBWHeader.Flush();

                mBWPayload.AddPadding();
                mBWPayload.Flush();

                nTotalSize = mBWPayload.GetBufferSize();

                IMLOGD_PACKET7(IM_PACKET_LOG_PH,
                        "[EncodePayloadEvs] Result = %02X %02X %02X %02X %02X %02X, len[%d]",
                        mPayload[0], mPayload[1], mPayload[2], mPayload[3], mPayload[4],
                        mPayload[5], nTotalSize);

                if (mTotalPayloadSize > 0)
                {
                    nTotalSize = CheckPaddingNecessity(nTotalSize);

                    SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD, mPayload, nTotalSize, mTimestamp,
                            mFirstFrame, 0);
                }

                mCurrNumOfFrame = 0;
                mTotalPayloadSize = 0;
                if (mFirstFrame)
                    mFirstFrame = false;
            }
        }
        else if (kEvsCodecMode == kEvsCodecModeAmrIo)
        {
            IMLOGE0("[EncodePayloadEvs] HF and AMR_WB_IO");
            // set CMR byte
            // at EVS AMR WB IO Mode, CMR field shall include.
#ifdef USE_CMR_TEST
            EVSCMRGeneratorForTest();
#endif
            uint32_t cmr_h, cmr_t, cmr_d;
            // Header Type identification bit(1bit) - always set to 1
            cmr_h = 1;
            /* Type of Request(3bits) - NB(000), IO(001), WB(010), SWB(011), FB(100), WB 13.2
             * channel-aware(101), SWB 13.2 channel-aware(110), reserved(111) */
            cmr_t = nEVSBW;
            // codec mode request(4bits) 1111 is no request.
            cmr_d = nEVSBR;

            // set ToC byte
            // Header Type identification bit(1bit) - always set to 0
            toc_h = 0;
            // (1bit)
            toc_f = (mCurrNumOfFrame == mMaxNumOfFrame) ? 0 : 1;
            // EVS mode(1bit), AMR-WB IO mode is 1
            toc_ft_m = 1;
            // Q bit(1bit) - 1 for AMR_WB_IO
            // for ORG EVS to avoid the issue -#EURAVOLTE-567
            toc_ft_q = 1;
            // EVS AMR WB IO bit rate(4bits)
            toc_ft_b = (uint32_t)ImsMediaAudioFmt::ConvertLenToAmrWbMode(nDataSize);
            nDataBitSize = ImsMediaAudioFmt::ConvertAmrWbModeToBitLen(toc_ft_b);

            // write CMR and seek the position of the first paylaod
            if (mCurrNumOfFrame == 1)
            {
                // write CMR byte
                mBWHeader.Write(cmr_h, 1);
                mBWHeader.Write(cmr_t, 3);
                mBWHeader.Write(cmr_d, 4);

                // seek the position of the first paylaod
                // add speech data after CMR and ToC
                mBWPayload.Seek(8 + mMaxNumOfFrame * 8);

                mTimestamp = nTimeStamp;  // set timestamp as the first frame
            }

            // write ToC
            mBWHeader.Write(toc_h, 1);
            mBWHeader.Write(toc_f, 1);
            mBWHeader.Write(toc_ft_m, 1);
            mBWHeader.Write(toc_ft_q, 1);
            mBWHeader.Write(toc_ft_b, 4);

            // write Speech Frame
            mBWPayload.WriteByteBuffer(pData, nDataBitSize);
            mBWPayload.AddPadding();

            mTotalPayloadSize += nDataSize;

            if (mCurrNumOfFrame == mMaxNumOfFrame)
            {
                // mBWHeader.AddPadding();
                mBWHeader.Flush();

                mBWPayload.AddPadding();
                mBWPayload.Flush();

                nTotalSize = mBWPayload.GetBufferSize();

                IMLOGD_PACKET7(IM_PACKET_LOG_PH,
                        "[EncodePayloadEvs] result = %02X %02X %02X %02X %02X %02X, len[%d]",
                        mPayload[0], mPayload[1], mPayload[2], mPayload[3], mPayload[4],
                        mPayload[5], nTotalSize);

                if (mTotalPayloadSize > 0)
                {
                    nTotalSize = CheckPaddingNecessity(nTotalSize);

                    SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD, mPayload, nTotalSize, mTimestamp,
                            mFirstFrame, 0);
                }

                mCurrNumOfFrame = 0;
                mTotalPayloadSize = 0;
                if (mFirstFrame)
                    mFirstFrame = false;
            }
        }
        else
        {
            IMLOGE0("[EncodePayloadEvs] invalid codec mode");
            return;
        }

    }  // end of if(eEVSPayloadFormat == kRtpPyaloadHeaderModeEvsHeaderFull)
    else
    {
        IMLOGE0("[EncodePayloadEvs] invalid payload format");
        return;
    }

    return;
}

void AudioRtpPayloadEncoderNode::EVSCMRGeneratorForTest()
{
    // cmr test code start
    // generate cmr
#ifdef USE_CMR_TEST

    static int sCount = 0;
    static int bUpdate = 0;
    static int nNextMode = 0;
    static int nInc = -1;
    static int sCodeType = 0;
    static int sCodeDefine = 0;
    static int sNextType = 1;

    sCount++;

    // cmr test code end
    if (ePHMode == kRtpPyaloadHeaderModeEvsCompact)
    {
        if (eCodecMode == kEvsCodecModeAmrIo)
        {
            if ((sCount & 0x7f) == 0)
            {
                bUpdate = 1;
            }

            if (bUpdate == 1 && DataSize > 0)
            {
                sCodeDefine = nNextMode;

                nNextMode += nInc;

                if (nNextMode < 0)
                {
                    nNextMode = 1;
                    nInc = 1;
                }
                else if (nNextMode > 7)
                {
                    nNextMode = 6;
                    nInc = -1;
                }

                bUpdate = 0;
            }
        }
    }
    else  // headerfull format.
    {
        int32_t nMaxCodeDefine = 0;
        int32_t nMinCodeDefine = 0;

        switch (sCodeType)
        {
            case 0:
                nMinCodeDefine = 0;
                nMaxCodeDefine = 6;
                break;  // NB
            case 1:
                nMinCodeDefine = 0;
                nMaxCodeDefine = 8;
                break;  // AMR IO Mode
            case 2:
                nMinCodeDefine = 0;
                nMaxCodeDefine = 6;
                break;  // WB
            case 3:
                nMinCodeDefine = 3;
                nMaxCodeDefine = 6;
                break;  // SWB
            case 5:
                nMinCodeDefine = 0;
                nMaxCodeDefine = 8;
                break;  // WB ChA
            case 6:
                nMinCodeDefine = 0;
                nMaxCodeDefine = 8;
                break;  // SWB ChA
            case 7:
                nMinCodeDefine = 15;
                nMaxCodeDefine = 15;
                break;  // no request
            default:
                return;
        }

        if ((sCount & 0x7f) == 0)
        {
            bUpdate = 1;
        }

        if (bUpdate == 1 && DataSize > 0)
        {
            sCodeDefine = nNextMode;
            nNextMode += 1;

            if ((nNextMode - 1) > nMaxCodeDefine)
            {
                sCodeType = sNextType;
                sNextType += 1;

                if (sNextType == 4)  // fb case - skipped
                    sNextType += 1;

                if (sCodeType == 3)
                {
                    sCodeDefine = 3;
                    nNextMode = 4;
                }
                else if (sCodeType == 7)
                {
                    sCodeDefine = 15;
                    nNextMode = 16;
                }
                else
                {
                    sCodeDefine = 0;
                    nNextMode = 1;
                }

                if (sNextType > 7)  // rounding case.(no request) goto NB CMR case.
                {
                    sNextType = 0;
                    nNextMode = 0;
                }
            }

            bUpdate = 0;
        }
    }

    *CodeType = sCodeType;
    *CodeDefine = sCodeDefine;
#endif
}

uint8_t AudioRtpPayloadEncoderNode::GenerateCMRForEVS(kRtpPyaloadHeaderMode eEVSPayloadFormat)
{
    uint8_t nCmrT = 0;
    uint8_t nCmrD = 0;
    uint8_t nCmr = 0;

    uint32_t nOriginBR = ImsMediaAudioFmt::FindMaxEVSBitrate(mEvsBitRate, mEvsCodecMode);
    uint32_t nOriginBW = ImsMediaAudioFmt::FindMaxEVSBandwidth(mEvsBandwidth);

    if (eEVSPayloadFormat == kRtpPyaloadHeaderModeEvsCompact)
    {
        // 0: 6.6, 1: 8.85, 2: 12.65, 3: 15.85, 4: 18.25, 5: 23.05, 6: 23.85, 7: none
        nCmr = nOriginBR;
    }
    else
    {
        if (nOriginBR == kEvsPrimaryModeBitrate01320)  // channel aware mode
        {
            nCmrD = mEvsOffset;

            switch (nOriginBW)
            {
                case kEvsBandwidthWB:
                    nCmrT = kEvsCmrCodeTypeWbCha;
                    break;
                case kEvsBandwidthSWB:
                    nCmrT = kEvsCmrCodeTypeSwbCha;
                    break;
                case kEvsBandwidthFB:
                case kEvsBandwidthNB:
                default:
                    IMLOGE0("[GenerateCMRForEVS] error");
                    break;
            }
        }
        else
        {
            if (mEvsCodecMode == kEvsCodecModeAmrIo)
            {
                nCmrT = kEvsCmrCodeTypeAmrIO;
                nCmrD = nOriginBR;
            }
            else
            {
                nCmrD = nOriginBR - 9;  // start : kEvsPrimaryModeBitrate00590 = 9 to 0

                switch (nOriginBW)
                {
                    case kEvsBandwidthWB:
                        nCmrT = kEvsCmrCodeTypeWb;
                        break;
                    case kEvsBandwidthSWB:
                        nCmrT = kEvsCmrCodeTypeSwb;
                        break;
                    case kEvsBandwidthFB:
                        nCmrT = kEvsCmrCodeTypeFb;
                        break;
                    case kEvsBandwidthNB:
                    default:
                        nCmrT = kEvsCmrCodeTypeNb;
                        break;
                }
            }
        }

        nCmr = 0x8 | nCmrT;
        nCmr = (nCmr << 4) | nCmrD;
    }

    IMLOGD_PACKET3(IM_PACKET_LOG_PH, "[GenerateCMRForEVS] nCmrD[%u], nCmrT[%d], nCMR[%d]", nCmrT,
            nCmrD, nCmr);

    return nCmr;
}

uint32_t AudioRtpPayloadEncoderNode::CheckPaddingNecessity(uint32_t nTotalSize)
{
    kEvsCodecMode kEvsCodecMode;
    uint32_t nEVSCompactId;
    uint32_t nSize = nTotalSize;

    // check EVS compact size
    while (nSize != 0 &&
            ImsMediaAudioFmt::ConvertEVSPayloadMode(nSize, &kEvsCodecMode, &nEVSCompactId) ==
                    kRtpPyaloadHeaderModeEvsCompact)
    {
        mPayload[nSize] = 0;
        nSize++;

        IMLOGD1("[CheckPaddingNecessity] Add Padding - size[%d]", nSize);
    }

    return nSize;
}
