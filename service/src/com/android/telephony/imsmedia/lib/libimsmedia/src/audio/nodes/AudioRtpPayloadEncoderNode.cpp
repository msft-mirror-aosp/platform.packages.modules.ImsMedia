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

#include <AudioRtpPayloadEncoderNode.h>
#include <ImsMediaAudioFmt.h>
#include <ImsMediaTrace.h>
#include <AudioConfig.h>

AudioRtpPayloadEncoderNode::AudioRtpPayloadEncoderNode() {
    mPtime = 0;
    mFirstFrame = false;
    mMaxNumOfFrame = 0;
    mCurrNumOfFrame = 0;
    mCurrFramePos = 0;
    mTotalPayloadSize = 0;
}

AudioRtpPayloadEncoderNode::~AudioRtpPayloadEncoderNode() {
}

BaseNode* AudioRtpPayloadEncoderNode::GetInstance() {
    return new AudioRtpPayloadEncoderNode();
}

void AudioRtpPayloadEncoderNode::ReleaseInstance(BaseNode* pNode) {
    delete (AudioRtpPayloadEncoderNode*)pNode;
}

BaseNodeID AudioRtpPayloadEncoderNode::GetNodeID() {
    return BaseNodeID::NODEID_RTPPAYLOAD_ENCODER_AUDIO;
}

ImsMediaResult AudioRtpPayloadEncoderNode::Start() {
    IMLOGD0("[Start]");
    std::lock_guard<std::mutex> guard(mMutexExit);
    mMaxNumOfFrame = mPtime / 20;

    if (mMaxNumOfFrame == 0 || mMaxNumOfFrame > MAX_FRAME_IN_PACKET) {
        IMLOGE1("[Start] Invalid ptime [%d]", mPtime);
        return RESULT_INVALID_PARAM;
    }

    mCurrNumOfFrame = 0;
    mCurrFramePos = 0;
    mFirstFrame = true;
    mTotalPayloadSize = 0;
    mNodeState = NODESTATE_RUNNING;
    return RESULT_SUCCESS;
}

void AudioRtpPayloadEncoderNode::Stop() {
    IMLOGD0("[Stop]");
    std::lock_guard<std::mutex> guard(mMutexExit);
    mNodeState = NODESTATE_STOPPED;
}

bool AudioRtpPayloadEncoderNode::IsRunTime() {
    return true;
}

bool AudioRtpPayloadEncoderNode::IsSourceNode() {
    return false;
}

void AudioRtpPayloadEncoderNode::OnDataFromFrontNode(ImsMediaSubType subtype,
    uint8_t* pData, uint32_t nDataSize, uint32_t nTimestamp, bool bMark, uint32_t nSeqNum,
    ImsMediaSubType nDataType) {
    (void)subtype;
    switch (mCodecType) {
        case AUDIO_EVRC:
        case AUDIO_EVRC_B:
            //Encode_PH_EVRCB(subtype, pData, nDataSize, nTimestamp, bMark);
            break;
        case AUDIO_AMR:
        case AUDIO_AMR_WB:
            Encode_PH_AMR(pData, nDataSize, nTimestamp, bMark);
            break;
        case AUDIO_G711_PCMU:
        case AUDIO_G711_PCMA:
            SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD, pData, nDataSize, nTimestamp,
                bMark, nSeqNum, nDataType);
            break;
        case AUDIO_EVS:
            //Encode_PH_EVS(pData, nDataSize, nTimestamp, bMark);
            break;
        default:
            IMLOGE1("[OnDataFromFrontNode] invalid codec type[%d]", mCodecType);
            SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD, pData, nDataSize, nTimestamp,
                bMark, nSeqNum, nDataType);
            break;
    }
}

void AudioRtpPayloadEncoderNode::SetConfig(void* config) {
    AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);
    if (pConfig != NULL) {
        SetCodec(pConfig->getCodecType());
        if (mCodecType == AUDIO_AMR || mCodecType == AUDIO_AMR_WB) {
            SetPayloadMode(pConfig->getAmrParams().getOctetAligned());
        }
        SetPtime(pConfig->getPtimeMillis());
    }
}

void AudioRtpPayloadEncoderNode::SetCodec(int32_t type) {
    switch (type) {
        case AudioConfig::CODEC_AMR:
            mCodecType = AUDIO_AMR;
            break;
        case AudioConfig::CODEC_AMR_WB:
            mCodecType = AUDIO_AMR_WB;
            break;
        case AudioConfig::CODEC_EVS:
            mCodecType = AUDIO_EVS;
            break;
        case AudioConfig::CODEC_PCMA:
            mCodecType = AUDIO_G711_PCMA;
            break;
        case AudioConfig::CODEC_PCMU:
            mCodecType = AUDIO_G711_PCMU;
            break;
        default:
            break;
    }
}

void AudioRtpPayloadEncoderNode::SetPayloadMode(bool bOctetAligned) {
    IMLOGD1("[SetPayloadMode] bOctetAligned[%d]", bOctetAligned);
    mOctetAligned = bOctetAligned;
}

void AudioRtpPayloadEncoderNode::SetPtime(uint32_t ptime) {
    mPtime = ptime;
}

void AudioRtpPayloadEncoderNode::Encode_PH_AMR(uint8_t* pData, uint32_t nDataSize,
    uint32_t nTimestamp, bool bMark) {
    (void)bMark;
    uint32_t nCmr = 15;
    uint32_t f, ft, q, nDataBitSize;
    uint32_t nTotalSize;
    std::lock_guard<std::mutex> guard(mMutexExit);

#ifndef LEGACY_AUDIO_ENABLED   //for ap audio test
    pData++;
    nDataSize -= 1;
#endif
    if (nDataSize > 4) {
        IMLOGD_PACKET5(IM_PACKET_LOG_PH, "[Encode_PH_AMR] src = %02X %02X %02X %02X, len[%d]",
            pData[0], pData[1], pData[2], pData[3], nDataSize);
    }

    IMLOGD_PACKET2(IM_PACKET_LOG_PH, "[Encode_PH_AMR] codectype[%d], octetAligned[%d]",
        mCodecType, mOctetAligned);

    mCurrNumOfFrame++;
    f = (mCurrNumOfFrame == mMaxNumOfFrame) ? 0 : 1;

    if (mCodecType == AUDIO_AMR) {
        nCmr = 0x0F;
        ft = ImsMediaAudioFmt::ConvertLenToAmrMode(nDataSize);
        nDataBitSize = ImsMediaAudioFmt::ConvertAmrModeToBitLen(ft);
    } else {
        nCmr = 0x0F;
        ft = ImsMediaAudioFmt::ConvertLenToAmrWbMode(nDataSize);
        nDataBitSize = ImsMediaAudioFmt::ConvertAmrWbModeToBitLen(ft);
    }

    q = 1;
#ifdef USE_CMR_TEST
    // cmr test code start
    // generate cmr
    static int sAMRCount = 0;
    static int bAMRUpdate = 0;
    static int nAMRNextMode = 7;
    static int nAMRInc = -1;
    sAMRCount++;
    if ((sAMRCount & 0x7F) == 0) {
        bAMRUpdate = 1;
    }
    nCmr = nAMRNextMode;
    if (bAMRUpdate == 1 && nDataSize > 0) {
        int max_mode;
        if (mCodecType == AUDIO_AMR) max_mode = 7;
        else max_mode = 8;
        nAMRNextMode += nAMRInc;
        if (nAMRNextMode < 0) {
            nAMRNextMode = 1;
            nAMRInc = 1;
        } else if (nAMRNextMode > max_mode) {
            nAMRNextMode = max_mode - 1;
            nAMRInc = -1;
        }
        bAMRUpdate = 0;
    }
    // cmr test code end
#endif
    // the first paylaod
    if (mCurrNumOfFrame == 1) {
        memset(mPayload, 0, MAX_AUDIO_PAYLOAD_SIZE);
        mBWHeader.SetBuffer(mPayload, MAX_AUDIO_PAYLOAD_SIZE);
        mBWPayload.SetBuffer(mPayload, MAX_AUDIO_PAYLOAD_SIZE);
        mBWHeader.Write(nCmr, 4);

        if (mOctetAligned == true) {
            mBWHeader.Write(0, 4);
            mBWPayload.Seek(8 + mMaxNumOfFrame*8);
        } else {
            mBWPayload.Seek(4 + mMaxNumOfFrame*6);
        }

        mTimestamp = nTimestamp;
    }

    // Payload ToC
    mBWHeader.Write(f, 1);
    mBWHeader.Write(ft, 4);
    mBWHeader.Write(q, 1);

    if (mOctetAligned == true) {
        mBWHeader.AddPadding();
    }

    IMLOGD_PACKET2(IM_PACKET_LOG_PH,
        "[Encode_PH_AMR] nDataBitSize[%d], nDataSize[%d]",
        nDataBitSize, nDataSize);

    // Speech Frame
    mBWPayload.WriteByteBuffer(pData, nDataBitSize);

    if (mOctetAligned == true) {
        mBWPayload.AddPadding();
    }

    mTotalPayloadSize += nDataSize;

    if (mCurrNumOfFrame == mMaxNumOfFrame) {
        mBWHeader.Flush();
        mBWPayload.AddPadding();
        mBWPayload.Flush();
        nTotalSize = mBWPayload.GetBufferSize();

        IMLOGD_PACKET7(IM_PACKET_LOG_PH,
            "[Encode_PH_AMR] result = %02X %02X %02X %02X %02X %02X, len[%d]",
            mPayload[0], mPayload[1], mPayload[2], mPayload[3], mPayload[4], mPayload[5],
            nTotalSize);

        if (mTotalPayloadSize > 0) {
            SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD,
                mPayload, nTotalSize, mTimestamp, mFirstFrame, 0);
        }

        mCurrNumOfFrame = 0;
        mTotalPayloadSize = 0;

        if (mFirstFrame) {
            mFirstFrame = false;
        }
    }
}