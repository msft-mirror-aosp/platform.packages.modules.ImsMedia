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

#include <RtcpHeader.h>

RtcpHeader::RtcpHeader() :
        m_ucVersion(RTP_ZERO),
        m_ucIsPadding(RTP_ZERO),
        m_ucRecepRepCnt(RTP_ZERO),
        m_ucPktType(RTP_ZERO),
        m_usLength(RTP_ZERO),
        m_uiSsrc(RTP_ZERO)
{
}

RtcpHeader::~RtcpHeader() {}

RtpDt_Void RtcpHeader::setVersion(IN RtpDt_UChar ucVersion)
{
    m_ucVersion = ucVersion;
}

RtpDt_UChar RtcpHeader::getVersion()
{
    return m_ucVersion;
}

RtpDt_Void RtcpHeader::setPadding()
{
    m_ucIsPadding = RTP_ONE;
}

RtpDt_UChar RtcpHeader::getPadding()
{
    return m_ucIsPadding;
}

RtpDt_Void RtcpHeader::setRecepRepCnt(IN RtpDt_UChar ucRecReport)
{
    m_ucRecepRepCnt = ucRecReport;
}

RtpDt_UChar RtcpHeader::getRecepRepCnt()
{
    return m_ucRecepRepCnt;
}

RtpDt_Void RtcpHeader::setPacketType(IN RtpDt_UChar ucPktType)
{
    m_ucPktType = ucPktType;
}

RtpDt_UChar RtcpHeader::getPacketType()
{
    return m_ucPktType;
}

RtpDt_Void RtcpHeader::setLength(IN RtpDt_UInt16 usLength)
{
    m_usLength = usLength;
}

RtpDt_UInt16 RtcpHeader::getLength()
{
    return m_usLength;
}

RtpDt_Void RtcpHeader::setSsrc(IN RtpDt_UInt32 uiSsrc)
{
    m_uiSsrc = uiSsrc;
}

RtpDt_UInt32 RtcpHeader::getSsrc()
{
    return m_uiSsrc;
}

eRtp_Bool RtcpHeader::decodeRtcpHeader(IN RtpDt_UChar* pucRtcpHdr)
{
    RtpDt_UInt32 uiTemp4Data = RtpOsUtil::Ntohl(*((RtpDt_UInt32*)pucRtcpHdr));
    pucRtcpHdr = pucRtcpHdr + RTP_WORD_SIZE;

    uiTemp4Data = uiTemp4Data >> RTP_24;
    // version
    m_ucVersion = RtpDt_UChar(uiTemp4Data & 0x000000C0) >> RTP_SIX;
    // padding
    m_ucIsPadding = RtpDt_UChar(uiTemp4Data & 0x00000020) >> RTP_FIVE;
    // RC
    m_ucRecepRepCnt = RtpDt_UChar(uiTemp4Data & 0x0000001F);

    m_uiSsrc = RtpOsUtil::Ntohl(*((RtpDt_UInt32*)pucRtcpHdr));

    return eRTP_SUCCESS;
}  // decodeRtcpHeader

eRtp_Bool RtcpHeader::formRtcpHeader(OUT RtpBuffer* pobjRtcpPktBuf)
{
    // do partial encoding
    formPartialRtcpHeader(pobjRtcpPktBuf);

    RtpDt_UChar* pcRtcpHdrBuf = pobjRtcpPktBuf->getBuffer();
    RtpDt_UInt32 uiBufPos = pobjRtcpPktBuf->getLength();
    pcRtcpHdrBuf = pcRtcpHdrBuf + uiBufPos;

    // ssrc
    *(RtpDt_UInt32*)pcRtcpHdrBuf = RtpOsUtil::Ntohl(m_uiSsrc);
    pcRtcpHdrBuf = pcRtcpHdrBuf + RTP_WORD_SIZE;

    uiBufPos = uiBufPos + RTP_WORD_SIZE;
    pobjRtcpPktBuf->setLength(uiBufPos);

    return eRTP_SUCCESS;
}  // formRtcpHeader

eRtp_Bool RtcpHeader::formPartialRtcpHeader(OUT RtpBuffer* pobjRtcpPktBuf)
{
    RtpDt_UInt16 usUtlData = RTP_ZERO;
    RtpDt_UInt16 usTmpData = RTP_ZERO;
    RtpDt_UChar* pcRtcpHdrBuf = pobjRtcpPktBuf->getBuffer();
    RtpDt_UInt32 uiBufPos = pobjRtcpPktBuf->getLength();

    pcRtcpHdrBuf = pcRtcpHdrBuf + uiBufPos;

    // version 2 bits
    RTP_FORM_HDR_UTL(usUtlData, m_ucVersion, RTP_VER_SHIFT_VAL, usTmpData);
    // padding 1 bit
    RTP_FORM_HDR_UTL(usUtlData, m_ucIsPadding, RTP_PAD_SHIFT_VAL, usTmpData);
    // RC 5 bits
    RTP_FORM_HDR_UTL(usUtlData, m_ucRecepRepCnt, RTCP_RC_SHIFT_VAL, usTmpData);
    // PT 8 bits
    RTP_FORM_HDR_UTL(usUtlData, m_ucPktType, RTCP_PT_SHIFT_VAL, usTmpData);

    RtpDt_UInt32 uiByte4Data = usTmpData;
    uiByte4Data = uiByte4Data << RTP_SIXTEEN;

    // convert m_usLength into words - 1
    m_usLength = m_usLength / RTP_WORD_SIZE;
    m_usLength = m_usLength - RTP_ONE;

    // length 16 bits
    uiByte4Data = uiByte4Data | m_usLength;

    *(RtpDt_UInt32*)pcRtcpHdrBuf = RtpOsUtil::Ntohl(uiByte4Data);
    pcRtcpHdrBuf = pcRtcpHdrBuf + RTP_WORD_SIZE;

    uiBufPos = uiBufPos + RTP_WORD_SIZE;
    pobjRtcpPktBuf->setLength(uiBufPos);

    return eRTP_SUCCESS;

}  // end formPartialRtcpHeader

RtpDt_Void RtcpHeader::populateRtcpHeader(
        IN RtpDt_UChar ucRecepRepCnt, IN RtpDt_UChar ucPktType, IN RtpDt_UInt32 uiSsrc)
{
    m_ucVersion = RTP_VERSION_NUM;
    m_ucRecepRepCnt = ucRecepRepCnt;
    m_ucPktType = ucPktType;
    m_uiSsrc = uiSsrc;
}
