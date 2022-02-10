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

#include <RtcpSdesPacket.h>
#include <RtcpChunk.h>
#include <rtp_trace.h>
#include <rtp_pf_memory.h>

eRtp_Bool Rtp_ReleaseSDesChunk(IN RtpDt_Void **ppObj)
{
    RtcpChunk *pobjChunkElm = RTP_NULL;
    pobjChunkElm = RTP_STATIC_CAST(RtcpChunk*,*ppObj);
    delete pobjChunkElm;
    return eRTP_SUCCESS;
}

eRtp_Bool Rtp_CompareSDesChunk(IN RtpDt_Void *pvNode1,
                                     IN RtpDt_Void *pvNode2)
{
    if((RTP_STATIC_CAST(RtcpChunk *,pvNode1))->getSsrc() != RTP_NULL
        && (RTP_STATIC_CAST(RtcpChunk *,pvNode2))->getSsrc() != RTP_NULL)
    {
        if((RTP_STATIC_CAST(RtcpChunk *,pvNode1))->getSsrc() ==
        (RTP_STATIC_CAST(RtcpChunk *,pvNode2))->getSsrc())
            return eRTP_TRUE;
        else
            return eRTP_FALSE;
    }
    else
       return eRTP_FALSE;
}

RtcpSdesPacket::RtcpSdesPacket()
{
    //initialize the SDES chunk list

    m_objSdesChunk.InitList(Rtp_ReleaseSDesChunk,
                            Rtp_CompareSDesChunk);
}

RtcpSdesPacket::~RtcpSdesPacket()
{
    RtpDt_UInt16 usSize = RTP_ZERO;
    RtpDt_UInt16 usError = RTP_ZERO;

    //m_objReportBlk
    m_objSdesChunk.GetSize(&usSize, &usError);
    for(RtpDt_UInt16 usCount = RTP_ZERO;usCount < usSize;
        usCount = usCount + RTP_ONE)
    {
        m_objSdesChunk.DeleteAtPos(RTP_ZERO, &usError);
    }
}

RtcpHeader* RtcpSdesPacket::getRtcpHdrInfo()
{
    return &m_objRtcpHdr;
}

RtpList* RtcpSdesPacket::getSdesChunkList()
{
    return &m_objSdesChunk;
}

eRTP_STATUS_CODE RtcpSdesPacket::decodeSdesPacket(IN RtpDt_UChar* pucSdesBuf,
                                                IN RtpDt_UInt16 usSdesLen,
                                                IN RtcpConfigInfo *pobjRtcpCfgInfo)
{
    eRTCP_TYPE eRtcpPktType = RTCP_SDES;

    m_objRtcpHdr.setLength(usSdesLen);
    m_objRtcpHdr.setPacketType((RtpDt_UChar)eRtcpPktType);

    RtpDt_UInt32 uiTemp4Data = RtpOsUtil::Ntohl(*((RtpDt_UInt32*)pucSdesBuf));
    pucSdesBuf = pucSdesBuf + RTP_WORD_SIZE;
    usSdesLen = usSdesLen - RTP_WORD_SIZE;

    uiTemp4Data = uiTemp4Data >> RTP_24;
    //version
    RtpDt_UChar ucByteData = (uiTemp4Data & 0x000000C0) >> RTP_SIX;
    m_objRtcpHdr.setVersion(ucByteData);

    //padding
    ucByteData = (uiTemp4Data & 0x00000020) >> RTP_FIVE;
    if(ucByteData != RTP_ZERO)
    {
        m_objRtcpHdr.setPadding();
    }

    //RC
    ucByteData = uiTemp4Data & 0x0000001F;
    m_objRtcpHdr.setRecepRepCnt(ucByteData);
    while((ucByteData > RTP_ZERO) && (usSdesLen > RTP_ZERO))
    {

        RtcpChunk *pobjChunk = new RtcpChunk();
        if(pobjChunk == RTP_NULL)
        {
            RTP_TRACE_WARNING("decodeSdesPacket, new returned NULL...!",
                RTP_ZERO,RTP_ZERO);
            return RTP_MEMORY_FAIL;
        }

        RtpDt_UInt16    usError;
        RtpDt_UInt16 usChunkSize = RTP_NULL;
        eRTP_STATUS_CODE eChuStatus = RTP_FAILURE;

        eChuStatus = pobjChunk->decodeRtcpChunk(pucSdesBuf, usChunkSize, pobjRtcpCfgInfo);
        m_objSdesChunk.Append(pobjChunk, &usError);
        if(eChuStatus != RTP_SUCCESS)
        {
            return eChuStatus;
        }

        RtpDt_UInt32 uiPadLen = RTP_ZERO;
        uiPadLen = usChunkSize % RTP_WORD_SIZE;

        if(uiPadLen != RTP_ZERO)
        {
            uiPadLen =     RTP_WORD_SIZE - uiPadLen;
            usChunkSize = usChunkSize + uiPadLen;
        }
        pucSdesBuf = pucSdesBuf + usChunkSize;
        usSdesLen = usSdesLen - usChunkSize;
        ucByteData = ucByteData - RTP_ONE;
    }

    return RTP_SUCCESS;
}//decodeSdesPacket

eRTP_STATUS_CODE RtcpSdesPacket::formSdesPacket(OUT RtpBuffer* pobjRtcpPktBuf)
{
    RtpDt_UInt32 uiSdesPktPos = pobjRtcpPktBuf->getLength();

    RtpDt_UInt32 uiCurPos = uiSdesPktPos;
    uiCurPos = uiCurPos + RTP_WORD_SIZE;

    // SDES packet does not have SSRC in header.
    pobjRtcpPktBuf->setLength(uiCurPos);

    //m_objSdesChunk
    RtpDt_UInt16 usError = RTP_ZERO;
    RtpDt_UInt16 usSize = RTP_ZERO;
    m_objSdesChunk.GetSize(&usSize, &usError);

    RtpDt_UChar *pucBuffer =  RTP_NULL;
    for(RtpDt_UInt32 uiCount = RTP_ZERO;uiCount < usSize;
        uiCount = uiCount + RTP_ONE)
    {
        RtpDt_Void    *pvElement = RTP_NULL;
        RtcpChunk *pobjChunk = RTP_NULL;
        eRTP_STATUS_CODE eChunkSta = RTP_FAILURE;

        m_objSdesChunk.GetElement(uiCount,&pvElement, &usError);
        pobjChunk = RTP_STATIC_CAST(RtcpChunk*,pvElement);
        eChunkSta = pobjChunk->formRtcpChunk(pobjRtcpPktBuf);

        if(eChunkSta != RTP_SUCCESS)
        {
            return eChunkSta;
        }

        RtpDt_UInt32 uiSdesPktLen = RTP_ZERO;

        uiCurPos = pobjRtcpPktBuf->getLength();
        pucBuffer = pobjRtcpPktBuf->getBuffer();
        pucBuffer = pucBuffer + uiCurPos;

        uiSdesPktLen = uiCurPos - uiSdesPktPos;
#ifdef ENABLE_PADDING
        RtpDt_UInt32 uiPadLen = uiSdesPktLen % RTP_WORD_SIZE;

        if(uiPadLen > RTP_ZERO)
        {
            uiPadLen = RTP_WORD_SIZE - uiPadLen;
            uiSdesPktLen = uiSdesPktLen + uiPadLen;
            uiCurPos = uiCurPos + uiPadLen;
            RtpPf_Memset(pucBuffer, RTP_ZERO, uiPadLen);
        }
#endif
        m_objRtcpHdr.setLength(uiSdesPktLen);
    }//for

    pobjRtcpPktBuf->setLength(uiSdesPktPos);
    m_objRtcpHdr.formPartialRtcpHeader(pobjRtcpPktBuf);

    RTP_TRACE_NORMAL("formSdesPacket, [SDES packet length] : %d]",
                        m_objRtcpHdr.getLength(), RTP_NULL);

    //set the actual position of the RTCP compound packet
    pobjRtcpPktBuf->setLength(uiCurPos);

    return RTP_SUCCESS;
}//formSdesPacket
