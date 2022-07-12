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

#ifndef RTCPENCODERNODE_H_INCLUDED
#define RTCPENCODERNODE_H_INCLUDED

#include <BaseNode.h>
#include <IRtpSession.h>
#include <ImsMediaTimer.h>
#include <mutex>

#define BLOCK_LENGTH_STATISTICS   40
#define BLOCK_LENGTH_VOIP_METRICS 36

class RtcpEncoderNode : public BaseNode, public IRtcpEncoderListener
{
private:
    RtcpEncoderNode();
    ~RtcpEncoderNode();

public:
    static BaseNode* GetInstance();
    static void ReleaseInstance(BaseNode* pNode);
    static void OnTimer(hTimerHandler hTimer, void* pUserData);
    virtual BaseNodeID GetNodeID();
    virtual ImsMediaResult Start();
    virtual void Stop();
    virtual bool IsRunTime();
    virtual bool IsSourceNode();
    virtual void SetConfig(void* config);
    virtual bool IsSameConfig(void* config);
    virtual void OnRtcpPacket(unsigned char* pData, uint32_t wLen);
    void ProcessTimer();
    void SetLocalAddress(const RtpAddress address);
    void SetPeerAddress(const RtpAddress address);
    void SetRtcpInterval(const uint32_t interval);
    void SetRtcpXrBlockType(const uint32_t rtcpXrBlockType);
    void SetRtcpByeEnable(const bool bEnable);

private:
    IRtpSession* mRtpSession;
    RtpAddress mLocalAddress;
    RtpAddress mPeerAddress;
    uint32_t mRtcpInterval;
    uint8_t* mRtcpXrPayload;
    bool mEnableRtcpBye;
    uint32_t mRtcpXrBlockType;
    int32_t mRtcpXrCounter;
    hTimerHandler mTimer;
    std::mutex mMutexTimer;
};

#endif  // RTCPENCODERNODE_H_INCLUDED
