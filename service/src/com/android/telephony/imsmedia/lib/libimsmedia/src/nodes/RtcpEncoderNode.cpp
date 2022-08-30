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
#include <VideoConfig.h>

#define RTCPFBMNGR_PLI_FIR_REQUEST_MIN_INTERVAL 1000

RtcpEncoderNode::RtcpEncoderNode(BaseSessionCallback* callback) :
        BaseNode(callback)
{
    mRtpSession = NULL;
    mRtcpInterval = 0;
    mRtcpXrPayload = NULL;
    mEnableRtcpBye = false;
    mRtcpXrBlockType = RtcpConfig::FLAG_RTCPXR_NONE;
    mRtcpXrCounter = 0;
    mTimer = NULL;
    mLastTimeSentPli = 0;
    mLastTimeSentFir = 0;
}

RtcpEncoderNode::~RtcpEncoderNode()
{
    if (mRtpSession != NULL)
    {
        mRtpSession->StopRtcp();
        mRtpSession->SetRtcpEncoderListener(NULL);
        IRtpSession::ReleaseInstance(mRtpSession);
        mRtpSession = NULL;
    }

    mRtcpXrBlockType = RtcpConfig::FLAG_RTCPXR_NONE;
    mRtcpXrCounter = 0;
}

kBaseNodeId RtcpEncoderNode::GetNodeId()
{
    return kNodeIdRtcpEncoder;
}

ImsMediaResult RtcpEncoderNode::Start()
{
    std::lock_guard<std::mutex> guard(mMutexTimer);

    if (mRtpSession == NULL)
    {
        mRtpSession = IRtpSession::GetInstance(mMediaType, mLocalAddress, mPeerAddress);

        if (mRtpSession == NULL)
        {
            IMLOGE0("[Start] Can't create rtp session");
            return RESULT_NOT_READY;
        }
    }

    IMLOGD4("[Start] interval[%d], rtcpBye[%d], rtcpXrBlock[%d], rtcpFbTypes[%d]", mRtcpInterval,
            mEnableRtcpBye, mRtcpXrBlockType, mRtcpFbTypes);
    mRtpSession->SetRtcpEncoderListener(this);
    mRtpSession->SetRtcpInterval(mRtcpInterval);

    if (mRtcpInterval > 0)
    {
        mRtpSession->StartRtcp(mEnableRtcpBye);
    }

    if (mTimer == NULL)
    {
        mTimer = ImsMediaTimer::TimerStart(1000, true, OnTimer, this);
        IMLOGD0("[Start] Rtcp Timer started");
    }

    mRtcpXrCounter = 0;
    mNodeState = kNodeStateRunning;
    return RESULT_SUCCESS;
}

void RtcpEncoderNode::Stop()
{
    IMLOGD0("[Stop]");
    std::lock_guard<std::mutex> guard(mMutexTimer);

    if (mRtpSession != NULL)
    {
        mRtpSession->StopRtcp();
    }

    if (mTimer != NULL)
    {
        ImsMediaTimer::TimerStop(mTimer, NULL);
        mTimer = NULL;
        IMLOGD0("[Stop] Rtcp Timer stopped");
    }

    mNodeState = kNodeStateStopped;
}

bool RtcpEncoderNode::IsRunTime()
{
    return true;
}

bool RtcpEncoderNode::IsSourceNode()
{
    return true;
}

void RtcpEncoderNode::SetConfig(void* config)
{
    RtpConfig* pConfig = reinterpret_cast<RtpConfig*>(config);
    mPeerAddress = RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort());
    mRtcpInterval = pConfig->getRtcpConfig().getIntervalSec();
    mRtcpXrBlockType = pConfig->getRtcpConfig().getRtcpXrBlockTypes();
    mEnableRtcpBye = false;

    IMLOGD4("[SetConfig] peer Ip[%s], port[%d], interval[%d], rtcpxr[%d]", mPeerAddress.ipAddress,
            mPeerAddress.port, mRtcpInterval, mRtcpXrBlockType);

    if (mMediaType == IMS_MEDIA_VIDEO)
    {
        VideoConfig* videoConfig = reinterpret_cast<VideoConfig*>(config);
        mRtcpFbTypes = videoConfig->getRtcpFbType();
        IMLOGD1("[SetConfig] rtcpFbTypes[%d]", mRtcpFbTypes);
    }
}

bool RtcpEncoderNode::IsSameConfig(void* config)
{
    if (config == NULL)
    {
        return true;
    }

    RtpConfig* pConfig = reinterpret_cast<RtpConfig*>(config);
    RtpAddress peerAddress =
            RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort());

    if (mMediaType == IMS_MEDIA_VIDEO)
    {
        VideoConfig* videoConfig = reinterpret_cast<VideoConfig*>(config);
        return (mPeerAddress == peerAddress &&
                mRtcpInterval == videoConfig->getRtcpConfig().getIntervalSec() &&
                mRtcpXrBlockType == videoConfig->getRtcpConfig().getRtcpXrBlockTypes() &&
                mRtcpFbTypes == videoConfig->getRtcpFbType());
    }
    else
    {
        return (mPeerAddress == peerAddress &&
                mRtcpInterval == pConfig->getRtcpConfig().getIntervalSec() &&
                mRtcpXrBlockType == pConfig->getRtcpConfig().getRtcpXrBlockTypes());
    }
}

void RtcpEncoderNode::OnRtcpPacket(unsigned char* pData, uint32_t wLen)
{
    ImsMediaSubType subtype = MEDIASUBTYPE_RTCPPACKET;

    if (mEnableRtcpBye == true)
    {
        uint8_t* pCurr;
        int32_t nRemainSize;
        pCurr = (uint8_t*)pData;
        nRemainSize = wLen;

        while (nRemainSize >= 4)
        {
            uint8_t PT = pCurr[1];
            uint32_t length;
            IMLOGD_PACKET1(IM_PACKET_LOG_RTCP, "[OnRtcpPacket] PT[%d]", PT);

            if (PT == 203)
            {
                subtype = MEDIASUBTYPE_RTCPPACKET_BYE;
                break;
            }

            length = pCurr[2];
            length <<= 8;
            length += pCurr[3];
            length = (length + 1) * 4;

            pCurr += length;
            nRemainSize -= length;
        }
    }

    SendDataToRearNode(subtype, pData, wLen, 0, 0, 0);
}

void RtcpEncoderNode::OnTimer(hTimerHandler hTimer, void* pUserData)
{
    (void)hTimer;
    RtcpEncoderNode* pNode = reinterpret_cast<RtcpEncoderNode*>(pUserData);

    if (pNode != NULL)
    {
        pNode->ProcessTimer();
    }
}

void RtcpEncoderNode::ProcessTimer()
{
    std::lock_guard<std::mutex> guard(mMutexTimer);

    if (mTimer == NULL || mRtpSession == NULL)
    {
        return;
    }

    mRtpSession->OnTimer();
}

void RtcpEncoderNode::SetLocalAddress(const RtpAddress address)
{
    mLocalAddress = address;
}

void RtcpEncoderNode::SetPeerAddress(const RtpAddress address)
{
    mPeerAddress = address;
}

bool RtcpEncoderNode::SendNack(NackParams* param)
{
    if (param == NULL)
    {
        return false;
    }

    if (mRtcpFbTypes & VideoConfig::RTP_FB_NACK)
    {
        IMLOGD3("[SendNack] PID[%d], BLP[%d], nSecNackCnt[%d]", param->PID, param->BLP,
                param->nSecNackCnt);

        /* Generic NACK format
            0                   1                   2                   3
            0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
           |            PID                |             BLP               |
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+*/

        // create a Nack payload
        static uint8_t pNackBuff[4];
        mBitWriter.SetBuffer(pNackBuff, 32);
        mBitWriter.Write(param->PID, 16);  // PID
        mBitWriter.Write(param->BLP, 16);  // BLP

        if (param->bNackReport)
        {
            if (mRtpSession != NULL)
            {
                return mRtpSession->SendRtcpFeedback(kRtpFbNack, pNackBuff, 4);
            }
        }
    }

    return false;
}

bool RtcpEncoderNode::SendPictureLost(const uint32_t type)
{
    if (mRtpSession == NULL)
    {
        return false;
    }

    IMLOGD1("[SendPictureLost] type[%d]", type);

    uint32_t nCurrentTime = ImsMediaTimer::GetTimeInMilliSeconds();

    if (type == kPsfbPli && mRtcpFbTypes & VideoConfig::PSFB_PLI)
    {
        if (mLastTimeSentPli == 0 ||
                (mLastTimeSentPli + RTCPFBMNGR_PLI_FIR_REQUEST_MIN_INTERVAL < nCurrentTime))
        {
            if (mRtpSession->SendRtcpFeedback(kPsfbPli, NULL, 0))
            {
                mLastTimeSentPli = nCurrentTime;
                return true;
            }
        }
    }
    else if (type == kPsfbFir && mRtcpFbTypes & VideoConfig::PSFB_FIR)
    {
        if (mLastTimeSentFir == 0 ||
                (mLastTimeSentFir + RTCPFBMNGR_PLI_FIR_REQUEST_MIN_INTERVAL < nCurrentTime))
        {
            if (mRtpSession->SendRtcpFeedback(kPsfbFir, NULL, 0))
            {
                mLastTimeSentFir = nCurrentTime;
                return true;
            }
        }
    }

    return false;
}

bool RtcpEncoderNode::SendTmmbrn(const uint32_t type, TmmbrParams* param)
{
    if (mRtpSession == NULL || param == NULL)
    {
        return false;
    }

    IMLOGD5("[SendTmmbrn] type[%d], ssrc[%x], exp[%d], mantissa[%d], overhead[%d]", type,
            param->ssrc, param->exp, param->mantissa, param->overhead);

    /** TMMBR/TMMBN message format
       0                   1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                              SSRC                             |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       | MxTBR Exp |  MxTBR Mantissa                 |Measured Overhead|
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */

    static uint8_t buffer[8];
    mBitWriter.SetBuffer(buffer, 64);
    mBitWriter.Write((param->ssrc & 0xFFFF0000) >> 16, 16);
    mBitWriter.Write(param->ssrc & 0x0000FFFF, 16);
    // MxTBR = mantissa * 2^exp
    mBitWriter.Write(param->exp, 6);        // MxTBR Exp
    mBitWriter.Write(param->mantissa, 17);  // MxTBR Mantissa
    // avg_OH (new) = 15/16*avg_OH (old) + 1/16*pckt_OH,
    mBitWriter.Write(param->overhead, 9);  // Measured Overhead

    if (type == kRtpFbTmmbr && mRtcpFbTypes & VideoConfig::RTP_FB_TMMBR)
    {
        return mRtpSession->SendRtcpFeedback(kRtpFbTmmbr, buffer, 8);
    }
    else if (type == kRtpFbTmmbn && mRtcpFbTypes & VideoConfig::RTP_FB_TMMBN)
    {
        return mRtpSession->SendRtcpFeedback(kRtpFbTmmbn, buffer, 8);
    }

    return false;
}
