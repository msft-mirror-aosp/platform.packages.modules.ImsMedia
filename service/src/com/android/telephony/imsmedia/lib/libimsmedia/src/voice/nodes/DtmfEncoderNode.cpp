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

#include <ImsMediaDefine.h>
#include <ImsMediaTrace.h>
#include <DtmfEncoderNode.h>

#define BUNDLE_DTMF_DATA_MAX 32    // Max bundling # : 8 X 4bytes

DtmfEncoderNode::DtmfEncoderNode() {
}

DtmfEncoderNode::~DtmfEncoderNode() {
}

BaseNode* DtmfEncoderNode::GetInstance() {
    BaseNode* pNode;
    pNode = new DtmfEncoderNode();
    if (pNode == NULL) {
        IMLOGE0("[GetInstance] Can't create DtmfEncoderNode");
    }
    return pNode;
}

void DtmfEncoderNode::ReleaseInstance(BaseNode* pNode) {
    delete (DtmfEncoderNode*)pNode;
}

BaseNodeID DtmfEncoderNode::GetNodeID() {
    return NODEID_DTMFENCODER;
}

ImsMediaResult DtmfEncoderNode::Start() {
    mAudioFrameDuration = mSamplingRate * 20 / 1000;
    IMLOGD1("[Start] interval[%d]", mAudioFrameDuration);
    return IMS_MEDIA_OK;
}

void DtmfEncoderNode::Stop() {
}

bool DtmfEncoderNode::IsRunTime() {
    return true;
}

bool DtmfEncoderNode::IsSourceNode() {
    return false;
}

void DtmfEncoderNode::OnDataFromFrontNode(ImsMediaSubType subtype,
    uint8_t* pData, uint32_t nDataSize, uint32_t nTimeStamp, bool bMark, uint32_t nSeqNum,
    ImsMediaSubType nDataType) {
    (void)subtype;
    (void)nDataSize;
    (void)nTimeStamp;
    (void)bMark;
    (void)nSeqNum;
    (void)nDataType;
    uint8_t* pDTMFEvent = pData;
    IMLOGD1("[OnDataFromFrontNode] Send DTMF string %c", *pDTMFEvent);
    SendDataToRearNode(MEDIASUBTYPE_DTMFSTART, NULL, 0, 0, 0, 0);    // set dtmf mode true
    SendDTMFEvent(*pDTMFEvent);
    SendDataToRearNode(MEDIASUBTYPE_DTMFEND, NULL, 0, 0, 0, 0);    // set dtmf mode false
}

void DtmfEncoderNode::SetSamplingRate(uint32_t samplingrate) {
    mSamplingRate = samplingrate;
}

void DtmfEncoderNode::SetDuration(uint32_t nDuration, uint32_t endBitDuration) {
    mDuration = nDuration;
    mRetransmitDuration = endBitDuration;
}

void DtmfEncoderNode::SetVolume(uint32_t nVolume) {
    mVolume = nVolume;
}

bool DtmfEncoderNode::SendDTMFEvent(uint8_t nEvent) {
    uint8_t nSignal;
    uint16_t nPeriod = 0;
    uint8_t pbPayload[BUNDLE_DTMF_DATA_MAX];
    uint32_t nPayloadSize;
    uint32_t nTimestamp = 0;
    bool bMarker = true;
    uint32_t nDTMFDuration = mDuration * (mAudioFrameDuration / 20);
    uint32_t nDTMFRetransmitDuration = mRetransmitDuration * (mAudioFrameDuration / 20);

    IMLOGD1("[SendDTMF] nEvent[%c]", nEvent);

    /* Set input signal */
    if ((nEvent >= '0') && (nEvent <= '9')) nSignal = nEvent - '0';
    else if (nEvent=='*')             nSignal = 10;
    else if (nEvent=='#')             nSignal = 11;
    else if (nEvent=='A')             nSignal = 12;
    else if (nEvent=='B')             nSignal = 13;
    else if (nEvent=='C')             nSignal = 14;
    else if (nEvent=='D')             nSignal = 15;
    else if (nEvent=='F')             nSignal = 16;
    else
    {
        IMLOGE1("[SendDTMF] Wrong DTMF Signal[%c]!",nEvent);
        return false;
    }

#if 0
    // support bundling...
    int32_t  nFrameBundlingSize = 1;

    if (nFrameBundlingSize > 1) {
        uint32_t    nBundlingDuration;
        uint8_t    nRemainedTime;

        nBundlingDuration = nFrameBundlingSize * AUDIO_FRAME_DURATION_INTERVAL;
        nRemainedTime = nDTMFRetransmitDuration % nBundlingDuration;

        if (nRemainedTime > 0) {
            nDTMFRetransmitDuration += (nBundlingDuration - nRemainedTime);
        }
    }
#endif

    // make and send DTMF packet
    for (nPeriod = mAudioFrameDuration; nPeriod < nDTMFDuration; nPeriod += mAudioFrameDuration) {
        nPayloadSize = MakeDTMFPayload(pbPayload, nSignal, false, mVolume, nPeriod);
        SendDataToRearNode(MEDIASUBTYPE_DTMFEVENT, pbPayload, nPayloadSize, nTimestamp, bMarker, 0);
        nTimestamp += 20;
        bMarker = false;
    }

    // make and send DTMF last packet and retransmit it
    nPayloadSize = MakeDTMFPayload(pbPayload, nSignal, true, mVolume, nPeriod);

    for (nPeriod = 0; nPeriod <= nDTMFRetransmitDuration; nPeriod += mAudioFrameDuration) {
        SendDataToRearNode(MEDIASUBTYPE_DTMFEVENT, pbPayload, nPayloadSize, nTimestamp, false, 0);
        nTimestamp += 20;
    }

    return true;
}

uint32_t DtmfEncoderNode::MakeDTMFPayload(uint8_t* pbPayload, uint8_t nEvent,
    bool bEnd, uint8_t nVolume, uint16_t nPeriod) {
    // Event: 8 bits
    pbPayload[0] = nEvent;

    // E: 1 bit
    if (bEnd) {
        pbPayload[1] = 0x80;
    } else {
        pbPayload[1] = 0x00;
    }

    // Volume: 6 bits
    pbPayload[1] += nVolume;

    // duration: 16 bits
    pbPayload[2] = (nPeriod >> 8) & 0xFF;
    pbPayload[3] = nPeriod & 0xFF;
    return 4;
}
