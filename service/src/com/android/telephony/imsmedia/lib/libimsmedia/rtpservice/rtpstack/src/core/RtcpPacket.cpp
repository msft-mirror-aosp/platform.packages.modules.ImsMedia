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
#include <RtpTrace.h>
#include <RtpError.h>
#include <RtpSession.h>

RtcpPacket::RtcpPacket() :
        m_objSrPktList(std::list<RtcpSrPacket*>()),
        m_objRrPktList(std::list<RtcpRrPacket*>()),
        m_objFbPktList(std::list<RtcpFbPacket*>()),
        m_pobjSdesPkt(RTP_NULL),
        m_pobjByePkt(RTP_NULL),
        m_pobjAppPkt(RTP_NULL),
        m_pobjRtcpFbPkt(RTP_NULL),
        m_pobjRtcpXrPkt(RTP_NULL)

{
}  // Constructor

RtcpPacket::~RtcpPacket()
{
    // delete all RtcpSrPacket objects.
    for (auto& pobjSrPkt : m_objSrPktList)
    {
        delete pobjSrPkt;
    }
    m_objSrPktList.clear();

    // delete all RtcpRrPacket objects
    for (auto& pobjRrPkt : m_objRrPktList)
    {
        delete pobjRrPkt;
    }
    m_objRrPktList.clear();

    // delete all RtcpFbPacket objects
    for (auto& pobjFbPkt : m_objFbPktList)
    {
        delete pobjFbPkt;
    }
    m_objFbPktList.clear();

    if (m_pobjSdesPkt != RTP_NULL)
    {
        delete m_pobjSdesPkt;
        m_pobjSdesPkt = RTP_NULL;
    }
    if (m_pobjByePkt != RTP_NULL)
    {
        delete m_pobjByePkt;
        m_pobjByePkt = RTP_NULL;
    }
    if (m_pobjAppPkt != RTP_NULL)
    {
        delete m_pobjAppPkt;
        m_pobjAppPkt = RTP_NULL;
    }
    if (m_pobjRtcpFbPkt != RTP_NULL)
    {
        delete m_pobjRtcpFbPkt;
        m_pobjRtcpFbPkt = RTP_NULL;
    }

    if (m_pobjRtcpXrPkt != RTP_NULL)
    {
        delete m_pobjRtcpXrPkt;
        m_pobjRtcpXrPkt = RTP_NULL;
    }
}  // Destructor

std::list<RtcpSrPacket*>& RtcpPacket::getSrPacketList()
{
    return m_objSrPktList;
}

std::list<RtcpRrPacket*>& RtcpPacket::getRrPacketList()
{
    return m_objRrPktList;
}

std::list<RtcpFbPacket*>& RtcpPacket::getFbPacketList()
{
    return m_objFbPktList;
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

eRTP_STATUS_CODE RtcpPacket::addSrPacketData(IN RtcpSrPacket* pobjSrPkt)
{
    if (pobjSrPkt == RTP_NULL)
    {
        return RTP_FAILURE;
    }
    m_objSrPktList.push_back(pobjSrPkt);
    return RTP_SUCCESS;
}

eRTP_STATUS_CODE RtcpPacket::addRrPacketData(IN RtcpRrPacket* pobjRrPkt)
{
    if (pobjRrPkt == RTP_NULL)
    {
        return RTP_FAILURE;
    }
    m_objRrPktList.push_back(pobjRrPkt);
    return RTP_SUCCESS;
}

eRTP_STATUS_CODE RtcpPacket::addFbPacketData(IN RtcpFbPacket* pobjFbPkt)
{
    if (pobjFbPkt == RTP_NULL)
    {
        return RTP_FAILURE;
    }
    m_objFbPktList.push_back(pobjFbPkt);
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

eRTP_STATUS_CODE RtcpPacket::decodeRtcpPacket(IN RtpBuffer* pobjRtcpPktBuf,
        IN RtpDt_UInt16 usExtHdrLen, IN RtcpConfigInfo* pobjRtcpCfgInfo)
{
    RtpDt_UInt32 uiCurPos = RTP_ZERO;
    eRtp_Bool bSrPkt = eRTP_FALSE;
    eRtp_Bool bRrPkt = eRTP_FALSE;
    eRtp_Bool bFbPkt = eRTP_FALSE;
    eRtp_Bool bOtherPkt = eRTP_FALSE;

    // Get RTCP COMPOUND packet
    RtpDt_UInt32 uiCompPktLen = pobjRtcpPktBuf->getLength();
    RtpDt_Int32 iTrackCompLen = uiCompPktLen;

    while (iTrackCompLen > RTP_ZERO)  // Process concatenated RTCP packets
    {
        RtpDt_UChar* pucBuffer = pobjRtcpPktBuf->getBuffer();
        pucBuffer = pucBuffer + uiCurPos;

        // get first 4 octets of the RTCP header
        RtpDt_UInt32 uiTemp4Data = RtpOsUtil::Ntohl(*((RtpDt_UInt32*)pucBuffer));

        // check version number
        RtpDt_UChar uiVersion = RTP_ZERO;
        RTP_GET_VERSION(uiTemp4Data, uiVersion)
        if (uiVersion != RTP_VERSION_NUM)
        {
            RTP_TRACE_WARNING(
                    "decodeRtcpPacket, RTCP version is Invalid ...!", uiVersion, RTP_ZERO);
            return RTP_INVALID_MSG;
        }

        // get length
        RtpDt_UInt16 usPktLen = uiTemp4Data & 0x0000FFFF;

        usPktLen = usPktLen * RTP_WORD_SIZE;  // Payload Size
        usPktLen = usPktLen + RTP_WORD_SIZE;  //+1 WORD header - Avinash

        if ((usPktLen > iTrackCompLen) || (usPktLen < RTP_WORD_SIZE))
        {
            RTP_TRACE_WARNING("decodeRtcpPacket, RTCP packet length is Invalid ...!", usPktLen,
                    iTrackCompLen);
            return RTP_INVALID_MSG;
        }

        RTP_TRACE_MESSAGE("decodeRtcpPacket [packet length: %d] [compound packet length: %d]",
                usPktLen, iTrackCompLen);

        // get packet type
        RtpDt_UInt32 uiPktType = RTP_ZERO;
        uiTemp4Data = uiTemp4Data >> RTP_16;
        uiPktType = uiTemp4Data & 0x000000FF;

        RTP_TRACE_MESSAGE("decodeRtcpPacket [packet type: %d] [%d]", uiPktType, RTP_ZERO);

        eRTP_STATUS_CODE eDecodeRes = RTP_FAILURE;

        switch (uiPktType)
        {
            case RTCP_SR:
            {
                RtcpSrPacket* pobjSrPkt = new RtcpSrPacket();
                if (pobjSrPkt == RTP_NULL)
                {
                    RTP_TRACE_WARNING(
                            "decodeRtcpPacket, new returned NULL...!", RTP_ZERO, RTP_ZERO);
                    return RTP_MEMORY_FAIL;
                }
                eDecodeRes = pobjSrPkt->decodeSrPacket(pucBuffer, usPktLen, usExtHdrLen);
                addSrPacketData(pobjSrPkt);
                bSrPkt = eRTP_TRUE;
                break;
            }  // RTCP_SR
            case RTCP_RR:
            {
                RtpDt_UInt16 uiRrPktLen = usPktLen;
                RtcpRrPacket* pobjRrPkt = new RtcpRrPacket();
                if (pobjRrPkt == RTP_NULL)
                {
                    RTP_TRACE_WARNING(
                            "decodeRtcpPacket, new returned NULL...!", RTP_ZERO, RTP_ZERO);
                    return RTP_MEMORY_FAIL;
                }
                eDecodeRes =
                        pobjRrPkt->decodeRrPacket(pucBuffer, uiRrPktLen, usExtHdrLen, eRTP_TRUE);
                addRrPacketData(pobjRrPkt);
                bRrPkt = eRTP_TRUE;
                break;
            }  // RTCP_RR
            case RTCP_SDES:
            {
                m_pobjSdesPkt = new RtcpSdesPacket();
                if (m_pobjSdesPkt == RTP_NULL)
                {
                    RTP_TRACE_WARNING(
                            "decodeRtcpPacket, new returned NULL...!", RTP_ZERO, RTP_ZERO);
                    return RTP_MEMORY_FAIL;
                }
                eDecodeRes = m_pobjSdesPkt->decodeSdesPacket(pucBuffer, usPktLen, pobjRtcpCfgInfo);
                bOtherPkt = eRTP_TRUE;
                break;
            }  // RTCP_SDES
            case RTCP_BYE:
            {
                m_pobjByePkt = new RtcpByePacket();
                if (m_pobjByePkt == RTP_NULL)
                {
                    RTP_TRACE_WARNING(
                            "decodeRtcpPacket, new returned NULL...!", RTP_ZERO, RTP_ZERO);
                    return RTP_MEMORY_FAIL;
                }
                eDecodeRes = m_pobjByePkt->decodeByePacket(pucBuffer, usPktLen);
                bOtherPkt = eRTP_TRUE;
                break;
            }  // RTCP_BYE
            case RTCP_APP:
            {
                m_pobjAppPkt = new RtcpAppPacket();
                if (m_pobjAppPkt == RTP_NULL)
                {
                    RTP_TRACE_WARNING(
                            "decodeRtcpPacket, new returned NULL...!", RTP_ZERO, RTP_ZERO);
                    return RTP_MEMORY_FAIL;
                }
                eDecodeRes = m_pobjAppPkt->decodeAppPacket(pucBuffer, usPktLen);
                bOtherPkt = eRTP_TRUE;
                break;
            }  // RTCP_APP
            case RTCP_RTPFB:
            case RTCP_PSFB:
            {
                RtcpFbPacket* pobjFbPkt = new RtcpFbPacket();
                if (pobjFbPkt == RTP_NULL)
                {
                    RTP_TRACE_WARNING(
                            "decodeRtcpPacket, new returned NULL...!", RTP_ZERO, RTP_ZERO);
                    return RTP_MEMORY_FAIL;
                }
                RTP_TRACE_MESSAGE("decodeRtcpPacket found fb packet", 0, 0);
                eDecodeRes = pobjFbPkt->decodeRtcpFbPacket(pucBuffer, usPktLen, uiPktType);
                addFbPacketData(pobjFbPkt);
                bFbPkt = eRTP_TRUE;
                break;
            }  // RTCP_RTPFB || RTCP_PSFB
            default:
            {
                RTP_TRACE_WARNING("decodeRtcpPacket, Invalid RTCP MSG type received ...!", RTP_ZERO,
                        RTP_ZERO);
                return RTP_INVALID_MSG;
            }  // default
        };     // switch
        if (eDecodeRes != RTP_SUCCESS)
        {
            RTP_TRACE_WARNING("decodeRtcpPacket, Decoding Error ...!", RTP_ZERO, RTP_ZERO);
            return eDecodeRes;
        }

        iTrackCompLen = iTrackCompLen - usPktLen;
        uiCurPos = uiCurPos + usPktLen;
    }  // while

    if ((bSrPkt == eRTP_FALSE) && (bRrPkt == eRTP_FALSE) && (bFbPkt == eRTP_FALSE) &&
            (bOtherPkt == eRTP_FALSE))
    {
        RTP_TRACE_MESSAGE("decodeRtcpPacket, no rtcp sr,rr,fb packets", 0, 0);
        return RTP_DECODE_ERROR;
    }

    return RTP_SUCCESS;
}  // decodeRtcpPacket

eRTP_STATUS_CODE RtcpPacket::formRtcpPacket(OUT RtpBuffer* pobjRtcpPktBuf)
{
    RtpDt_UInt16 usSrSize = m_objSrPktList.size();
    RtpDt_UInt16 usRrSize = m_objRrPktList.size();

    pobjRtcpPktBuf->setLength(RTP_ZERO);

    if ((m_pobjRtcpFbPkt == RTP_NULL) && (usSrSize == RTP_ZERO) && (usRrSize == RTP_ZERO) &&
            (m_pobjByePkt == RTP_NULL))
    {
        RTP_TRACE_WARNING("formRtcpPacket, m_pobjSrPkt is NULL!", RTP_ZERO, RTP_ZERO);
        return RTP_FAILURE;
    }

    if ((m_pobjByePkt == RTP_NULL) && (m_pobjSdesPkt == RTP_NULL) && (m_pobjAppPkt == RTP_NULL) &&
            (m_pobjRtcpFbPkt == RTP_NULL))
    {
        RTP_TRACE_WARNING("formRtcpPacket, Not present 2nd pkt in Comp pkt!", RTP_ZERO, RTP_ZERO);
        return RTP_FAILURE;
    }

    eRTP_STATUS_CODE eEncodeRes = RTP_FAILURE;

    for (auto& pobjSrPkt : m_objSrPktList)
    {
        // get key material element from list.
        eEncodeRes = pobjSrPkt->formSrPacket(pobjRtcpPktBuf);
        if (eEncodeRes != RTP_SUCCESS)
        {
            RTP_TRACE_WARNING("formRtcpPacket, Error in SR pkt encoding ..!", RTP_ZERO, RTP_ZERO);
            return eEncodeRes;
        }
    }

    for (auto& pobjRrPkt : m_objRrPktList)
    {
        // get key material element from list.
        eEncodeRes = pobjRrPkt->formRrPacket(pobjRtcpPktBuf, eRTP_TRUE);
        if (eEncodeRes != RTP_SUCCESS)
        {
            RTP_TRACE_WARNING("formRtcpPacket, Error in RR pkt encoding ..!", RTP_ZERO, RTP_ZERO);
            return eEncodeRes;
        }
    }

    if (m_pobjSdesPkt != RTP_NULL)
    {
        eEncodeRes = m_pobjSdesPkt->formSdesPacket(pobjRtcpPktBuf);
        if (eEncodeRes != RTP_SUCCESS)
        {
            RTP_TRACE_WARNING("formRtcpPacket, Error in SDES pkt encoding ..!", RTP_ZERO, RTP_ZERO);
            return eEncodeRes;
        }
    }

    if (m_pobjAppPkt != RTP_NULL)
    {
        eEncodeRes = m_pobjAppPkt->formAppPacket(pobjRtcpPktBuf);
        if (eEncodeRes != RTP_SUCCESS)
        {
            RTP_TRACE_WARNING("formRtcpPacket, Error in APP pkt encoding ..!", RTP_ZERO, RTP_ZERO);
            return eEncodeRes;
        }
    }
    if (m_pobjByePkt != RTP_NULL)
    {
        eEncodeRes = m_pobjByePkt->formByePacket(pobjRtcpPktBuf);
        if (eEncodeRes != RTP_SUCCESS)
        {
            RTP_TRACE_WARNING("formRtcpPacket, Error in BYE pkt encoding ..!", RTP_ZERO, RTP_ZERO);
            return eEncodeRes;
        }
    }

    if (m_pobjRtcpFbPkt != RTP_NULL)
    {
        eEncodeRes = m_pobjRtcpFbPkt->formRtcpFbPacket(pobjRtcpPktBuf);
        if (eEncodeRes != RTP_SUCCESS)
        {
            RTP_TRACE_WARNING("formRtcpPacket, Error in Fb pkt encoding ..!", RTP_ZERO, RTP_ZERO);
            return eEncodeRes;
        }
    }

    if (m_pobjRtcpXrPkt != RTP_NULL)
    {
        eEncodeRes = m_pobjRtcpXrPkt->formRtcpXrPacket(pobjRtcpPktBuf);
        if (eEncodeRes != RTP_SUCCESS)
        {
            RTP_TRACE_WARNING("formRtcpPacket, Error in XR pkt encoding ..!", RTP_ZERO, RTP_ZERO);
            return eEncodeRes;
        }
    }

    return RTP_SUCCESS;
}  // formRtcpPacket
