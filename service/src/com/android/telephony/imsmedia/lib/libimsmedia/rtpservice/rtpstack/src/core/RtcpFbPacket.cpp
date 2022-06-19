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

#include <RtcpFbPacket.h>
#include <RtpTrace.h>
#include <string.h>

RtcpFbPacket::RtcpFbPacket() :
        m_pFCI(RTP_NULL)
{
}

RtcpFbPacket::~RtcpFbPacket()
{
    if (m_pFCI)
    {
        delete (m_pFCI);
        m_pFCI = RTP_NULL;
    }
}

RtcpHeader* RtcpFbPacket::getRtcpHdrInfo()
{
    return &m_objRtcpHdr;
}

RtpDt_Void RtcpFbPacket::setSsrc(RtpDt_UInt32 uiSsrc)
{
    m_objRtcpHdr.setSsrc(uiSsrc);
}

RtpDt_Void RtcpFbPacket::setMediaSsrc(RtpDt_UInt32 uiMediaSsrc)
{
    m_uiMediaSsrc = uiMediaSsrc;
}

RtpDt_UInt32 RtcpFbPacket::getSsrc()
{
    return m_objRtcpHdr.getSsrc();
}

RtpDt_UInt32 RtcpFbPacket::getMediaSsrc()
{
    return m_uiMediaSsrc;
}

RtpBuffer* RtcpFbPacket::getFCI()
{
    return m_pFCI;
}

RtpDt_Void RtcpFbPacket::setFCI(IN RtpBuffer* pFCI)
{
    m_pFCI = pFCI;
}

RtpDt_Void RtcpFbPacket::setPayloadType(IN eRTCP_TYPE ePayloadType)
{
    m_ePayloadType = ePayloadType;
}

eRTCP_TYPE RtcpFbPacket::getPayloadType()
{
    return m_ePayloadType;
}

eRTP_STATUS_CODE RtcpFbPacket::decodeRtcpFbPacket(
        IN RtpDt_UChar* pucRtcpFbBuf, IN RtpDt_UInt16 usRtcpFbLen, IN RtpDt_UChar ucPktType)
{
    RtpDt_UInt32 ufciLength = usRtcpFbLen - RTP_12;  // Exclude Headers length to get FCI Length

    m_objRtcpHdr.setLength(usRtcpFbLen);
    m_objRtcpHdr.setPacketType(ucPktType);

    // get rtcp fb headers
    {
        m_objRtcpHdr.decodeRtcpHeader(pucRtcpFbBuf);
        pucRtcpFbBuf = pucRtcpFbBuf + RTCP_FIXED_HDR_LEN;
    }

    // get media/peer SSRC
    {
        RtpDt_UInt32 uiMediaSsrc = RtpOsUtil::Ntohl(*((RtpDt_UInt32*)pucRtcpFbBuf));
        setMediaSsrc(uiMediaSsrc);
        pucRtcpFbBuf = pucRtcpFbBuf + RTP_WORD_SIZE;
    }

    // get the FCI buffer
    if (ufciLength > 0)
    {
        RtpBuffer* pFCI = new RtpBuffer(ufciLength, (RtpDt_UChar*)pucRtcpFbBuf);
        if (pFCI == RTP_NULL)
        {
            RTP_TRACE_WARNING("decodeRtcpRtpFbPacket, new returned NULL...!", RTP_ZERO, RTP_ZERO);
            return RTP_MEMORY_FAIL;
        }
        setFCI(pFCI);
    }

    return RTP_SUCCESS;
}

eRTP_STATUS_CODE RtcpFbPacket::formRtcpFbPacket(OUT RtpBuffer* pobjRtcpPktBuf)
{
    if (getPayloadType() == RTCP_RTPFB)
    {
        return formRtcpRtpFbPacket(pobjRtcpPktBuf);
    }
    else if (getPayloadType() == RTCP_PSFB)
    {
        return formRtcpPayloadFbPacket(pobjRtcpPktBuf);
    }

    return RTP_FAILURE;
}

eRTP_STATUS_CODE RtcpFbPacket::formRtcpRtpFbPacket(OUT RtpBuffer* pobjRtcpPktBuf)
{
    RtpDt_UInt32 uiFbPktPos = pobjRtcpPktBuf->getLength();
    RtpDt_UInt32 uiCurPos = pobjRtcpPktBuf->getLength();
    RtpDt_UChar* pucBuffer = pobjRtcpPktBuf->getBuffer();

    if (!pucBuffer)
    {
        RTP_TRACE_ERROR("formFbPacket with null buffer", RTP_ZERO, RTP_ZERO);
        return RTP_FAILURE;
    }

    uiCurPos = uiCurPos + RTCP_FIXED_HDR_LEN;
    pucBuffer = pucBuffer + uiCurPos;

    // set the media/peer SSRC
    {
        RtpDt_UInt32 uiMediaSsrc = getMediaSsrc();

        *(RtpDt_UInt32*)pucBuffer = RtpOsUtil::Ntohl(uiMediaSsrc);
        pucBuffer = pucBuffer + RTP_WORD_SIZE;
        uiCurPos = uiCurPos + RTP_WORD_SIZE;
    }

    // set the FCI buffer
    {
        RtpBuffer* pFCI = this->getFCI();

        memcpy(pucBuffer, pFCI->getBuffer(), pFCI->getLength());  // copy buffer not Fci object
        pucBuffer = pucBuffer + pFCI->getLength();
        uiCurPos = uiCurPos + pFCI->getLength();
    }

    // padding
    {
        RtpDt_UInt32 uiFbPktLen = uiCurPos - uiFbPktPos;

#ifdef ENABLE_PADDING
        RtpDt_UInt32 uiPadLen = uiFbPktLen % RTP_WORD_SIZE;
        if (uiPadLen > RTP_ZERO)
        {
            uiPadLen = RTP_WORD_SIZE - uiPadLen;
            uiFbPktLen = uiFbPktLen + uiPadLen;
            uiCurPos = uiCurPos + uiPadLen;
            memset(pucBuffer, RTP_ZERO, uiPadLen);

            pucBuffer = pucBuffer + uiPadLen;
            pucBuffer = pucBuffer - RTP_ONE;
            *(RtpDt_UChar*)pucBuffer = (RtpDt_UChar)uiPadLen;

            // set pad bit in header
            m_objRtcpHdr.setPadding();
            // set length in header
            m_objRtcpHdr.setLength(uiFbPktLen);
        }
        else
#endif
        {
            // set length in header
            m_objRtcpHdr.setLength(uiFbPktLen);
        }

        pobjRtcpPktBuf->setLength(uiFbPktPos);
        m_objRtcpHdr.formRtcpHeader(pobjRtcpPktBuf);
    }  // padding

    // set the current position of the RTCP compound packet
    pobjRtcpPktBuf->setLength(uiCurPos);

    return RTP_SUCCESS;
}  // formFbPacket

eRTP_STATUS_CODE RtcpFbPacket::formRtcpPayloadFbPacket(OUT RtpBuffer* pobjRtcpPktBuf)
{
    RtpDt_UInt32 uiFbPktPos = pobjRtcpPktBuf->getLength();
    RtpDt_UInt32 uiCurPos = pobjRtcpPktBuf->getLength();
    RtpDt_UChar* pucBuffer = pobjRtcpPktBuf->getBuffer();

    if (!pucBuffer)
    {
        RTP_TRACE_ERROR("formFbPacket with null buffer", RTP_ZERO, RTP_ZERO);
        return RTP_FAILURE;
    }

    uiCurPos = uiCurPos + RTCP_FIXED_HDR_LEN;
    pucBuffer = pucBuffer + uiCurPos;

    // set the media/peer SSRC
    {
        RtpDt_UInt32 uiMediaSsrc = getMediaSsrc();

        *(RtpDt_UInt32*)pucBuffer = RtpOsUtil::Ntohl(uiMediaSsrc);
        pucBuffer = pucBuffer + RTP_WORD_SIZE;
        uiCurPos = uiCurPos + RTP_WORD_SIZE;
    }

    // set the FCI buffer
    {
        RtpBuffer* pFCI = this->getFCI();

        memcpy(pucBuffer, pFCI, pFCI->getLength());
        pucBuffer = pucBuffer + pFCI->getLength();
        uiCurPos = uiCurPos + pFCI->getLength();
    }

    // padding
    {
        RtpDt_UInt32 uiFbPktLen = uiCurPos - uiFbPktPos;
        RtpDt_UInt32 uiPadLen = RTP_ZERO;

        uiPadLen = uiFbPktLen % RTP_WORD_SIZE;
        if (uiPadLen > RTP_ZERO)
        {
            uiPadLen = RTP_WORD_SIZE - uiPadLen;
            uiFbPktLen = uiFbPktLen + uiPadLen;
            uiCurPos = uiCurPos + uiPadLen;
            memset(pucBuffer, RTP_ZERO, uiPadLen);

            pucBuffer = pucBuffer + uiPadLen;
            pucBuffer = pucBuffer - RTP_ONE;
            *(RtpDt_UChar*)pucBuffer = (RtpDt_UChar)uiPadLen;

            // set pad bit in header
            m_objRtcpHdr.setPadding();
            // set length in header
            m_objRtcpHdr.setLength(uiFbPktLen);
        }
        else
        {
            // set length in header
            m_objRtcpHdr.setLength(uiFbPktLen);
        }

        pobjRtcpPktBuf->setLength(uiFbPktPos);
        m_objRtcpHdr.formRtcpHeader(pobjRtcpPktBuf);
    }  // padding

    // set the current position of the RTCP compound packet
    pobjRtcpPktBuf->setLength(uiCurPos);

    return RTP_SUCCESS;
}  // formRtcpPayloadFbPacket
