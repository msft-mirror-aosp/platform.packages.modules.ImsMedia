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

#ifndef RTCPDECODERNODE_H
#define RTCPDECODERNODE_H

#include <BaseNode.h>
#include <IRtpSession.h>

class RtcpDecoderNode : public BaseNode, public IRtcpDecoderListener
{
public:
    RtcpDecoderNode(BaseSessionCallback* callback = NULL);
    virtual ~RtcpDecoderNode();
    virtual kBaseNodeId GetNodeId();
    virtual ImsMediaResult Start();
    virtual void Stop();
    virtual void OnDataFromFrontNode(ImsMediaSubType subtype, uint8_t* pData, uint32_t nDataSize,
            uint32_t nTimeStamp, bool bMark, uint32_t nSeqNum,
            ImsMediaSubType nDataType = MEDIASUBTYPE_UNDEFINED);
    virtual bool IsRunTime();
    virtual bool IsSourceNode();
    void SetConfig(void* config);
    virtual bool IsSameConfig(void* config);
    virtual void OnRtcpInd(tRtpSvc_IndicationFromStack eIndType, void* pMsg);
    virtual void OnNumReceivedPacket(uint32_t nNumRTCPSRPacket, uint32_t nNumRTCPRRPacket);
    void SetLocalAddress(const RtpAddress address);
    void SetPeerAddress(const RtpAddress address);
    void SetInactivityTimerSec(const uint32_t time);

private:
    IRtpSession* mRtpSession;
    RtpAddress mLocalAddress;
    RtpAddress mPeerAddress;
    uint32_t mInactivityTime;
    uint32_t mNoRtcpTime;
};

#endif  // RTCPDECODERNODE_H
