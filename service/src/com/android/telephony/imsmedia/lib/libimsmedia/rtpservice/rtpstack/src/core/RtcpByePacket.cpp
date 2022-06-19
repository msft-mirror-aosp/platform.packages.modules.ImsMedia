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

#include <RtcpByePacket.h>
#include <RtpTrace.h>

RtcpByePacket::RtcpByePacket() :
        m_uiSsrcList(std::list<RtpDt_UInt32*>()),
        m_pReason(RTP_NULL)
{
}  // Constructor

RtcpByePacket::~RtcpByePacket()
{
    for (auto& puiSsrc : m_uiSsrcList)
    {
        delete puiSsrc;
    }
    m_uiSsrcList.clear();

    // m_pReason
    if (m_pReason != RTP_NULL)
    {
        delete m_pReason;
        m_pReason = RTP_NULL;
    }
}  // Destructor

RtcpHeader* RtcpByePacket::getRtcpHdrInfo()
{
    return &m_objRtcpHdr;
}

std::list<RtpDt_UInt32*>& RtcpByePacket::getSsrcList()
{
    return m_uiSsrcList;
}

RtpBuffer* RtcpByePacket::getReason()
{
    return m_pReason;
}

RtpDt_Void RtcpByePacket::setReason(IN RtpBuffer* pobjReason)
{
    m_pReason = pobjReason;
}

eRTP_STATUS_CODE RtcpByePacket::decodeByePacket(IN RtpDt_UChar* pucByeBuf, IN RtpDt_UInt16 usByeLen)
{
    m_objRtcpHdr.setLength(usByeLen);
    m_objRtcpHdr.setPacketType((RtpDt_UChar)RTCP_BYE);

    // m_objRtcpHdr
    m_objRtcpHdr.decodeRtcpHeader(pucByeBuf);
    pucByeBuf = pucByeBuf + RTCP_FIXED_HDR_LEN;

    RtpDt_UChar ucSsrcCnt = m_objRtcpHdr.getRecepRepCnt();
    // m_uiSsrcList
    while (ucSsrcCnt > RTP_ONE)
    {
        RtpDt_UInt32* puiRcvdSsrc = RTP_NULL;
        puiRcvdSsrc = new RtpDt_UInt32();
        if (puiRcvdSsrc == RTP_NULL)
        {
            RTP_TRACE_WARNING("decodeByePacket, new returned NULL...!", RTP_ZERO, RTP_ZERO);
            return RTP_MEMORY_FAIL;
        }

        (*puiRcvdSsrc) = RtpOsUtil::Ntohl(*((RtpDt_UInt32*)pucByeBuf));
        pucByeBuf = pucByeBuf + RTP_WORD_SIZE;

        m_uiSsrcList.push_back(puiRcvdSsrc);
        ucSsrcCnt = ucSsrcCnt - RTP_ONE;
    }  // while

    // m_pReason
    RtpDt_UInt32 uiByte4Data = RtpOsUtil::Ntohl(*((RtpDt_UInt32*)pucByeBuf));
    pucByeBuf = pucByeBuf + RTP_ONE;
    uiByte4Data = uiByte4Data >> RTP_24;
    if (uiByte4Data > RTP_ZERO)
    {
        RtpDt_UChar* pucReason = new RtpDt_UChar[uiByte4Data];
        if (pucReason == RTP_NULL)
        {
            RTP_TRACE_WARNING("decodeByePacket, new returned NULL...!", RTP_ZERO, RTP_ZERO);
            return RTP_MEMORY_FAIL;
        }

        m_pReason = new RtpBuffer();
        if (m_pReason == RTP_NULL)
        {
            RTP_TRACE_WARNING("decodeByePacket, new returned NULL...!", RTP_ZERO, RTP_ZERO);
            delete[] pucReason;
            return RTP_MEMORY_FAIL;
        }
        memset(pucReason, RTP_ZERO, uiByte4Data);
        memcpy(pucReason, pucByeBuf, uiByte4Data);
        m_pReason->setBufferInfo(uiByte4Data, pucReason);
    }  // if

    return RTP_SUCCESS;
}  // decodeByePacket

eRTP_STATUS_CODE RtcpByePacket::formByePacket(OUT RtpBuffer* pobjRtcpPktBuf)
{
    RtpDt_UInt32 uiCurPos = pobjRtcpPktBuf->getLength();
    RtpDt_UChar* pucBuffer = pobjRtcpPktBuf->getBuffer();

    uiCurPos = uiCurPos + RTCP_FIXED_HDR_LEN;
    pucBuffer = pucBuffer + uiCurPos;

    for (auto& puiSsrc : m_uiSsrcList)
    {
        // ssrc
        *(RtpDt_UInt32*)pucBuffer = RtpOsUtil::Ntohl(*puiSsrc);
        pucBuffer = pucBuffer + RTP_WORD_SIZE;
        uiCurPos = uiCurPos + RTP_WORD_SIZE;
    }

    // m_pReason
    if (m_pReason != RTP_NULL)
    {
        // length
        *(RtpDt_UChar*)pucBuffer = (RtpDt_UChar)m_pReason->getLength();
        pucBuffer = pucBuffer + RTP_ONE;
        uiCurPos = uiCurPos + RTP_ONE;

        memcpy(pucBuffer, m_pReason->getBuffer(), m_pReason->getLength());
        pucBuffer = pucBuffer + m_pReason->getLength();
        uiCurPos = uiCurPos + m_pReason->getLength();
    }

    // padding
    {
        RtpDt_UInt32 uiByePktPos = pobjRtcpPktBuf->getLength();
        RtpDt_UInt32 uiByePktLen = uiCurPos - uiByePktPos;

#ifdef ENABLE_PADDING
        RtpDt_UInt32 uiPadLen = uiByePktLen % RTP_WORD_SIZE;
        if (uiPadLen > RTP_ZERO)
        {
            uiPadLen = RTP_WORD_SIZE - uiPadLen;
            uiByePktLen = uiByePktLen + uiPadLen;
            uiCurPos = uiCurPos + uiPadLen;
            memset(pucBuffer, RTP_ZERO, uiPadLen);

            pucBuffer = pucBuffer + uiPadLen;
            pucBuffer = pucBuffer - RTP_ONE;
            *(RtpDt_UChar*)pucBuffer = (RtpDt_UChar)uiPadLen;

            // set pad bit in header
            m_objRtcpHdr.setPadding();
            // set length in header
            m_objRtcpHdr.setLength(uiByePktLen);
        }
        else
#endif
        {
            // set length in header
            m_objRtcpHdr.setLength(uiByePktLen);
        }

        pobjRtcpPktBuf->setLength(uiByePktPos);
        m_objRtcpHdr.formRtcpHeader(pobjRtcpPktBuf);
    }  // padding

    // set the current position of the RTCP compound packet
    pobjRtcpPktBuf->setLength(uiCurPos);

    return RTP_SUCCESS;
}  // formByePacket
