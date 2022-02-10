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

#include <RtcpPacket.h>
#include <rtp_trace.h>
#include <rtp_error.h>
#include <RtpSession.h>

eRtp_Bool Rtp_FreeSRElm(IN RtpDt_Void **ppObj)
{
    RtcpSrPacket *pobjSrPkt = RTP_STATIC_CAST(RtcpSrPacket*,*ppObj);
    delete pobjSrPkt;

    return eRTP_TRUE;
}

eRtp_Bool Rtp_CompareRtpSRElm(IN RtpDt_Void *pvNode1,
                                     IN RtpDt_Void *pvNode2)
{
    (RtpDt_Void)pvNode1, (RtpDt_Void)pvNode2;
    return eRTP_TRUE;
}

eRtp_Bool Rtp_FreeRRElm(IN RtpDt_Void **ppObj)
{
    RtcpRrPacket *pobjRrPkt = RTP_STATIC_CAST(RtcpRrPacket*,*ppObj);
    delete pobjRrPkt;

    return eRTP_TRUE;
}

eRtp_Bool Rtp_CompareRtpRRElm(IN RtpDt_Void *pvNode1,
                                     IN RtpDt_Void *pvNode2)
{
    (RtpDt_Void)pvNode1, (RtpDt_Void)pvNode2;

    return eRTP_TRUE;
}

eRtp_Bool Rtp_FreeFBElm(IN RtpDt_Void **ppObj)
{

    RtcpFbPacket *pobjFbPkt = RTP_STATIC_CAST(RtcpFbPacket*,*ppObj);
    delete pobjFbPkt;

    return eRTP_TRUE;
}

eRtp_Bool Rtp_CompareRtpFBElm(IN RtpDt_Void *pvNode1,
                                     IN RtpDt_Void *pvNode2)
{

    //remove warnings
    (RtpDt_Void)pvNode1;
    (RtpDt_Void)pvNode2;

    return eRTP_TRUE;
}

RtcpPacket::RtcpPacket():
                    m_pobjSdesPkt(RTP_NULL),
                    m_pobjByePkt(RTP_NULL),
                    m_pobjAppPkt(RTP_NULL),
                    m_pobjRtcpFbPkt(RTP_NULL),
                    m_pobjRtcpXrPkt(RTP_NULL)

{
    m_objSrPkt.InitList(Rtp_FreeSRElm,Rtp_CompareRtpSRElm);
    m_objRrPkt.InitList(Rtp_FreeRRElm, Rtp_CompareRtpRRElm);
    m_objFbPkt.InitList(Rtp_FreeFBElm, Rtp_CompareRtpFBElm);
}// Constructor


RtcpPacket::~RtcpPacket()
{
    RtpDt_UInt16 usSize = RTP_ZERO;
    RtpDt_UInt16 usError = RTP_ZERO;

    //delete all RtcpSrPacket objects.
    m_objSrPkt.GetSize(&usSize, &usError);
    for(RtpDt_UInt16 usCount = RTP_ZERO;usCount < usSize;
        usCount = usCount + RTP_ONE)
    {
        m_objSrPkt.DeleteAtPos(RTP_ZERO, &usError);
    }

    //delete all RtcpRrPacket objects
    m_objRrPkt.GetSize(&usSize, &usError);
    for(RtpDt_UInt16 usCount = RTP_ZERO;usCount < usSize;
        usCount = usCount + RTP_ONE)
    {
        m_objRrPkt.DeleteAtPos(RTP_ZERO, &usError);
    }

    //delete all RtcpFbPacket objects
    m_objFbPkt.GetSize(&usSize, &usError);
    for(RtpDt_UInt16 usCount = RTP_ZERO;usCount < usSize;
        usCount = usCount + RTP_ONE)
    {
        m_objFbPkt.DeleteAtPos(RTP_ZERO, &usError);
    }

    if(m_pobjSdesPkt != RTP_NULL)
    {
        delete m_pobjSdesPkt;
        m_pobjSdesPkt = RTP_NULL;
    }
    if(m_pobjByePkt != RTP_NULL)
    {
        delete m_pobjByePkt;
        m_pobjByePkt = RTP_NULL;
    }
    if(m_pobjAppPkt != RTP_NULL)
    {
        delete m_pobjAppPkt;
        m_pobjAppPkt = RTP_NULL;
    }
    if(m_pobjRtcpFbPkt != RTP_NULL)
    {
        delete m_pobjRtcpFbPkt;
        m_pobjRtcpFbPkt = RTP_NULL;
    }

    if (m_pobjRtcpXrPkt != RTP_NULL)
    {
        delete m_pobjRtcpXrPkt;
        m_pobjRtcpXrPkt = RTP_NULL;
    }
} //Destructor

RtpList* RtcpPacket::getSrPacketList()
{
    return &m_objSrPkt;
}

RtpList* RtcpPacket::getRrPacketList()
{
    return &m_objRrPkt;
}

RtpList* RtcpPacket::getFbPacketList()
{
    return &m_objFbPkt;
}

RtcpSdesPacket* RtcpPacket::getSdesPacket()
{
    return m_pobjSdesPkt;
}

RtpDt_Void RtcpPacket::setSdesPacketData(IN RtcpSdesPacket* pobjSdesData)
{
    m_pobjSdesPkt = pobjSdesData;
}

RtcpByePacket* RtcpPacket::getByePacket()
{
    return m_pobjByePkt;
}

RtpDt_Void RtcpPacket::setByePacketData(IN RtcpByePacket* pobjByePktData)
{
    m_pobjByePkt = pobjByePktData;
}

RtcpAppPacket* RtcpPacket::getAppPacket()
{
    return m_pobjAppPkt;
}

RtpDt_Void RtcpPacket::setAppPktData(IN RtcpAppPacket* pobjAppData)
{
    m_pobjAppPkt = pobjAppData;
}

RtcpFbPacket* RtcpPacket::getRtcpFbPacket()
{
    return m_pobjRtcpFbPkt;
}

RtpDt_Void RtcpPacket::setRtcpFbPktData(IN RtcpFbPacket* pobjRtcpFbData)
{
    m_pobjRtcpFbPkt = pobjRtcpFbData;
}

eRTP_STATUS_CODE RtcpPacket::addRrPacketData(IN RtcpRrPacket *pobjRrPkt)
{
    RtpDt_UInt16 usError = RTP_ZERO;
    m_objRrPkt.Append(pobjRrPkt, &usError);
    if(usError == ERR_MALLOC_FAILED)
    {
        return RTP_MEMORY_FAIL;
    }
    return RTP_SUCCESS;
}

eRTP_STATUS_CODE RtcpPacket::addSrPacketData(IN RtcpSrPacket *pobjSrPkt)
{
    RtpDt_UInt16 usError = RTP_ZERO;
    m_objSrPkt.Append(pobjSrPkt, &usError);
    if(usError == ERR_MALLOC_FAILED)
    {
        return RTP_MEMORY_FAIL;
    }
    return RTP_SUCCESS;
}

RtcpXrPacket* RtcpPacket::getXrPacket()
{
    return m_pobjRtcpXrPkt;
}

RtpDt_Void RtcpPacket::setXrPktData(IN RtcpXrPacket* pobjRtcpXrData)
{
    m_pobjRtcpXrPkt = pobjRtcpXrData;
}

eRTP_STATUS_CODE RtcpPacket::addFbPacketData(IN RtcpFbPacket *pobjFbPkt)
{
    RtpDt_UInt16 usError = RTP_ZERO;
    m_objFbPkt.Append(pobjFbPkt, &usError);
    if(usError == ERR_MALLOC_FAILED)
    {
        return RTP_MEMORY_FAIL;
    }
    return RTP_SUCCESS;
}

eRTP_STATUS_CODE RtcpPacket::decodeRtcpPacket(IN RtpBuffer* pobjRtcpPktBuf,
                             IN RtpDt_UInt16 usExtHdrLen,
                             IN RtcpConfigInfo *pobjRtcpCfgInfo)
{
    RtpDt_UInt32 uiCurPos = RTP_ZERO;
    eRtp_Bool bSrPkt = eRTP_FALSE;
    eRtp_Bool bRrPkt = eRTP_FALSE;
    eRtp_Bool bFbPkt = eRTP_FALSE;
    eRtp_Bool bOtherPkt = eRTP_FALSE;


    // Get RTCP COMPOUND packet
    RtpDt_UInt32 uiCompPktLen = pobjRtcpPktBuf->getLength();
    RtpDt_Int32 iTrackCompLen = uiCompPktLen;


    while(iTrackCompLen > RTP_ZERO) //Process concatenated RTCP packets
    {
        RtpDt_UChar *pucBuffer = pobjRtcpPktBuf->getBuffer();
        pucBuffer = pucBuffer + uiCurPos;

        // get first 4 octets of the RTCP header
        RtpDt_UInt32 uiTemp4Data = RtpOsUtil::Ntohl(*((RtpDt_UInt32*)pucBuffer));

        //check version number
        RtpDt_UChar uiVersion = RTP_ZERO;
        RTP_GET_VERSION(uiTemp4Data, uiVersion)
        if(uiVersion != RTP_VERSION_NUM)
        {
            RTP_TRACE_WARNING("decodeRtcpPacket, RTCP version is Invalid ...!",
                uiVersion, RTP_ZERO);
            return RTP_INVALID_MSG;
        }

        //get length
        RtpDt_UInt16 usPktLen = uiTemp4Data & 0x0000FFFF;

        usPktLen = usPktLen * RTP_WORD_SIZE; //Payload Size
        usPktLen = usPktLen + RTP_WORD_SIZE; //+1 WORD header - Avinash

        if((usPktLen > iTrackCompLen) || (usPktLen < RTP_WORD_SIZE))
        {
            RTP_TRACE_WARNING("decodeRtcpPacket, RTCP packet length is Invalid ...!",
                usPktLen, iTrackCompLen);
            return RTP_INVALID_MSG;
        }

        RTP_TRACE_NORMAL("decodeRtcpPacket [packet length: %d] [compound packet length: %d]",
            usPktLen,iTrackCompLen);

        //get packet type
        RtpDt_UInt32 uiPktType = RTP_ZERO;
        uiTemp4Data = uiTemp4Data >> RTP_16;
        uiPktType =  uiTemp4Data & 0x000000FF;

        RTP_TRACE_NORMAL("decodeRtcpPacket [packet type: %d] [%d]",uiPktType,RTP_ZERO);

        eRTP_STATUS_CODE eDecodeRes = RTP_FAILURE;

        switch(uiPktType)
        {
            case RTCP_SR:
            {
                RtcpSrPacket *pobjSrPkt = new RtcpSrPacket();
                if(pobjSrPkt == RTP_NULL)
                {
                    RTP_TRACE_WARNING("decodeRtcpPacket, new returned NULL...!",
                        RTP_ZERO,RTP_ZERO);
                    return RTP_MEMORY_FAIL;
                }
                eDecodeRes = pobjSrPkt->decodeSrPacket(pucBuffer, usPktLen, usExtHdrLen);
                addSrPacketData(pobjSrPkt);
                bSrPkt = eRTP_TRUE;
                break;
            }//RTCP_SR
            case RTCP_RR:
            {
                RtpDt_UInt16 uiRrPktLen = usPktLen;
                RtcpRrPacket *pobjRrPkt = new RtcpRrPacket();
                if(pobjRrPkt == RTP_NULL)
                {
                    RTP_TRACE_WARNING("decodeRtcpPacket, new returned NULL...!",
                        RTP_ZERO,RTP_ZERO);
                    return RTP_MEMORY_FAIL;
                }
                eDecodeRes = pobjRrPkt->decodeRrPacket(pucBuffer, uiRrPktLen,
                                                usExtHdrLen, eRTP_TRUE);
                addRrPacketData(pobjRrPkt);
                bRrPkt = eRTP_TRUE;
                break;
            }//RTCP_RR
            case RTCP_SDES:
            {
                m_pobjSdesPkt = new RtcpSdesPacket();
                if(m_pobjSdesPkt == RTP_NULL)
                {
                    RTP_TRACE_WARNING("decodeRtcpPacket, new returned NULL...!",
                        RTP_ZERO,RTP_ZERO);
                    return RTP_MEMORY_FAIL;
                }
                eDecodeRes = m_pobjSdesPkt->decodeSdesPacket(pucBuffer, usPktLen, pobjRtcpCfgInfo);
                bOtherPkt = eRTP_TRUE;
                break;
            }//RTCP_SDES
            case RTCP_BYE:
            {
                m_pobjByePkt = new RtcpByePacket();
                if(m_pobjByePkt == RTP_NULL)
                {
                    RTP_TRACE_WARNING("decodeRtcpPacket, new returned NULL...!",
                        RTP_ZERO,RTP_ZERO);
                    return RTP_MEMORY_FAIL;
                }
                eDecodeRes = m_pobjByePkt->decodeByePacket(pucBuffer, usPktLen);
                bOtherPkt = eRTP_TRUE;
                break;
            }//RTCP_BYE
            case RTCP_APP:
            {
                m_pobjAppPkt = new RtcpAppPacket();
                if(m_pobjAppPkt == RTP_NULL)
                {
                    RTP_TRACE_WARNING("decodeRtcpPacket, new returned NULL...!",
                        RTP_ZERO,RTP_ZERO);
                    return RTP_MEMORY_FAIL;
                }
                eDecodeRes = m_pobjAppPkt->decodeAppPacket(pucBuffer, usPktLen);
                bOtherPkt = eRTP_TRUE;
                break;
            }//RTCP_APP
            case RTCP_RTPFB:
            case RTCP_PSFB:
            {
                RtcpFbPacket *pobjFbPkt = new RtcpFbPacket();
                if(pobjFbPkt == RTP_NULL)
                {
                    RTP_TRACE_WARNING("decodeRtcpPacket, new returned NULL...!",
                        RTP_ZERO,RTP_ZERO);
                    return RTP_MEMORY_FAIL;
                }
                RTP_TRACE_MESSAGE("decodeRtcpPacket found fb packet", 0, 0);
                eDecodeRes = pobjFbPkt->decodeRtcpFbPacket(pucBuffer, usPktLen, uiPktType);
                addFbPacketData(pobjFbPkt);
                bFbPkt = eRTP_TRUE;
                break;
            }//RTCP_RTPFB || RTCP_PSFB
            default:
            {
                RTP_TRACE_WARNING("decodeRtcpPacket, Invalid RTCP MSG type received ...!",
                        RTP_ZERO,RTP_ZERO);
                return RTP_INVALID_MSG;
            }//default
        };//switch
        if(eDecodeRes != RTP_SUCCESS)
        {
            RTP_TRACE_WARNING("decodeRtcpPacket, Decoding Error ...!",
                    RTP_ZERO,RTP_ZERO);
            return eDecodeRes;
        }

        iTrackCompLen = iTrackCompLen - usPktLen;
        uiCurPos = uiCurPos + usPktLen;
    }//while


    if((bSrPkt == eRTP_FALSE) && (bRrPkt == eRTP_FALSE)
        && (bFbPkt == eRTP_FALSE) && (bOtherPkt == eRTP_FALSE))
    {
        RTP_TRACE_MESSAGE("decodeRtcpPacket, no rtcp sr,rr,fb packets", 0, 0);
        return RTP_DECODE_ERROR;
    }

    return RTP_SUCCESS;
}//decodeRtcpPacket


eRTP_STATUS_CODE RtcpPacket::formRtcpPacket(OUT RtpBuffer* pobjRtcpPktBuf)
{
    RtpDt_UInt16 usSrSize = RTP_ZERO;
    RtpDt_UInt16 usRrSize = RTP_ZERO;
    RtpDt_UInt16 usError = RTP_ZERO;


    pobjRtcpPktBuf->setLength(RTP_ZERO);
    m_objSrPkt.GetSize(&usSrSize, &usError);
    m_objRrPkt.GetSize(&usRrSize, &usError);


    if((m_pobjRtcpFbPkt == RTP_NULL) && (usSrSize == RTP_ZERO)
        && (usRrSize == RTP_ZERO) && (m_pobjByePkt == RTP_NULL))
    {
        RTP_TRACE_WARNING("formRtcpPacket, m_pobjSrPkt is NULL!",
                    RTP_ZERO,RTP_ZERO);
        return RTP_FAILURE;
    }

    if((m_pobjByePkt == RTP_NULL)
        && (m_pobjSdesPkt == RTP_NULL)
        && (m_pobjAppPkt == RTP_NULL)
        && (m_pobjRtcpFbPkt == RTP_NULL))
    {
        RTP_TRACE_WARNING("formRtcpPacket, Not present 2nd pkt in Comp pkt!",
            RTP_ZERO,RTP_ZERO);
        return RTP_FAILURE;
    }

    eRTP_STATUS_CODE eEncodeRes = RTP_FAILURE;

    for(RtpDt_UInt32 uiCount=RTP_ZERO; uiCount < usSrSize; uiCount++)
    {
        RtpDt_Void    *pvElement = RTP_NULL;
        RtcpSrPacket *pobjSrPkt = RTP_NULL;

        //get key material element from list.
        m_objSrPkt.GetElement(uiCount, &pvElement, &usError);
        //typecast to RtcpSrPacket
        pobjSrPkt = RTP_STATIC_CAST(RtcpSrPacket*,pvElement);

        eEncodeRes = pobjSrPkt->formSrPacket(pobjRtcpPktBuf);
        if(eEncodeRes != RTP_SUCCESS)
        {
            RTP_TRACE_WARNING("formRtcpPacket, Error in SR pkt encoding ..!",
                RTP_ZERO,RTP_ZERO);
            return eEncodeRes;
        }

    }

    for(RtpDt_UInt32 uiCount=RTP_ZERO; uiCount < usRrSize; uiCount++)
    {
        RtpDt_Void    *pvElement = RTP_NULL;
        RtcpRrPacket *pobjRrPkt = RTP_NULL;

        //get key material element from list.
        m_objRrPkt.GetElement(uiCount, &pvElement, &usError);
        //typecast to RtcpRrPacket
        pobjRrPkt = RTP_STATIC_CAST(RtcpRrPacket*,pvElement);

        eEncodeRes = pobjRrPkt->formRrPacket(pobjRtcpPktBuf, eRTP_TRUE);
        if(eEncodeRes != RTP_SUCCESS)
        {
            RTP_TRACE_WARNING("formRtcpPacket, Error in RR pkt encoding ..!",
                RTP_ZERO,RTP_ZERO);
            return eEncodeRes;
        }
    }

    if(m_pobjSdesPkt != RTP_NULL)
    {
        eEncodeRes = m_pobjSdesPkt->formSdesPacket(pobjRtcpPktBuf);
        if(eEncodeRes != RTP_SUCCESS)
        {
            RTP_TRACE_WARNING("formRtcpPacket, Error in SDES pkt encoding ..!",
                RTP_ZERO,RTP_ZERO);
            return eEncodeRes;
        }
    }

    if(m_pobjAppPkt != RTP_NULL)
    {
        eEncodeRes = m_pobjAppPkt->formAppPacket(pobjRtcpPktBuf);
        if(eEncodeRes != RTP_SUCCESS)
        {
            RTP_TRACE_WARNING("formRtcpPacket, Error in APP pkt encoding ..!",
                RTP_ZERO,RTP_ZERO);
            return eEncodeRes;
        }
    }
    if(m_pobjByePkt != RTP_NULL)
    {
        eEncodeRes = m_pobjByePkt->formByePacket(pobjRtcpPktBuf);
        if(eEncodeRes != RTP_SUCCESS)
        {
            RTP_TRACE_WARNING("formRtcpPacket, Error in BYE pkt encoding ..!",
                RTP_ZERO,RTP_ZERO);
            return eEncodeRes;
        }
    }

    if(m_pobjRtcpFbPkt != RTP_NULL)
    {
        eEncodeRes = m_pobjRtcpFbPkt->formRtcpFbPacket(pobjRtcpPktBuf);
        if(eEncodeRes != RTP_SUCCESS)
        {
            RTP_TRACE_WARNING("formRtcpPacket, Error in Fb pkt encoding ..!",
                RTP_ZERO,RTP_ZERO);
            return eEncodeRes;
        }
    }

    if (m_pobjRtcpXrPkt != RTP_NULL)
    {
        eEncodeRes = m_pobjRtcpXrPkt->formRtcpXrPacket(pobjRtcpPktBuf);
        if (eEncodeRes != RTP_SUCCESS)
        {
            RTP_TRACE_WARNING("formRtcpPacket, Error in XR pkt encoding ..!",
                RTP_ZERO,RTP_ZERO);
            return eEncodeRes;
        }
    }

    return RTP_SUCCESS;
}//formRtcpPacket
