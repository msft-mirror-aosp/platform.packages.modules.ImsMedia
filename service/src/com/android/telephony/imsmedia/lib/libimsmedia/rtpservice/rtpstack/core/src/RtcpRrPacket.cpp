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

#include <RtcpRrPacket.h>
#include <RtcpReportBlock.h>
#include <rtp_trace.h>
#include <rtp_pf_memory.h>

eRtp_Bool Rtp_ReleaseReportBlk(IN RtpDt_Void **ppObj)
{
    RtcpReportBlock *pobjRBElm = RTP_NULL;
    pobjRBElm = RTP_STATIC_CAST(RtcpReportBlock*,*ppObj);
    delete pobjRBElm;
    return eRTP_SUCCESS;
}

eRtp_Bool Rtp_CompareReportBlkElm(IN RtpDt_Void *pvNode1,
                                     IN RtpDt_Void *pvNode2)
{
    if((RTP_STATIC_CAST(RtcpReportBlock *,pvNode1))->getSsrc() != RTP_NULL
        && (RTP_STATIC_CAST(RtcpReportBlock *,pvNode2))->getSsrc() != RTP_NULL)
    {
        if((RTP_STATIC_CAST(RtcpReportBlock *,pvNode1))->getSsrc() ==
            (RTP_STATIC_CAST(RtcpReportBlock *,pvNode2))->getSsrc())
            return eRTP_TRUE;
        else
            return eRTP_FALSE;
    }
    else
       return eRTP_FALSE;
}

RtcpRrPacket::RtcpRrPacket():
                        m_pobjExt(RTP_NULL)
{
    //initialize the report block list

    m_objReportBlk.InitList(Rtp_ReleaseReportBlk,
                            Rtp_CompareReportBlkElm);
}

RtcpRrPacket::~RtcpRrPacket()
{
    RtpDt_UInt16 usSize = RTP_ZERO;
    RtpDt_UInt16 usError = RTP_ZERO;

    //m_objReportBlk
    m_objReportBlk.GetSize(&usSize, &usError);
    for(RtpDt_UInt16 usCount = RTP_ZERO;usCount < usSize;
        usCount = usCount + RTP_ONE)
    {
        m_objReportBlk.DeleteAtPos(RTP_ZERO, &usError);
    }

    //m_pobjExt
    if(m_pobjExt != RTP_NULL)
    {
        delete m_pobjExt;
        m_pobjExt = RTP_NULL;
    }
}

RtpDt_Void RtcpRrPacket::addReportBlkElm(IN RtcpReportBlock* pobjReptBlk)
{
    RtpDt_UInt16 usTmpError = RTP_ZERO;
    m_objReportBlk.Append(pobjReptBlk, &usTmpError);
}

RtcpHeader* RtcpRrPacket::getRtcpHdrInfo()
{
    return &m_objRtcpHdr;
}

RtpList* RtcpRrPacket::getReportBlkList()
{
    return &m_objReportBlk;
}

RtpBuffer* RtcpRrPacket::getExtHdrInfo()
{
    return m_pobjExt;
}

RtpDt_Void RtcpRrPacket::setExtHdrInfo(IN RtpBuffer* pobjExtHdr)
{
    m_pobjExt = pobjExtHdr;
}

eRTP_STATUS_CODE RtcpRrPacket::decodeRrPacket(IN RtpDt_UChar* pucRrBuf,
                           IN RtpDt_UInt16 &usRrLen,
                           IN RtpDt_UInt16 usProfExtLen,
                           IN eRtp_Bool bIsRrPkt)
{
    //check the received data is a report block or RR packet.
    if(bIsRrPkt == eRTP_TRUE)
    {
        m_objRtcpHdr.setLength(usRrLen);
        m_objRtcpHdr.setPacketType((RtpDt_UChar)RTCP_RR);

        m_objRtcpHdr.decodeRtcpHeader(pucRrBuf);
        pucRrBuf = pucRrBuf + RTP_EIGHT;
        usRrLen = usRrLen - RTP_EIGHT;
    }

    RtpDt_UInt16 usRepBlkLen = usRrLen - usProfExtLen;
    while(usRepBlkLen >= RTP_24)
    {
        RtpDt_UInt16    usError;
        RtcpReportBlock *pobjRptBlk = new RtcpReportBlock();
        if(pobjRptBlk == RTP_NULL)
        {
            RTP_TRACE_WARNING("decodeRrPacket, new returned NULL...!",
                RTP_ZERO,RTP_ZERO);
            return RTP_MEMORY_FAIL;
        }
        pobjRptBlk->decodeReportBlk(pucRrBuf);
        pucRrBuf = pucRrBuf + RTP_24;
        usRepBlkLen = usRepBlkLen - RTP_24;
        m_objReportBlk.Append(pobjRptBlk, &usError);
    }

    //profile specific extensions
    if(usProfExtLen > RTP_ZERO)
    {
        RtpDt_UChar *pcProfExtBuf = new RtpDt_UChar[usProfExtLen];
        if(pcProfExtBuf == RTP_NULL)
        {
            RTP_TRACE_WARNING("decodeRrPacket, new returned NULL...!",
                RTP_ZERO,RTP_ZERO);
            return RTP_MEMORY_FAIL;
        }

        m_pobjExt = new RtpBuffer();
        if(m_pobjExt == RTP_NULL)
        {
            RTP_TRACE_WARNING("decodeRrPacket, new returned NULL...!",
                RTP_ZERO,RTP_ZERO);
            delete[] pcProfExtBuf;
            return RTP_MEMORY_FAIL;
        }

        RtpPf_Memcpy(pcProfExtBuf, pucRrBuf, usProfExtLen);
        m_pobjExt->setBufferInfo(usProfExtLen, pcProfExtBuf);
    }

    return RTP_SUCCESS;
}//decodeRrPacket

eRTP_STATUS_CODE RtcpRrPacket::formRrPacket(OUT RtpBuffer* pobjRtcpPktBuf,
                                         IN eRtp_Bool bHdrInfo)
{
    RtpDt_UInt32 uiRtPktPos = pobjRtcpPktBuf->getLength();

    if(bHdrInfo == RTP_TRUE)
    {
        RtpDt_UInt32 uiRepBlkPos = uiRtPktPos + RTP_EIGHT;
        pobjRtcpPktBuf->setLength(uiRepBlkPos);
    }

    RtpDt_UInt16 usSize = RTP_ZERO;
    RtpDt_UInt16 usError = RTP_ZERO;

    //m_objReportBlk
    m_objReportBlk.GetSize(&usSize, &usError);
    for(RtpDt_UInt32 uiCount = RTP_ZERO;uiCount < usSize;
        uiCount = uiCount + RTP_ONE)
    {
        RtpDt_Void    *pvElement = RTP_NULL;
        RtcpReportBlock *pobjRepBlk = RTP_NULL;
        m_objReportBlk.GetElement(uiCount,&pvElement, &usError);
        pobjRepBlk = RTP_STATIC_CAST(RtcpReportBlock*,pvElement);
        pobjRepBlk->formReportBlk(pobjRtcpPktBuf);
    }//for

    RtpDt_UChar *pucBuffer = RTP_NULL;

    RtpDt_UInt32 uiCurPos = pobjRtcpPktBuf->getLength();
    pucBuffer = pobjRtcpPktBuf->getBuffer();
#ifdef ENABLE_RTCPEXT
    if(m_pobjExt != RTP_NULL)
    {
        RtpDt_UChar *pucExtHdr = m_pobjExt->getBuffer();
        RtpDt_UInt32 uiExtHdrLen = m_pobjExt->getLength();
        RtpPf_Memcpy(pucBuffer+uiCurPos, pucExtHdr, uiExtHdrLen);
        uiCurPos = uiCurPos + uiExtHdrLen;
        pobjRtcpPktBuf->setLength(uiCurPos);
    } // extension header
#endif
    pucBuffer = pucBuffer + uiCurPos;
    if(bHdrInfo == RTP_TRUE)
    {
        RtpDt_UInt32 uiRrPktLen = uiCurPos - uiRtPktPos;

#ifdef ENABLE_PADDING
        RtpDt_UInt32 uiPadLen = uiRrPktLen % RTP_WORD_SIZE;
        if(uiPadLen > RTP_ZERO)
        {
            uiPadLen = RTP_WORD_SIZE - uiPadLen;
            uiRrPktLen = uiRrPktLen + uiPadLen;
            uiCurPos = uiCurPos + uiPadLen;
            RtpPf_Memset(pucBuffer, RTP_ZERO, uiPadLen);

            pucBuffer = pucBuffer + uiPadLen;
            pucBuffer = pucBuffer - RTP_ONE;
            *(RtpDt_UChar*)pucBuffer = (RtpDt_UChar)uiPadLen;

            //set pad bit in header
            m_objRtcpHdr.setPadding();
            //set length in header
            m_objRtcpHdr.setLength(uiRrPktLen);
        }
        else
#endif
        {
            //set length in header
            m_objRtcpHdr.setLength(uiRrPktLen);
        }

        pobjRtcpPktBuf->setLength(uiRtPktPos);
        m_objRtcpHdr.formRtcpHeader(pobjRtcpPktBuf);
    }
    //set the actual position of the RTCP compound packet
    pobjRtcpPktBuf->setLength(uiCurPos);

    return RTP_SUCCESS;
} //formRrPacket
