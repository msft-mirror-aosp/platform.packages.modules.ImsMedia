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

#include <RtpStackUtil.h>
#include <rtp_trace.h>

/*********************************************************
 * Function name        : RtpStackUtil
 * Description          : Constructor
 * Return type          : None
 * Argument             : None
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpStackUtil::RtpStackUtil() {}

/*********************************************************
 * Function name        : ~RtpStackUtil
 * Description          : Constructor
 * Return type          : None
 * Argument             : None
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpStackUtil::~RtpStackUtil() {}

/*********************************************************
 * Function name        : getSeqNum
 * Description          : Gets sequence number
 * Return type          : RtpDt_UInt16
 *                          Sequence number
 * Argument             : RtpBuffer* : In
 *                          Received RTP packet
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpDt_UInt16 RtpStackUtil::getSeqNum(IN RtpDt_UChar* pcRtpHdrBuf)
{
    RtpDt_UInt32 uiByte4Data = RTP_ZERO;
    RtpDt_UInt16 usSeqNum = RTP_ZERO;

    uiByte4Data = RtpOsUtil::Ntohl(*((RtpDt_UInt32*)pcRtpHdrBuf));
    usSeqNum = (RtpDt_UInt16)(uiByte4Data & RTP_HEX_16_BIT_MAX);

    return usSeqNum;
}

/*********************************************************
 * Function name        : getRtpSsrc
 * Description          : It gets SSRC from RTP message
 * Return type          : RtpDt_UInt32
 *                          Synchronization Source
 * Argument             : RtpBuffer* : In
 *                          Received RTP packet
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpDt_UInt32 RtpStackUtil::getRtpSsrc(IN RtpBuffer* pobjRecvdPkt)
{
    RtpDt_UInt32 uiByte4Data = RTP_ZERO;
    RtpDt_UChar* pcRtpHdrBuf = pobjRecvdPkt->getBuffer();
    pcRtpHdrBuf = pcRtpHdrBuf + RTP_EIGHT;

    uiByte4Data = RtpOsUtil::Ntohl(*((RtpDt_UInt32*)pcRtpHdrBuf));
    return uiByte4Data;
}

/*********************************************************
 * Function name        : getRtcpSsrc
 * Description          : It gets SSRC from RTCP
 * Return type          : RtpDt_UInt32
 *                          Synchronization Source
 * Argument             : RtpBuffer* : In
 *                          Received RTP packet
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpDt_UInt32 RtpStackUtil::getRtcpSsrc(IN RtpBuffer* pobjRecvdPkt)
{
    RtpDt_UChar* pcRtpHdrBuf = pobjRecvdPkt->getBuffer();
    pcRtpHdrBuf = pcRtpHdrBuf + RTP_WORD_SIZE;

    RtpDt_UInt32 uiByte4Data = RtpOsUtil::Ntohl(*((RtpDt_UInt32*)pcRtpHdrBuf));
    return uiByte4Data;
}

/*********************************************************
 * Function name        : generateNewSsrc
 * Description          : generates sequence number
 * Return type          : RtpDt_UInt32
 *                          Synchronization source
 * Argument             : RtpDt_UInt32 : In
 *                          Terminal number
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpDt_UInt32 RtpStackUtil::generateNewSsrc(IN RtpDt_UInt32 uiTermNum)
{
    RtpDt_UInt32 uiTmpRand = RTP_ZERO;

    uiTmpRand = RtpOsUtil::Rand();
    uiTmpRand = uiTmpRand << RTP_EIGHT;
    uiTmpRand = uiTmpRand & RTP_SSRC_GEN_UTL;
    uiTmpRand = uiTmpRand | uiTermNum;

    return uiTmpRand;
}

/*********************************************************
 * Function name        : getMidFourOctets
 * Description          : It gets middle four octets from Ntp timestamp
 * Return type          : RtpDt_UInt32
 * Argument             : tRTP_NTP_TIME* : In
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpDt_UInt32 RtpStackUtil::getMidFourOctets(IN tRTP_NTP_TIME* pstNtpTs)
{
    RtpDt_UInt32 uiNtpTs = pstNtpTs->m_uiNtpHigh32Bits;
    uiNtpTs = uiNtpTs << RTP_BYTE2_BIT_SIZE;
    RtpDt_UInt32 uiNtpLowTs = pstNtpTs->m_uiNtpLow32Bits;
    uiNtpLowTs = uiNtpLowTs >> RTP_BYTE2_BIT_SIZE;
    uiNtpTs = uiNtpTs | uiNtpLowTs;
    return uiNtpTs;
}  // getMidFourOctets

/*********************************************************
 * Function name        : generateNewSsrc
 * Description          : generates sequence number
 * Return type          : RtpDt_UInt32
 *                          Synchronization source
 * Argument             : RtpDt_UInt32 : In
 *                          Terminal number
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpDt_UInt32 RtpStackUtil::calcRtpTimestamp(IN RtpDt_UInt32 uiPrevRtpTs,
        IN tRTP_NTP_TIME* pstCurNtpTs, IN tRTP_NTP_TIME* pstPrevNtpTs,
        IN RtpDt_UInt32 uiSamplingRate)
{
    RtpDt_Int32 iTimeDiffHigh32Bits = RTP_ZERO;
    RtpDt_Int32 iTimeDiffLow32Bits = RTP_ZERO;

    if ((RTP_ZERO != pstPrevNtpTs->m_uiNtpHigh32Bits) ||
            (RTP_ZERO != pstPrevNtpTs->m_uiNtpLow32Bits))
    {
        iTimeDiffHigh32Bits = pstCurNtpTs->m_uiNtpHigh32Bits - pstPrevNtpTs->m_uiNtpHigh32Bits;
        iTimeDiffLow32Bits = (pstCurNtpTs->m_uiNtpLow32Bits / 4294UL) -
                (pstPrevNtpTs->m_uiNtpLow32Bits / 4294UL);
    }
    else
    {
        iTimeDiffHigh32Bits = RTP_ZERO;
        iTimeDiffLow32Bits = RTP_ZERO;
    }

    // calc iTimeDiff in millisec
    RtpDt_Int32 iTimeDiff = (iTimeDiffHigh32Bits * 1000 * 1000) + iTimeDiffLow32Bits;

    /* the time diff high bit is in seconds and
       the time diff low bit is in micro seconds */

    RtpDt_UInt32 uiNewRtpTs = RTP_ZERO;

    if (RTP_ZERO == iTimeDiff)
    {
        uiNewRtpTs = uiPrevRtpTs;
    }
    else
    {
        RTP_TRACE_MESSAGE("PTime:", iTimeDiff, 0);
        RtpDt_Int32 temp = uiSamplingRate / 1000;
        uiNewRtpTs = uiPrevRtpTs + (temp * iTimeDiff / 1000);
    }
    return uiNewRtpTs;
}
