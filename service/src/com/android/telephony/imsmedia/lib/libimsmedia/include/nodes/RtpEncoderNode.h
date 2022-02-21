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

#ifndef RTP_ENCODER_NODE_H
#define RTP_ENCODER_NODE_H

#include <ImsMediaHal.h>
#include <BaseNode.h>
#include <IRtpSession.h>

class RtpEncoderNode : public BaseNode, public IRtpEncoderListener
{
private:
    RtpEncoderNode();
    virtual ~RtpEncoderNode();

public:
    static BaseNode* GetInstance();
    static void ReleaseInstance(BaseNode* pNode);
    //BaseNode methods
    virtual BaseNodeID GetNodeID();
    virtual ImsMediaResult Start();
    virtual void Stop();
    virtual void ProcessData();
    virtual bool IsRunTime();
    virtual bool IsSourceNode();
    virtual void SetConfig(void* config);
    virtual bool UpdateConfig(void* config);
    //IRtpEncoderListener method
    virtual void OnRtpPacket(unsigned char* pData, uint32_t nSize);
    void SetLocalAddress(const RtpAddress address);
    void SetPeerAddress(const RtpAddress address);

private:
    void ProcessAudioData(ImsMediaSubType eSubType, uint8_t* pData, uint32_t nDataSize,
        uint32_t nTimestamp);
    std::shared_ptr<ImsMediaHal::RtpSessionParams> mSessionParams;
    IRtpSession* mRtpSession;
    RtpAddress mLocalAddress;
    RtpAddress mPeerAddress;
    bool mDTMFMode;
    bool mAudioMark;
    uint32_t mPrevTimestamp;
    uint32_t mDTMFTimestamp;
    uint32_t mPeerPayload;
    uint32_t mDTMFPeerPayload;
    uint32_t mSamplingRate;
#ifdef DEBUG_JITTER_GEN_SIMULATION_DELAY
    uint32_t mNextTime;
#endif
#ifdef DEBUG_JITTER_GEN_SIMULATION_REORDER
    ImsMediaDataQueue jitterData;
    uint32_t mReorderDataCount;
#endif
};

#endif