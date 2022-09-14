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
#include <TextConfig.h>
#include <string.h>

RtpEncoderNode::RtpEncoderNode(BaseSessionCallback* callback) :
        BaseNode(callback)
{
    mRtpSession = NULL;
    mDTMFMode = false;
    mMark = false;
    mPrevTimestamp = 0;
    mDTMFTimestamp = 0;
    mSamplingRate = 0;
    mRtpPayloadTx = 0;
    mRtpPayloadRx = 0;
    mRtpDtmfPayload = 0;
    mDtmfSamplingRate = 0;
    mCvoValue = CVO_DEFINE_NONE;
    mRedundantLevel = 0;
    mRedundantPayload = 0;
}

RtpEncoderNode::~RtpEncoderNode()
{
    // remove IRtpSession here to avoid shared instance in other node from unable to use
    if (mRtpSession)
    {
        mRtpSession->StopRtp();
        mRtpSession->SetRtpEncoderListener(NULL);
        IRtpSession::ReleaseInstance(mRtpSession);
        mRtpSession = NULL;
    }
}

kBaseNodeId RtpEncoderNode::GetNodeId()
{
    return kNodeIdRtpEncoder;
}

ImsMediaResult RtpEncoderNode::Start()
{
    IMLOGD1("[Start] type[%d]", mMediaType);

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

    if (mMediaType == IMS_MEDIA_AUDIO)
    {
        mRtpSession->SetRtpPayloadParam(mRtpPayloadTx, mRtpPayloadRx, mSamplingRate * 1000,
                mRtpDtmfPayload, mDtmfSamplingRate * 1000);
    }
    else if (mMediaType == IMS_MEDIA_VIDEO)
    {
        mRtpSession->SetRtpPayloadParam(mRtpPayloadTx, mRtpPayloadRx, mSamplingRate * 1000);
    }
    else if (mMediaType == IMS_MEDIA_TEXT)
    {
        if (mRedundantPayload > 0)
        {
            mRtpSession->SetRtpPayloadParam(mRtpPayloadTx, mRtpPayloadRx, mSamplingRate * 1000,
                    mRedundantPayload, mSamplingRate * 1000);
        }
        else
        {
            mRtpSession->SetRtpPayloadParam(mRtpPayloadTx, mRtpPayloadRx, mSamplingRate * 1000);
        }
    }

    mRtpSession->StartRtp();
    mDTMFMode = false;
    mMark = true;
    mPrevTimestamp = 0;
    mDTMFTimestamp = 0;
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
    IMLOGD1("[Stop] type[%d]", mMediaType);

    if (mRtpSession)
    {
        mRtpSession->StopRtp();
    }

    mNodeState = kNodeStateStopped;
}

void RtpEncoderNode::ProcessData()
{
    ImsMediaSubType subtype;
    uint8_t* pData = NULL;
    uint32_t nDataSize = 0;
    uint32_t nTimestamp = 0;
    bool bMark = false;
    uint32_t nSeqNum = 0;

    if (GetData(&subtype, &pData, &nDataSize, &nTimestamp, &bMark, &nSeqNum, NULL))
    {
        if (mMediaType == IMS_MEDIA_AUDIO)
        {
            ProcessAudioData(subtype, pData, nDataSize, nTimestamp);
        }
        else if (mMediaType == IMS_MEDIA_VIDEO)
        {
            ProcessVideoData(subtype, pData, nDataSize, nTimestamp, bMark);
        }
        else if (mMediaType == IMS_MEDIA_TEXT)
        {
            ProcessTextData(subtype, pData, nDataSize, nTimestamp, bMark);
        }

        DeleteData();
    }
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
        AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);
        mPeerAddress = RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort());
        mSamplingRate = pConfig->getSamplingRateKHz();
        mRtpPayloadTx = pConfig->getTxPayloadTypeNumber();
        mRtpPayloadRx = pConfig->getRxPayloadTypeNumber();
        mRtpDtmfPayload = pConfig->getDtmfPayloadTypeNumber();
        mDtmfSamplingRate = pConfig->getDtmfsamplingRateKHz();
    }
    else if (mMediaType == IMS_MEDIA_VIDEO)
    {
        VideoConfig* pConfig = reinterpret_cast<VideoConfig*>(config);
        mPeerAddress = RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort());
        mSamplingRate = pConfig->getSamplingRateKHz();
        mRtpPayloadTx = pConfig->getTxPayloadTypeNumber();
        mRtpPayloadRx = pConfig->getRxPayloadTypeNumber();
        mCvoValue = pConfig->getCvoValue();
    }
    else if (mMediaType == IMS_MEDIA_TEXT)
    {
        TextConfig* pConfig = reinterpret_cast<TextConfig*>(config);
        mPeerAddress = RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort());
        mSamplingRate = pConfig->getSamplingRateKHz();
        mRtpPayloadTx = pConfig->getTxPayloadTypeNumber();
        mRtpPayloadRx = pConfig->getRxPayloadTypeNumber();
        mRedundantPayload = pConfig->getRedundantPayload();
        mRedundantLevel = pConfig->getRedundantLevel();
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
        return (mPeerAddress ==
                        RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort()) &&
                mSamplingRate == pConfig->getSamplingRateKHz() &&
                mRtpPayloadTx == pConfig->getTxPayloadTypeNumber() &&
                mRtpPayloadRx == pConfig->getRxPayloadTypeNumber() &&
                mRtpDtmfPayload == pConfig->getDtmfPayloadTypeNumber() &&
                mDtmfSamplingRate == pConfig->getDtmfsamplingRateKHz());
    }
    else if (mMediaType == IMS_MEDIA_VIDEO)
    {
        VideoConfig* pConfig = reinterpret_cast<VideoConfig*>(config);
        return (mPeerAddress ==
                        RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort()) &&
                mSamplingRate == pConfig->getSamplingRateKHz() &&
                mRtpPayloadTx == pConfig->getTxPayloadTypeNumber() &&
                mRtpPayloadRx == pConfig->getRxPayloadTypeNumber() &&
                mCvoValue == pConfig->getCvoValue());
    }
    else if (mMediaType == IMS_MEDIA_TEXT)
    {
        TextConfig* pConfig = reinterpret_cast<TextConfig*>(config);
        return (mPeerAddress ==
                        RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort()) &&
                mSamplingRate == pConfig->getSamplingRateKHz() &&
                mRtpPayloadTx == pConfig->getTxPayloadTypeNumber() &&
                mRtpPayloadRx == pConfig->getRxPayloadTypeNumber() &&
                mRedundantPayload == pConfig->getRedundantPayload() &&
                mRedundantLevel == pConfig->getRedundantLevel());
    }

    return false;
}

void RtpEncoderNode::OnRtpPacket(unsigned char* pData, uint32_t nSize)
{
    SendDataToRearNode(MEDIASUBTYPE_RTPPACKET, pData, nSize, 0, 0, 0);
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
        ImsMediaSubType subtype, uint8_t* pData, uint32_t nDataSize, uint32_t nTimestamp)
{
    uint32_t nCurrTimestamp;
    uint32_t nTimeDiff;
    uint32_t nTimeDiff_RTPTSUnit;

    if (subtype == MEDIASUBTYPE_DTMFSTART)
    {
        IMLOGD0("[ProcessAudioData] SetDTMF mode true");
        mDTMFMode = true;
        mMark = true;
    }
    else if (subtype == MEDIASUBTYPE_DTMFEND)
    {
        IMLOGD0("[ProcessAudioData] SetDTMF mode false");
        mDTMFMode = false;
        mMark = true;
    }
    else if (subtype == MEDIASUBTYPE_DTMF_PAYLOAD)
    {
        if (mDTMFMode)
        {
            IMLOGD_PACKET2(IM_PACKET_LOG_RTP, "[ProcessAudioData] DTMF - size[%d], TS[%d]",
                    nDataSize, mDTMFTimestamp);
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

            nTimeDiff_RTPTSUnit = nTimeDiff * mSamplingRate;
            mRtpSession->SendRtpPacket(
                    mRtpDtmfPayload, pData, nDataSize, mDTMFTimestamp, mMark, nTimeDiff_RTPTSUnit);

            if (mMark)
            {
                mMark = false;
            }
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
                    IMLOGW2("[ProcessAudioData] skip this turn orev[%u] curr[%u]", mPrevTimestamp,
                            nCurrTimestamp);
                    return;
                }
                else
                {
                    mPrevTimestamp += nTimeDiff;
                }
            }

            nTimeDiff_RTPTSUnit = nTimeDiff * mSamplingRate;
            IMLOGD_PACKET3(IM_PACKET_LOG_RTP, "[ProcessAudioData] PayloadTx[%d], Size[%d], TS[%d]",
                    mRtpPayloadTx, nDataSize, nCurrTimestamp);
            mRtpSession->SendRtpPacket(
                    mRtpPayloadTx, pData, nDataSize, nCurrTimestamp, mMark, nTimeDiff_RTPTSUnit);

            if (mMark)
            {
                mMark = false;
            }
        }
    }
}

void RtpEncoderNode::ProcessVideoData(ImsMediaSubType subtype, uint8_t* pData, uint32_t nDataSize,
        uint32_t nTimestamp, bool bMark)
{
    IMLOGD_PACKET2(IM_PACKET_LOG_RTP, "[ProcessVideoData] nSize[%d], nTimestamp[%u]", nDataSize,
            nTimestamp);

    if (mCvoValue > 0 && bMark == true && subtype == MEDIASUBTYPE_VIDEO_IDR_FRAME)
    {
        mRtpSession->SendRtpPacket(
                mRtpPayloadTx, pData, nDataSize, nTimestamp, bMark, 0, true, &mRtpExtension);
    }
    else
    {
        mRtpSession->SendRtpPacket(
                mRtpPayloadTx, pData, nDataSize, nTimestamp, bMark, 0, false, NULL);
    }
}

void RtpEncoderNode::ProcessTextData(ImsMediaSubType subtype, uint8_t* pData, uint32_t nDataSize,
        uint32_t nTimestamp, bool bMark)
{
    IMLOGD_PACKET4(IM_PACKET_LOG_RTP,
            "[ProcessTextData] subtype[%d], nDataSize[%d], nTimestamp[%d], bMark[%d]", subtype,
            nDataSize, nTimestamp, bMark);

    uint32_t nTimeDiff;

    if (mMark == true)
    {
        nTimeDiff = 0;
    }
    else
    {
        nTimeDiff = nTimestamp - mPrevTimestamp;
    }

    if (subtype == MEDIASUBTYPE_BITSTREAM_T140)
    {
        if (mRedundantLevel > 1 && mRedundantPayload > 0)
        {
            mRtpSession->SendRtpPacket(
                    mRedundantPayload, pData, nDataSize, nTimestamp, bMark, nTimeDiff, 0, NULL);
        }
        else
        {
            mRtpSession->SendRtpPacket(
                    mRtpPayloadRx, pData, nDataSize, nTimestamp, bMark, nTimeDiff, 0, NULL);
        }
    }
    else if (subtype == MEDIASUBTYPE_BITSTREAM_T140_RED)
    {
        mRtpSession->SendRtpPacket(
                mRtpPayloadTx, pData, nDataSize, nTimestamp, bMark, nTimeDiff, 0, NULL);
    }

    mMark = false;
    mPrevTimestamp = nTimestamp;
}
