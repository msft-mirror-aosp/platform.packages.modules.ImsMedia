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

#include <RtcpDecoderNode.h>
#include <ImsMediaTrace.h>

RtcpDecoderNode::RtcpDecoderNode()
{
    mRtpSession = NULL;
    mInactivityTime = 0;
    mNoRtcpTime = 0;
}

RtcpDecoderNode::~RtcpDecoderNode() {}

BaseNode* RtcpDecoderNode::GetInstance()
{
    BaseNode* pNode;
    pNode = new RtcpDecoderNode();

    if (pNode == NULL)
    {
        IMLOGE0("[GetInstance] Can't create RtcpDecoderNode");
    }
    return pNode;
}

void RtcpDecoderNode::ReleaseInstance(BaseNode* pNode)
{
    delete (RtcpDecoderNode*)pNode;
}

BaseNodeID RtcpDecoderNode::GetNodeID()
{
    return NODEID_RTCPDECODER;
}

ImsMediaResult RtcpDecoderNode::Start()
{
    IMLOGD0("[Start]");
    if (mRtpSession == NULL)
    {
        mRtpSession = IRtpSession::GetInstance(mMediaType, mLocalAddress, mPeerAddress);
        if (mRtpSession == NULL)
        {
            IMLOGE0("[Start] Can't create rtp session");
            return RESULT_NOT_READY;
        }
    }
    mRtpSession->SetRtcpDecoderListener(this);
    mRtpSession->StartRtcp();
    mNoRtcpTime = 0;
    mNodeState = kNodeStateRunning;
    return RESULT_SUCCESS;
}

void RtcpDecoderNode::Stop()
{
    IMLOGD0("[Stop]");
    if (mRtpSession)
    {
        mRtpSession->StopRtcp();
        mRtpSession->SetRtcpDecoderListener(NULL);
        IRtpSession::ReleaseInstance(mRtpSession);
        mRtpSession = NULL;
    }
    mNodeState = kNodeStateStopped;
}

void RtcpDecoderNode::OnDataFromFrontNode(ImsMediaSubType subtype, uint8_t* pData,
        uint32_t nDataSize, uint32_t nTimeStamp, bool bMark, uint32_t nSeqNum,
        ImsMediaSubType nDataType)
{
    (void)nDataType;

    IMLOGD_PACKET6(IM_PACKET_LOG_RTCP,
            "[OnMediaDataInd] media[%d] subtype[%d], Size[%d], TS[%u], Mark[%d], Seq[%d]",
            mMediaType, subtype, nDataSize, nTimeStamp, bMark, nSeqNum);
    mRtpSession->ProcRtcpPacket(pData, nDataSize);
}

bool RtcpDecoderNode::IsRunTime()
{
    return true;
}

bool RtcpDecoderNode::IsSourceNode()
{
    return false;
}

void RtcpDecoderNode::SetConfig(void* config)
{
    if (config == NULL)
        return;
    RtpConfig* pConfig = reinterpret_cast<RtpConfig*>(config);
    mPeerAddress = RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort());
    IMLOGD2("[SetConfig] peer Ip[%s], port[%d]", mPeerAddress.ipAddress, mPeerAddress.port);
}

bool RtcpDecoderNode::IsSameConfig(void* config)
{
    if (config == NULL)
        return true;
    RtpConfig* pConfig = reinterpret_cast<RtpConfig*>(config);
    RtpAddress peerAddress =
            RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort());

    return (mPeerAddress == peerAddress);
}

void RtcpDecoderNode::OnRtcpInd(tRtpSvc_IndicationFromStack eIndType, void* pMsg)
{
    (void)pMsg;
    IMLOGD_PACKET1(IM_PACKET_LOG_RTCP, "[OnRtcpInd] type[%d]", eIndType);
}

void RtcpDecoderNode::OnNumReceivedPacket(uint32_t nNumRtcpSRPacket, uint32_t nNumRtcpRRPacket)
{
    IMLOGD_PACKET3(IM_PACKET_LOG_RTCP,
            "[OnNumReceivedPacket] InactivityTime[%d], numRtcpSR[%d], numRtcpRR[%d]",
            mInactivityTime, nNumRtcpSRPacket, nNumRtcpRRPacket);

    if (nNumRtcpSRPacket == 0 && nNumRtcpRRPacket == 0)
    {
        mNoRtcpTime++;
    }
    else
    {
        mNoRtcpTime = 0;
    }

    if (mInactivityTime != 0 && mNoRtcpTime == mInactivityTime)
    {
        if (mCallback != NULL)
        {
            mCallback->SendEvent(EVENT_NOTIFY_MEDIA_INACITIVITY, RTCP, mInactivityTime);
        }
    }
}

void RtcpDecoderNode::SetLocalAddress(const RtpAddress address)
{
    mLocalAddress = address;
}

void RtcpDecoderNode::SetPeerAddress(const RtpAddress address)
{
    mPeerAddress = address;
}

void RtcpDecoderNode::SetInactivityTimerSec(const uint32_t time)
{
    IMLOGD1("[SetInactivityTimerSec] time[%d]", time);
    mInactivityTime = time;
}