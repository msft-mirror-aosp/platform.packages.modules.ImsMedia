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

#ifndef DTMFENCODERNODE_H_INCLUDED
#define DTMFENCODERNODE_H_INCLUDED

#include <ImsMediaDefine.h>
#include <BaseNode.h>
#include <IImsMediaThread.h>
#include <ImsMediaCondition.h>
#include <mutex>

class DtmfEncoderNode : public BaseNode, IImsMediaThread {
private:
    DtmfEncoderNode();
    ~DtmfEncoderNode();

public:
    static BaseNode* GetInstance();
    static void ReleaseInstance(BaseNode* pNode);
    virtual BaseNodeID GetNodeID();
    virtual ImsMediaResult Start();
    virtual void Stop();
    virtual bool IsRunTime();
    virtual bool IsSourceNode();
    virtual void SetConfig(void* config);
    virtual void OnDataFromFrontNode(ImsMediaSubType subtype,
        uint8_t* pData, uint32_t nDataSize, uint32_t nTimeStamp, bool bMark, uint32_t nSeqNum,
        ImsMediaSubType nDataType = MEDIASUBTYPE_UNDEFINED);
    void SetSamplingRate(uint32_t samplingrate);
    void SetDuration(uint32_t nDuration, uint32_t endBitDuration);
    void SetVolume(uint32_t nVolume);
    virtual void* run();
private:
    bool SendDTMFEvent(uint8_t digit, uint32_t duration);
    uint32_t calculateDtmfDuration(uint32_t duration);
    bool convertSignal(uint8_t digit, uint8_t& signal);
    uint32_t MakeDTMFPayload(uint8_t* pbPayload, uint8_t nEvent, bool bEnd,
        uint8_t nVolume, uint16_t nPeriod);
    ImsMediaCondition mCond;
    std::mutex mMutex;
    std::list<uint8_t> mListDtmfDigit;
    bool mStopDtmf;
    uint32_t mSamplingRate; //audio sampling rate
    uint32_t mDuration; // msec unit, duration of one DTMF tone
    uint32_t mRetransmitDuration; // msec unit, duration of retransmitting the last packet
    uint32_t mVolume;   // Volume of DTMF, 0~63, default value is 10.
    uint32_t mAudioFrameDuration;
};

#endif // DTMFENCODERNODE_H_INCLUDED
