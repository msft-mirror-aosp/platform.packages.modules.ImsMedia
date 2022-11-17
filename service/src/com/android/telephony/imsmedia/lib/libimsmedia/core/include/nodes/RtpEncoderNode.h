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

#include <BaseNode.h>
#include <IRtpSession.h>

class RtpEncoderNode : public BaseNode, public IRtpEncoderListener
{
public:
    RtpEncoderNode(BaseSessionCallback* callback = NULL);
    virtual ~RtpEncoderNode();
    virtual kBaseNodeId GetNodeId();
    virtual ImsMediaResult Start();
    virtual void Stop();
    virtual void ProcessData();
    virtual bool IsRunTime();
    virtual bool IsSourceNode();
    virtual void SetConfig(void* config);
    virtual bool IsSameConfig(void* config);
    // IRtpEncoderListener method
    virtual void OnRtpPacket(unsigned char* pData, uint32_t nSize);

    /**
     * @brief Set the local ip address and port number
     */
    void SetLocalAddress(const RtpAddress address);

    /**
     * @brief Set the peer ip address and port number
     */
    void SetPeerAddress(const RtpAddress address);

    /**
     * @brief Set the camera facing and device orientation parameter for cvo extension in rtp header
     */
    void SetCvoExtension(const int64_t facing, const int64_t orientation);

    /**
     * @brief Set the rtp header extension parameter
     */
    void SetRtpHeaderExtension(tRtpHeaderExtensionInfo& tExtension);

private:
    bool ProcessAudioData(
            ImsMediaSubType subtype, uint8_t* pData, uint32_t nDataSize, uint32_t timestamp);
    void ProcessVideoData(ImsMediaSubType subtype, uint8_t* pData, uint32_t nDataSize,
            uint32_t timestamp, bool mark);
    void ProcessTextData(ImsMediaSubType subtype, uint8_t* pData, uint32_t nDataSize,
            uint32_t timestamp, bool mark);
    IRtpSession* mRtpSession;
    RtpAddress mLocalAddress;
    RtpAddress mPeerAddress;
    bool mDTMFMode;
    bool mMark;
    uint32_t mPrevTimestamp;
    uint32_t mDTMFTimestamp;
    uint32_t mSamplingRate;
    int32_t mRtpPayloadTx;
    int32_t mRtpPayloadRx;
    int32_t mRtpTxDtmfPayload;
    int32_t mRtpRxDtmfPayload;
    int32_t mDtmfSamplingRate;
    int32_t mCvoValue;
    int32_t mRedundantPayload;
    int32_t mRedundantLevel;
    tRtpHeaderExtensionInfo mRtpExtension;
};

#endif
