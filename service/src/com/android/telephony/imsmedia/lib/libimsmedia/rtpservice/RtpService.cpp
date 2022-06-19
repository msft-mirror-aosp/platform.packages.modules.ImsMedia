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

#include <RtpService.h>
#include <RtpString.h>
#include <RtpGlobal.h>
#include <RtpImpl.h>
#include <RtpStack.h>
#include <RtpTrace.h>
#include <RtpError.h>

RtpStack* g_pobjRtpStack = RTP_NULL;

RtpDt_Void addSdesItem(
        OUT RtcpConfigInfo* pobjRtcpCfgInfo, IN RtpDt_UChar* sdesName, IN RtpDt_UInt32 uiLength)
{
    tRTCP_SDES_ITEM stSdesItem;
    memset(&stSdesItem, RTP_ZERO, sizeof(tRTCP_SDES_ITEM));
    RtpDt_UInt32 uiIndex = RTP_ZERO;
    stSdesItem.ucType = RTP_ONE;  // RTCP_SDES_CNAME
    stSdesItem.ucLength = uiLength;
    stSdesItem.pValue = sdesName;
    pobjRtcpCfgInfo->addRtcpSdesItem(&stSdesItem, uiIndex);
    return;
}  // addSdesItem

RtpDt_Void populateReceiveRtpIndInfo(
        OUT tRtpSvcIndSt_ReceiveRtpInd* pstRtpIndMsg, IN RtpPacket* pobjRtpPkt)
{
    RtpHeader* pobjRtpHeader = pobjRtpPkt->getRtpHeader();
    pstRtpIndMsg->bMbit = pobjRtpHeader->getMarker() > 0 ? eRTP_TRUE : eRTP_FALSE;
    pstRtpIndMsg->dwTimestamp = pobjRtpHeader->getRtpTimestamp();
    pstRtpIndMsg->dwPayloadType = pobjRtpHeader->getPldType();
    pstRtpIndMsg->dwSeqNum = pobjRtpHeader->getSeqNum();

    // Header length
    pstRtpIndMsg->wMsgHdrLen = RTP_FIXED_HDR_LEN;
    pstRtpIndMsg->wMsgHdrLen += 4 * pobjRtpHeader->getCsrcCount();
    if (pobjRtpPkt->getExtHeader())
    {
        pstRtpIndMsg->wMsgHdrLen += pobjRtpPkt->getExtHeader()->getLength();
        pstRtpIndMsg->bExtension = eRTP_TRUE;
        RtpDt_UChar* pbuf = pobjRtpPkt->getExtHeader()->getBuffer();
        pstRtpIndMsg->extensionData = ((((unsigned)*pbuf) << 8) & 0xff00) | ((pbuf[1] & 0x00ff));
    }
    else
    {
        pstRtpIndMsg->bExtension = eRTP_FALSE;
        pstRtpIndMsg->extensionData = RTP_NULL;
    }
    // End Header length
    // play the payload
    RtpBuffer* pobjPlayPl = pobjRtpPkt->getRtpPayload();
    // Header

    // body
    if (!pobjPlayPl)
    {
        pstRtpIndMsg->wMsgBodyLen = 0;
        pstRtpIndMsg->pMsgBody = NULL;
        return;
    }

    pstRtpIndMsg->wMsgBodyLen = pobjPlayPl->getLength();
    pstRtpIndMsg->pMsgBody = (RtpDt_UChar*)pobjPlayPl->getBuffer();
}

eRtp_Bool populateRcvdReportFromStk(
        IN std::list<RtcpReportBlock*>& pobjRepBlkList, OUT tRtpSvcRecvReport* pstRcvdReport)
{
    if (pobjRepBlkList.size() > RTP_ZERO)
    {
        // application supports one RR
        RtcpReportBlock* pobjRepBlkElm = pobjRepBlkList.front();
        if (pobjRepBlkElm == RTP_NULL)
        {
            return eRTP_FALSE;
        }

        pstRcvdReport->ssrc = pobjRepBlkElm->getSsrc();
        pstRcvdReport->fractionLost = pobjRepBlkElm->getFracLost();
        pstRcvdReport->cumPktsLost = pobjRepBlkElm->getCumNumPktLost();
        pstRcvdReport->extHighSeqNum = pobjRepBlkElm->getExtHighSeqRcv();
        pstRcvdReport->jitter = pobjRepBlkElm->getJitter();
        pstRcvdReport->lsr = pobjRepBlkElm->getLastSR();
        pstRcvdReport->delayLsr = pobjRepBlkElm->getDelayLastSR();

        RTP_TRACE_MESSAGE("Received RR info :  [SSRC = %u] [FRAC LOST = %u]", pstRcvdReport->ssrc,
                pstRcvdReport->fractionLost);

        RTP_TRACE_MESSAGE("Received RR info :  [CUM PKTS LOST = %u] [EXT HIGE SEQ NUM = %u]",
                pstRcvdReport->cumPktsLost, pstRcvdReport->extHighSeqNum);

        RTP_TRACE_MESSAGE("Received RR info :  [JITTER = %u] [LSR = %u]", pstRcvdReport->jitter,
                pstRcvdReport->lsr);

        RTP_TRACE_MESSAGE(
                "Received RR info :  [DELAY SINCE LSR = %u] ", pstRcvdReport->delayLsr, RTP_NULL);
    }
    else
    {
        pstRcvdReport->ssrc = RTP_ZERO;
        pstRcvdReport->fractionLost = RTP_ZERO;
        pstRcvdReport->cumPktsLost = RTP_ZERO;
        pstRcvdReport->extHighSeqNum = RTP_ZERO;
        pstRcvdReport->jitter = RTP_ZERO;
        pstRcvdReport->lsr = RTP_ZERO;
        pstRcvdReport->delayLsr = RTP_ZERO;
    }

    return eRTP_TRUE;
}  // populateRcvdReportFromStk

eRtp_Bool populateRcvdRrInfoFromStk(
        IN std::list<RtcpRrPacket*>& pobjRrList, OUT tNotifyReceiveRtcpRrInd* pstRrInfo)
{
    // application supports one RR
    RtcpRrPacket* pobjRrPkt = pobjRrList.front();
    if (pobjRrPkt == RTP_NULL)
    {
        return eRTP_FALSE;
    }

    tRtpSvcRecvReport* pstRcvdReport = &(pstRrInfo->stRecvRpt);
    std::list<RtcpReportBlock*>& pobjRepBlkList = pobjRrPkt->getReportBlkList();
    return populateRcvdReportFromStk(pobjRepBlkList, pstRcvdReport);
}  // populateRcvdRrInfoFromStk

eRtp_Bool populateRcvdSrInfoFromStk(
        IN std::list<RtcpSrPacket*>& pobjSrList, OUT tNotifyReceiveRtcpSrInd* pstSrInfo)
{
    // get SR packet data
    RtcpSrPacket* pobjSrPkt = pobjSrList.front();
    if (pobjSrPkt == RTP_NULL)
    {
        return eRTP_FALSE;
    }

    pstSrInfo->ntpTimestampMsw = pobjSrPkt->getNtpTime()->m_uiNtpHigh32Bits;
    pstSrInfo->ntpTimestampLsw = pobjSrPkt->getNtpTime()->m_uiNtpLow32Bits;
    pstSrInfo->rtpTimestamp = pobjSrPkt->getRtpTimestamp();
    pstSrInfo->sendPktCount = pobjSrPkt->getSendPktCount();
    pstSrInfo->sendOctCount = pobjSrPkt->getSendOctetCount();

    RTP_TRACE_MESSAGE("Received SR info :  [NTP High 32 = %u] [NTP LOW 32 = %u]",
            pstSrInfo->ntpTimestampMsw, pstSrInfo->ntpTimestampLsw);

    RTP_TRACE_MESSAGE(
            "Received SR info :  [RTP timestamp = %u] ", pstSrInfo->rtpTimestamp, RTP_NULL);

    RTP_TRACE_MESSAGE("Received SR info :  [SEND PKT COUNT = %u] [SEND OCTET COUNT = %u]",
            pstSrInfo->sendPktCount, pstSrInfo->sendOctCount);

    // populate tRtpSvcRecvReport
    tRtpSvcRecvReport* pstRcvdReport = &(pstSrInfo->stRecvRpt);
    RtcpRrPacket* pobjRepBlk = pobjSrPkt->getRrPktInfo();
    std::list<RtcpReportBlock*>& pobjRepBlkList = pobjRepBlk->getReportBlkList();
    return populateRcvdReportFromStk(pobjRepBlkList, pstRcvdReport);
}

eRtp_Bool populateRcvdFbInfoFromStk(
        IN RtcpFbPacket* m_pobjRtcpFbPkt, OUT tRtpSvcIndSt_ReceiveRtcpFeedbackInd* stFbRtcpMsg)
{
    if (m_pobjRtcpFbPkt != RTP_NULL)
    {
        stFbRtcpMsg->wPayloadType = m_pobjRtcpFbPkt->getRtcpHdrInfo()->getPacketType();
        stFbRtcpMsg->wFmt = m_pobjRtcpFbPkt->getRtcpHdrInfo()->getRecepRepCnt();
        stFbRtcpMsg->dwMediaSsrc = m_pobjRtcpFbPkt->getMediaSsrc();
        stFbRtcpMsg->wMsgLen = m_pobjRtcpFbPkt->getRtcpHdrInfo()->getLength();
        if (m_pobjRtcpFbPkt->getFCI() != RTP_NULL)
        {
            stFbRtcpMsg->pMsg = m_pobjRtcpFbPkt->getFCI()->getBuffer();
        }
        return eRTP_TRUE;
    }

    return eRTP_FALSE;
}

RtpDt_Void populateRtpProfile(OUT RtpStackProfile* pobjStackProfile)
{
    pobjStackProfile->setRtcpBw(RTP_DEF_RTCP_BW_SIZE);
    pobjStackProfile->setMtuSize(RTP_CONF_MTU_SIZE);
    pobjStackProfile->setTermNumber(RTP_CONF_SSRC_SEED);
}

RtpBuffer* GetCVOXHdr(IN tRtpSvc_SendRtpPacketParm* pstRtpParam)
{
    RtpBuffer* pobjXHdr = new RtpBuffer();

    // HDR extension
    if (pstRtpParam->bXbit)
    {
        RtpDt_UChar* pBuf = new RtpDt_UChar[8];
        pBuf[0] = (((unsigned)pstRtpParam->nDefinedByProfile) >> 8) & 0x00ff;
        pBuf[1] = pstRtpParam->nDefinedByProfile & 0x00ff;

        pBuf[2] = (((unsigned)pstRtpParam->nLength) >> 8) & 0x00ff;
        pBuf[3] = (pstRtpParam->nLength) & 0x00ff;

        pBuf[4] = (((unsigned)pstRtpParam->nExtensionData) >> 8) & 0x00ff;
        pBuf[5] = (pstRtpParam->nExtensionData) & 0x00ff;

        pBuf[6] = 0;
        pBuf[7] = 0;

        pobjXHdr->setBufferInfo(8, pBuf);
    }
    else
    {
        pobjXHdr->setBufferInfo(0, RTP_NULL);
    }

    return pobjXHdr;
}

RtpDt_UInt16 GetCVOXHdrLen(eRtp_Bool bEnableCVO)
{
    if (bEnableCVO)
        return RTP_CVO_XHDR_LEN;

    return 0;
}

GLOBAL eRtp_Bool IMS_RtpSvc_Initialize()
{
    if (g_pobjRtpStack == RTP_NULL)
    {
        g_pobjRtpStack = new RtpStack();
        if (g_pobjRtpStack == RTP_NULL)
        {
            return eRTP_FALSE;
        }

        RtpStackProfile* pobjStackProfile = new RtpStackProfile();
        if (pobjStackProfile == RTP_NULL)
        {
            return eRTP_FALSE;
        }

        populateRtpProfile(pobjStackProfile);
        g_pobjRtpStack->setStackProfile(pobjStackProfile);
    }

    return eRTP_TRUE;
}

GLOBAL eRtp_Bool IMS_RtpSvc_Deinitialize()
{
    if (g_pobjRtpStack)
    {
        delete g_pobjRtpStack;
        g_pobjRtpStack = RTP_NULL;
    }

    return eRTP_TRUE;
}

GLOBAL eRtp_Bool IMS_RtpSvc_CreateSession(IN RtpDt_Char* szLocalIP, IN RtpDt_UInt32 port,
        IN RtpDt_Void* pAppData, OUT RtpDt_UInt32* puSsrc, OUT RTPSESSIONID* hRtpSession)
{
    if (g_pobjRtpStack == RTP_NULL)
        return eRTP_FALSE;

    RtpSession* pobjRtpSession = g_pobjRtpStack->createRtpSession();
    if (pobjRtpSession == RTP_NULL)
    {
        return eRTP_FALSE;
    }

    // set ip and port
    RtpBuffer* pobjTransAddr = new RtpBuffer();
    RtpDt_UInt32 uiIpLen = Rtp_Strlen(szLocalIP) + 1;
    RtpDt_UChar* pcIpAddr = new RtpDt_UChar[uiIpLen];
    memcpy(pcIpAddr, szLocalIP, uiIpLen);
    pobjTransAddr->setBufferInfo(uiIpLen, pcIpAddr);

    pobjRtpSession->setRtpTransAddr(pobjTransAddr);
    pobjRtpSession->setRtpPort((RtpDt_UInt16)port);

    *puSsrc = pobjRtpSession->getSsrc();
    *hRtpSession = (RtpDt_Void*)pobjRtpSession;

    RtpImpl* pobjRtpImpl = new RtpImpl();
    if (pobjRtpImpl == RTP_NULL)
    {
        return eRTP_FALSE;
    }
    pobjRtpImpl->setAppdata(pAppData);

    RtcpConfigInfo* pobjRtcpConfigInfo = RTP_NULL;
    if (szLocalIP)
    {
        pobjRtcpConfigInfo = new RtcpConfigInfo;
        addSdesItem(pobjRtcpConfigInfo, (RtpDt_UChar*)szLocalIP, Rtp_Strlen(szLocalIP) + 1);
    }

    eRTP_STATUS_CODE eInitSta = pobjRtpSession->initSession(pobjRtpImpl, pobjRtcpConfigInfo);
    if (eInitSta != RTP_SUCCESS)
    {
        return eRTP_FALSE;
    }
    return eRTP_TRUE;
}

GLOBAL eRtp_Bool IMS_RtpSvc_SetPayload(IN RTPSESSIONID hRtpSession,
        IN tRtpSvc_SetPayloadParam* pstPayloadInfo, IN eRtp_Bool bEnableXHdr,
        IN RtpDt_UInt32 nNumOfPayloadParam)
{
    RtpDt_UInt32 payloadType[RTP_MAX_PAYLOAD_TYPE] = {0};

    for (RtpDt_UInt32 i = 0; i < nNumOfPayloadParam; i++)
    {
        RTP_TRACE_MESSAGE(
                "IMS_RtpSvc_SetPayload   payloadtype = %d", pstPayloadInfo[i].payloadType, 0);
        payloadType[i] = pstPayloadInfo[i].payloadType;
    }
    RtpPayloadInfo* pobjlPayloadInfo =
            new RtpPayloadInfo(payloadType, pstPayloadInfo[0].samplingRate, nNumOfPayloadParam);
    if (pobjlPayloadInfo == RTP_NULL)
    {
        return eRTP_FALSE;
    }

    RtpSession* pobjRtpSession = (RtpSession*)hRtpSession;

    if (g_pobjRtpStack == RTP_NULL ||
            g_pobjRtpStack->isRtpSessionPresent(pobjRtpSession) == eRTP_FAILURE)
    {
        delete pobjlPayloadInfo;
        return eRTP_FALSE;
    }

    eRTP_STATUS_CODE eInitSta =
            pobjRtpSession->setPayload(pobjlPayloadInfo, GetCVOXHdrLen(bEnableXHdr));
    delete pobjlPayloadInfo;
    if (eInitSta != RTP_SUCCESS)
    {
        return eRTP_FALSE;
    }

    return eRTP_TRUE;
}

GLOBAL eRtp_Bool IMS_RtpSvc_SetRTCPInterval(IN RTPSESSIONID hRtpSession, IN RtpDt_UInt32 nInterval)
{
    if (g_pobjRtpStack == RTP_NULL ||
            g_pobjRtpStack->isRtpSessionPresent((RtpSession*)hRtpSession) == eRTP_FAILURE)
        return eRTP_FALSE;

    ((RtpSession*)hRtpSession)->setRTCPTimerValue(nInterval);
    return eRTP_TRUE;
}

GLOBAL eRtp_Bool IMS_RtpSvc_DeleteSession(IN RTPSESSIONID hRtpSession)
{
    RtpSession* pobjRtpSession = (RtpSession*)hRtpSession;

    if (g_pobjRtpStack == RTP_NULL ||
            g_pobjRtpStack->isRtpSessionPresent(pobjRtpSession) == eRTP_FAILURE)
        return eRTP_FALSE;

    eRTP_STATUS_CODE eDelRtpStrm = g_pobjRtpStack->deleteRtpSession(pobjRtpSession);
    if (eDelRtpStrm != RTP_SUCCESS)
    {
        return eRTP_FALSE;
    }

    delete pobjRtpSession;
    return eRTP_TRUE;
}

GLOBAL eRtp_Bool IMS_RtpSvc_SendRtpPacket(IN RtpServiceListener* pobjIRtpSession,
        IN RTPSESSIONID hRtpSession, IN RtpDt_Char* pBuffer, IN RtpDt_UInt16 wBufferLength,
        IN tRtpSvc_SendRtpPacketParm* pstRtpParam)
{
    RtpSession* pobjRtpSession = (RtpSession*)hRtpSession;

    if (g_pobjRtpStack == RTP_NULL ||
            g_pobjRtpStack->isRtpSessionPresent(pobjRtpSession) == eRTP_FAILURE)
        return eRTP_FALSE;

    if (pobjRtpSession->isRtpEnabled() == eRTP_FALSE)
    {
        return eRTP_FALSE;
    }

    RtpBuffer* pobjRtpPayload = new RtpBuffer();
    if (pobjRtpPayload == RTP_NULL)
    {
        RTP_TRACE_WARNING(
                "IMS_RtpSvc_SendRtpPacket pobjRtpPayload - Malloc failed", RTP_ZERO, RTP_ZERO);
        return eRTP_FALSE;
    }

    RtpBuffer* pobjRtpBuf = new RtpBuffer();
    if (pobjRtpBuf == RTP_NULL)
    {
        delete pobjRtpPayload;
        RTP_TRACE_MESSAGE(
                "IMS_RtpSvc_SendRtpPacket pobjRtpBuf - Malloc failed", RTP_ZERO, RTP_ZERO);
        return eRTP_FALSE;
    }

    // Create RTP packet
    //  1. Set Marker bit
    eRtp_Bool bMbit = eRTP_FALSE;
    if (pstRtpParam->bMbit == eRTP_TRUE)
    {
        bMbit = eRTP_TRUE;
    }

    // 2. Set Payload
    pobjRtpPayload->setBufferInfo(wBufferLength, (RtpDt_UChar*)pBuffer);
    eRtp_Bool bUseLastTimestamp = pstRtpParam->bUseLastTimestamp ? eRTP_TRUE : eRTP_FALSE;
    eRTP_STATUS_CODE eRtpCreateStat = pobjRtpSession->createRtpPacket(pobjRtpPayload, bMbit,
            pstRtpParam->byPayLoadType, bUseLastTimestamp, pstRtpParam->diffFromLastRtpTimestamp,
            GetCVOXHdr(pstRtpParam), pobjRtpBuf);

    // 3. de-init and free the temp variable both in success and failure case
    pobjRtpPayload->setBufferInfo(RTP_ZERO, RTP_NULL);
    delete pobjRtpPayload;

    if (eRtpCreateStat != RTP_SUCCESS)
    {
        delete pobjRtpBuf;
        RTP_TRACE_WARNING(
                "IMS_RtpSvc_SendRtpPacket - eRtpCreateStat != RTP_SUCCESS ", RTP_ZERO, RTP_ZERO);

        return eRTP_FALSE;
    }
    // End Create RTP packet

    if (pobjRtpSession->isRtpEnabled() == eRTP_FALSE)
    {
        delete pobjRtpBuf;
        return eRTP_FALSE;
    }

    // dispatch to peer
    if (pobjIRtpSession->OnRtpPacket(pobjRtpBuf->getBuffer(), pobjRtpBuf->getLength()) == -1)
    {
        delete pobjRtpBuf;
        RTP_TRACE_WARNING("On Rtp packet failed ..! OnRtpPacket", RTP_ZERO, RTP_ZERO);
        return eRTP_FALSE;
    }

    delete pobjRtpBuf;

    return eRTP_TRUE;
}

GLOBAL eRtp_Bool IMS_RtpSvc_ProcRtpPacket(IN RtpServiceListener* pvIRtpSession,
        IN RTPSESSIONID hRtpSession, IN RtpDt_UChar* pMsg, IN RtpDt_UInt16 wMsgLength,
        IN RtpDt_Char* pDestIp, IN RtpDt_UInt16 wDestPort, OUT RtpDt_UInt32* uiDestSsrc)
{
    (RtpDt_Void) uiDestSsrc;

    tRtpSvc_IndicationFromStack stackInd = RTPSVC_RECEIVE_RTP_IND;
    RtpSession* pobjRtpSession = (RtpSession*)hRtpSession;

    if (g_pobjRtpStack == RTP_NULL ||
            g_pobjRtpStack->isRtpSessionPresent(pobjRtpSession) == eRTP_FAILURE)
    {
        return eRTP_FALSE;
    }

    if (pobjRtpSession->isRtpEnabled() == eRTP_FALSE)
    {
        return eRTP_FALSE;
    }

    RtpPacket* pobjRtpPkt = new RtpPacket();
    if (pobjRtpPkt == RTP_NULL)
    {
        RTP_TRACE_WARNING(
                "IMS_RtpSvc_ProcRtpPacket pobjRtpPkt - Malloc failed", RTP_ZERO, RTP_ZERO);
        return eRTP_FALSE;
    }

    RtpBuffer objRtpBuf;
    objRtpBuf.setBufferInfo(wMsgLength, pMsg);
    RtpDt_UInt32 uiTransLen = Rtp_Strlen((const RtpDt_Char*)pDestIp);
    RtpBuffer objRmtAddr;
    objRmtAddr.setBufferInfo(uiTransLen + 1, (RtpDt_UChar*)pDestIp);

    eRTP_STATUS_CODE eStatus =
            pobjRtpSession->processRcvdRtpPkt(&objRmtAddr, wDestPort, &objRtpBuf, pobjRtpPkt);
    objRtpBuf.setBufferInfo(RTP_ZERO, RTP_NULL);
    objRmtAddr.setBufferInfo(RTP_ZERO, RTP_NULL);
    if (eStatus != RTP_SUCCESS)
    {
        if (eStatus == RTP_OWN_SSRC_COLLISION)
            pobjRtpSession->sendRtcpByePacket();

        RTP_TRACE_WARNING("process packet failed", eStatus, RTP_ZERO);
        delete pobjRtpPkt;
        return eRTP_FALSE;
    }

    // populate stRtpIndMsg
    tRtpSvcIndSt_ReceiveRtpInd stRtpIndMsg;
    stRtpIndMsg.pMsgHdr = pMsg;
    populateReceiveRtpIndInfo(&stRtpIndMsg, pobjRtpPkt);

    if (pobjRtpSession->isRtpEnabled() == eRTP_FALSE)
    {
        delete pobjRtpPkt;
        return eRTP_FALSE;
    }

    pvIRtpSession->OnPeerInd(stackInd, (RtpDt_Void*)&stRtpIndMsg);

    delete pobjRtpPkt;
    return eRTP_TRUE;
}

GLOBAL eRtp_Bool IMS_RtpSvc_SessionEnableRTP(IN RTPSESSIONID rtpSessionId)
{
    RtpSession* pobjRtpSession = (RtpSession*)rtpSessionId;

    if (g_pobjRtpStack == RTP_NULL ||
            g_pobjRtpStack->isRtpSessionPresent(pobjRtpSession) == eRTP_FAILURE)
        return eRTP_FALSE;

    if (pobjRtpSession->enableRtp() == RTP_SUCCESS)
        return eRTP_TRUE;

    return eRTP_FALSE;
}

GLOBAL eRtp_Bool IMS_RtpSvc_SessionDisableRTP(IN RTPSESSIONID rtpSessionId)
{
    RtpSession* pobjRtpSession = (RtpSession*)rtpSessionId;

    if (g_pobjRtpStack == RTP_NULL ||
            g_pobjRtpStack->isRtpSessionPresent(pobjRtpSession) == eRTP_FAILURE)
        return eRTP_FALSE;

    if (pobjRtpSession->disableRtp() == RTP_SUCCESS)
        return eRTP_TRUE;

    return eRTP_FALSE;
}

GLOBAL eRtp_Bool IMS_RtpSvc_SessionEnableRTCP(
        IN RTPSESSIONID hRtpSession, IN eRtp_Bool enableRTCPBye)
{
    RtpSession* pobjRtpSession = (RtpSession*)hRtpSession;

    if (g_pobjRtpStack == RTP_NULL ||
            g_pobjRtpStack->isRtpSessionPresent(pobjRtpSession) == eRTP_FAILURE)
        return eRTP_FALSE;

    eRTP_STATUS_CODE eRtcpStatus = pobjRtpSession->enableRtcp((eRtp_Bool)enableRTCPBye);
    if (eRtcpStatus != RTP_SUCCESS)
    {
        return eRTP_FALSE;
    }
    return eRTP_TRUE;
}

GLOBAL eRtp_Bool IMS_RtpSvc_SessionDisableRTCP(IN RTPSESSIONID hRtpSession)
{
    RtpSession* pobjRtpSession = (RtpSession*)hRtpSession;
    eRTP_STATUS_CODE eRtcpStatus = RTP_SUCCESS;

    if (g_pobjRtpStack == RTP_NULL ||
            g_pobjRtpStack->isRtpSessionPresent(pobjRtpSession) == eRTP_FAILURE)
        return eRTP_FALSE;

    eRtcpStatus = pobjRtpSession->disableRtcp();
    if (eRtcpStatus != RTP_SUCCESS)
    {
        return eRTP_FALSE;
    }

    return eRTP_TRUE;
}

GLOBAL eRtp_Bool IMS_RtpSvc_SendRtcpByePacket(IN RTPSESSIONID hRtpSession)
{
    RtpSession* pobjRtpSession = (RtpSession*)hRtpSession;

    if (g_pobjRtpStack == RTP_NULL ||
            g_pobjRtpStack->isRtpSessionPresent(pobjRtpSession) == eRTP_FAILURE)
        return eRTP_FALSE;

    pobjRtpSession->sendRtcpByePacket();
    return eRTP_TRUE;
}

GLOBAL eRtp_Bool IMS_RtpSvc_SendRtcpRtpFbPacket(IN RTPSESSIONID hRtpSession,
        IN RtpDt_UInt32 uiFbType, IN RtpDt_Char* pcBuff, IN RtpDt_UInt32 uiLen,
        IN RtpDt_UInt32 uiMediaSsrc)
{
    RtpSession* pobjRtpSession = (RtpSession*)hRtpSession;
    if (g_pobjRtpStack == RTP_NULL ||
            g_pobjRtpStack->isRtpSessionPresent(pobjRtpSession) == eRTP_FAILURE)
        return eRTP_FALSE;

    pobjRtpSession->sendRtcpRtpFbPacket(uiFbType, pcBuff, uiLen, uiMediaSsrc);

    return eRTP_TRUE;
}

GLOBAL eRtp_Bool IMS_RtpSvc_SendRtcpPayloadFbPacket(IN RTPSESSIONID hRtpSession,
        IN RtpDt_UInt32 uiFbType, IN RtpDt_Char* pcBuff, IN RtpDt_UInt32 uiLen,
        IN RtpDt_UInt32 uiMediaSsrc)
{
    RtpSession* pobjRtpSession = (RtpSession*)hRtpSession;
    if (g_pobjRtpStack == RTP_NULL ||
            g_pobjRtpStack->isRtpSessionPresent(pobjRtpSession) == eRTP_FAILURE)
        return eRTP_FALSE;

    pobjRtpSession->sendRtcpPayloadFbPacket(uiFbType, pcBuff, uiLen, uiMediaSsrc);

    return eRTP_TRUE;
}

GLOBAL eRtp_Bool IMS_RtpSvc_ProcRtcpPacket(IN RtpServiceListener* pobjIRtpSession,
        IN RTPSESSIONID hRtpSession, IN RtpDt_UChar* pMsg, IN RtpDt_UInt16 wMsgLength,
        IN RtpDt_Char* pcIpAddr, IN RtpDt_UInt32 uiRtcpPort, OUT RtpDt_UInt32* uiDestSsrc)
{
    (RtpDt_Void) uiDestSsrc;

    RtpSession* pobjRtpSession = (RtpSession*)hRtpSession;

    if (g_pobjRtpStack == RTP_NULL ||
            g_pobjRtpStack->isRtpSessionPresent(pobjRtpSession) == eRTP_FAILURE)
        return eRTP_FALSE;

    if (pMsg == RTP_NULL || wMsgLength == RTP_ZERO || pcIpAddr == RTP_NULL)
    {
        return eRTP_FALSE;
    }

    RtpBuffer objRmtAddr;
    objRmtAddr.setBuffer((RtpDt_UChar*)pcIpAddr);
    objRmtAddr.setLength(Rtp_Strlen(pcIpAddr) + 1);

    // decrypt RTCP message
    RtpBuffer objRtcpBuf;
    objRtcpBuf.setBufferInfo(wMsgLength, pMsg);

    // process RTCP message
    RtcpPacket objRtcpPkt;
    eRTP_STATUS_CODE eProcRtcpSta =
            pobjRtpSession->processRcvdRtcpPkt(&objRmtAddr, uiRtcpPort, &objRtcpBuf, &objRtcpPkt);

    // clean the data
    objRtcpBuf.setBufferInfo(RTP_ZERO, RTP_NULL);
    objRmtAddr.setBufferInfo(RTP_ZERO, RTP_NULL);
    RtpBuffer objRtpPayload;
    objRtpPayload.setBufferInfo(RTP_ZERO, RTP_NULL);

    if (eProcRtcpSta != RTP_SUCCESS)
    {
        RTP_TRACE_WARNING("Rtcp packet processing is  failed", RTP_ZERO, RTP_ZERO);
        return eRTP_FALSE;
    }

    // inform to application
    std::list<RtcpSrPacket*>& pobjSrList = objRtcpPkt.getSrPacketList();
    if (pobjSrList.size() > RTP_ZERO)
    {
        tRtpSvc_IndicationFromStack stackInd = RTPSVC_RECEIVE_RTCP_SR_IND;
        tNotifyReceiveRtcpSrInd stSrRtcpMsg;

        if (populateRcvdSrInfoFromStk(pobjSrList, &stSrRtcpMsg) == eRTP_TRUE)
        {
            pobjIRtpSession->OnPeerInd(stackInd, (RtpDt_Void*)&stSrRtcpMsg);
        }

        // RtpDt_UInt32 rttd = pobjRtpSession->getRTTD();
        // pobjIRtpSession->OnPeerRtcpComponents((RtpDt_Void*)&rttd);
    }
    else
    {
        std::list<RtcpRrPacket*>& pobjRrList = objRtcpPkt.getRrPacketList();
        if (pobjRrList.size() > RTP_ZERO)
        {
            tRtpSvc_IndicationFromStack stackInd = RTPSVC_RECEIVE_RTCP_RR_IND;
            tNotifyReceiveRtcpRrInd stRrRtcpMsg;

            if (populateRcvdRrInfoFromStk(pobjRrList, &stRrRtcpMsg) == eRTP_TRUE)
            {
                pobjIRtpSession->OnPeerInd(stackInd, (RtpDt_Void*)&stRrRtcpMsg);
            }

            // RtpDt_UInt32 rttd = pobjRtpSession->getRTTD();
            // pobjIRtpSession->OnPeerRtcpComponents((RtpDt_Void*)&rttd);
        }
    }  // end else

    // process rtcp fb packet and inform to application
    std::list<RtcpFbPacket*>& pobjFbList = objRtcpPkt.getFbPacketList();

    for (auto& pobjFbPkt : pobjFbList)
    {
        // get Fb packet data
        if (pobjFbPkt == RTP_NULL)
        {
            return eRTP_FALSE;
        }
        tRtpSvc_IndicationFromStack stackInd = RTPSVC_RECEIVE_RTCP_FB_IND;
        if (pobjFbPkt->getRtcpHdrInfo()->getPacketType() == RTCP_PSFB)
        {
            stackInd = RTPSVC_RECEIVE_RTCP_PAYLOAD_FB_IND;
        }
        tRtpSvcIndSt_ReceiveRtcpFeedbackInd stFbRtcpMsg;
        if (populateRcvdFbInfoFromStk(pobjFbPkt, &stFbRtcpMsg) == eRTP_TRUE)
            pobjIRtpSession->OnPeerInd(stackInd, (RtpDt_Void*)&stFbRtcpMsg);
    }  // pobjFbList End

    return eRTP_TRUE;
}

GLOBAL eRtp_Bool IMS_RtpSvc_SendRtcpXrPacket(IN RTPSESSIONID hRtpSession,
        IN RtpDt_UChar* m_pBlockBuffer, IN RtpDt_UInt16 nblockLength, IN RtpDt_UInt16 nRttdOffset)
{
    RTP_TRACE_MESSAGE("IMS_RtpSvc_SendRtcpXrPacket", 0, 0);

    RtpSession* pobjRtpSession = (RtpSession*)hRtpSession;
    pobjRtpSession->sendRtcpXrPacket(m_pBlockBuffer, nblockLength, nRttdOffset);

    return eRTP_TRUE;
}
