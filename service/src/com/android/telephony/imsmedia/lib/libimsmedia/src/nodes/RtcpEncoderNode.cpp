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
