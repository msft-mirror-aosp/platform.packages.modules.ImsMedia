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

#include <AudioRtpPayloadDecoderNode.h>
#include <ImsMediaAudioFmt.h>
#include <ImsMediaTrace.h>
#include <AudioConfig.h>

AudioRtpPayloadDecoderNode::AudioRtpPayloadDecoderNode()
{
    mPrevCMR = 15;
}

AudioRtpPayloadDecoderNode::~AudioRtpPayloadDecoderNode()
{
    std::lock_guard<std::mutex> guard(mMutexExit);
}

BaseNode* AudioRtpPayloadDecoderNode::GetInstance()
{
    return new AudioRtpPayloadDecoderNode();
}

void AudioRtpPayloadDecoderNode::ReleaseInstance(BaseNode* pNode)
{
    delete (AudioRtpPayloadDecoderNode*)pNode;
}

BaseNodeID AudioRtpPayloadDecoderNode::GetNodeID()
{
    return BaseNodeID::NODEID_RTPPAYLOAD_DECODER_AUDIO;
}

ImsMediaResult AudioRtpPayloadDecoderNode::Start()
{
    IMLOGD0("[Start]");
    std::lock_guard<std::mutex> guard(mMutexExit);
    mPrevCMR = 15;
    mNodeState = kNodeStateRunning;
    return RESULT_SUCCESS;
}

void AudioRtpPayloadDecoderNode::Stop()
{
    IMLOGD0("[Stop]");
    std::lock_guard<std::mutex> guard(mMutexExit);
    mNodeState = kNodeStateStopped;
}

bool AudioRtpPayloadDecoderNode::IsRunTime()
{
    return true;
}

bool AudioRtpPayloadDecoderNode::IsSourceNode()
{
    return false;
}

void AudioRtpPayloadDecoderNode::SetConfig(void* config)
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
            setEvsBandwidth((eEVSBandwidth)pConfig->getEvsParams().getEvsBandwidth());
            setEvsPayloadHeaderMode(
                    ((eRTPPyaloadHeaderMode)pConfig->getEvsParams().getUseHeaderFullOnlyOnRx()));
            setEvsCodecMode((pConfig->getEvsParams().getEvsMode()));
            setEvsMode(((eEVSBitrate)pConfig->getEvsParams().getEvsMode()));
            mEvsModetoBitRate =
                    ImsMediaAudioFmt::getEVSModeToBitRate(pConfig->getEvsParams().getEvsMode());
            setEvsChannelAwareOffset(pConfig->getEvsParams().getChannelAwareMode());
        }
    }
}

bool AudioRtpPayloadDecoderNode::IsSameConfig(void* config)
{
    if (config == NULL)
        return true;
    AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);

    if (mCodecType == ImsMediaAudioFmt::ConvertCodecType(pConfig->getCodecType()))
    {
        if (mCodecType == kAudioCodecAmr || mCodecType == kAudioCodecAmrWb)
        {
            if (mOctetAligned == pConfig->getAmrParams().getOctetAligned())
            {
                return true;
            }
        }
    }

    return false;
}

void AudioRtpPayloadDecoderNode::setEvsBandwidth(eEVSBandwidth bandwidth)
{
    mEvsBandwidth = bandwidth;
}

void AudioRtpPayloadDecoderNode::setEvsPayloadHeaderMode(eRTPPyaloadHeaderMode evsPayloadHeaderMode)
{
    mEvsPayloadHeaderMode = evsPayloadHeaderMode;
}

void AudioRtpPayloadDecoderNode::setEvsCodecMode(int32_t evsMode)
{
    if (evsMode > 8 && evsMode <= 20)
    {
        mEvsCodecMode = EVS_PRIMARY;
    }
    else if (evsMode >= 0 && evsMode <= 8)
    {
        mEvsCodecMode = EVS_AMR_WB_IO;
    }
    else
    {
        mEvsCodecMode = EVS_CODEC_MODE_ENUM_MAX;
    }
}

void AudioRtpPayloadDecoderNode::setEvsMode(eEVSBitrate evsMode)
{
    mEvsMode = evsMode;
}

void AudioRtpPayloadDecoderNode::setEvsChannelAwareOffset(int32_t EvsChAOffset)
{
    mEvsOffset = EvsChAOffset;
}

void AudioRtpPayloadDecoderNode::OnDataFromFrontNode(ImsMediaSubType subtype, uint8_t* pData,
        uint32_t nDataSize, uint32_t nTimestamp, bool bMark, uint32_t nSeqNum,
        ImsMediaSubType nDataType)
{
    if (subtype == MEDIASUBTYPE_REFRESHED || pData == NULL || nDataSize == 0)
    {
        return;
    }

    switch (mCodecType)
    {
        case kAudioCodecAmr:
        case kAudioCodecAmrWb:
            Decode_PH_AMR(pData, nDataSize, nTimestamp, bMark, nSeqNum);
            break;
        case kAudioCodecPcmu:
        case kAudioCodecPcma:
            SendDataToRearNode(
                    MEDIASUBTYPE_RTPPAYLOAD, pData, nDataSize, nTimestamp, bMark, nSeqNum);
            break;
        case kAudioCodecEvs:
            Decode_PH_EVS(pData, nDataSize, nTimestamp, bMark, nSeqNum);
            break;
        default:
            IMLOGE1("[OnDataFromFrontNode] invalid codec type[%d]", mCodecType);
            SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD, pData, nDataSize, nTimestamp, bMark,
                    nSeqNum, nDataType);
            break;
    }
}

void AudioRtpPayloadDecoderNode::Decode_PH_AMR(
        uint8_t* pData, uint32_t nDataSize, uint32_t nTimestamp, bool bMark, uint32_t nSeqNum)
{
    (void)bMark;
    uint32_t timestamp = nTimestamp;
    static std::list<uint32_t> listFrameType;  // defined as static variable for memory management
    uint32_t eRate;
    uint32_t f;
    uint32_t cmr;
    uint32_t QbitPos;  // Q_Speech_Sid_Bad

    std::lock_guard<std::mutex> guard(mMutexExit);

    IMLOGD_PACKET4(IM_PACKET_LOG_PH,
            "[Decode_PH_AMR] GetCodectype[%d], octetAligned[%d], nSeqNum[%d], TS[%u]", mCodecType,
            mOctetAligned, nSeqNum, timestamp);

    mBitReader.SetBuffer(pData, nDataSize);
    // read cmr
    cmr = mBitReader.Read(4);

    if (mOctetAligned == true)
    {
        mBitReader.Read(4);
    }

    if (cmr != 15 && cmr != mPrevCMR)
    {
        if ((mCodecType == kAudioCodecAmr && cmr <= 7) ||
                (mCodecType == kAudioCodecAmrWb && cmr <= 8))
        {
            IMLOGD2("[Decode_PH_AMR] CMR %d->%d", mPrevCMR, cmr);
            // send internal event to operate cmr operation in encoder side
            mPrevCMR = cmr;
        }
        else
        {
            IMLOGE1("[Decode_PH_AMR] invalid cmr value %d", cmr);
        }
    }
    else if (cmr == 15)
    {
        // send internal event to operate cmr operation in encoder side
    }
    else
    {
        IMLOGE0("[Decode_PH_AMR] Can't find Highest Negotiated Mode in negotiated mode-set");
    }

    // get num of frame
    do
    {
        f = mBitReader.Read(1);        // f(1)
        eRate = mBitReader.Read(4);    // ft(4)
        QbitPos = mBitReader.Read(1);  // q(2)
        IMLOGD_PACKET2(IM_PACKET_LOG_PH, "[Decode_PH_AMR] f[%d], ft[%d]", f, eRate);
        listFrameType.push_back(eRate);
        if (mOctetAligned == true)
        {
            mBitReader.Read(2);  // padding
        }
    } while (f == 1);

    IMLOGD3("[Decode_PH_AMR] Q_Speech_SID <Q_Speech_SID> f[%d] eRate[%d] QbitPos[%d]", f, eRate,
            QbitPos);  // Q_Speech_SID

    // read speech frames
    while (listFrameType.size() > 0)
    {
        uint32_t nDataBitSize;
        uint32_t mode = listFrameType.front();
        if (mCodecType == kAudioCodecAmr)
        {
            nDataBitSize = ImsMediaAudioFmt::ConvertAmrModeToBitLen(mode);
        }
        else
        {
            nDataBitSize = ImsMediaAudioFmt::ConvertAmrWbModeToBitLen(mode);
        }

        listFrameType.pop_front();
        mBitWriter.SetBuffer(mPayload, MAX_AUDIO_PAYLOAD_SIZE);
        // set payload header
        mBitWriter.Write(f, 1);
        mBitWriter.Write(eRate, 4);
        mBitWriter.Write(QbitPos, 1);
        mBitWriter.Write(0, 2);
        mBitReader.ReadByteBuffer(mPayload + 1, nDataBitSize);
        // add payload header to payload size
        uint32_t nBufferSize = ((nDataBitSize + 7) >> 3) + 1;
        IMLOGD_PACKET6(IM_PACKET_LOG_PH,
                "[Decode_PH_AMR] result = %02X %02X %02X %02X, len[%d], eRate[%d]", mPayload[0],
                mPayload[1], mPayload[2], mPayload[3], nBufferSize, eRate);
        // send remaining packet number in bundle as bMark value
        SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD, mPayload, nBufferSize, timestamp,
                listFrameType.size(), nSeqNum);

        timestamp += 20;
    }
}

// [EVS] Decode EVS payload header
void AudioRtpPayloadDecoderNode::Decode_PH_EVS(
        uint8_t* pData, uint32_t nDataSize, uint32_t nTimeStamp, bool bMark, uint32_t nSeqNum)
{
    eRTPPyaloadHeaderMode eEVSPHFormat = RTPPAYLOADHEADER_MODE_EVS_COMPACT;
    eRTPPyaloadHeaderMode eEVSReceivedPHFormat = RTPPAYLOADHEADER_MODE_EVS_COMPACT;
    eEVSCodecMode eEVSCodecMode = EVS_PRIMARY;

    // uint32_t nEVSBW = 0;
    // uint32_t nEVSBR = 0;
    uint32_t nEVSCompactId = 0;

    uint32_t nFrameType = 0;
    uint32_t nDataBitSize = 0;

    uint32_t timestamp = nTimeStamp;

    uint32_t cmr = 15;

    // CMR byte
    uint32_t h = 0;      // 1bit, Header Type identification bit(always set to 1)
    uint32_t cmr_t = 0;  // 3bits, Type of Request
    uint32_t cmr_d = 0;  // 4bits, Codec Mode Request

    // ToC byte
    static std::list<uint32_t> listFrameType;  // defined as static variable for memory management
    uint32_t toc_f = 0;                        // 1bit, follow another speech frame
    uint32_t toc_ft_m = 0;                     // 1bit, EVS mode
    uint32_t toc_ft_q = 0;                     // 1bit, AMR-WB Q bit
    uint32_t toc_ft_b = 0;                     // 4bits, EVS bit rate

    std::lock_guard<std::mutex> guard(mMutexExit);

    eEVSPHFormat = mEvsPayloadHeaderMode;
    mBitReader.SetBuffer(pData, nDataSize);

    // check RTP payload format
    eEVSReceivedPHFormat =
            ImsMediaAudioFmt::Check_EVS_Payload(nDataSize, &eEVSCodecMode, &nEVSCompactId);

    if ((eEVSCodecMode == EVS_PRIMARY) && (nEVSCompactId == 0))  // special case
    {
        // first bit of the EVS Primary 2.8kbps in compact format is always set to '0'
        if ((pData[0] >> 7) == 0)
        {
            // EVS Primary 2.8 kbps frame in Compact format
            eEVSReceivedPHFormat = RTPPAYLOADHEADER_MODE_EVS_COMPACT;
        }
        else
        {
            // EVS AMR-WB IO SID frame in Header-Full format with one CMR byte
            eEVSReceivedPHFormat = RTPPAYLOADHEADER_MODE_EVS_HEADER_FULL;
        }
    }

    /*  remove payload format check
    if(eEVSPHFormat != eEVSReceivedPHFormat)
    {
        IMLOGD_PACKET2(MMPF_PACKET_LOG_PH, "[AudioRtpPayloadDecoderNode::Decode_PH_EVS]
            receive different payload format[%d/%d]", eEVSPHFormat, eEVSReceivedPHFormat);

        // exceptional case handling
        if ( (eEVSPHFormat == RTPPAYLOADHEADER_MODE_EVS_HEADER_FULL)
                && (( nDataSize == 20 ) || (nDataSize == 60)))
        {
            // Data Size 20 : headerfull format 7.2k size == compact format 8.0 size
            // Data Size 60 : headerfull format 23.05k size == compact format 23.85k size
            eEVSReceivedPHFormat = eEVSPHFormat;
        }

    }
    */

    if (eEVSReceivedPHFormat == RTPPAYLOADHEADER_MODE_EVS_COMPACT)
    {
        if (eEVSCodecMode == EVS_PRIMARY)
        {
            // calculate nDataBitSize from nDataSize
            nFrameType = (uint32_t)ImsMediaAudioFmt::ConvertLenToEVSAudioRate(nDataSize);
            // TODO: remove IMSVOC_EVS_PRIMARY_ENTYPE
            nDataBitSize = ImsMediaAudioFmt::ConvertEVSAudioRateToBitLen(
                    (IMSVOC_EVS_PRIMARY_ENTYPE)nFrameType);

            mBitReader.ReadByteBuffer(mPayload, nDataBitSize);

            IMLOGD6("[Decode_PH_EVS] Result =%02X %02X %02X %02X, len=%d,nFrameType=%d",
                    mPayload[0], mPayload[1], mPayload[2], mPayload[3], nDataSize, nFrameType);

            SendDataToRearNode(
                    MEDIASUBTYPE_RTPPAYLOAD, mPayload, nDataSize, timestamp, bMark, nSeqNum);
        }
        else if (eEVSCodecMode == EVS_AMR_WB_IO)
        {
            // calculate nDataBitSize from nDataSize
            nFrameType = (uint32_t)ImsMediaAudioFmt::ConvertLenToAmrWbMode(nDataSize);

            nDataBitSize = ImsMediaAudioFmt::ConvertAmrWbModeToBitLen(nFrameType);

            // read cmr except SID
            // at EVS AMR WB IO Mode, SID packet does not include cmr field...
            if (nFrameType != IMSVOC_AMRWB_MODE_SID)
            {
                cmr = mBitReader.Read(3);

                // process cmr
                if (cmr != 7 && cmr != mPrevCMR)
                {
                    // Process CMR
                    ProcessCMRForEVS(RTPPAYLOADHEADER_MODE_EVS_COMPACT, 7, cmr);
                    // Save Prev CMR value for checking
                    mPrevCMR = cmr;
                }
                // cmr == 7 && cmr != mPrevCMR -> returned original codec mode.
                else if (cmr != mPrevCMR)
                {
                    // Process CMR
                    /*
                    uint32_t nOriginBR = FindMaxEVSBitrate( m_Property.GetEVSBitrate(),
                    MMPF_EVS_AMR_WB_IO); ProcessCMRForEVS(RTPPAYLOADHEADER_MODE_EVS_HEADER_FULL ,
                    (uint32_t)EVS_CMR_CODETYPE_AMRIO, nOriginBR);
                    */
                    uint32_t nOriginBW = ImsMediaAudioFmt::FindMaxEVSBandwidth(mEvsBandwidth);
                    uint32_t nOriginBR =
                            ImsMediaAudioFmt::FindMaxEVSBitrate(mEvsModetoBitRate, mEvsCodecMode);
                    IMLOGD3("[Decode_PH_EVS] Returned Codec Mode Request CodecMode[%d], "
                            "bnadwidth[%d], bitrate[%d]",
                            mEvsCodecMode, nOriginBW, nOriginBR);

                    uint32_t nCodeType = (uint32_t)EVS_CMR_CODETYPE_NO_REQ;
                    uint32_t nCodeDefine = (uint32_t)EVS_CMR_CODEDEFINE_NO_REQ;

                    if (mEvsCodecMode == EVS_PRIMARY)
                    {
                        // to convert EVS CMR type
                        nOriginBR -= (uint32_t)EVS_PRIMARY_MODE_00590;
                    }

                    // check AMR IO and ChA
                    if (mEvsCodecMode == EVS_AMR_WB_IO)
                    {
                        nCodeType = (uint32_t)EVS_CMR_CODETYPE_AMRIO;
                        nCodeDefine = nOriginBR;
                    }
                    else if ((EVS_PRIMARY_MODE_01320 ==
                                     (nOriginBR + (uint32_t)EVS_PRIMARY_MODE_00590)) &&
                            ((EvsChAOffset > 0) && (EvsChAOffset < 8)))  // ChA case.
                    {
                        if (nOriginBW == (uint32_t)EVS_VOC_BANDWIDTH_SWB)
                        {
                            nCodeType = (uint32_t)EVS_CMR_CODETYPE_SWB_CHA;
                        }
                        else
                        {
                            nCodeType = (uint32_t)EVS_CMR_CODETYPE_WB_CHA;
                        }

                        switch (EvsChAOffset)
                        {
                            case 2:
                                nCodeDefine = (uint32_t)EVS_CMR_CODEDEFINE_CHA_OFFSET_H2;
                                break;
                            case 3:
                                nCodeDefine = (uint32_t)EVS_CMR_CODEDEFINE_CHA_OFFSET_H3;
                                break;
                            case 5:
                                nCodeDefine = (uint32_t)EVS_CMR_CODEDEFINE_CHA_OFFSET_H5;
                                break;
                            case 7:
                                nCodeDefine = (uint32_t)EVS_CMR_CODEDEFINE_CHA_OFFSET_H7;
                                break;
                            default:
                                IMLOGD3("[Decode_PH_EVS] no selected chmode offset[%d], "
                                        "originBW[%d], originBR[%d]",
                                        EvsChAOffset, nOriginBW, nOriginBR);
                                nCodeType = (uint32_t)EVS_CMR_CODETYPE_NO_REQ;
                                nCodeDefine = (uint32_t)EVS_CMR_CODEDEFINE_NO_REQ;
                                break;
                        }
                    }
                    else  // primary case
                    {
                        nCodeDefine = nOriginBR;

                        switch (nOriginBW)
                        {
                            case 0:
                                nCodeType = (uint32_t)EVS_CMR_CODETYPE_NB;
                                break;
                            case 1:
                                nCodeType = (uint32_t)EVS_CMR_CODETYPE_WB;
                                break;
                            case 2:
                                nCodeType = (uint32_t)EVS_CMR_CODETYPE_SWB;
                                break;
                            case 3:
                                nCodeType = (uint32_t)EVS_CMR_CODETYPE_FB;
                                break;
                            default:
                                IMLOGD2("[Decode_PH_EVS] no CodeType - primary mode, originBW[%d], "
                                        "originBR[%d]",
                                        nOriginBW, nOriginBR);
                                nCodeType = (uint32_t)EVS_CMR_CODETYPE_NO_REQ;
                                nCodeDefine = (uint32_t)EVS_CMR_CODEDEFINE_NO_REQ;
                                break;
                        }
                    }

                    // process CMR
                    ProcessCMRForEVS(RTPPAYLOADHEADER_MODE_EVS_HEADER_FULL, nCodeType, nCodeDefine);

                    // Save Prev CMR value for checking
                    mPrevCMR = cmr;
                }
            }
            mBitReader.ReadByteBuffer(mPayload, nDataBitSize);

            // last data bit is speech first bit..
            if (nFrameType != IMSVOC_AMRWB_MODE_SID)
            {
                uint8_t nLastBit0 = 0;
                uint32_t i = 0;
                uint32_t remain = 0;

                remain = nDataBitSize % 8;
                if (remain == 0)
                {
                    nLastBit0 = mPayload[nDataSize - 1] & 0x01;
                }
                else
                {
                    nLastBit0 = (mPayload[nDataSize - 1] << (remain - 1)) >> 7;
                    mPayload[nDataSize - 1] = (mPayload[nDataSize - 1] >> (9 - remain))
                            << (9 - remain);
                }

                for (i = (nDataSize - 1); i > 0; i--)
                {
                    mPayload[i] = mPayload[i] >> 1;
                    mPayload[i] = mPayload[i] + ((mPayload[i - 1] & 0x01) << 7);
                }
                mPayload[0] = (mPayload[0] >> 1);
                mPayload[0] = mPayload[0] + (nLastBit0 << 7);
            }

            IMLOGD6("[Decode_PH_EVS] result = %02X %02X %02X %02X, len=%d, nFrameType=%d",
                    mPayload[0], mPayload[1], mPayload[2], mPayload[3], nDataSize, nFrameType);

            SendDataToRearNode(
                    MEDIASUBTYPE_RTPPAYLOAD, mPayload, nDataSize, timestamp, bMark, nSeqNum);
        }
        else
        {
            IMLOGD0("[Decode_PH_EVS] Invalid codec mode");
            return;
        }
    }
    else if (eEVSReceivedPHFormat == RTPPAYLOADHEADER_MODE_EVS_HEADER_FULL)
    {
        do
        {
            h = mBitReader.Read(1);
            toc_f = 1;

            if (h == 1)  // CMR byte
            {
                cmr_t = mBitReader.Read(
                        3);  // 000: NB, 001: IO, 010: WB, 011: SWB, 100: FB, 101:WB(13.2 CA mode),
                             // 110:SWB(13.2 CA mode), 111: Reserved
                cmr_d = mBitReader.Read(4);
                uint32_t nChecnCMR = 0;
                nChecnCMR = (cmr_t << 4) + cmr_d;

                // temp log
                // IMLOGD1("[AudioRtpPayloadDecoderNode::Decode_PH_EVS] nChecnCMR[0x%x]",
                // nChecnCMR);

                // process cmr
                if (nChecnCMR != 127 && nChecnCMR != mPrevCMR)  // 127 is 111 1111
                {
                    IMLOGD0("[Decode_PH_EVS] Process CMR");
                    // Process CMR
                    ProcessCMRForEVS(RTPPAYLOADHEADER_MODE_EVS_HEADER_FULL, cmr_t, cmr_d);

                    // Save Prev CMR value for checking
                    mPrevCMR = nChecnCMR;
                }
                else if (nChecnCMR !=
                        mPrevCMR)  // cmr == 127 && cmr != mPrevCMR -> returned original codec mode.
                {
                    uint32_t nOriginBW = ImsMediaAudioFmt::FindMaxEVSBandwidth(mEvsBandwidth);
                    uint32_t nOriginBR =
                            ImsMediaAudioFmt::FindMaxEVSBitrate(mEvsModetoBitRate, mEvsCodecMode);
                    IMLOGD3("[Decode_PH_EVS] Returned Codec Mode Request CodecMode[%d], "
                            "bnadwidth[%d], bitrate[%d]",
                            mEvsMode, nOriginBW, nOriginBR);

                    uint32_t nCodeType = (uint32_t)EVS_CMR_CODETYPE_NO_REQ;
                    uint32_t nCodeDefine = (uint32_t)EVS_CMR_CODEDEFINE_NO_REQ;

                    if (mEvsCodecMode == EVS_PRIMARY)
                    {
                        // to convert EVS CMR type
                        nOriginBR -= (uint32_t)EVS_PRIMARY_MODE_00590;
                    }

                    // check AMR IO and ChA
                    if (mEvsCodecMode == EVS_AMR_WB_IO)
                    {
                        nCodeType = (uint32_t)EVS_CMR_CODETYPE_AMRIO;
                        nCodeDefine = nOriginBR;
                    }
                    else if ((EVS_PRIMARY_MODE_01320 ==
                                     (nOriginBR + (uint32_t)EVS_PRIMARY_MODE_00590)) &&
                            ((EvsChAOffset > 0) && (EvsChAOffset < 8)))  // ChA case.
                    {
                        if (nOriginBW == (uint32_t)EVS_VOC_BANDWIDTH_SWB)
                        {
                            nCodeType = (uint32_t)EVS_CMR_CODETYPE_SWB_CHA;
                        }
                        else
                        {
                            nCodeType = (uint32_t)EVS_CMR_CODETYPE_WB_CHA;
                        }

                        switch (EvsChAOffset)
                        {
                            case 2:
                                nCodeDefine = (uint32_t)EVS_CMR_CODEDEFINE_CHA_OFFSET_H2;
                                break;
                            case 3:
                                nCodeDefine = (uint32_t)EVS_CMR_CODEDEFINE_CHA_OFFSET_H3;
                                break;
                            case 5:
                                nCodeDefine = (uint32_t)EVS_CMR_CODEDEFINE_CHA_OFFSET_H5;
                                break;
                            case 7:
                                nCodeDefine = (uint32_t)EVS_CMR_CODEDEFINE_CHA_OFFSET_H7;
                                break;
                            default:
                                IMLOGD3("[Decode_PH_EVS] No selected chmode offset[%d], "
                                        "originBW[%d], originBR[%d]",
                                        EvsChAOffset, nOriginBW, nOriginBR);
                                nCodeType = (uint32_t)EVS_CMR_CODETYPE_NO_REQ;
                                nCodeDefine = (uint32_t)EVS_CMR_CODEDEFINE_NO_REQ;
                                break;
                        }
                    }
                    else  // primary case
                    {
                        nCodeDefine = nOriginBR;

                        switch (nOriginBW)
                        {
                            case 0:
                                nCodeType = (uint32_t)EVS_CMR_CODETYPE_NB;
                                break;
                            case 1:
                                nCodeType = (uint32_t)EVS_CMR_CODETYPE_WB;
                                break;
                            case 2:
                                nCodeType = (uint32_t)EVS_CMR_CODETYPE_SWB;
                                break;
                            case 3:
                                nCodeType = (uint32_t)EVS_CMR_CODETYPE_FB;
                                break;
                            default:
                                IMLOGD2("[Decode_PH_EVS] No CodeType - primary mode, originBW[%d], "
                                        "originBR[%d]",
                                        nOriginBW, nOriginBR);
                                nCodeType = (uint32_t)EVS_CMR_CODETYPE_NO_REQ;
                                nCodeDefine = (uint32_t)EVS_CMR_CODEDEFINE_NO_REQ;
                                break;
                        }
                    }

                    // process CMR
                    ProcessCMRForEVS(RTPPAYLOADHEADER_MODE_EVS_HEADER_FULL, nCodeType, nCodeDefine);

                    // Save Prev CMR value for checking
                    mPrevCMR = nChecnCMR;
                }
            }
            else  // ToC byte
            {
                IMLOGD0("[Decode_PH_EVS] Decoding TOC header");
                toc_f = mBitReader.Read(1);
                toc_ft_m = mBitReader.Read(1);
                toc_ft_q = mBitReader.Read(1);
                toc_ft_b = mBitReader.Read(4);

                listFrameType.push_back(toc_ft_b);
            }
        } while (toc_f == 1);

        //
        // read speech frames
        //
        while (listFrameType.size() > 0)
        {
            // uint32_t mode = listFrameType.front();
            listFrameType.pop_front();

            if (toc_ft_m == 0)  // EVS Primary mode
            {
                nDataBitSize = ImsMediaAudioFmt::ConvertEVSAudioRateToBitLen(
                        (IMSVOC_EVS_PRIMARY_ENTYPE)toc_ft_b);
            }
            else  // AMR-WB IO mode
            {
                nDataBitSize = ImsMediaAudioFmt::ConvertAmrWbModeToBitLen(toc_ft_b);
            }

            mBitReader.ReadByteBuffer(mPayload, nDataBitSize);

            // remove padding bit
            {
                uint32_t nPaddingSize;
                nPaddingSize = (8 - (nDataBitSize & 0x07)) & 0x07;
                mBitReader.Read(nPaddingSize);
            }

            IMLOGD6("[Decode_PH_EVS] result = %02X %02X %02X %02X, len=%d, eRate=%d", mPayload[0],
                    mPayload[1], mPayload[2], mPayload[3], nDataSize, toc_ft_b);

            SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD, mPayload, (nDataBitSize + 7) >> 3,
                    timestamp, listFrameType.size(),
                    nSeqNum);  // send remaining packet number in bundle as bMark value

            timestamp += 20;
        }
    }
    else
    {
        IMLOGE0("[Decode_PH_EVS] Invalid payload format");
        return;
    }
}

bool AudioRtpPayloadDecoderNode::ProcessCMRForEVS(
        eRTPPyaloadHeaderMode eEVSPayloadHeaderMode, uint32_t cmr_t, uint32_t cmr_d)
{
    eEVSCMRCodeType eNewEVSCMRCodeType = EVS_CMR_CODETYPE_NO_REQ;
    eEVSCMRCodeDefine eNewEVSCMRCodeDefine = EVS_CMR_CODEDEFINE_NO_REQ;

    if (eEVSPayloadHeaderMode == RTPPAYLOADHEADER_MODE_EVS_HEADER_FULL)
    {
        if (cmr_t < 8)  // cmr_type is 3bit validation.
            eNewEVSCMRCodeType = (eEVSCMRCodeType)cmr_t;

        if (cmr_d < 16)  // cmr_define is 4bit validation
            eNewEVSCMRCodeDefine = (eEVSCMRCodeDefine)cmr_d;
    }
    else if (eEVSPayloadHeaderMode == RTPPAYLOADHEADER_MODE_EVS_COMPACT)  // only EVS AMR IO Mode
    {
        eNewEVSCMRCodeType = EVS_CMR_CODETYPE_AMRIO;
        switch (cmr_d)
        {
            case 0:
                eNewEVSCMRCodeDefine = EVS_CMR_CODEDEFINE_AMRIO_660;
                break;
            case 1:
                eNewEVSCMRCodeDefine = EVS_CMR_CODEDEFINE_AMRIO_885;
                break;
            case 2:
                eNewEVSCMRCodeDefine = EVS_CMR_CODEDEFINE_AMRIO_1265;
                break;
            case 3:
                eNewEVSCMRCodeDefine = EVS_CMR_CODEDEFINE_AMRIO_1585;
                break;
            case 4:
                eNewEVSCMRCodeDefine = EVS_CMR_CODEDEFINE_AMRIO_1825;
                break;
            case 5:
                eNewEVSCMRCodeDefine = EVS_CMR_CODEDEFINE_AMRIO_2305;
                break;
            case 6:
                eNewEVSCMRCodeDefine = EVS_CMR_CODEDEFINE_AMRIO_2385;
                break;
            case 7:
            default:
                eNewEVSCMRCodeDefine = EVS_CMR_CODEDEFINE_NO_REQ;
                break;
        }
    }
    else
    {
        IMLOGD0("[ProcessCMRForEVS] Invalid EVS codec mode");
        return false;
    }

    if (eNewEVSCMRCodeDefine == EVS_CMR_CODEDEFINE_NO_REQ)
    {
        IMLOGD0("[ProcessCMRForEVS] Invalid CMR Value");
        return true;
    }

    IMLOGD2("[ProcessCMRForEVS] Change request bnadwidth[%d], bitrate[%d]", eNewEVSCMRCodeType,
            eNewEVSCMRCodeDefine);
// TODo: replace this with latest params
#if 0
    tMMPFRequestEvent_InternalRequestParam* pInternalReqParam = (tMMPFRequestEvent_InternalRequestParam*) MMPF_MemAlloc(sizeof(tMMPFRequestEvent_InternalRequestParam));

    pInternalReqParam->type = MMPFINTERNALREQUESTEVENT_CMR_EVS;
    pInternalReqParam->tEVSCMRPram.eNewCMRCodeType = eNewEVSCMRCodeType;
    pInternalReqParam->tEVSCMRPram.eNewCMRCodeDefine = eNewEVSCMRCodeDefine;
    if(!MMPFEventHandler::SendEvent("RequestHandler", MMPFREQUESTEVENT_INTERNAL_REQUEST, (mmpf_param)(((MMPFGraph*)GetGraph())->GetSession()), (mmpf_param)pInternalReqParam))
    {
        if(pInternalReqParam != NULL)
        {
            free((void*)pInternalReqParam);
            pInternalReqParam = NULL;
        }
    }
#endif

    return true;
}
