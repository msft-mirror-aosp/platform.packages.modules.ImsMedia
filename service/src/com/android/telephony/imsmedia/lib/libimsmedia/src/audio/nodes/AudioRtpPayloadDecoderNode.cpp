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
#include <ImsMediaAudioUtil.h>
#include <ImsMediaTrace.h>
#include <AudioConfig.h>

AudioRtpPayloadDecoderNode::AudioRtpPayloadDecoderNode(BaseSessionCallback* callback) :
        BaseNode(callback)
{
    mPrevCMR = 15;
}

AudioRtpPayloadDecoderNode::~AudioRtpPayloadDecoderNode()
{
    std::lock_guard<std::mutex> guard(mMutexExit);
}

kBaseNodeId AudioRtpPayloadDecoderNode::GetNodeId()
{
    return kNodeIdAudioPayloadDecoder;
}

ImsMediaResult AudioRtpPayloadDecoderNode::Start()
{
    IMLOGD0("[Start]");
    mEvsMode = (kEvsBitrate)ImsMediaAudioUtil::GetMaximumEvsMode(mCoreEvsMode);
    mEvsCodecMode = (kEvsCodecMode)ImsMediaAudioUtil::ConvertEvsCodecMode(mEvsMode);
    mEvsModetoBitRate = ImsMediaAudioUtil::ConvertEVSModeToBitRate(mEvsMode);

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
        mCodecType = ImsMediaAudioUtil::ConvertCodecType(pConfig->getCodecType());
        if (mCodecType == kAudioCodecAmr || mCodecType == kAudioCodecAmrWb)
        {
            mOctetAligned = pConfig->getAmrParams().getOctetAligned();
        }
        else if (mCodecType == kAudioCodecEvs)
        {
            mEvsBandwidth = (kEvsBandwidth)pConfig->getEvsParams().getEvsBandwidth();
            mCoreEvsMode = pConfig->getEvsParams().getEvsMode();
            mEvsPayloadHeaderMode =
                    (kRtpPyaloadHeaderMode)pConfig->getEvsParams().getUseHeaderFullOnly();
            mEvsOffset = pConfig->getEvsParams().getChannelAwareMode();
        }
    }
}

bool AudioRtpPayloadDecoderNode::IsSameConfig(void* config)
{
    if (config == NULL)
        return true;
    AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);

    if (mCodecType == ImsMediaAudioUtil::ConvertCodecType(pConfig->getCodecType()))
    {
        if (mCodecType == kAudioCodecAmr || mCodecType == kAudioCodecAmrWb)
        {
            return (mOctetAligned == pConfig->getAmrParams().getOctetAligned());
        }
        else if (mCodecType == kAudioCodecEvs)
        {
            return (mEvsBandwidth == (kEvsBandwidth)pConfig->getEvsParams().getEvsBandwidth() &&
                    mEvsPayloadHeaderMode ==
                            (kRtpPyaloadHeaderMode)pConfig->getEvsParams().getUseHeaderFullOnly() &&
                    mCoreEvsMode ==
                            ImsMediaAudioUtil::GetMaximumEvsMode(
                                    pConfig->getEvsParams().getEvsMode()) &&
                    mEvsOffset == pConfig->getEvsParams().getChannelAwareMode());
        }
    }

    return false;
}

void AudioRtpPayloadDecoderNode::OnDataFromFrontNode(ImsMediaSubType subtype, uint8_t* pData,
        uint32_t nDataSize, uint32_t nTimestamp, bool bMark, uint32_t nSeqNum,
        ImsMediaSubType nDataType, uint32_t arrivalTime)
{
    if (subtype == MEDIASUBTYPE_REFRESHED)
    {
        SendDataToRearNode(subtype, NULL, nDataSize, 0, 0, 0, MEDIASUBTYPE_UNDEFINED);
        return;
    }

    switch (mCodecType)
    {
        case kAudioCodecAmr:
        case kAudioCodecAmrWb:
            DecodePayloadAmr(pData, nDataSize, nTimestamp, bMark, nSeqNum, arrivalTime);
            break;
        case kAudioCodecPcmu:
        case kAudioCodecPcma:
            SendDataToRearNode(
                    MEDIASUBTYPE_RTPPAYLOAD, pData, nDataSize, nTimestamp, bMark, nSeqNum);
            break;
        case kAudioCodecEvs:
            DecodePayloadEvs(pData, nDataSize, nTimestamp, bMark, nSeqNum, arrivalTime);
            break;
        default:
            IMLOGE1("[OnDataFromFrontNode] invalid codec type[%d]", mCodecType);
            SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD, pData, nDataSize, nTimestamp, bMark,
                    nSeqNum, nDataType, arrivalTime);
            break;
    }
}

void AudioRtpPayloadDecoderNode::DecodePayloadAmr(uint8_t* pData, uint32_t nDataSize,
        uint32_t nTimestamp, bool bMark, uint32_t nSeqNum, uint32_t arrivalTime)
{
    if (pData == NULL || nDataSize == 0)
    {
        return;
    }

    (void)bMark;
    uint32_t timestamp = nTimestamp;
    static std::list<uint32_t> listFrameType;  // defined as static variable for memory management
    uint32_t eRate;
    uint32_t f;
    uint32_t cmr;
    uint32_t QbitPos;  // Q_Speech_Sid_Bad

    std::lock_guard<std::mutex> guard(mMutexExit);

    IMLOGD_PACKET5(IM_PACKET_LOG_PH,
            "[DecodePayloadAmr] GetCodectype[%d], octetAligned[%d], nSeqNum[%d], TS[%u], "
            "arrivalTime[%d]",
            mCodecType, mOctetAligned, nSeqNum, timestamp, arrivalTime);

    mBitReader.SetBuffer(pData, nDataSize);
    // read cmr
    cmr = mBitReader.Read(4);

    if (mOctetAligned == true)
    {
        mBitReader.Read(4);
    }

    if (cmr != mPrevCMR)
    {
        if ((mCodecType == kAudioCodecAmr && cmr <= 7) ||
                (mCodecType == kAudioCodecAmrWb && cmr <= 8))
        {
            IMLOGD2("[DecodePayloadAmr] CMR %d->%d", mPrevCMR, cmr);
            // send internal event to operate cmr operation in encoder side
            mCallback->SendEvent(kRequestAudioCmr, cmr);
            mPrevCMR = cmr;
        }
        else if (cmr == 15)
        {
            mCallback->SendEvent(kRequestAudioCmr, cmr);
            mPrevCMR = cmr;
        }
        else
        {
            IMLOGE1("[DecodePayloadAmr] invalid cmr value %d", cmr);
        }
    }

    // get num of frame
    do
    {
        f = mBitReader.Read(1);        // f(1)
        eRate = mBitReader.Read(4);    // ft(4)
        QbitPos = mBitReader.Read(1);  // q(2)
        IMLOGD_PACKET3(
                IM_PACKET_LOG_PH, "[DecodePayloadAmr] cmr[%d], f[%d], ft[%d]", cmr, f, eRate);
        listFrameType.push_back(eRate);
        if (mOctetAligned == true)
        {
            mBitReader.Read(2);  // padding
        }
    } while (f == 1);

    IMLOGD_PACKET3(IM_PACKET_LOG_PH,
            "[DecodePayloadAmr] Q_Speech_SID <Q_Speech_SID> f[%d] eRate[%d] QbitPos[%d]", f, eRate,
            QbitPos);  // Q_Speech_SID

    // read speech frames
    while (listFrameType.size() > 0)
    {
        uint32_t nDataBitSize;
        uint32_t mode = listFrameType.front();
        if (mCodecType == kAudioCodecAmr)
        {
            nDataBitSize = ImsMediaAudioUtil::ConvertAmrModeToBitLen(mode);
        }
        else
        {
            nDataBitSize = ImsMediaAudioUtil::ConvertAmrWbModeToBitLen(mode);
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
                "[DecodePayloadAmr] result = %02X %02X %02X %02X, len[%d], eRate[%d]", mPayload[0],
                mPayload[1], mPayload[2], mPayload[3], nBufferSize, eRate);
        // send remaining packet number in bundle as bMark value
        SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD, mPayload, nBufferSize, timestamp,
                listFrameType.size(), nSeqNum, MEDIASUBTYPE_UNDEFINED, arrivalTime);

        timestamp += 20;
    }
}

void AudioRtpPayloadDecoderNode::DecodePayloadEvs(uint8_t* pData, uint32_t nDataSize,
        uint32_t nTimeStamp, bool bMark, uint32_t nSeqNum, uint32_t arrivalTime)
{
    if (pData == NULL || nDataSize == 0)
    {
        return;
    }

    IMLOGD_PACKET5(IM_PACKET_LOG_PH,
            "[DecodePayloadEvs] GetCodectype[%d], octetAligned[%d], nSeqNum[%d], TS[%u], "
            "arrivalTime[%d]",
            mCodecType, mOctetAligned, nSeqNum, nTimeStamp, arrivalTime);

    kRtpPyaloadHeaderMode eEVSPHFormat = kRtpPyaloadHeaderModeEvsCompact;
    kRtpPyaloadHeaderMode eEVSReceivedPHFormat = kRtpPyaloadHeaderModeEvsCompact;
    kEvsCodecMode kEvsCodecMode = kEvsCodecModePrimary;

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
            ImsMediaAudioUtil::ConvertEVSPayloadMode(nDataSize, &kEvsCodecMode, &nEVSCompactId);

    if ((kEvsCodecMode == kEvsCodecModePrimary) && (nEVSCompactId == 0))  // special case
    {
        // first bit of the EVS Primary 2.8kbps in compact format is always set to '0'
        if ((pData[0] >> 7) == 0)
        {
            // EVS Primary 2.8 kbps frame in Compact format
            eEVSReceivedPHFormat = kRtpPyaloadHeaderModeEvsCompact;
        }
        else
        {
            // EVS AMR-WB IO SID frame in Header-Full format with one CMR byte
            eEVSReceivedPHFormat = kRtpPyaloadHeaderModeEvsHeaderFull;
        }
    }

    if (eEVSReceivedPHFormat == kRtpPyaloadHeaderModeEvsCompact)
    {
        if (kEvsCodecMode == kEvsCodecModePrimary)
        {
            // calculate nDataBitSize from nDataSize
            nFrameType = (uint32_t)ImsMediaAudioUtil::ConvertLenToEVSAudioMode(nDataSize);
            // TODO: remove kImsAudioEvsMode
            nDataBitSize =
                    ImsMediaAudioUtil::ConvertEVSAudioModeToBitLen((kImsAudioEvsMode)nFrameType);

            mBitReader.ReadByteBuffer(mPayload, nDataBitSize);

            IMLOGD6("[DecodePayloadEvs] Result =%02X %02X %02X %02X, len=%d,nFrameType=%d",
                    mPayload[0], mPayload[1], mPayload[2], mPayload[3], nDataSize, nFrameType);

            SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD, mPayload, nDataSize, timestamp, bMark,
                    nSeqNum, MEDIASUBTYPE_UNDEFINED, arrivalTime);
        }
        else if (kEvsCodecMode == kEvsCodecModeAmrIo)
        {
            // calculate nDataBitSize from nDataSize
            nFrameType = (uint32_t)ImsMediaAudioUtil::ConvertLenToAmrWbMode(nDataSize);

            nDataBitSize = ImsMediaAudioUtil::ConvertAmrWbModeToBitLen(nFrameType);

            // read cmr except SID
            // at EVS AMR WB IO Mode, SID packet does not include cmr field...
            if (nFrameType != kImsAudioAmrWbModeSID)
            {
                cmr = mBitReader.Read(3);

                // process cmr
                if (cmr != 7 && cmr != mPrevCMR)
                {
                    // Process CMR
                    ProcessCMRForEVS(kRtpPyaloadHeaderModeEvsCompact, 7, cmr);
                    // Save Prev CMR value for checking
                    mPrevCMR = cmr;
                }
                // cmr == 7 && cmr != mPrevCMR -> returned original codec mode.
                else if (cmr != mPrevCMR)
                {
                    // TODO : Process CMR
                    uint32_t nOriginBW = ImsMediaAudioUtil::FindMaxEVSBandwidth(mEvsBandwidth);
                    uint32_t nOriginBR =
                            ImsMediaAudioUtil::FindMaxEVSBitrate(mEvsModetoBitRate, mEvsCodecMode);
                    IMLOGD3("[DecodePayloadEvs] Returned Codec Mode Request CodecMode[%d], "
                            "bnadwidth[%d], bitrate[%d]",
                            mEvsCodecMode, nOriginBW, nOriginBR);

                    uint32_t nCodeType = (uint32_t)kEvsCmrCodeTypeNoReq;
                    uint32_t nCodeDefine = (uint32_t)kEvsCmrCodeDefineNoReq;

                    if (mEvsCodecMode == kEvsCodecModePrimary)
                    {
                        // to convert EVS CMR type
                        nOriginBR -= (uint32_t)kEvsPrimaryModeBitrate00590;
                    }

                    // check AMR IO and ChA
                    if (mEvsCodecMode == kEvsCodecModeAmrIo)
                    {
                        nCodeType = (uint32_t)kEvsCmrCodeTypeAmrIO;
                        nCodeDefine = nOriginBR;
                    }
                    else if ((kEvsPrimaryModeBitrate01320 ==
                                     (nOriginBR + (uint32_t)kEvsPrimaryModeBitrate00590)) &&
                            ((EvsChAOffset > 0) && (EvsChAOffset < 8)))  // ChA case.
                    {
                        if (nOriginBW == (uint32_t)kEvsBandwidthSWB)
                        {
                            nCodeType = (uint32_t)kEvsCmrCodeTypeSwbCha;
                        }
                        else
                        {
                            nCodeType = (uint32_t)kEvsCmrCodeTypeWbCha;
                        }

                        switch (EvsChAOffset)
                        {
                            case 2:
                                nCodeDefine = (uint32_t)kEvsCmrCodeDefineChaOffsetH2;
                                break;
                            case 3:
                                nCodeDefine = (uint32_t)kEvsCmrCodeDefineChaOffsetH3;
                                break;
                            case 5:
                                nCodeDefine = (uint32_t)kEvsCmrCodeDefineChaOffsetH5;
                                break;
                            case 7:
                                nCodeDefine = (uint32_t)kEvsCmrCodeDefineChaOffsetH7;
                                break;
                            default:
                                IMLOGD3("[DecodePayloadEvs] no selected chmode offset[%d], "
                                        "originBW[%d], originBR[%d]",
                                        EvsChAOffset, nOriginBW, nOriginBR);
                                nCodeType = (uint32_t)kEvsCmrCodeTypeNoReq;
                                nCodeDefine = (uint32_t)kEvsCmrCodeDefineNoReq;
                                break;
                        }
                    }
                    else  // primary case
                    {
                        nCodeDefine = nOriginBR;

                        switch (nOriginBW)
                        {
                            case 0:
                                nCodeType = (uint32_t)kEvsCmrCodeTypeNb;
                                break;
                            case 1:
                                nCodeType = (uint32_t)kEvsCmrCodeTypeWb;
                                break;
                            case 2:
                                nCodeType = (uint32_t)kEvsCmrCodeTypeSwb;
                                break;
                            case 3:
                                nCodeType = (uint32_t)kEvsCmrCodeTypeFb;
                                break;
                            default:
                                IMLOGD2("[DecodePayloadEvs] no CodeType - primary mode, "
                                        "originBW[%d], "
                                        "originBR[%d]",
                                        nOriginBW, nOriginBR);
                                nCodeType = (uint32_t)kEvsCmrCodeTypeNoReq;
                                nCodeDefine = (uint32_t)kEvsCmrCodeDefineNoReq;
                                break;
                        }
                    }

                    // process CMR
                    ProcessCMRForEVS(kRtpPyaloadHeaderModeEvsHeaderFull, nCodeType, nCodeDefine);

                    // Save Prev CMR value for checking
                    mPrevCMR = cmr;
                }
            }
            mBitReader.ReadByteBuffer(mPayload, nDataBitSize);

            // last data bit is speech first bit..
            if (nFrameType != kImsAudioAmrWbModeSID)
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

            IMLOGD6("[DecodePayloadEvs] result = %02X %02X %02X %02X, len=%d, nFrameType=%d",
                    mPayload[0], mPayload[1], mPayload[2], mPayload[3], nDataSize, nFrameType);

            SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD, mPayload, nDataSize, timestamp, bMark,
                    nSeqNum, MEDIASUBTYPE_UNDEFINED, arrivalTime);
        }
        else
        {
            IMLOGD0("[DecodePayloadEvs] Invalid codec mode");
            return;
        }
    }
    else if (eEVSReceivedPHFormat == kRtpPyaloadHeaderModeEvsHeaderFull)
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
                // IMLOGD1("[AudioRtpPayloadDecoderNode::DecodePayloadEvs] nChecnCMR[0x%x]",
                // nChecnCMR);

                // process cmr
                if (nChecnCMR != 127 && nChecnCMR != mPrevCMR)  // 127 is 111 1111
                {
                    IMLOGD0("[DecodePayloadEvs] Process CMR");
                    // Process CMR
                    ProcessCMRForEVS(kRtpPyaloadHeaderModeEvsHeaderFull, cmr_t, cmr_d);

                    // Save Prev CMR value for checking
                    mPrevCMR = nChecnCMR;
                }
                else if (nChecnCMR !=
                        mPrevCMR)  // cmr == 127 && cmr != mPrevCMR -> returned original codec mode.
                {
                    uint32_t nOriginBW = ImsMediaAudioUtil::FindMaxEVSBandwidth(mEvsBandwidth);
                    uint32_t nOriginBR =
                            ImsMediaAudioUtil::FindMaxEVSBitrate(mEvsModetoBitRate, mEvsCodecMode);
                    IMLOGD3("[DecodePayloadEvs] Returned Codec Mode Request CodecMode[%d], "
                            "bnadwidth[%d], bitrate[%d]",
                            mEvsMode, nOriginBW, nOriginBR);

                    uint32_t nCodeType = (uint32_t)kEvsCmrCodeTypeNoReq;
                    uint32_t nCodeDefine = (uint32_t)kEvsCmrCodeDefineNoReq;

                    if (mEvsCodecMode == kEvsCodecModePrimary)
                    {
                        // to convert EVS CMR type
                        nOriginBR -= (uint32_t)kEvsPrimaryModeBitrate00590;
                    }

                    // check AMR IO and ChA
                    if (mEvsCodecMode == kEvsCodecModeAmrIo)
                    {
                        nCodeType = (uint32_t)kEvsCmrCodeTypeAmrIO;
                        nCodeDefine = nOriginBR;
                    }
                    else if ((kEvsPrimaryModeBitrate01320 ==
                                     (nOriginBR + (uint32_t)kEvsPrimaryModeBitrate00590)) &&
                            ((EvsChAOffset > 0) && (EvsChAOffset < 8)))  // ChA case.
                    {
                        if (nOriginBW == (uint32_t)kEvsBandwidthSWB)
                        {
                            nCodeType = (uint32_t)kEvsCmrCodeTypeSwbCha;
                        }
                        else
                        {
                            nCodeType = (uint32_t)kEvsCmrCodeTypeWbCha;
                        }

                        switch (EvsChAOffset)
                        {
                            case 2:
                                nCodeDefine = (uint32_t)kEvsCmrCodeDefineChaOffsetH2;
                                break;
                            case 3:
                                nCodeDefine = (uint32_t)kEvsCmrCodeDefineChaOffsetH3;
                                break;
                            case 5:
                                nCodeDefine = (uint32_t)kEvsCmrCodeDefineChaOffsetH5;
                                break;
                            case 7:
                                nCodeDefine = (uint32_t)kEvsCmrCodeDefineChaOffsetH7;
                                break;
                            default:
                                IMLOGD3("[DecodePayloadEvs] No selected chmode offset[%d], "
                                        "originBW[%d], originBR[%d]",
                                        EvsChAOffset, nOriginBW, nOriginBR);
                                nCodeType = (uint32_t)kEvsCmrCodeTypeNoReq;
                                nCodeDefine = (uint32_t)kEvsCmrCodeDefineNoReq;
                                break;
                        }
                    }
                    else  // primary case
                    {
                        nCodeDefine = nOriginBR;

                        switch (nOriginBW)
                        {
                            case 0:
                                nCodeType = (uint32_t)kEvsCmrCodeTypeNb;
                                break;
                            case 1:
                                nCodeType = (uint32_t)kEvsCmrCodeTypeWb;
                                break;
                            case 2:
                                nCodeType = (uint32_t)kEvsCmrCodeTypeSwb;
                                break;
                            case 3:
                                nCodeType = (uint32_t)kEvsCmrCodeTypeFb;
                                break;
                            default:
                                IMLOGD2("[DecodePayloadEvs] No CodeType - primary mode, "
                                        "originBW[%d], "
                                        "originBR[%d]",
                                        nOriginBW, nOriginBR);
                                nCodeType = (uint32_t)kEvsCmrCodeTypeNoReq;
                                nCodeDefine = (uint32_t)kEvsCmrCodeDefineNoReq;
                                break;
                        }
                    }

                    // process CMR
                    ProcessCMRForEVS(kRtpPyaloadHeaderModeEvsHeaderFull, nCodeType, nCodeDefine);

                    // Save Prev CMR value for checking
                    mPrevCMR = nChecnCMR;
                }
            }
            else  // ToC byte
            {
                IMLOGD0("[DecodePayloadEvs] Decoding TOC header");
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
                nDataBitSize =
                        ImsMediaAudioUtil::ConvertEVSAudioModeToBitLen((kImsAudioEvsMode)toc_ft_b);
            }
            else  // AMR-WB IO mode
            {
                nDataBitSize = ImsMediaAudioUtil::ConvertAmrWbModeToBitLen(toc_ft_b);
            }

            mBitWriter.SetBuffer(mPayload, MAX_AUDIO_PAYLOAD_SIZE);

            mBitWriter.Write(h, 1);
            mBitWriter.Write(toc_f, 1);
            mBitWriter.Write(toc_ft_m, 1);
            mBitWriter.Write(toc_ft_q, 1);
            mBitWriter.Write(toc_ft_b, 4);

            mBitReader.ReadByteBuffer(mPayload + 1, nDataBitSize);

            // remove padding bit
            {
                uint32_t nPaddingSize;
                nPaddingSize = (8 - (nDataBitSize & 0x07)) & 0x07;
                mBitReader.Read(nPaddingSize);
            }

            IMLOGD6("[DecodePayloadEvs] result = %02X %02X %02X %02X, len=%d, eRate=%d",
                    mPayload[0], mPayload[1], mPayload[2], mPayload[3], nDataSize, toc_ft_b);

            SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD, mPayload, (((nDataBitSize + 7) >> 3)),
                    timestamp, listFrameType.size(), nSeqNum, MEDIASUBTYPE_UNDEFINED, arrivalTime);

            timestamp += 20;
        }
    }
    else
    {
        IMLOGE0("[DecodePayloadEvs] Invalid payload format");
        return;
    }
}

bool AudioRtpPayloadDecoderNode::ProcessCMRForEVS(
        kRtpPyaloadHeaderMode eEVSPayloadHeaderMode, uint32_t cmr_t, uint32_t cmr_d)
{
    kEvsCmrCodeType eNewEVSCMRCodeType = kEvsCmrCodeTypeNoReq;
    kEvsCmrCodeDefine eNewEVSCMRCodeDefine = kEvsCmrCodeDefineNoReq;

    if (eEVSPayloadHeaderMode == kRtpPyaloadHeaderModeEvsHeaderFull)
    {
        if (cmr_t < 8)  // cmr_type is 3bit validation.
            eNewEVSCMRCodeType = (kEvsCmrCodeType)cmr_t;

        if (cmr_d < 16)  // cmr_define is 4bit validation
            eNewEVSCMRCodeDefine = (kEvsCmrCodeDefine)cmr_d;
    }
    else if (eEVSPayloadHeaderMode == kRtpPyaloadHeaderModeEvsCompact)  // only EVS AMR IO Mode
    {
        eNewEVSCMRCodeType = kEvsCmrCodeTypeAmrIO;
        switch (cmr_d)
        {
            case 0:
                eNewEVSCMRCodeDefine = kEvsCmrCodeDefineAmrIo660;
                break;
            case 1:
                eNewEVSCMRCodeDefine = kEvsCmrCodeDefineAmrIo885;
                break;
            case 2:
                eNewEVSCMRCodeDefine = kEvsCmrCodeDefineAmrIo1265;
                break;
            case 3:
                eNewEVSCMRCodeDefine = kEvsCmrCodeDefineAmrIo1585;
                break;
            case 4:
                eNewEVSCMRCodeDefine = kEvsCmrCodeDefineAmrIo1825;
                break;
            case 5:
                eNewEVSCMRCodeDefine = kEvsCmrCodeDefineAmrIo2305;
                break;
            case 6:
                eNewEVSCMRCodeDefine = kEvsCmrCodeDefineAmrIo2385;
                break;
            case 7:
            default:
                eNewEVSCMRCodeDefine = kEvsCmrCodeDefineNoReq;
                break;
        }
    }
    else
    {
        IMLOGD0("[ProcessCMRForEVS] Invalid EVS codec mode");
        return false;
    }

    if (eNewEVSCMRCodeDefine == kEvsCmrCodeDefineNoReq)
    {
        IMLOGD0("[ProcessCMRForEVS] Invalid CMR Value");
        return true;
    }

    IMLOGD2("[ProcessCMRForEVS] Change request bnadwidth[%d], bitrate[%d]", eNewEVSCMRCodeType,
            eNewEVSCMRCodeDefine);
    // TODO: replace this with latest params
    return true;
}
