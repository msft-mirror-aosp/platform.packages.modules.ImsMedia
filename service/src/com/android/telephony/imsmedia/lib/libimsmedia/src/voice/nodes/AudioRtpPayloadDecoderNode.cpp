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

#include <AudioRtpPayloadDecoderNode.h>
#include <ImsMediaAudioFmt.h>
#include <ImsMediaTrace.h>

AudioRtpPayloadDecoderNode::AudioRtpPayloadDecoderNode() {
    mPrevCMR = 15;
}

AudioRtpPayloadDecoderNode::~AudioRtpPayloadDecoderNode() {
    std::lock_guard<std::mutex> guard(mMutexExit);
}

BaseNode* AudioRtpPayloadDecoderNode::GetInstance() {
    return new AudioRtpPayloadDecoderNode();
}

void AudioRtpPayloadDecoderNode::ReleaseInstance(BaseNode* pNode) {
    delete (AudioRtpPayloadDecoderNode*)pNode;
}

BaseNodeID AudioRtpPayloadDecoderNode::GetNodeID() {
    return BaseNodeID::NODEID_RTPPAYLOAD_DECODER_AUDIO;
}

ImsMediaResult AudioRtpPayloadDecoderNode::Start() {
    IMLOGD0("[Start]");
    std::lock_guard<std::mutex> guard(mMutexExit);
    mPrevCMR = 15;
    mNodeState = NODESTATE_RUNNING;
    return IMS_MEDIA_OK;
}

void AudioRtpPayloadDecoderNode::Stop() {
    IMLOGD0("[Stop]");
    std::lock_guard<std::mutex> guard(mMutexExit);
    mNodeState = NODESTATE_STOPPED;
}

bool AudioRtpPayloadDecoderNode::IsRunTime() {
    return true;
}

bool AudioRtpPayloadDecoderNode::IsSourceNode() {
    return false;
}

void AudioRtpPayloadDecoderNode::OnDataFromFrontNode(ImsMediaSubType subtype,
    uint8_t* pData, uint32_t nDataSize, uint32_t nTimestamp, bool bMark,
    uint32_t nSeqNum, ImsMediaSubType nDataType) {
    (void)subtype;
    switch (mCodecType) {
        case AUDIO_EVRC:
        case AUDIO_EVRC_B:
            //Decode_PH_EVRCB(pData, nDataSize, nTimestamp, bMark, nSeqNum);
            break;
        case AUDIO_AMR:
        case AUDIO_AMR_WB:
            Decode_PH_AMR(pData, nDataSize, nTimestamp, bMark, nSeqNum);
            break;
        case AUDIO_G711_PCMU:
        case AUDIO_G711_PCMA:
            SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD,
                pData, nDataSize, nTimestamp, bMark, nSeqNum);
            break;
        case AUDIO_EVS:
            //Decode_PH_EVS(pData, nDataSize, nTimestamp, bMark, nSeqNum);
            break;
        default:
            IMLOGE1("[OnDataFromFrontNode] invalid codec type[%d]", mCodecType);
            SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD, pData, nDataSize, nTimestamp,
                bMark, nSeqNum, nDataType);
            break;
    }
}

void AudioRtpPayloadDecoderNode::SetConfig(void* config) {
    (void)config;
}

void AudioRtpPayloadDecoderNode::SetCodec(eAudioCodecType eCodecType) {
    mCodecType = eCodecType;
}

void AudioRtpPayloadDecoderNode::SetPayloadMode(uint32_t mode) {
    mHeaderMode = mode;
}

void AudioRtpPayloadDecoderNode::Decode_PH_AMR(uint8_t* pData,
    uint32_t nDataSize, uint32_t nTimestamp, bool bMark, uint32_t nSeqNum) {
    (void)bMark;
    uint32_t timestamp = nTimestamp;
    static std::list<uint32_t> listFrameType;    // defined as static variable for memory management
    uint32_t eRate;
    uint32_t f;
    uint32_t cmr;
    uint32_t QbitPos;  // Q_Speech_Sid_Bad

    std::lock_guard<std::mutex> guard(mMutexExit);

    IMLOGD_PACKET4(IM_PACKET_LOG_PH,
        "[Decode_PH_AMR] GetCodectype[%d], mHeaderMode[%d], nSeqNum[%d], timestamp[%u]",
        mCodecType, mHeaderMode, nSeqNum, timestamp);

    mBitReader.SetBuffer(pData, nDataSize);
    // read cmr
    cmr = mBitReader.Read(4);

    if (mHeaderMode == RTPPAYLOADHEADER_MODE_AMR_OCTETALIGNED) {
        mBitReader.Read(4);
    }

    if (cmr != 15 && cmr != mPrevCMR) {
        if ((mCodecType == AUDIO_AMR && cmr <= 7) || (mCodecType == AUDIO_AMR_WB && cmr <= 8)) {
            IMLOGD2("[Decode_PH_AMR] CMR %d->%d", mPrevCMR, cmr);
            //send internal event to operate cmr operation in encoder side
            mPrevCMR = cmr;
        } else {
            IMLOGE1("[Decode_PH_AMR] invalid cmr value %d", cmr);
        }
    }
    else if (cmr == 15) {
        //send internal event to operate cmr operation in encoder side
    } else {
        IMLOGE0("[Decode_PH_AMR] Can't find Highest Negotiated Mode in negotiated mode-set");
    }

    // get num of frame
    do {
        f = mBitReader.Read(1);   // f(1)
        eRate = mBitReader.Read(4);   // ft(4)
        QbitPos = mBitReader.Read(1);   // q(2)
        IMLOGD_PACKET2(IM_PACKET_LOG_PH,
            "[Decode_PH_AMR] f[%d], ft[%d]", f, eRate);
        listFrameType.push_back(eRate);
        if (mHeaderMode == RTPPAYLOADHEADER_MODE_AMR_OCTETALIGNED) {
            mBitReader.Read(2);   //padding
        }
    }
    while (f == 1);

    IMLOGD3("[Decode_PH_AMR] Q_Speech_SID <Q_Speech_SID> f[%d] eRate[%d] QbitPos[%d]",
        f, eRate, QbitPos); // Q_Speech_SID

    // read speech frames
    while (listFrameType.size() > 0) {
        uint32_t nDataBitSize;
        uint32_t mode = listFrameType.front();
        if (mCodecType == AUDIO_AMR) {
            nDataBitSize = ImsMediaAudioFmt::ConvertAmrModeToBitLen(mode);
        } else {
            nDataBitSize = ImsMediaAudioFmt::ConvertAmrWbModeToBitLen(mode);
        }

        listFrameType.pop_front();
        mBitWriter.SetBuffer(mPayload, MAX_AUDIO_PAYLOAD_SIZE);
        //set payload header
        mBitWriter.Write(f, 1);
        mBitWriter.Write(eRate, 4);
        mBitWriter.Write(QbitPos, 1);
        mBitWriter.Write(0, 2);
        mBitReader.ReadByteBuffer(mPayload + 1, nDataBitSize);
        //add payload header to payload size
        uint32_t nBufferSize = ((nDataBitSize + 7) >> 3) + 1;
        IMLOGD_PACKET6(IM_PACKET_LOG_PH,
            "[Decode_PH_AMR] result = %02X %02X %02X %02X, len[%d], eRate[%d]",
            mPayload[0], mPayload[1], mPayload[2], mPayload[3], nBufferSize, eRate);
        // send remaining packet number in bundle as bMark value
        SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD, mPayload, nBufferSize,
            timestamp, listFrameType.size(), nSeqNum);

        timestamp += 20;
    }
}