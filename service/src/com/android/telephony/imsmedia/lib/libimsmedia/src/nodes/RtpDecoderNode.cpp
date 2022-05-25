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

#include <RtpDecoderNode.h>
#include <ImsMediaTrace.h>
#include <AudioConfig.h>
#include <VideoConfig.h>

RtpDecoderNode::RtpDecoderNode()
{
    mRtpSession = NULL;
    mReceivingSSRC = 0;
    mInactivityTime = 0;
    mNoRtpTime = 0;
    mConfig = NULL;
    mCvoValue = CVO_DEFINE_NONE;
}

RtpDecoderNode::~RtpDecoderNode() {}

BaseNode* RtpDecoderNode::GetInstance()
{
    BaseNode* pNode = new RtpDecoderNode();
    if (pNode == NULL)
    {
        IMLOGE0("[GetInstance] Can't create RtpDecoderNode");
    }

    return pNode;
}

void RtpDecoderNode::ReleaseInstance(BaseNode* pNode)
{
    delete (RtpDecoderNode*)pNode;
}

BaseNodeID RtpDecoderNode::GetNodeID()
{
    return NODEID_RTPDECODER;
}

ImsMediaResult RtpDecoderNode::Start()
{
    if (mRtpSession == NULL)
    {
        mRtpSession = IRtpSession::GetInstance(mMediaType, mLocalAddress, mPeerAddress);
        if (mRtpSession == NULL)
        {
            IMLOGE0("[Start] - Can't create rtp session");
            return RESULT_NOT_READY;
        }
    }

    mRtpSession->SetPayloadParam(mConfig);
    mRtpSession->SetRtpDecoderListener(this);
    mRtpSession->StartRtp();
    mReceivingSSRC = 0;
    mNoRtpTime = 0;
    mNodeState = kNodeStateRunning;
    return RESULT_SUCCESS;
}

void RtpDecoderNode::Stop()
{
    mReceivingSSRC = 0;
    if (mRtpSession)
    {
        mRtpSession->SetRtpDecoderListener(NULL);
        mRtpSession->StopRtp();
        IRtpSession::ReleaseInstance(mRtpSession);
        mRtpSession = NULL;
    }
    mNodeState = kNodeStateStopped;
}

void RtpDecoderNode::OnDataFromFrontNode(ImsMediaSubType subtype, uint8_t* pData,
        uint32_t nDataSize, uint32_t nTimestamp, bool bMark, uint32_t nSeqNum,
        ImsMediaSubType nDataType)
{
    IMLOGD_PACKET6(IM_PACKET_LOG_RTP,
            "[OnDataFromFrontNode] subtype[%d] Size[%d], TS[%d], Mark[%d], Seq[%d], datatype[%d]",
            subtype, nDataSize, nTimestamp, bMark, nSeqNum, nDataType);
    mRtpSession->ProcRtpPacket(pData, nDataSize);
}

bool RtpDecoderNode::IsRunTime()
{
    return true;
}

bool RtpDecoderNode::IsSourceNode()
{
    return false;
}

void RtpDecoderNode::SetConfig(void* config)
{
    IMLOGD0("[SetConfig]");
    if (config == NULL)
        return;
    if (mMediaType == IMS_MEDIA_AUDIO)
    {
        AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);
        mPeerAddress = RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort());
        mSamplingRate = pConfig->getSamplingRateKHz() * 1000;
    }
    else if (mMediaType == IMS_MEDIA_VIDEO)
    {
        VideoConfig* pConfig = reinterpret_cast<VideoConfig*>(config);
        mPeerAddress = RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort());
        mSamplingRate = pConfig->getSamplingRateKHz() * 1000;
        mCvoValue = pConfig->getCvoValue();
    }

    IMLOGD2("[SetConfig] peer Ip[%s], port[%d]", mPeerAddress.ipAddress, mPeerAddress.port);
}

bool RtpDecoderNode::IsSameConfig(void* config)
{
    if (config == NULL)
        return true;
    RtpConfig* pConfig = reinterpret_cast<RtpConfig*>(config);
    RtpAddress peerAddress =
            RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort());

    return (mPeerAddress == peerAddress && mSamplingRate == (pConfig->getSamplingRateKHz() * 1000));
}

void RtpDecoderNode::OnMediaDataInd(unsigned char* pData, uint32_t nDataSize, uint32_t nTimestamp,
        bool bMark, uint16_t nSeqNum, uint32_t nPayloadType, uint32_t nSSRC, bool bExtension,
        uint16_t nExtensionData)
{
    static ImsMediaSubType subtype = MEDIASUBTYPE_RTPPAYLOAD;

    IMLOGD_PACKET6(IM_PACKET_LOG_RTP,
            "[OnMediaDataInd] media[%d] Size[%d], TS[%d], Mark[%d], Seq[%d],\
 SamplingRate[%d]",
            mMediaType, nDataSize, nTimestamp, bMark, nSeqNum, mSamplingRate);

    // no need to change to timestamp to msec in video or text packet
    if (mMediaType != IMS_MEDIA_VIDEO && mSamplingRate != 0)
    {
        nTimestamp = nTimestamp / (mSamplingRate / 1000);
    }

    if (mReceivingSSRC != nSSRC)
    {
        IMLOGD3("[OnMediaDataInd] media[%d] SSRC changed, received SSRC[%x], nSSRC[%x]", mMediaType,
                mReceivingSSRC, nSSRC);
        mReceivingSSRC = nSSRC;
        SendDataToRearNode(MEDIASUBTYPE_REFRESHED, NULL, 0, 0, 0, 0);
    }

    // TODO : add checking incoming dtmf by the payload type number
    (void)nPayloadType;

    if (bExtension == true)
    {
        // send rtp header extension received event
        mCallback->SendEvent(kImsMediaEventHeaderExtensionReceived, nExtensionData);

        if (mMediaType == IMS_MEDIA_VIDEO && mCvoValue != CVO_DEFINE_NONE)
        {
            uint16_t nExtensionID;
            uint16_t nCamID;
            uint16_t nRotation;
            nExtensionID = nExtensionData;
            nExtensionID = nExtensionID >> 12;

            nCamID = nExtensionData;  // 0: Front-facing camera, 1: Back-facing camera
            nCamID = nCamID << 12;
            nCamID = nCamID >> 15;

            nRotation = nExtensionData;
            nRotation = nRotation << 13;
            nRotation = nRotation >> 13;

            switch (nRotation)
            {
                case 0:  // No rotation (Rotated 0CW/CCW = To rotate 0CW/CCW)
                case 4:  // + Horizontal Flip, but it's treated as same as above
                    subtype = MEDIASUBTYPE_ROT0;
                    break;
                case 1:  // Rotated 270CW(90CCW) = To rotate 90CW(270CCW)
                case 5:  // + Horizontal Flip, but it's treated as same as above
                    subtype = MEDIASUBTYPE_ROT90;
                    break;
                case 2:  // Rotated 180CW = To rotate 180CW
                case 6:  // + Horizontal Flip, but it's treated as same as above
                    subtype = MEDIASUBTYPE_ROT180;
                    break;
                case 3:  // Rotated 90CW(270CCW) = To rotate 270CW(90CCW)
                case 7:  // + Horizontal Flip, but it's treated as same as above
                    subtype = MEDIASUBTYPE_ROT270;
                    break;
                default:
                    break;
            }

            IMLOGD4("[OnMediaDataInd] extensionId[%d], camId[%d], rot[%d], subtype[%d]",
                    nExtensionID, nCamID, nRotation, subtype);
        }
    }

    SendDataToRearNode(subtype, pData, nDataSize, nTimestamp, bMark, nSeqNum);
}

void RtpDecoderNode::OnNumReceivedPacket(uint32_t nNumRtpPacket)
{
    IMLOGD_PACKET2(IM_PACKET_LOG_RTP, "[OnNumReceivedPacket] InactivityTime[%d], numRtp[%d]",
            mInactivityTime, nNumRtpPacket);

    if (nNumRtpPacket == 0)
    {
        mNoRtpTime++;
    }
    else
    {
        mNoRtpTime = 0;
    }

    if (mInactivityTime != 0 && mNoRtpTime == mInactivityTime)
    {
        if (mCallback != NULL)
        {
            mCallback->SendEvent(kImsMediaEventMediaInactivity, RTP, mInactivityTime);
        }
    }
}

void RtpDecoderNode::SetLocalAddress(const RtpAddress address)
{
    mLocalAddress = address;
}

void RtpDecoderNode::SetPeerAddress(const RtpAddress address)
{
    mPeerAddress = address;
}

void RtpDecoderNode::SetInactivityTimerSec(const uint32_t time)
{
    IMLOGD1("[SetInactivityTimerSec] time[%d]", time);
    mInactivityTime = time;
}