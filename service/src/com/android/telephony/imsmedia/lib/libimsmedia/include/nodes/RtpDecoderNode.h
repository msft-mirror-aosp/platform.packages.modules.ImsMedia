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
public:
    RtpDecoderNode(BaseSessionCallback* callback = NULL);
    virtual ~RtpDecoderNode();
    virtual kBaseNodeId GetNodeId();
    virtual ImsMediaResult Start();
    virtual void Stop();
    virtual bool IsRunTime();
    virtual bool IsSourceNode();
    virtual void SetConfig(void* config);
    virtual bool IsSameConfig(void* config);
    virtual void OnDataFromFrontNode(ImsMediaSubType subtype, uint8_t* pData, uint32_t nDataSize,
            uint32_t nTimestamp, bool bMark, uint32_t nSeqNum, ImsMediaSubType nDataType);
    virtual void OnMediaDataInd(unsigned char* pData, uint32_t nDataSize, uint32_t nTimestamp,
            bool bMark, uint16_t nSeqNum, uint32_t nPayloadType, uint32_t nSSRC, bool bExtension,
            uint16_t nExtensionData);
    // IRtpDecoderListener
    void OnNumReceivedPacket(uint32_t nNumRtpPacket);
    void SetLocalAddress(const RtpAddress address);
    void SetPeerAddress(const RtpAddress address);
    void SetInactivityTimerSec(const uint32_t time);

private:
    IRtpSession* mRtpSession;
    RtpAddress mLocalAddress;
    RtpAddress mPeerAddress;
    uint32_t mSamplingRate;
    int32_t mRtpPayloadTx;
    int32_t mRtpPayloadRx;
    int32_t mRtpDtmfPayload;
    int32_t mDtmfSamplingRate;
    int32_t mCvoValue;
    uint32_t mReceivingSSRC;
    uint32_t mInactivityTime;
    uint32_t mNoRtpTime;
};

#endif