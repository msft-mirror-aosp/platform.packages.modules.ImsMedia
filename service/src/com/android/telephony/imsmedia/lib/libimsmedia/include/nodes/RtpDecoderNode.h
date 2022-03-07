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

#ifndef RTP_DECODER_NODE_H
#define RTP_DECODER_NODE_H

#include <BaseNode.h>
#include <IRtpSession.h>

/**
 * @brief      This class describes a rtp decoder.
 */
class RtpDecoderNode : public BaseNode, public IRtpDecoderListener
{
private:
    RtpDecoderNode();
    virtual ~RtpDecoderNode();

public:
    static BaseNode* GetInstance();
    static void ReleaseInstance(BaseNode* pNode);
    virtual BaseNodeID GetNodeID();
    virtual ImsMediaResult Start();
    virtual void Stop();
    virtual bool IsRunTime();
    virtual bool IsSourceNode();
    virtual void SetConfig(void* config);
    virtual bool IsSameConfig(void* config);
    virtual void OnDataFromFrontNode(ImsMediaSubType subtype, uint8_t* pData,
        uint32_t nDataSize, uint32_t nTimestamp, bool bMark, uint32_t nSeqNum,
        ImsMediaSubType nDataType);
    virtual void OnMediaDataInd(unsigned char* pData, uint32_t nDataSize,
        uint32_t nTimestamp, bool bMark, uint16_t nSeqNum,
        uint32_t nPayloadType, uint32_t nSSRC, bool bExtension, uint16_t nExtensionData);
    //IRtpDecoderListener
    void OnNumReceivedPacket(uint32_t nNumRtpPacket);
    void SetLocalAddress(const RtpAddress address);
    void SetPeerAddress(const RtpAddress address);
    void SetSamplingRate(const uint32_t data);
    void SetInactivityTimerSec(const uint32_t time);

private:
    IRtpSession* mRtpSession;
    RtpAddress mLocalAddress;
    RtpAddress mPeerAddress;
    uint32_t mSamplingRate;
    uint32_t mReceivingSSRC;
    uint32_t mInactivityTime;
    uint32_t mNoRtpTime;
};

#endif