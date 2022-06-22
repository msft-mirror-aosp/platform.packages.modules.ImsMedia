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

#include <RtpEncoderNode.h>
#include <ImsMediaTimer.h>
#include <ImsMediaTrace.h>
#include <ImsMediaVideoUtil.h>
#include <AudioConfig.h>
#include <VideoConfig.h>
#include <string.h>

#ifdef DEBUG_JITTER_GEN_SIMULATION_DELAY
#define DEBUG_JITTER_MAX_PACKET_INTERVAL 70  // msec, minimum value is 30
#endif
#ifdef DEBUG_JITTER_GEN_SIMULATION_REORDER
#include <ImsMediaDataQueue.h>
#define DEBUG_JITTER_REORDER_MAX 4
#define DEBUG_JITTER_REORDER_MIN 4
#define DEBUG_JITTER_NORMAL      2
#endif
#ifdef DEBUG_JITTER_GEN_SIMULATION_LOSS
#define DEBUG_JITTER_LOSS_NORMAT_PACKET 49
#define DEBUG_JITTER_LOSS_LOSS_PACKET   1
#endif

RtpEncoderNode::RtpEncoderNode()
{
    mRtpSession = NULL;
    mConfig = NULL;
    mDTMFMode = false;
    mAudioMark = false;
    mPrevTimestamp = 0;
    mDTMFTimestamp = 0;
    mRtpPayload = 0;
    mDtmfPayload = 0;
    mSamplingRate = 0;
    mCvoValue = CVO_DEFINE_NONE;
#ifdef DEBUG_JITTER_GEN_SIMULATION_DELAY
    mNextTime = 0;
#endif
#ifdef DEBUG_JITTER_GEN_SIMULATION_REORDER
    mReorderDataCount = 0;
#endif
}

RtpEncoderNode::~RtpEncoderNode()
{
    if (mConfig != NULL)
    {
        delete mConfig;
        mConfig = NULL;
    }
}

BaseNode* RtpEncoderNode::GetInstance()
{
    return new RtpEncoderNode();
}

void RtpEncoderNode::ReleaseInstance(BaseNode* pNode)
{
    delete (RtpEncoderNode*)pNode;
}

BaseNodeID RtpEncoderNode::GetNodeID()
{
    return BaseNodeID::NODEID_RTPENCODER;
}

ImsMediaResult RtpEncoderNode::Start()
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
    mRtpSession->SetRtpEncoderListener(this);
    mRtpSession->SetRtpPayloadParam(mConfig);
    mRtpSession->StartRtp();
    mDTMFMode = false;
    mAudioMark = true;
    mPrevTimestamp = 0;
    mDTMFTimestamp = 0;
    // mRtpSession->EnableRtpMonitoring(true);
#ifdef DEBUG_JITTER_GEN_SIMULATION_DELAY
    mNextTime = 0;
#endif
#ifdef DEBUG_JITTER_GEN_SIMULATION_REORDER
    jitterData.Clear();
    mReorderDataCount = 0;
#endif
    mNodeState = kNodeStateRunning;
    return RESULT_SUCCESS;
}

void RtpEncoderNode::Stop()
{
    IMLOGD0("[Stop]");
    if (mRtpSession)
    {
        mRtpSession->SetRtpEncoderListener(NULL);
        mRtpSession->StopRtp();
        IRtpSession::ReleaseInstance(mRtpSession);
        mRtpSession = NULL;
    }
    mNodeState = kNodeStateStopped;
}

void RtpEncoderNode::ProcessData()
{
    ImsMediaSubType eSubType;
    uint8_t* pData = NULL;
    uint32_t nDataSize = 0;
    uint32_t nTimestamp = 0;
    bool bMark = false;
    uint32_t nSeqNum = 0;

    if (GetData(&eSubType, &pData, &nDataSize, &nTimestamp, &bMark, &nSeqNum, NULL) == false)
    {
        return;
    }
#ifdef DEBUG_JITTER_GEN_SIMULATION_DELAY
    {
        uint32_t nCurrTime = ImsMediaTimer::GetTimeInMilliSeconds();
        if ((GetDataCount(mMediaType) <= 1) && mNextTime && nCurrTime < mNextTime)
        {
            return;
        }
        else
        {
            uint32_t max_interval_1 = GenerateRandom(
                    (DEBUG_JITTER_MAX_PACKET_INTERVAL - 30) / GetDataCount(mMediaType));
            uint32_t max_interval_2 = 30 + max_interval_1;
            mNextTime = nCurrTime + GenerateRandom(max_interval_2);
        }
    }
#endif
    if (mMediaType == IMS_MEDIA_AUDIO)
    {
        ProcessAudioData(eSubType, pData, nDataSize, nTimestamp);
    }
    else if (mMediaType == IMS_MEDIA_VIDEO)
    {
        ProcessVideoData(eSubType, pData, nDataSize, nTimestamp, bMark);
    }

    DeleteData();
}

bool RtpEncoderNode::IsRunTime()
{
    return false;
}

bool RtpEncoderNode::IsSourceNode()
{
    return false;
}

void RtpEncoderNode::SetConfig(void* config)
{
    IMLOGD1("[SetConfig] type[%d]", mMediaType);
    if (config == NULL)
    {
        return;
    }

    if (mMediaType == IMS_MEDIA_AUDIO)
    {
        AudioConfig* pConfig = new AudioConfig(reinterpret_cast<AudioConfig*>(config));
        mPeerAddress = RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort());
        mRtpPayload = pConfig->getTxPayloadTypeNumber();
        mDtmfPayload = pConfig->getDtmfPayloadTypeNumber();
        mConfig = pConfig;
    }
    else if (mMediaType == IMS_MEDIA_VIDEO)
    {
        VideoConfig* pConfig = new VideoConfig(reinterpret_cast<VideoConfig*>(config));
        mPeerAddress = RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort());
        mRtpPayload = pConfig->getTxPayloadTypeNumber();
        mCvoValue = pConfig->getCvoValue();
        mConfig = pConfig;
    }

    IMLOGD2("[SetConfig] peer Ip[%s], port[%d]", mPeerAddress.ipAddress, mPeerAddress.port);
}

bool RtpEncoderNode::IsSameConfig(void* config)
{
    if (config == NULL)
    {
        return true;
    }

    if (mMediaType == IMS_MEDIA_AUDIO)
    {
        AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);
        return (*mConfig == *pConfig);
    }
    else if (mMediaType == IMS_MEDIA_VIDEO)
    {
        VideoConfig* pConfig = reinterpret_cast<VideoConfig*>(config);
        return (*mConfig == *pConfig);
    }

    return false;
}

// IRtpEncoderListener
void RtpEncoderNode::OnRtpPacket(unsigned char* pData, uint32_t nSize)
{
#ifdef DEBUG_JITTER_GEN_SIMULATION_LOSS
    bool nLossFlag = false;
    {
        static uint32_t nLossNormalCount = 0;
        static uint32_t nLossLossCount = 0;
        if (nLossNormalCount < DEBUG_JITTER_LOSS_NORMAT_PACKET)
        {
            nLossNormalCount++;
        }
        else
        {
            if (nLossLossCount < DEBUG_JITTER_LOSS_LOSS_PACKET)
            {
                nLossLossCount++;
                nLossFlag = true;
            }
            else
            {
                nLossNormalCount = 0;
                nLossLossCount = 0;
            }
        }
    }
#endif
#ifdef DEBUG_JITTER_GEN_SIMULATION_REORDER
    {
        // add data to jitter gen buffer
        DataEntry entry;
        entry.subtype = MEDIASUBTYPE_RTPPACKET;
        entry.pbBuffer = pData;
        entry.nBufferSize = nSize;
        entry.nTimestamp = 0;
        entry.bMark = 0;
        entry.nSeqNum = 0;

        if (mReorderDataCount < DEBUG_JITTER_NORMAL)
        {
            jitterData.push_back(&entry);
        }
        else if (mReorderDataCount < DEBUG_JITTER_NORMAL + DEBUG_JITTER_REORDER_MAX)
        {
            int32_t nCurrReorderSize;
            int32_t nInsertPos;
            uint32_t nCurrJitterBufferSize;
            nCurrJitterBufferSize = jitterData.GetCount();
            if (DEBUG_JITTER_REORDER_MAX > DEBUG_JITTER_REORDER_MIN)
            {
                nCurrReorderSize = mReorderDataCount - DEBUG_JITTER_NORMAL + 1 -
                        GenerateRandom(DEBUG_JITTER_REORDER_MAX - DEBUG_JITTER_REORDER_MIN + 1);
            }
            else
            {
                nCurrReorderSize = mReorderDataCount - DEBUG_JITTER_NORMAL + 1;
            }

            if (nCurrReorderSize > 0)
                nCurrReorderSize = GenerateRandom(nCurrReorderSize + 1);

            nInsertPos = nCurrJitterBufferSize - nCurrReorderSize;
            if (nInsertPos < 0)
                nInsertPos = 0;
            jitterData.InsertAt(nInsertPos, &entry);
        }

        mReorderDataCount++;

        if (mReorderDataCount >= DEBUG_JITTER_NORMAL + DEBUG_JITTER_REORDER_MAX)
        {
            mReorderDataCount = 0;
        }

        // send
        while (jitterData.GetCount() >= DEBUG_JITTER_REORDER_MAX)
        {
            DataEntry* pEntry;
            if (jitterData.Get(&pEntry))
            {
#ifdef DEBUG_JITTER_GEN_SIMULATION_LOSS
                if (nLossFlag == false)
                {
                    SendDataToRearNode(
                            MEDIASUBTYPE_RTPPACKET, pEntry->pbBuffer, pEntry->nBufferSize, 0, 0, 0);
                }
#else
                SendDataToRearNode(
                        MEDIASUBTYPE_RTPPACKET, pEntry->pbBuffer, pEntry->nBufferSize, 0, 0, 0);
#endif
                jitterData.Delete();
            }
        }
    }
#else

#ifdef DEBUG_JITTER_GEN_SIMULATION_LOSS
    if (nLossFlag == false)
    {
        SendDataToRearNode(MEDIASUBTYPE_RTPPACKET, pData, nSize, 0, 0, 0);
    }
#else
    SendDataToRearNode(MEDIASUBTYPE_RTPPACKET, pData, nSize, 0, 0, 0);
#endif
#endif
}

void RtpEncoderNode::SetLocalAddress(const RtpAddress address)
{
    mLocalAddress = address;
}

void RtpEncoderNode::SetPeerAddress(const RtpAddress address)
{
    mPeerAddress = address;
}

void RtpEncoderNode::SetCvoExtension(const int64_t facing, const int64_t orientation)
{
    IMLOGD3("[SetCvoExtension] cvoValue[%d], facing[%ld], orientation[%ld]", mCvoValue, facing,
            orientation);

    if (mCvoValue == -1)
    {
        return;
    }

    uint32_t nRotation = 0;
    uint32_t nCamera = 0;

    if (facing == kCameraFacingRear)
    {
        nCamera = 1;
    }

    switch (orientation)
    {
        default:
        case 0:
            nRotation = 0;
            break;
        case 270:
            nRotation = 1;
            break;
        case 180:
            nRotation = 2;
            break;
        case 90:
            nRotation = 3;
            break;
    }

    if (nCamera == 1)  // rear camera
    {
        if (nRotation == 1)  // CCW90
        {
            nRotation = 3;
        }
        else if (nRotation == 3)  // CCW270
        {
            nRotation = 1;
        }
    }

    uint16_t nExtensionData;
    uint16_t nDataID = mCvoValue;

    IMLOGD3("[SetCvoExtension] cvoValue[%d], facing[%d], orientation[%d]", nDataID, nCamera,
            nRotation);

    nExtensionData = ((nDataID << 12) | (1 << 8)) | ((nCamera << 3) | nRotation);
    mRtpExtension.nDefinedByProfile = 0xBEDE;
    mRtpExtension.nLength = 1;
    mRtpExtension.nExtensionData = nExtensionData;
}

void RtpEncoderNode::SetRtpHeaderExtension(tRtpHeaderExtensionInfo& tExtension)
{
    mRtpExtension = tExtension;
}

void RtpEncoderNode::ProcessAudioData(
        ImsMediaSubType eSubType, uint8_t* pData, uint32_t nDataSize, uint32_t nTimestamp)
{
    uint32_t nCurrTimestamp;
    uint32_t nTimeDiff;
    uint32_t nTimeDiff_RTPTSUnit;

    if (eSubType == MEDIASUBTYPE_DTMFSTART)
    {
        IMLOGD0("[ProcessData] SetDTMF mode true");
        mDTMFMode = true;
        mAudioMark = true;
    }
    else if (eSubType == MEDIASUBTYPE_DTMFEND)
    {
        IMLOGD0("[ProcessData] SetDTMF mode false");
        mDTMFMode = false;
        mAudioMark = true;
    }
    else if (eSubType == MEDIASUBTYPE_DTMF_PAYLOAD)
    {
        if (mDTMFMode)
        {
            IMLOGD_PACKET2(IM_PACKET_LOG_RTP, "[ProcessData] DTMF - nSize[%d], TS[%d]", nDataSize,
                    mDTMFTimestamp);
            // the first dtmf event
            if (nTimestamp == 0)
            {
                nCurrTimestamp = ImsMediaTimer::GetTimeInMilliSeconds();
                mDTMFTimestamp = nCurrTimestamp;
                nTimeDiff = ((nCurrTimestamp - mPrevTimestamp) + 10) / 20 * 20;

                if (nTimeDiff == 0)
                    nTimeDiff = 20;

                mPrevTimestamp += nTimeDiff;
            }
            else
            {
                nTimeDiff = 0;
            }

            nTimeDiff_RTPTSUnit = nTimeDiff * (mSamplingRate / 1000);
            mRtpSession->SendRtpPacket(mDtmfPayload, pData, nDataSize, mDTMFTimestamp, mAudioMark,
                    nTimeDiff_RTPTSUnit);

            if (mAudioMark)
                mAudioMark = false;
        }
    }
    else
    {
        if (mDTMFMode == false)
        {
            nCurrTimestamp = ImsMediaTimer::GetTimeInMilliSeconds();

            if (mPrevTimestamp == 0)
            {
                nTimeDiff = 0;
                mPrevTimestamp = nCurrTimestamp;
            }
            else
            {
                nTimeDiff = ((nCurrTimestamp - mPrevTimestamp) + 10) / 20 * 20;

                if (nTimeDiff > 20)
                {
                    mPrevTimestamp = nCurrTimestamp;
                }
                else if (nTimeDiff == 0)
                {
                    IMLOGW2("[ProcessData] skip this turn orev[%u] curr[%u]", mPrevTimestamp,
                            nCurrTimestamp);
                    return;
                }
                else
                {
                    mPrevTimestamp += nTimeDiff;
                }
            }

            nTimeDiff_RTPTSUnit = nTimeDiff * (mSamplingRate / 1000);
            IMLOGD_PACKET3(IM_PACKET_LOG_RTP, "[ProcessData] mRtpPayload[%d], nSize[%d], nTS[%d]",
                    mRtpPayload, nDataSize, nCurrTimestamp);
            mRtpSession->SendRtpPacket(
                    mRtpPayload, pData, nDataSize, nCurrTimestamp, mAudioMark, nTimeDiff_RTPTSUnit);

            if (mAudioMark)
            {
                mAudioMark = false;
            }
        }
    }
}

void RtpEncoderNode::ProcessVideoData(ImsMediaSubType eSubType, uint8_t* pData, uint32_t nDataSize,
        uint32_t nTimestamp, bool bMark)
{
    IMLOGD_PACKET2(
            IM_PACKET_LOG_RTP, "[ProcessData] nSize[%d], nTimestamp[%u]", nDataSize, nTimestamp);

    if (mCvoValue > 0 && bMark == true && eSubType == MEDIASUBTYPE_VIDEO_IDR_FRAME)
    {
        mRtpSession->SendRtpPacket(
                mRtpPayload, pData, nDataSize, nTimestamp, bMark, 0, true, &mRtpExtension);
    }
    else
    {
        mRtpSession->SendRtpPacket(
                mRtpPayload, pData, nDataSize, nTimestamp, bMark, 0, false, NULL);
    }
}