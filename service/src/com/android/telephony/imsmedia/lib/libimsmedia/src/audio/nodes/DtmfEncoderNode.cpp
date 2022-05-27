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
#include <ImsMediaTimer.h>
#include <AudioConfig.h>

#define BUNDLE_DTMF_DATA_MAX             32  // Max bundling # : 8 X 4bytes
#define DTMF_DEFAULT_DURATION            200
#define DTMF_MINIMUM_DURATION            40
#define DTMF_DEFAULT_RETRANSMIT_DURATION 2
#define DTMF_DEFAULT_VOLUME              10

DtmfEncoderNode::DtmfEncoderNode()
{
    mStopDtmf = true;
    mListDtmfDigit.clear();
    mSamplingRate = 0;
    mDuration = DTMF_DEFAULT_DURATION;
    mRetransmitDuration = DTMF_DEFAULT_RETRANSMIT_DURATION;
    mVolume = DTMF_DEFAULT_VOLUME;
    mAudioFrameDuration = 0;
}

DtmfEncoderNode::~DtmfEncoderNode() {}

BaseNode* DtmfEncoderNode::GetInstance()
{
    BaseNode* pNode;
    pNode = new DtmfEncoderNode();
    if (pNode == NULL)
    {
        IMLOGE0("[GetInstance] Can't create DtmfEncoderNode");
    }
    return pNode;
}

void DtmfEncoderNode::ReleaseInstance(BaseNode* pNode)
{
    delete (DtmfEncoderNode*)pNode;
}

BaseNodeID DtmfEncoderNode::GetNodeID()
{
    return NODEID_DTMFENCODER;
}

ImsMediaResult DtmfEncoderNode::Start()
{
    mAudioFrameDuration = mSamplingRate * 20 / 1000;
    IMLOGD1("[Start] interval[%d]", mAudioFrameDuration);
    StartThread();
    mNodeState = kNodeStateRunning;
    return RESULT_SUCCESS;
}

void DtmfEncoderNode::Stop()
{
    StopThread();
    mCond.signal();
    mNodeState = kNodeStateStopped;
}

bool DtmfEncoderNode::IsRunTime()
{
    return true;
}

bool DtmfEncoderNode::IsSourceNode()
{
    return false;
}

void DtmfEncoderNode::SetConfig(void* config)
{
    AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);
    if (pConfig != NULL)
    {
        SetSamplingRate(pConfig->getSamplingRateKHz() * 1000);
        // test parameter
        SetDuration(DTMF_DEFAULT_DURATION, DTMF_DEFAULT_RETRANSMIT_DURATION);
        SetVolume(DTMF_DEFAULT_VOLUME);
    }
}

bool DtmfEncoderNode::IsSameConfig(void* config)
{
    if (config == NULL)
    {
        return true;
    }

    AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);
    return (mSamplingRate == (pConfig->getSamplingRateKHz() * 1000));
}

void DtmfEncoderNode::OnDataFromFrontNode(ImsMediaSubType subtype, uint8_t* pData,
        uint32_t nDataSize, uint32_t volume, bool bMark, uint32_t duration,
        ImsMediaSubType nDataType)
{
    (void)bMark;
    (void)nDataType;
    (void)volume;
    if (duration != 0 && subtype == MEDIASUBTYPE_DTMF_PAYLOAD)
    {
        mStopDtmf = true;
        IMLOGD1("[OnDataFromFrontNode] Send DTMF string %c", *pData);
        uint8_t nSignal;
        if (nDataSize == 0 || convertSignal(*pData, nSignal) == false)
        {
            return;
        }
        SendDataToRearNode(MEDIASUBTYPE_DTMFSTART, NULL, 0, 0, 0, 0);  // set dtmf mode true
        SendDTMFEvent(nSignal, duration);
        SendDataToRearNode(MEDIASUBTYPE_DTMFEND, NULL, 0, 0, 0, 0);  // set dtmf mode false
    }
    else
    {
        if (subtype == MEDIASUBTYPE_DTMFEND)
        {
            mMutex.lock();
            mStopDtmf = true;
            mMutex.unlock();
        }
        else if (MEDIASUBTYPE_DTMFSTART)
        {
            uint8_t nSignal;
            if (convertSignal(*pData, nSignal) == false)
            {
                return;
            }
            mMutex.lock();
            mStopDtmf = false;
            mMutex.unlock();
            mListDtmfDigit.push_back(nSignal);
            mCond.signal();
        }
    }
}

void DtmfEncoderNode::SetSamplingRate(uint32_t samplingrate)
{
    mSamplingRate = samplingrate;
}

void DtmfEncoderNode::SetDuration(uint32_t nDuration, uint32_t endBitDuration)
{
    mDuration = nDuration;
    mRetransmitDuration = endBitDuration;
}

void DtmfEncoderNode::SetVolume(uint32_t nVolume)
{
    mVolume = nVolume;
}

void* DtmfEncoderNode::run()
{
    IMLOGD0("[run] enter");
    for (;;)
    {
        IMLOGD0("[run] wait");
        mCond.wait();
        if (IsThreadStopped())
        {
            IMLOGD0("[run] terminated thread");
            break;
        }
        uint16_t nPeriod = 0;
        uint8_t pbPayload[BUNDLE_DTMF_DATA_MAX];
        uint32_t nPayloadSize;
        uint32_t nTimestamp = ImsMediaTimer::GetTimeInMilliSeconds();
        if (mListDtmfDigit.size() == 0)
            continue;
        bool bMarker = true;
        nPayloadSize = MakeDTMFPayload(pbPayload, mListDtmfDigit.front(), false, mVolume, nPeriod);
        SendDataToRearNode(MEDIASUBTYPE_DTMFSTART, NULL, 0, 0, 0, 0);  // set dtmf mode true
        for (;;)
        {
            mMutex.lock();
            if (mStopDtmf)
            {
                // make and send DTMF last packet and retransmit it
                nPayloadSize =
                        MakeDTMFPayload(pbPayload, mListDtmfDigit.front(), true, mVolume, nPeriod);
                uint32_t nDTMFRetransmitDuration = mRetransmitDuration * (mAudioFrameDuration / 20);
                for (nPeriod = 0; nPeriod <= nDTMFRetransmitDuration;
                        nPeriod += mAudioFrameDuration)
                {
                    IMLOGD1("[run] send dtmf timestamp[%d]", nTimestamp);
                    SendDataToRearNode(MEDIASUBTYPE_DTMF_PAYLOAD, pbPayload, nPayloadSize,
                            nTimestamp, false, 0);
                    nTimestamp += 20;
                    uint32_t nCurrTime = ImsMediaTimer::GetTimeInMilliSeconds();
                    IMLOGD1("[run] nCurrTime[%u]", nCurrTime);
                    if (nTimestamp > nCurrTime)
                        ImsMediaTimer::Sleep(nTimestamp - nCurrTime);
                }
                SendDataToRearNode(MEDIASUBTYPE_DTMFEND, NULL, 0, 0, 0, 0);
                mListDtmfDigit.pop_front();
                mMutex.unlock();
                break;
            }
            mMutex.unlock();
            SendDataToRearNode(
                    MEDIASUBTYPE_DTMF_PAYLOAD, pbPayload, nPayloadSize, nTimestamp, bMarker, 0);
            nTimestamp += 20;
            uint32_t nCurrTime = ImsMediaTimer::GetTimeInMilliSeconds();
            IMLOGD1("[run] nCurrTime[%u]", nCurrTime);
            if (nTimestamp > nCurrTime)
                ImsMediaTimer::Sleep(nTimestamp - nCurrTime);
            bMarker = false;

            if (IsThreadStopped())
            {
                IMLOGD0("[run] terminated thread");
                break;
            }
        }
    }
    return NULL;
}

uint32_t DtmfEncoderNode::calculateDtmfDuration(uint32_t duration)
{
    if (duration < DTMF_MINIMUM_DURATION)
    {
        // Force minimum duration
        duration = DTMF_MINIMUM_DURATION;
    }

    return (((duration + 10) / 20) * (mAudioFrameDuration / 20));
}

bool DtmfEncoderNode::SendDTMFEvent(uint8_t digit, uint32_t duration)
{
    uint16_t nPeriod = 0;
    uint8_t pbPayload[BUNDLE_DTMF_DATA_MAX];
    uint32_t nPayloadSize;
    uint32_t nTimestamp = 0;
    bool bMarker = true;
    uint32_t nDTMFDuration = calculateDtmfDuration(duration);
    uint32_t nDTMFRetransmitDuration = mRetransmitDuration * (mAudioFrameDuration / 20);

#if 0
    // support bundling...
    int32_t  nFrameBundlingSize = 1;
    if (nFrameBundlingSize > 1) {
        uint32_t nBundlingDuration;
        uint8_t nRemainedTime;
        nBundlingDuration = nFrameBundlingSize * AUDIO_FRAME_DURATION_INTERVAL;
        nRemainedTime = nDTMFRetransmitDuration % nBundlingDuration;
        if (nRemainedTime > 0) {
            nDTMFRetransmitDuration += (nBundlingDuration - nRemainedTime);
        }
    }
#endif

    // make and send DTMF packet
    for (nPeriod = mAudioFrameDuration; nPeriod < nDTMFDuration; nPeriod += mAudioFrameDuration)
    {
        nPayloadSize = MakeDTMFPayload(pbPayload, digit, false, mVolume, nPeriod);
        SendDataToRearNode(
                MEDIASUBTYPE_DTMF_PAYLOAD, pbPayload, nPayloadSize, nTimestamp, bMarker, 0);
        nTimestamp += 20;
        bMarker = false;
    }

    // make and send DTMF last packet and retransmit it
    nPayloadSize = MakeDTMFPayload(pbPayload, digit, true, mVolume, nPeriod);

    for (nPeriod = 0; nPeriod <= nDTMFRetransmitDuration; nPeriod += mAudioFrameDuration)
    {
        SendDataToRearNode(
                MEDIASUBTYPE_DTMF_PAYLOAD, pbPayload, nPayloadSize, nTimestamp, false, 0);
        nTimestamp += 20;
    }

    return true;
}

bool DtmfEncoderNode::convertSignal(uint8_t digit, uint8_t& signal)
{
    IMLOGD1("[convertSignal] digit[%c]", digit);
    /* Set input signal */
    if ((digit >= '0') && (digit <= '9'))
        signal = digit - '0';
    else if (digit == '*')
        signal = 10;
    else if (digit == '#')
        signal = 11;
    else if (digit == 'A')
        signal = 12;
    else if (digit == 'B')
        signal = 13;
    else if (digit == 'C')
        signal = 14;
    else if (digit == 'D')
        signal = 15;
    else if (digit == 'F')
        signal = 16;
    else
    {
        IMLOGE1("[SendDTMF] Wrong DTMF Signal[%c]!", digit);
        return false;
    }
    return true;
}

uint32_t DtmfEncoderNode::MakeDTMFPayload(
        uint8_t* pbPayload, uint8_t digit, bool bEnd, uint8_t nVolume, uint16_t nPeriod)
{
    // Event: 8 bits
    pbPayload[0] = digit;

    // E: 1 bit
    if (bEnd)
    {
        pbPayload[1] = 0x80;
    }
    else
    {
        pbPayload[1] = 0x00;
    }

    // Volume: 6 bits
    pbPayload[1] += nVolume;

    // duration: 16 bits
    pbPayload[2] = (nPeriod >> 8) & 0xFF;
    pbPayload[3] = nPeriod & 0xFF;
    return 4;
}
