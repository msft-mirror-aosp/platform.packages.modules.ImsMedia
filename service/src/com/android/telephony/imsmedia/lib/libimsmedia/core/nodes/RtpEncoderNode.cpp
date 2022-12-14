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
    mRtpTxDtmfPayload = 0;
    mRtpRxDtmfPayload = 0;
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

    if (mRtpPayloadTx == 0 || mRtpPayloadRx == 0)
    {
        IMLOGE0("[Start] invalid payload number");
        return RESULT_INVALID_PARAM;
    }

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
                mRtpTxDtmfPayload, mRtpRxDtmfPayload, mDtmfSamplingRate * 1000);
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
    std::lock_guard<std::mutex> guard(mMutex);

    if (mRtpSession)
    {
        mRtpSession->StopRtp();
    }

    mNodeState = kNodeStateStopped;
}

void RtpEncoderNode::ProcessData()
{
    std::lock_guard<std::mutex> guard(mMutex);

    if (mNodeState != kNodeStateRunning)
    {
        return;
    }

    ImsMediaSubType subtype;
    uint8_t* data = NULL;
    uint32_t size = 0;
    uint32_t timestamp = 0;
    bool mark = false;
    uint32_t seq = 0;
    ImsMediaSubType datatype;
    uint32_t arrivalTime = 0;

    if (GetData(&subtype, &data, &size, &timestamp, &mark, &seq, &datatype, &arrivalTime))
    {
        if (mMediaType == IMS_MEDIA_AUDIO)
        {
            if (!ProcessAudioData(subtype, data, size, timestamp))
            {
                return;
            }
        }
        else if (mMediaType == IMS_MEDIA_VIDEO)
        {
            ProcessVideoData(subtype, data, size, timestamp, mark);
        }
        else if (mMediaType == IMS_MEDIA_TEXT)
        {
            ProcessTextData(subtype, data, size, timestamp, mark);
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
    IMLOGD1("[SetConfig] media[%d]", mMediaType);

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
        mRtpTxDtmfPayload = pConfig->getTxDtmfPayloadTypeNumber();
        mRtpRxDtmfPayload = pConfig->getRxDtmfPayloadTypeNumber();
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
                mRtpTxDtmfPayload == pConfig->getTxDtmfPayloadTypeNumber() &&
                mRtpRxDtmfPayload == pConfig->getRxDtmfPayloadTypeNumber() &&
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

void RtpEncoderNode::OnRtpPacket(unsigned char* data, uint32_t nSize)
{
    SendDataToRearNode(MEDIASUBTYPE_RTPPACKET, data, nSize, 0, 0, 0);
}

void RtpEncoderNode::SetLocalAddress(const RtpAddress address)
{
    mLocalAddress = address;
}

void RtpEncoderNode::SetPeerAddress(const RtpAddress address)
{
    mPeerAddress = address;
}

bool RtpEncoderNode::SetCvoExtension(const int64_t facing, const int64_t orientation)
{
    IMLOGD3("[SetCvoExtension] cvoValue[%d], facing[%ld], orientation[%ld]", mCvoValue, facing,
            orientation);

    if (mCvoValue != -1)
    {
        uint32_t rotation = 0;
        uint32_t cameraId = 0;

        if (facing == kCameraFacingRear)
        {
            cameraId = 1;
        }

        switch (orientation)
        {
            default:
            case 0:
                rotation = 0;
                break;
            case 270:
                rotation = 1;
                break;
            case 180:
                rotation = 2;
                break;
            case 90:
                rotation = 3;
                break;
        }

        if (cameraId == 1)  // rear camera
        {
            if (rotation == 1)  // CCW90
            {
                rotation = 3;
            }
            else if (rotation == 3)  // CCW270
            {
                rotation = 1;
            }
        }

        uint16_t extensionData;
        IMLOGD3("[SetCvoExtension] cvoValue[%d], facing[%d], orientation[%d]", mCvoValue, cameraId,
                rotation);

        extensionData = ((mCvoValue << 12) | (1 << 8)) | ((cameraId << 3) | rotation);
        mRtpExtension.nDefinedByProfile = 0xBEDE;
        mRtpExtension.nLength = 1;
        mRtpExtension.nExtensionData = extensionData;

        return true;
    }

    return false;
}

void RtpEncoderNode::SetRtpHeaderExtension(tRtpHeaderExtensionInfo& tExtension)
{
    mRtpExtension = tExtension;
}

bool RtpEncoderNode::ProcessAudioData(
        ImsMediaSubType subtype, uint8_t* data, uint32_t size, uint32_t timestamp)
{
    uint32_t currentTimestamp;
    uint32_t timeDiff;
    uint32_t timestampDiff;

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
            IMLOGD_PACKET2(IM_PACKET_LOG_RTP, "[ProcessAudioData] DTMF - size[%d], TS[%d]", size,
                    mDTMFTimestamp);
            // the first dtmf event
            if (timestamp == 0)
            {
                currentTimestamp = ImsMediaTimer::GetTimeInMilliSeconds();
                mDTMFTimestamp = currentTimestamp;
                timeDiff = ((currentTimestamp - mPrevTimestamp) + 10) / 20 * 20;

                if (timeDiff == 0)
                {
                    timeDiff = 20;
                }

                mPrevTimestamp += timeDiff;
            }
            else
            {
                timeDiff = 0;
            }

            timestampDiff = timeDiff * mSamplingRate;
            mRtpSession->SendRtpPacket(
                    mRtpTxDtmfPayload, data, size, mDTMFTimestamp, mMark, timestampDiff);

            if (mMark)
            {
                mMark = false;
            }
        }
    }
    else  // MEDIASUBTYPE_RTPPAYLOAD
    {
        if (mDTMFMode == false)
        {
            currentTimestamp = ImsMediaTimer::GetTimeInMilliSeconds();

            if (mPrevTimestamp == 0)
            {
                timeDiff = 0;
                mPrevTimestamp = currentTimestamp;
            }
            else
            {
                timeDiff = ((currentTimestamp - mPrevTimestamp) + 5) / 20 * 20;

                if (timeDiff > 20)
                {
                    mPrevTimestamp = currentTimestamp;
                }
                else if (timeDiff == 0)
                {
                    IMLOGD_PACKET2(IM_PACKET_LOG_RTP, "[ProcessAudioData] skip, prev[%u] curr[%u]",
                            mPrevTimestamp, currentTimestamp);
                    return false;
                }
                else
                {
                    mPrevTimestamp += timeDiff;
                }
            }

            RtpPacket* packet = new RtpPacket();
            packet->rtpDataType = kRtpDataTypeNormal;
            mCallback->SendEvent(
                    kCollectPacketInfo, kStreamRtpTx, reinterpret_cast<uint64_t>(packet));

            timestampDiff = timeDiff * mSamplingRate;
            IMLOGD_PACKET3(IM_PACKET_LOG_RTP, "[ProcessAudioData] PayloadTx[%d], Size[%d], TS[%d]",
                    mRtpPayloadTx, size, currentTimestamp);
            mRtpSession->SendRtpPacket(
                    mRtpPayloadTx, data, size, currentTimestamp, mMark, timestampDiff);

            if (mMark)
            {
                mMark = false;
            }
        }
    }

    return true;
}

void RtpEncoderNode::ProcessVideoData(
        ImsMediaSubType subtype, uint8_t* data, uint32_t size, uint32_t timestamp, bool mark)
{
    IMLOGD_PACKET2(
            IM_PACKET_LOG_RTP, "[ProcessVideoData] nSize[%d], timestamp[%u]", size, timestamp);

    if (mCvoValue > 0 && mark && subtype == MEDIASUBTYPE_VIDEO_IDR_FRAME)
    {
        mRtpSession->SendRtpPacket(
                mRtpPayloadTx, data, size, timestamp, mark, 0, true, &mRtpExtension);
    }
    else
    {
        mRtpSession->SendRtpPacket(mRtpPayloadTx, data, size, timestamp, mark, 0, false, NULL);
    }
}

void RtpEncoderNode::ProcessTextData(
        ImsMediaSubType subtype, uint8_t* data, uint32_t size, uint32_t timestamp, bool mark)
{
    IMLOGD_PACKET4(IM_PACKET_LOG_RTP,
            "[ProcessTextData] subtype[%d], size[%d], timestamp[%d], mark[%d]", subtype, size,
            timestamp, mark);

    uint32_t timeDiff;

    if (mMark == true)
    {
        timeDiff = 0;
    }
    else
    {
        timeDiff = timestamp - mPrevTimestamp;
    }

    if (subtype == MEDIASUBTYPE_BITSTREAM_T140)
    {
        if (mRedundantLevel > 1 && mRedundantPayload > 0)
        {
            mRtpSession->SendRtpPacket(
                    mRedundantPayload, data, size, timestamp, mark, timeDiff, 0, NULL);
        }
        else
        {
            mRtpSession->SendRtpPacket(
                    mRtpPayloadRx, data, size, timestamp, mark, timeDiff, 0, NULL);
        }
    }
    else if (subtype == MEDIASUBTYPE_BITSTREAM_T140_RED)
    {
        mRtpSession->SendRtpPacket(mRtpPayloadTx, data, size, timestamp, mark, timeDiff, 0, NULL);
    }

    mMark = false;
    mPrevTimestamp = timestamp;
}
