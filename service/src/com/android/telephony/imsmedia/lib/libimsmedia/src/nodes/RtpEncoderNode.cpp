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
#include <string.h>

#ifdef DEBUG_JITTER_GEN_SIMULATION_DELAY
#define DEBUG_JITTER_MAX_PACKET_INTERVAL    70     // msec, minimum value is 30
#endif
#ifdef DEBUG_JITTER_GEN_SIMULATION_REORDER
    #include <ImsMediaDataQueue.h>
    #define DEBUG_JITTER_REORDER_MAX 4
    #define DEBUG_JITTER_REORDER_MIN     4
    #define DEBUG_JITTER_NORMAL             2
#endif
#ifdef DEBUG_JITTER_GEN_SIMULATION_LOSS
    #define DEBUG_JITTER_LOSS_NORMAT_PACKET    49
    #define DEBUG_JITTER_LOSS_LOSS_PACKET        1
#endif

RtpEncoderNode::RtpEncoderNode() {
    mRtpSession = NULL;
    mDTMFMode = false;
    mAudioMark = false;
    mPrevTimestamp = 0;
    mDTMFTimestamp = 0;
    // mLocalAddress = RtpAddress("0,0,0,0", 0);
    // mPeerAddress = RtpAddress("0,0,0,0", 0);
    mPeerPayload = 0;
    mDTMFPeerPayload = 0;
    mSamplingRate = 0;
#ifdef DEBUG_JITTER_GEN_SIMULATION_DELAY
    mNextTime = 0;
#endif
#ifdef DEBUG_JITTER_GEN_SIMULATION_REORDER
    mReorderDataCount = 0;
#endif
}

RtpEncoderNode::~RtpEncoderNode() {
    if (mRtpSession) {
        mRtpSession->SetRtpEncoderListener(NULL);
        mRtpSession->StopRtp();
        IRtpSession::ReleaseInstance(mRtpSession);
    }

    mRtpSession = NULL;
}

BaseNode* RtpEncoderNode::GetInstance() {
    return new RtpEncoderNode();
}

void RtpEncoderNode::ReleaseInstance(BaseNode* pNode) {
    delete (RtpEncoderNode*)pNode;
}

BaseNodeID RtpEncoderNode::GetNodeID() {
    return BaseNodeID::NODEID_RTPENCODER;
}

ImsMediaResult RtpEncoderNode::Start() {
    //start here
    IMLOGD0("[Start]");
    if (mRtpSession == NULL) {
        mRtpSession = IRtpSession::GetInstance(mMediaType, mLocalAddress, mPeerAddress);
        if (mRtpSession == NULL) {
            IMLOGE0("[Start] Can't create rtp session");
            return IMS_MEDIA_ERROR_UNKNOWN;
        }
    }
    mRtpSession->SetRtpEncoderListener(this);

    //test
    ImsMediaHal::RtpSessionParams params;
    params.pTimeMillis = 20;
    params.maxPtimeMillis = static_cast<char>(240);
    params.maxMtuBytes = 1500;
    params.dscp = 0;
    params.dtmfParams.payloadTypeNumber = 100;
    params.dtmfParams.samplingRateKHz = static_cast<char>(16);
    params.codecParams.codec.codecType = ImsMediaHal::CodecType::AMR_WB;
    params.codecParams.codec.bandwidth = ImsMediaHal::EvsBandwidth::WIDE_BAND;
    params.codecParams.rxPayloadTypeNumber = 96;
    params.codecParams.txPayloadTypeNumber = 96;
    params.codecParams.samplingRateKHz = static_cast<char>(16);
    params.codecParams.txCodecModeRequest = 0;
    params.codecParams.dtxEnabled = 1;
    params.codecParams.codecSpecificParams.amr.amrMode = ImsMediaHal::AmrMode::AMR_MODE_8;
    params.codecParams.codecSpecificParams.amr.octetAligned = 1;
    params.codecParams.codecSpecificParams.amr.maxRedundancyMillis = 0;

    mPeerPayload = params.codecParams.rxPayloadTypeNumber;
    mDTMFPeerPayload = params.dtmfParams.payloadTypeNumber;
    mSamplingRate = params.codecParams.samplingRateKHz;

    mRtpSession->SetRtpPayloadParam(params);
    mRtpSession->StartRtp();
    mDTMFMode = false;
    mAudioMark = true;
    mPrevTimestamp = 0;
    mDTMFTimestamp = 0;
    //mRtpSession->EnableRtpMonitoring(true);
#ifdef DEBUG_JITTER_GEN_SIMULATION_DELAY
    mNextTime = 0;
#endif
#ifdef DEBUG_JITTER_GEN_SIMULATION_REORDER
    jitterData.Clear();
    mReorderDataCount = 0;
#endif
    mNodeState = NODESTATE_RUNNING;
    return IMS_MEDIA_OK;
}

void RtpEncoderNode::Stop() {
    IMLOGD0("[Stop]");
    //mRtpSession->EnableRtpMonitoring(false);
    //mRtpSession->EnableRtcpTx(true);
    mNodeState = NODESTATE_STOPPED;
}

void RtpEncoderNode::ProcessData() {
    ImsMediaSubType eSubType;
    uint8_t* pData = NULL;
    uint32_t nDataSize = 0;
    uint32_t nTimestamp = 0;
    bool bMark = false;
    uint32_t nSeqNum = 0;

    if (GetData(&eSubType, &pData, &nDataSize, &nTimestamp, &bMark, &nSeqNum, NULL) == false) {
        return;
    }
#ifdef DEBUG_JITTER_GEN_SIMULATION_DELAY
    {
        uint32_t nCurrTime = ImsMediaTimer::GetTimeInMilliSeconds();
        if ((GetDataCount(mMediaType) <= 1)
            && mNextTime && nCurrTime < mNextTime) {
            return;
        } else {
            uint32_t max_interval_1 =
                GenerateRandom((DEBUG_JITTER_MAX_PACKET_INTERVAL-30) / GetDataCount(mMediaType));
            uint32_t max_interval_2 = 30 + max_interval_1;
            mNextTime = nCurrTime + GenerateRandom(max_interval_2);
        }
    }
#endif
    if (mMediaType == IMS_MEDIA_AUDIO) {
        ProcessAudioData(eSubType, pData, nDataSize, nTimestamp);
    }

    DeleteData();
}

bool RtpEncoderNode::IsRunTime() {
    return false;
}

bool RtpEncoderNode::IsSourceNode() {
    return false;
}

//IRtpEncoderListener
void RtpEncoderNode::OnRtpPacket(unsigned char* pData, uint32_t nSize) {
#ifdef DEBUG_JITTER_GEN_SIMULATION_LOSS
    bool nLossFlag = false;
    {
        static uint32_t nLossNormalCount = 0;
        static uint32_t nLossLossCount = 0;
        if (nLossNormalCount < DEBUG_JITTER_LOSS_NORMAT_PACKET) {
            nLossNormalCount++;
        } else {
            if (nLossLossCount < DEBUG_JITTER_LOSS_LOSS_PACKET) {
                nLossLossCount++;
                nLossFlag = true;
            } else {
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

        if (mReorderDataCount < DEBUG_JITTER_NORMAL) {
            jitterData.push_back(&entry);
        }
        else if (mReorderDataCount < DEBUG_JITTER_NORMAL+DEBUG_JITTER_REORDER_MAX) {
            int32_t nCurrReorderSize;
            int32_t nInsertPos;
            uint32_t nCurrJitterBufferSize;
            nCurrJitterBufferSize = jitterData.GetCount();
            if (DEBUG_JITTER_REORDER_MAX > DEBUG_JITTER_REORDER_MIN) {
                nCurrReorderSize = mReorderDataCount - DEBUG_JITTER_NORMAL + 1 -
                    GenerateRandom(DEBUG_JITTER_REORDER_MAX-DEBUG_JITTER_REORDER_MIN+1);
            } else {
                nCurrReorderSize = mReorderDataCount - DEBUG_JITTER_NORMAL + 1;
            }

            if (nCurrReorderSize > 0) nCurrReorderSize = GenerateRandom(nCurrReorderSize+1);

            nInsertPos = nCurrJitterBufferSize-nCurrReorderSize;
            if (nInsertPos < 0) nInsertPos = 0;
            jitterData.InsertAt(nInsertPos, &entry);
        }

        mReorderDataCount++;

        if (mReorderDataCount >= DEBUG_JITTER_NORMAL+DEBUG_JITTER_REORDER_MAX) {
            mReorderDataCount = 0;
        }

        // send
        while (jitterData.GetCount() >= DEBUG_JITTER_REORDER_MAX) {
            DataEntry* pEntry;
            if (jitterData.Get(&pEntry)) {
#ifdef DEBUG_JITTER_GEN_SIMULATION_LOSS
                if (nLossFlag == false) {
                    SendDataToRearNode(MEDIASUBTYPE_RTPPACKET,
                        pEntry->pbBuffer, pEntry->nBufferSize, 0, 0, 0);
                }
#else
                SendDataToRearNode(MEDIASUBTYPE_RTPPACKET,
                    pEntry->pbBuffer, pEntry->nBufferSize, 0, 0, 0);
#endif
                jitterData.Delete();
            }
        }
    }
#else

#ifdef DEBUG_JITTER_GEN_SIMULATION_LOSS
    if (nLossFlag == false) {
        SendDataToRearNode(MEDIASUBTYPE_RTPPACKET, pData, nSize, 0, 0, 0);
    }
#else
    SendDataToRearNode(MEDIASUBTYPE_RTPPACKET, pData, nSize, 0, 0, 0);
#endif
#endif
}

void RtpEncoderNode::SetRtpSessionParams(ImsMediaHal::RtpSessionParams* params) {
    mSessionParams = std::make_shared<ImsMediaHal::RtpSessionParams>();
    memcpy(mSessionParams.get(), params, sizeof(ImsMediaHal::RtpSessionParams));
}

void RtpEncoderNode::SetLocalAddress(const RtpAddress address) {
    mLocalAddress = address;
}

void RtpEncoderNode::SetPeerAddress(const RtpAddress address) {
    mPeerAddress = address;
}

void RtpEncoderNode::ProcessAudioData(ImsMediaSubType eSubType, uint8_t* pData, uint32_t nDataSize,
    uint32_t nTimestamp) {
    uint32_t nCurrTimestamp;
    uint32_t nTimeDiff;
    uint32_t nTimeDiff_RTPTSUnit;

    if (eSubType == MEDIASUBTYPE_DTMFSTART) {
        IMLOGD0("[ProcessData] SetDTMF mode true");
        mDTMFMode = true;
        mAudioMark = true;
    }
    else if (eSubType == MEDIASUBTYPE_DTMFEND) {
        IMLOGD0("[ProcessData] SetDTMF mode false");
        mDTMFMode = false;
        mAudioMark = true;
    }
    else if (eSubType == MEDIASUBTYPE_DTMFEVENT) {
        if (mDTMFMode) {
            IMLOGD_PACKET2(IM_PACKET_LOG_RTP,
                "[ProcessData] DTMF - nSize[%d], TS[%d]",
                nDataSize, mDTMFTimestamp);
            // the first dtmf event
            if (nTimestamp == 0) {
                nCurrTimestamp = ImsMediaTimer::GetTimeInMilliSeconds();
                mDTMFTimestamp = nCurrTimestamp;
                nTimeDiff = ((nCurrTimestamp - mPrevTimestamp) + 10) / 20 * 20;

                if (nTimeDiff == 0) nTimeDiff = 20;

                mPrevTimestamp += nTimeDiff;
            } else {
                nTimeDiff = 0;
            }

            nTimeDiff_RTPTSUnit = nTimeDiff * (mSamplingRate / 1000);
            mRtpSession->SendRtpPacket(mDTMFPeerPayload, pData,
                nDataSize, mDTMFTimestamp, mAudioMark, nTimeDiff_RTPTSUnit, 0 , NULL);

            if (mAudioMark) mAudioMark = false;
        }
    } else {
        if (mDTMFMode == false) {
            nCurrTimestamp = ImsMediaTimer::GetTimeInMilliSeconds();

            if (mPrevTimestamp == 0) {
                nTimeDiff = 0;
                mPrevTimestamp = nCurrTimestamp;
            } else {
                nTimeDiff = ((nCurrTimestamp - mPrevTimestamp) + 10) / 20 * 20;

                if (nTimeDiff > 20) {
                    mPrevTimestamp = nCurrTimestamp;
                } else if (nTimeDiff == 0) {
                    IMLOGW2("[ProcessData] skip this turn orev[%u] curr[%u]",
                        mPrevTimestamp, nCurrTimestamp);
                    return;
                } else {
                    mPrevTimestamp += nTimeDiff;
                }
            }

            nTimeDiff_RTPTSUnit = nTimeDiff * (mSamplingRate / 1000);
            IMLOGD_PACKET3(IM_PACKET_LOG_RTP,
                "[ProcessData] mPeerPayload[%d], nSize[%d], nTS[%d]",
                mPeerPayload, nDataSize, nCurrTimestamp);
            mRtpSession->SendRtpPacket(mPeerPayload, pData, nDataSize, nCurrTimestamp,
                mAudioMark, nTimeDiff_RTPTSUnit, 0 , NULL);

            if (mAudioMark) mAudioMark = false;
        }
    }
}
