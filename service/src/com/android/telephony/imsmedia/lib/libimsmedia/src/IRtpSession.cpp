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

#include <IRtpSession.h>
#include <RtpService.h>
#include <ImsMediaTrace.h>

std::list<IRtpSession*> IRtpSession::mListRtpSession;

IRtpSession* IRtpSession::GetInstance(
        ImsMediaType eMediaType, const RtpAddress local, const RtpAddress peer)
{
    IMLOGD1("[GetInstance] eMediaType[%d]", eMediaType);
    for (auto& i : mListRtpSession)
    {
        if (i->isSameInstance(eMediaType, local, peer))
        {
            i->increaseRefCounter();
            return i;
        }
    }
    if (mListRtpSession.size() == 0)
    {
        IMLOGD0("[GetInstance] Initialize Rtp Stack");
        IMS_RtpSvc_Initialize();
    }
    IRtpSession* pSession = new IRtpSession(eMediaType, local, peer);
    mListRtpSession.push_back(pSession);
    pSession->increaseRefCounter();
    return pSession;
}

void IRtpSession::ReleaseInstance(IRtpSession* pSession)
{
    IMLOGD2("[ReleaseInstance] MediaType[%x], mRefCount[%d]", pSession->getMediaType(),
            pSession->getRefCounter());
    pSession->decreaseRefCounter();
    if (pSession->getRefCounter() == 0)
    {
        mListRtpSession.remove(pSession);
        delete pSession;
    }
    if (mListRtpSession.size() == 0)
    {
        IMLOGD0("[ReleaseInstance] Deinitialize Rtp Stack");
        IMS_RtpSvc_Deinitialize();
    }

    IMLOGD0("[ReleaseInstance] Exit");
}

IRtpSession::IRtpSession(ImsMediaType eMediaType, const RtpAddress local, const RtpAddress peer)
{
    mMediaType = eMediaType;
    mRefCount = 0;
    mLocalRtpSsrc = 0;
    mPeerRtpSsrc = 0;
    mPrevTimestamp = -1;
    mEnableDTMF = false;
    mDtmfPayloadType = 0;
    mNumRtpProcPacket = 0;
    mNumRtcpProcPacket = 0;
    mNumRtpPacket = 0;
    mNumSRPacket = 0;
    mNumRRPacket = 0;
    mNumRtpDataToSend = 0;
    mNumRtpPacketSent = 0;
    mNumRtcpPacketSent = 0;
    mRttd = -1;
    mRtpSessionId = 0;
    mLocalAddress = local;
    mPeerAddress = peer;
    mRtpStarted = 0;
    mRtcpStarted = 0;
    mRtpEncoderListener = NULL;
    mRtpDecoderListener = NULL;
    mRtcpEncoderListener = NULL;
    mRtcpDecoderListener = NULL;
    std::memset(mPayloadParam, 0, sizeof(tRtpSvc_SetPayloadParam) * MAX_NUM_PAYLOAD_PARAM);
    // create rtp stack session
    IMS_RtpSvc_CreateSession(
            mLocalAddress.ipAddress, mLocalAddress.port, this, &mLocalRtpSsrc, &mRtpSessionId);
    IMLOGD5("[IRtpSession] localIp[%s], localPort[%d], peerIp[%s], peerPort[%d], sessionId[%d]",
            mLocalAddress.ipAddress, mLocalAddress.port, mPeerAddress.ipAddress, mPeerAddress.port,
            mRtpSessionId);
}

IRtpSession::~IRtpSession()
{
    IMS_RtpSvc_DeleteSession(mRtpSessionId);
    mRtpEncoderListener = NULL;
    mRtpDecoderListener = NULL;
    mRtcpEncoderListener = NULL;
    mRtcpDecoderListener = NULL;
}

bool IRtpSession::operator==(const IRtpSession& obj2)
{
    if (mMediaType == obj2.mMediaType && mLocalAddress == obj2.mLocalAddress &&
            mPeerAddress == obj2.mPeerAddress)
    {
        return true;
    }
    return false;
}

bool IRtpSession::isSameInstance(
        ImsMediaType eMediaType, const RtpAddress local, const RtpAddress peer)
{
    if (mMediaType == eMediaType && mLocalAddress == local && mPeerAddress == peer)
    {
        return true;
    }
    return false;
}

void IRtpSession::SetRtpEncoderListener(IRtpEncoderListener* pRtpEncoderListener)
{
    std::lock_guard<std::mutex> guard(mutexEncoder);
    mRtpEncoderListener = pRtpEncoderListener;
}

void IRtpSession::SetRtpDecoderListener(IRtpDecoderListener* pRtpDecoderListener)
{
    std::lock_guard<std::mutex> guard(mutexDecoder);
    mRtpDecoderListener = pRtpDecoderListener;
}

void IRtpSession::SetRtcpEncoderListener(IRtcpEncoderListener* pRtcpEncoderListener)
{
    std::lock_guard<std::mutex> guard(mutexEncoder);
    mRtcpEncoderListener = pRtcpEncoderListener;
}

void IRtpSession::SetRtcpDecoderListener(IRtcpDecoderListener* pRtcpDecoderListener)
{
    std::lock_guard<std::mutex> guard(mutexDecoder);
    mRtcpDecoderListener = pRtcpDecoderListener;
}

void IRtpSession::SetRtpPayloadParam(RtpConfig* config)
{
    if (config == NULL)
        return;
    // separate local & peer payload type
    mNumPayloadParam = 0;
    std::memset(mPayloadParam, 0, sizeof(tRtpSvc_SetPayloadParam) * MAX_NUM_PAYLOAD_PARAM);
    IMLOGD3("[SetRtpPayloadParam] localPayload[%d], peerPayload[%d], sampling[%d]",
            config->getTxPayloadTypeNumber(), config->getRxPayloadTypeNumber(),
            config->getSamplingRateKHz());

    if (mNumPayloadParam >= 3)
    {
        IMLOGE1("[SetRtpPayloadParam] overflow[%d]", mNumPayloadParam);
        return;
    }

    // tx parameter
    mPayloadParam[mNumPayloadParam].frameInterval = 100;  // not used in stack
    mPayloadParam[mNumPayloadParam].payloadType = config->getTxPayloadTypeNumber();
    mPayloadParam[mNumPayloadParam].samplingRate = config->getSamplingRateKHz() * 1000;
    mNumPayloadParam++;

    if (config->getTxPayloadTypeNumber() != config->getRxPayloadTypeNumber())
    {
        if (mNumPayloadParam >= 3)
        {
            IMLOGE1("[SetRtpPayloadParam] overflow[%d]", mNumPayloadParam);
            return;
        }
        // rx parameter
        mPayloadParam[mNumPayloadParam].frameInterval = 100;  // not used in stack
        mPayloadParam[mNumPayloadParam].payloadType = config->getRxPayloadTypeNumber();
        mPayloadParam[mNumPayloadParam].samplingRate = config->getSamplingRateKHz() * 1000;
        mNumPayloadParam++;
    }
}

void IRtpSession::SetRtpDtmfPayloadParam(AudioConfig* config)
{
    if (config == NULL)
        return;
    IMLOGD2("[SetRtpDtmfPayloadParam] payload[%d], samplingRate[%d]",
            config->getDtmfPayloadTypeNumber(), config->getDtmfsamplingRateKHz());

    mEnableDTMF = false;
    if (mMediaType != IMS_MEDIA_AUDIO)
        return;
    if (config->getDtmfPayloadTypeNumber() == 0)
        return;

    mEnableDTMF = true;
    mDtmfPayloadType = config->getDtmfPayloadTypeNumber();
    mPayloadParam[mNumPayloadParam].frameInterval = 100;  // not used in stack
    mPayloadParam[mNumPayloadParam].payloadType = config->getDtmfPayloadTypeNumber();
    mPayloadParam[mNumPayloadParam].samplingRate = config->getDtmfsamplingRateKHz() * 1000;
    mNumPayloadParam++;
}

void IRtpSession::SetRtcpInterval(int32_t nInterval)
{
    IMLOGD1("[SetRtcpInterval] nInterval[%d]", nInterval);
    IMS_RtpSvc_SetRTCPInterval(mRtpSessionId, nInterval);
}

void IRtpSession::StartRtp()
{
    IMLOGD1("[StartRtp] mbRtpStarted[%d]", mRtpStarted);
    if (mRtpStarted == 0)
    {
        IMLOGD1("[StartRtp] IMS_RtpSvc_SessionEnableRTP, numPayloadParam[%d]", mNumPayloadParam);
        IMS_RtpSvc_SetPayload(mRtpSessionId, mPayloadParam,
                eRTP_FALSE,  // fix later
                mNumPayloadParam);
        IMS_RtpSvc_SessionEnableRTP(mRtpSessionId);
    }

    mRtpStarted++;
}

void IRtpSession::StopRtp()
{
    IMLOGD1("[StopRtp] mbRtpStarted[%d]", mRtpStarted);
    if (mRtpStarted == 0)
        return;
    mRtpStarted--;
    if (mRtpStarted == 0)
    {
        IMS_RtpSvc_SessionDisableRTP(mRtpSessionId);
        IMLOGD0("[StopRtp] IMS_RtpSvc_SessionDisableRTP");
    }
    mEnableRtcpTx = false;
}

void IRtpSession::StartRtcp(bool bSendRtcpBye)
{
    IMLOGD1("[StartRtcp] nRtcpStarted[%d]", mRtcpStarted);
    if (mRtcpStarted == 0)
    {
        IMS_RtpSvc_SessionEnableRTCP(mRtpSessionId, static_cast<eRtp_Bool>(bSendRtcpBye));
    }
    mEnableRtcpTx = true;
    mRtcpStarted++;
}

void IRtpSession::StopRtcp()
{
    IMLOGD1("[StopRtcp] nRtcpStarted[%d]", mRtcpStarted);
    if (mRtcpStarted == 0)
        return;
    mRtcpStarted--;
    if (mRtcpStarted == 0)
    {
        IMLOGD0("[StopRtcp] IMS_RtpSvc_SessionDisableRtcp");
        IMS_RtpSvc_SessionDisableRTCP(mRtpSessionId);
    }
}

bool IRtpSession::SendRtpPacket(uint32_t nPayloadType, uint8_t* pData, uint32_t nDataSize,
        uint32_t nTimestamp, bool bMark, uint32_t nTimeDiff, bool bExtension, void* pExtensionInfo)
{
    (void)pExtensionInfo;

    tRtpSvc_SendRtpPacketParm stRtpPacketParam;
    memset(&stRtpPacketParam, 0, sizeof(tRtpSvc_SendRtpPacketParm));
    IMLOGD_PACKET5(IM_PACKET_LOG_RTP,
            "SendRtpPacket, payloadType[%d], size[%d], nTS[%d], bMark[%d], bExtension[%d]",
            nPayloadType, nDataSize, nTimestamp, bMark, bExtension);
    stRtpPacketParam.bMbit = bMark ? eRTP_TRUE : eRTP_FALSE;
    stRtpPacketParam.byPayLoadType = nPayloadType;
    stRtpPacketParam.diffFromLastRtpTimestamp = nTimeDiff;
    stRtpPacketParam.bXbit = bExtension ? eRTP_TRUE : eRTP_FALSE;

    /* TBD
    if (bExtension) {
        stRtpPacketParam.nDefinedByProfile = pExtensionInfo->localId;
        //rtp extension param is 1 byte
        stRtpPacketParam.nLength = 1;
        stRtpPacketParam.nExtensionData = pExtensionInfo->data;
    }
    */

    if (mPrevTimestamp == nTimestamp)
    {
        stRtpPacketParam.bUseLastTimestamp = eRTP_TRUE;
    }
    else
    {
        stRtpPacketParam.bUseLastTimestamp = eRTP_FALSE;
        mPrevTimestamp = nTimestamp;
    }

    mNumRtpDataToSend++;
    IMS_RtpSvc_SendRtpPacket(this, mRtpSessionId, (char*)pData, nDataSize, &stRtpPacketParam);
    return true;
}

bool IRtpSession::ProcRtpPacket(uint8_t* pData, uint32_t nDataSize)
{
    IMLOGD_PACKET1(IM_PACKET_LOG_RTP, "[ProcRtpPacket] size[%d]", nDataSize);
    mNumRtpProcPacket++;
    // test loopback
    unsigned int ssrc;
    ssrc = *(unsigned int*)(pData + 8);
    ssrc++;
    *(unsigned int*)(pData + 8) = ssrc;
    IMS_RtpSvc_ProcRtpPacket(this, mRtpSessionId, pData, nDataSize, mPeerAddress.ipAddress,
            mPeerAddress.port, &mPeerRtpSsrc);
    return true;
}

bool IRtpSession::ProcRtcpPacket(uint8_t* pData, uint32_t nDataSize)
{
    IMLOGD_PACKET1(IM_PACKET_LOG_RTCP, "[ProcRtcpPacket] size[%d]", nDataSize);
    mNumRtcpProcPacket++;
    IMS_RtpSvc_ProcRtcpPacket(this, mRtpSessionId, pData, nDataSize, mPeerAddress.ipAddress,
            mPeerAddress.port, &mLocalRtpSsrc);
    return true;
}

int IRtpSession::OnRtpPacket(unsigned char* pData, RtpSvc_Length wLen)
{
    std::lock_guard<std::mutex> guard(mutexEncoder);
    IMLOGD_PACKET1(IM_PACKET_LOG_RTP, "[OnRtpPacket] size[%d]", wLen);

    if (mRtpEncoderListener)
    {
        mNumRtpPacketSent++;
        /*if (mbLoopback == true) {
            unsigned int ssrc;
            ssrc = *(unsigned int*)(pData+8);
            ssrc++;
            *(unsigned int*)(pData+8) = ssrc;
            ProcRtpPacket(pData, wLen);
        } else {*/
        mRtpEncoderListener->OnRtpPacket(pData, wLen);
        //}
        return wLen;
    }
    return 0;
}

int IRtpSession::OnRtcpPacket(unsigned char* pData, RtpSvc_Length wLen)
{
    IMLOGD_PACKET0(IM_PACKET_LOG_RTCP, "[OnRtcpPacket] Enter");
    if (mEnableRtcpTx == false)
    {
        IMLOGD_PACKET0(IM_PACKET_LOG_RTCP, "[OnRtcpPacket] disabled");
        return wLen;
    }

    std::lock_guard<std::mutex> guard(mutexEncoder);
    if (mRtcpEncoderListener)
    {
        if (pData != NULL)
        {
            mNumRtcpPacketSent++;
            mRtcpEncoderListener->OnRtcpPacket(pData, wLen);
            IMLOGD_PACKET0(IM_PACKET_LOG_RTCP, "[OnRtcpPacket] Send, Exit");
            return wLen;
        }
        else
        {
            IMLOGD_PACKET1(IM_PACKET_LOG_RTCP, "[OnRtcpPacket] pData[%x]", pData);
            return 0;
        }
    }
    return 0;
}

void IRtpSession::OnPeerInd(tRtpSvc_IndicationFromStack eIndType, void* pMsg)
{
    std::lock_guard<std::mutex> guard(mutexDecoder);
    switch (eIndType)
    {
        case RTPSVC_RECEIVE_RTP_IND:
            IMLOGD_PACKET1(IM_PACKET_LOG_RTP, "[OnPeerInd] RTP eIndType[%d]", eIndType);
            mNumRtpPacket++;
            if (mRtpDecoderListener)
            {
                tRtpSvcIndSt_ReceiveRtpInd* pstRtp = (tRtpSvcIndSt_ReceiveRtpInd*)pMsg;
                uint32_t nSSRC = 0;

                if (pstRtp->wMsgHdrLen >= 12)
                {
                    nSSRC = pstRtp->pMsgHdr[8];
                    nSSRC <<= 8;
                    nSSRC += pstRtp->pMsgHdr[9];
                    nSSRC <<= 8;
                    nSSRC += pstRtp->pMsgHdr[10];
                    nSSRC <<= 8;
                    nSSRC += pstRtp->pMsgHdr[11];
                }
                if ((mEnableDTMF == false || mDtmfPayloadType != pstRtp->dwPayloadType) &&
                        pstRtp->dwPayloadType != 20)
                {
                    mRtpDecoderListener->OnMediaDataInd(pstRtp->pMsgBody, pstRtp->wMsgBodyLen,
                            pstRtp->dwTimestamp, pstRtp->bMbit, pstRtp->dwSeqNum,
                            pstRtp->dwPayloadType, nSSRC, pstRtp->bExtension,
                            pstRtp->extensionData);
                }
            }
            break;

        case RTPSVC_RECEIVE_RTCP_SR_IND:
            IMLOGD_PACKET1(IM_PACKET_LOG_RTCP, "[OnPeerInd] RtcpSr-eIndType[%d]", eIndType);
            mNumSRPacket++;
            if (mRtcpDecoderListener)
            {
                tNotifyReceiveRtcpSrInd* pstRtcp = (tNotifyReceiveRtcpSrInd*)pMsg;
                IMLOGD_PACKET2(IM_PACKET_LOG_RTCP,
                        "[OnPeerInd] RtcpSr-fractionLost[%d], jitter[%d]",
                        pstRtcp->stRecvRpt.fractionLost, pstRtcp->stRecvRpt.jitter);
                mRtcpDecoderListener->OnRtcpInd(eIndType, pstRtcp);
            }
            break;
        case RTPSVC_RECEIVE_RTCP_RR_IND:
            IMLOGD_PACKET1(IM_PACKET_LOG_RTCP, "[OnPeerInd] RtcpRr-eIndType[%d]", eIndType);
            mNumRRPacket++;
            if (mRtcpDecoderListener)
            {
                tNotifyReceiveRtcpRrInd* pstRtcp = (tNotifyReceiveRtcpRrInd*)pMsg;
                IMLOGD_PACKET2(IM_PACKET_LOG_RTCP,
                        "[OnPeerInd] RtcpRr-fractionLost[%d], jitter[%d]",
                        pstRtcp->stRecvRpt.fractionLost, pstRtcp->stRecvRpt.jitter);
                mRtcpDecoderListener->OnRtcpInd(eIndType, pstRtcp);
            }
            break;
        case RTPSVC_SSRC_COLLISION_CHANGED_IND:
            IMLOGD_PACKET0(IM_PACKET_LOG_RTCP, "[OnPeerInd] RTPSVC_SSRC_COLLISION_CHANGED_IND]");
            break;
        case RTPSVC_RECEIVE_RTCP_FB_IND:
        case RTPSVC_RECEIVE_RTCP_PAYLOAD_FB_IND:
            IMLOGD1("[OnPeerInd] RtpSvc_RtcpFeedbackInd[%d]", eIndType);
            break;
        default:
            IMLOGD1("[OnPeerInd] unhandled[%d]", eIndType);
            break;
    }
}

void IRtpSession::OnTimer()
{
    std::lock_guard<std::mutex> guard(mutexDecoder);

    IMLOGD7("[OnTimer] RX_Rtp[%03d/%03d], RX_Rtcp[%02d/%02d], TX_Rtp[%03d/%03d], TX_Rtcp[%02d]",
            mNumRtpProcPacket, mNumRtpPacket, mNumRtcpProcPacket, mNumSRPacket + mNumRRPacket,
            mNumRtpDataToSend, mNumRtpPacketSent, mNumRtcpPacketSent);

    if (mRtpDecoderListener)
    {
        mRtpDecoderListener->OnNumReceivedPacket(mNumRtpProcPacket);
    }

    if (mRtcpDecoderListener)
    {
        mRtcpDecoderListener->OnNumReceivedPacket(mNumRtcpProcPacket, mNumRRPacket);
    }

    mNumRtpProcPacket = 0;
    mNumRtcpProcPacket = 0;
    mNumRtpPacket = 0;
    mNumSRPacket = 0;
    mNumRRPacket = 0;
    mNumRtpDataToSend = 0;
    mNumRtpPacketSent = 0;
    mNumRtcpPacketSent = 0;
}

void IRtpSession::SendRtcpXr(uint8_t* pPayload, uint32_t nSize, uint32_t nRttdOffset)
{
    IMLOGD1("SendRtcpXr, nSize[%d]", nSize);
    if (mRtpSessionId)
    {
        IMS_RtpSvc_SendRtcpXrPacket(mRtpSessionId, pPayload, nSize, nRttdOffset);
    }
}

ImsMediaType IRtpSession::getMediaType()
{
    return mMediaType;
}

void IRtpSession::increaseRefCounter()
{
    ++mRefCount;
    IMLOGD1("[increaseRefCounter] count[%d]", mRefCount.load());
}

void IRtpSession::decreaseRefCounter()
{
    --mRefCount;
    IMLOGD1("[decreaseRefCounter] count[%d]", mRefCount.load());
}

uint32_t IRtpSession::getRefCounter()
{
    return mRefCount.load();
}