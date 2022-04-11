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

#include <RtcpEncoderNode.h>
#include <ImsMediaTrace.h>

RtcpEncoderNode::RtcpEncoderNode() {
    mRtpSession = NULL;
    mEnableRtcpBye = false;
    mRtcpXrPayload = NULL;
    mRtcpXrBlockType = RtcpConfig::FLAG_RTCPXR_NONE;
    mRtcpXrCounter = 0;
    m_hTimer = NULL;
}

RtcpEncoderNode::~RtcpEncoderNode() {
    mRtcpXrBlockType = RtcpConfig::FLAG_RTCPXR_NONE;
    mRtcpXrCounter = 0;
}

BaseNode* RtcpEncoderNode::GetInstance() {
    BaseNode* pNode;
    pNode = new RtcpEncoderNode();

    if (pNode == NULL) {
        IMLOGE0("[GetInstance] - Can't create RtcpEncoderNode");
    }
    return pNode;
}

void RtcpEncoderNode::ReleaseInstance(BaseNode* pNode) {
    delete (RtcpEncoderNode*)pNode;
}

BaseNodeID RtcpEncoderNode::GetNodeID() {
    return NODEID_RTCPENCODER;
}

ImsMediaResult RtcpEncoderNode::Start() {
    std::lock_guard<std::mutex> guard(mMutexTimer);
    if (mRtpSession == NULL) {
        mRtpSession = IRtpSession::GetInstance(mMediaType, mLocalAddress, mPeerAddress);
        if (mRtpSession == NULL) {
            IMLOGE0("[Start] Can't create rtp session");
            return RESULT_NOT_READY;
        }
    }

    IMLOGD3("[Start] interval[%d], rtcpBye[%d], rtcpXrBlock[%d]",
        mRtcpInterval, mEnableRtcpBye, mRtcpXrBlockType);

    mRtpSession->SetRtcpEncoderListener(this);
    mRtpSession->SetRtcpInterval(mRtcpInterval);
    mRtpSession->StartRtcp(mEnableRtcpBye);

    if (m_hTimer == NULL) {
        m_hTimer = ImsMediaTimer::TimerStart(1000, true, OnTimer, this);
        IMLOGD0("[Start] Rtcp Timer started");
    }
    mRtcpXrCounter = 0;
    mNodeState = NODESTATE_RUNNING;
    return RESULT_SUCCESS;
}

void RtcpEncoderNode::Stop() {
    IMLOGD0("[Stop]");
    std::lock_guard<std::mutex> guard(mMutexTimer);
    if (mRtpSession != NULL) {
        mRtpSession->StopRtcp();
        mRtpSession->SetRtcpEncoderListener(NULL);
        IRtpSession::ReleaseInstance(mRtpSession);
        mRtpSession = NULL;
    }

    if (m_hTimer != NULL) {
        ImsMediaTimer::TimerStop(m_hTimer, NULL);
        m_hTimer = NULL;
        IMLOGD0("[Stop] Rtcp Timer stopped");
    }
    mNodeState = NODESTATE_STOPPED;
}

bool RtcpEncoderNode::IsRunTime() {
    return true;
}

bool RtcpEncoderNode::IsSourceNode() {
    return true;
}

void RtcpEncoderNode::SetConfig(void* config) {
    RtpConfig* pConfig = reinterpret_cast<RtpConfig*>(config);
    mPeerAddress = RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort());
    SetRtcpInterval(pConfig->getRtcpConfig().getIntervalSec());
    SetRtcpXrBlockType(pConfig->getRtcpConfig().getRtcpXrBlockTypes());
    SetRtcpByeEnable(false);
    IMLOGD4("[SetConfig] peer Ip[%s], port[%d], interval[%d], rtcpxr[%d]", mPeerAddress.ipAddress,
        mPeerAddress.port, mRtcpInterval, mRtcpXrBlockType);
}

bool RtcpEncoderNode::IsSameConfig(void* config) {
    if (config == NULL) return true;
    RtpConfig* pConfig = reinterpret_cast<RtpConfig*>(config);
    RtpAddress peerAddress = RtpAddress(pConfig->getRemoteAddress().c_str(),
        pConfig->getRemotePort());

    return (mPeerAddress == peerAddress
        && mRtcpInterval == pConfig->getRtcpConfig().getIntervalSec()
        && mRtcpXrBlockType == pConfig->getRtcpConfig().getRtcpXrBlockTypes());
}

void RtcpEncoderNode::OnRtcpPacket(unsigned char* pData, uint32_t wLen) {
    ImsMediaSubType subtype = MEDIASUBTYPE_RTCPPACKET;

    if (mEnableRtcpBye == true) {
        uint8_t* pCurr;
        int32_t nRemainSize;
        pCurr = (uint8_t*)pData;
        nRemainSize = wLen;

        while (nRemainSize >= 4) {
            uint8_t PT = pCurr[1];
            uint32_t length;
            IMLOGD_PACKET1(IM_PACKET_LOG_RTCP, "[OnRtcpPacket] PT[%d]", PT);

            if (PT == 203) {
                subtype = MEDIASUBTYPE_RTCPPACKET_BYE;
                break;
            }

            length = pCurr[2];
            length <<= 8;
            length += pCurr[3];
            length = (length+1)*4;

            pCurr += length;
            nRemainSize -= length;
        }
    }

    SendDataToRearNode(subtype, pData, wLen, 0, 0, 0);
}

void RtcpEncoderNode::OnTimer(hTimerHandler hTimer, void* pUserData) {
    (void)hTimer;
    RtcpEncoderNode* pNode = (RtcpEncoderNode*)pUserData;

    if (pNode != NULL) {
        pNode->ProcessTimer();
    }
}

void RtcpEncoderNode::ProcessTimer() {
    std::lock_guard<std::mutex> guard(mMutexTimer);

    if (m_hTimer == NULL || mRtpSession == NULL) return;

    mRtpSession->OnTimer();
}

void RtcpEncoderNode::SetLocalAddress(const RtpAddress address) {
    mLocalAddress = address;
}

void RtcpEncoderNode::SetPeerAddress(const RtpAddress address) {
    mPeerAddress = address;
}

void RtcpEncoderNode::SetRtcpInterval(const uint32_t interval) {
    mRtcpInterval = interval;
}

void RtcpEncoderNode::SetRtcpXrBlockType(const uint32_t rtcpXrBlockType) {
    mRtcpXrBlockType = rtcpXrBlockType;
}

void RtcpEncoderNode::SetRtcpByeEnable(const bool bEnable) {
    mEnableRtcpBye = bEnable;
}

/*
void RtcpEncoderNode::BuildRtcpXrStatisticsBuffer(uint8_t* pBuffer,
    RTCPXR_StatisticsParam* statisticParam) {

    if (pBuffer == NULL || statisticParam == NULL)  return;

    MMPFMemory::MemSet(pBuffer, 0, BLOCK_LENGTH_STATISTICS);
    m_objBitWriter.SetBuffer(pBuffer, BLOCK_LENGTH_STATISTICS);
    m_objBitWriter.Write(6, 8);
    //flag of lost and duplicated packets --> always set
    m_objBitWriter.Write(1, 1);
    m_objBitWriter.Write(1, 1);
    //flag of jitter
    m_objBitWriter.Write(1, 1);
    // TTL and HL : 0 - not using, 1 - IPv4, 2 - IPv6, 3 must not used
#ifdef QCT_SOCKET_TTL_RECEIVE
    m_objBitWriter.Write(2, 2);
#else
    m_objBitWriter.Write(0, 2);
#endif
    //padding
    m_objBitWriter.Write(0, 3);
    //block length
    m_objBitWriter.Write(9, 16);
    //ssrc of source
    m_objBitWriter.WriteByteBuffer(statisticParam->nSSRC);
    //sequence number
    m_objBitWriter.Write(statisticParam->nBeginSeqNum, 16);
    m_objBitWriter.Write(statisticParam->nEndSeqNum, 16);
    //lost packets
    m_objBitWriter.WriteByteBuffer(statisticParam->nCntLossPkts);
    //dup packets
    m_objBitWriter.WriteByteBuffer(statisticParam->nCntDuplicatedPkts);
    IMLOGD4("[BuildRtcpXrStatisticsBuffer] nMinJ[%d], nMaxJ[%d], nMeanJ[%d], nDevJ[%d]",
        statisticParam->nMinJitter, statisticParam->nMaxJitter,
        statisticParam->nMeanJitter, statisticParam->nDevJitter);
    //min, max, mean, dev jitter
    m_objBitWriter.WriteByteBuffer(statisticParam->nMinJitter);
    m_objBitWriter.WriteByteBuffer(statisticParam->nMaxJitter);
    m_objBitWriter.WriteByteBuffer(statisticParam->nMeanJitter);
    m_objBitWriter.WriteByteBuffer(statisticParam->nDevJitter);
    IMLOGD4("[BuildRtcpXrStatisticsBuffer] nMinTTL[%d], nMaxTTL[%d], nMeanTTL[%d], nDevTTL[%d]",
        statisticParam->nMinTTL, statisticParam->nMaxTTL,
        statisticParam->nMeanTTL, statisticParam->nDevTTL);
    //min, max, mean, dev ttl/hl
    m_objBitWriter.Write(statisticParam->nMinTTL, 8);
    m_objBitWriter.Write(statisticParam->nMaxTTL, 8);
    m_objBitWriter.Write(statisticParam->nMeanTTL, 8);
    m_objBitWriter.Write(statisticParam->nDevTTL, 8);
}

void RtcpEncoderNode::BuildRtcpXrVoipMetricsBuffer(uint8_t* pBuffer,
    RTCPXR_VoIPMatricParam* voipParam) {

    if (pBuffer == NULL || voipParam == NULL)  return;

    MMPFMemory::MemSet(pBuffer, 0, BLOCK_LENGTH_VOIP_METRICS);
    m_objBitWriter.SetBuffer(pBuffer, BLOCK_LENGTH_VOIP_METRICS);
    m_objBitWriter.Write(7, 8);
    m_objBitWriter.Write(0, 8);
    //block length
    m_objBitWriter.Write(8, 16);
    //ssrc of source
    m_objBitWriter.WriteByteBuffer(voipParam->nSSRC);
    m_objBitWriter.Write(voipParam->nLossRate, 8);
    m_objBitWriter.Write(voipParam->nDiscardRate, 8);
    m_objBitWriter.Write(voipParam->nBurstDensity, 8);
    m_objBitWriter.Write(voipParam->nGapDensity, 8);
    m_objBitWriter.Write(voipParam->nBurstDuration, 16);
    m_objBitWriter.Write(voipParam->nGapDuration, 16);
    m_objBitWriter.Write(voipParam->nRoundTripDelay, 16);
    m_objBitWriter.Write(voipParam->nEndSystemDelay, 16);
    //signal level - 127 unavailable
    m_objBitWriter.Write(127, 8);
    m_objBitWriter.Write(127, 8);
    m_objBitWriter.Write(127, 8);
    m_objBitWriter.Write(voipParam->nGmin, 8);
    //R factor - 127 unavailable
    m_objBitWriter.Write(127, 8);
    m_objBitWriter.Write(127, 8);
    //MOS - 127 unavailable
    m_objBitWriter.Write(127, 8);
    m_objBitWriter.Write(127, 8);
    //receiver configuration byte(Rx Config)
    m_objBitWriter.Write(voipParam->nRxConfig, 8);
    m_objBitWriter.Write(0, 8);
    //Jitter Buffer - milliseconds
    m_objBitWriter.Write(voipParam->nJBNominal, 16);
    m_objBitWriter.Write(voipParam->nJBMaximum, 16);
    m_objBitWriter.Write(voipParam->nJBAbsMaximum, 16);
}

bool RtcpEncoderNode::SendRtcpXrPacket(
    RTCPXR_StatisticsParam* statisticParam, RTCPXR_VoIPMatricParam* voipParam) {
    IMLOGD0("[SendRtcpXrPacket] Enter");

    if (mRtpSession == NULL) return false;

    if (mRtcpXrPayload == NULL) {
        mRtcpXrPayload = (uint8_t*)malloc(sizeof(uint8_t) *
            (BLOCK_LENGTH_STATISTICS + BLOCK_LENGTH_VOIP_METRICS));
    }

    uint8_t* pblockPt = mRtcpXrPayload;
    uint32_t nblockLength = 0;
    uint32_t nRttdOffset = 0;

    if (pblockPt != NULL && statisticParam != NULL) {
        BuildRtcpXrStatisticsBuffer(pblockPt, statisticParam);
        pblockPt += BLOCK_LENGTH_STATISTICS;
        nblockLength += BLOCK_LENGTH_STATISTICS;
    }

    if (pblockPt != NULL && voipParam != NULL) {
        BuildRtcpXrVoipMetricsBuffer(pblockPt, voipParam);
        pblockPt += BLOCK_LENGTH_VOIP_METRICS;
        nRttdOffset = nblockLength + 16;        //round trip delay offset
        nblockLength += BLOCK_LENGTH_VOIP_METRICS;
    }

    //send buffer to packets
    mRtpSession->SendRtcpXr(mRtcpXrPayload, nblockLength, nRttdOffset);
    IMLOGD1("[SendRtcpXrPacket] Exit. blockLength[%d]", nblockLength);
    return true;
}
*/