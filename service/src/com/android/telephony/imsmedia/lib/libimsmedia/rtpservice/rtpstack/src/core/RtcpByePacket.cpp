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
#include <rtp_trace.h>
#include <rtp_pf_memory.h>

eRtp_Bool Rtp_ReleaseSsrcList(IN RtpDt_Void **ppObj)
{
    RtpDt_UInt32 *puiSsrcElm = RTP_NULL;
    puiSsrcElm = RTP_STATIC_CAST(RtpDt_UInt32*,*ppObj);
    delete puiSsrcElm;
    return eRTP_SUCCESS;
}

eRtp_Bool Rtp_CompareSsrcElm(IN RtpDt_Void *pvNode1,
                                     IN RtpDt_Void *pvNode2)
{
    if(*(RTP_STATIC_CAST(RtpDt_UInt32*,pvNode1)) != RTP_NULL
        && *(RTP_STATIC_CAST(RtpDt_UInt32*,pvNode2)) != RTP_NULL)
    {
        if(*(RTP_STATIC_CAST(RtpDt_UInt32*,pvNode1)) == *(RTP_STATIC_CAST(RtpDt_UInt32*,pvNode2)))
            return eRTP_TRUE;
        else
            return eRTP_FALSE;
    }
    else
    {
        return eRTP_FALSE;
    }
}

RtcpByePacket::RtcpByePacket():
                    m_pReason(RTP_NULL)
{
    //initialize the report block list
    m_objSsrcList.InitList(Rtp_ReleaseSsrcList,
                            Rtp_CompareSsrcElm);
}//Constructor

RtcpByePacket::~RtcpByePacket()
{
    RtpDt_UInt16 usSize = RTP_ZERO;
    RtpDt_UInt16 usError = RTP_ZERO;

    //m_objReportBlk
    m_objSsrcList.GetSize(&usSize, &usError);
    for(RtpDt_UInt16 usCount = RTP_ZERO;usCount < usSize;
        usCount = usCount + RTP_ONE)
    {
        m_objSsrcList.DeleteAtPos(RTP_ZERO, &usError);
    }//for

    //m_pReason
    if(m_pReason != RTP_NULL)
    {
        delete m_pReason;
        m_pReason = RTP_NULL;
    }
}//Destructor

RtcpHeader* RtcpByePacket::getRtcpHdrInfo()
{
    return &m_objRtcpHdr;
}

RtpList* RtcpByePacket::getSsrcList()
{
    return &m_objSsrcList;
}

RtpBuffer* RtcpByePacket::getReason()
{
    return m_pReason;
}

RtpDt_Void RtcpByePacket::setReason(IN RtpBuffer* pobjReason)
{
    m_pReason = pobjReason;
}

eRTP_STATUS_CODE RtcpByePacket::decodeByePacket(IN RtpDt_UChar* pucByeBuf,
                                             IN RtpDt_UInt16 usByeLen)
{
    m_objRtcpHdr.setLength(usByeLen);
    m_objRtcpHdr.setPacketType((RtpDt_UChar)RTCP_BYE);

    // m_objRtcpHdr
    m_objRtcpHdr.decodeRtcpHeader(pucByeBuf);
    pucByeBuf = pucByeBuf + RTCP_FIXED_HDR_LEN;

    RtpDt_UChar ucSsrcCnt = m_objRtcpHdr.getRecepRepCnt();
    // m_objSsrcList
    while(ucSsrcCnt > RTP_ONE)
    {
        RtpDt_UInt16    usError;
        RtpDt_UInt32 *puiRcvdSsrc = RTP_NULL;
        puiRcvdSsrc = new RtpDt_UInt32();
        if(puiRcvdSsrc == RTP_NULL)
        {
            RTP_TRACE_WARNING("decodeByePacket, new returned NULL...!",
                RTP_ZERO,RTP_ZERO);
            return RTP_MEMORY_FAIL;
        }

        (*puiRcvdSsrc) = RtpOsUtil::Ntohl(*((RtpDt_UInt32*)pucByeBuf));
        pucByeBuf = pucByeBuf + RTP_WORD_SIZE;

        m_objSsrcList.Append(puiRcvdSsrc, &usError);
        ucSsrcCnt = ucSsrcCnt - RTP_ONE;
    }//while

    // m_pReason
    RtpDt_UInt32 uiByte4Data = RtpOsUtil::Ntohl(*((RtpDt_UInt32*)pucByeBuf));
    pucByeBuf = pucByeBuf + RTP_ONE;
    uiByte4Data = uiByte4Data >> RTP_24;
    if(uiByte4Data > RTP_ZERO)
    {
        RtpDt_UChar *pucReason = new RtpDt_UChar[uiByte4Data];
        if(pucReason == RTP_NULL)
        {
            RTP_TRACE_WARNING("decodeByePacket, new returned NULL...!",
                RTP_ZERO,RTP_ZERO);
            return RTP_MEMORY_FAIL;
        }

        m_pReason = new RtpBuffer();
        if(m_pReason == RTP_NULL)
        {
            RTP_TRACE_WARNING("decodeByePacket, new returned NULL...!",
                RTP_ZERO,RTP_ZERO);
            delete[] pucReason;
            return RTP_MEMORY_FAIL;
        }
        RtpPf_Memset(pucReason, RTP_ZERO, uiByte4Data);
        RtpPf_Memcpy(pucReason, pucByeBuf, uiByte4Data);
        m_pReason->setBufferInfo(uiByte4Data, pucReason);
    }//if


    return RTP_SUCCESS;
}//decodeByePacket

eRTP_STATUS_CODE RtcpByePacket::formByePacket(OUT RtpBuffer* pobjRtcpPktBuf)
{
    RtpDt_UInt32 uiCurPos = pobjRtcpPktBuf->getLength();
    RtpDt_UChar *pucBuffer = pobjRtcpPktBuf->getBuffer();

    uiCurPos = uiCurPos + RTCP_FIXED_HDR_LEN;
    pucBuffer = pucBuffer + uiCurPos;

    //m_objSsrcList
    RtpDt_UInt16 usSize = RTP_ZERO;
    RtpDt_UInt16 usError = RTP_ZERO;

    m_objSsrcList.GetSize(&usSize, &usError);
    for(RtpDt_UInt32 uiCount = RTP_ZERO;uiCount < usSize;
        uiCount = uiCount + RTP_ONE)
    {
        RtpDt_Void    *pvElement = RTP_NULL;
        RtpDt_UInt32 *puiSsrc = RTP_NULL;
        m_objSsrcList.GetElement(uiCount,&pvElement, &usError);
        puiSsrc = RTP_STATIC_CAST(RtpDt_UInt32*,pvElement);

        //ssrc
        *(RtpDt_UInt32*)pucBuffer = RtpOsUtil::Ntohl(*puiSsrc);
        pucBuffer = pucBuffer + RTP_WORD_SIZE;
        uiCurPos = uiCurPos + RTP_WORD_SIZE;
    }//for

    //m_pReason
    if(m_pReason != RTP_NULL)
    {
        //length
        *(RtpDt_UChar*)pucBuffer = (RtpDt_UChar)m_pReason->getLength();
        pucBuffer = pucBuffer + RTP_ONE;
        uiCurPos = uiCurPos + RTP_ONE;

        RtpPf_Memcpy(pucBuffer, m_pReason->getBuffer(), m_pReason->getLength());
        pucBuffer = pucBuffer + m_pReason->getLength();
        uiCurPos = uiCurPos + m_pReason->getLength();
    }

    // padding
    {
        RtpDt_UInt32 uiByePktPos = pobjRtcpPktBuf->getLength();
        RtpDt_UInt32 uiByePktLen = uiCurPos - uiByePktPos;

#ifdef ENABLE_PADDING
        RtpDt_UInt32 uiPadLen = uiByePktLen % RTP_WORD_SIZE;
        if(uiPadLen > RTP_ZERO)
        {
            uiPadLen = RTP_WORD_SIZE - uiPadLen;
            uiByePktLen = uiByePktLen + uiPadLen;
            uiCurPos = uiCurPos + uiPadLen;
            RtpPf_Memset(pucBuffer, RTP_ZERO, uiPadLen);

            pucBuffer = pucBuffer + uiPadLen;
            pucBuffer = pucBuffer - RTP_ONE;
            *(RtpDt_UChar*)pucBuffer = (RtpDt_UChar)uiPadLen;

            //set pad bit in header
            m_objRtcpHdr.setPadding();
            //set length in header
            m_objRtcpHdr.setLength(uiByePktLen);
        }
        else
#endif
        {
            //set length in header
            m_objRtcpHdr.setLength(uiByePktLen);
        }

        pobjRtcpPktBuf->setLength(uiByePktPos);
        m_objRtcpHdr.formRtcpHeader(pobjRtcpPktBuf);
    }//padding

    //set the current position of the RTCP compound packet
    pobjRtcpPktBuf->setLength(uiCurPos);


    return RTP_SUCCESS;
}//formByePacket
