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

#ifndef AUDIO_RTP_PAYLOAD_DECODER_NODE_H
#define AUDIO_RTP_PAYLOAD_DECODER_NODE_H

#include <ImsMediaDefine.h>
#include <BaseNode.h>
#include <ImsMediaBitReader.h>
#include <ImsMediaBitWriter.h>
#include <mutex>

class AudioRtpPayloadDecoderNode : public BaseNode
{
private:
    AudioRtpPayloadDecoderNode();
    virtual ~AudioRtpPayloadDecoderNode();
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
        uint8_t* pData, uint32_t nDataSize, uint32_t nTimestamp, bool bMark,
        uint32_t nSeqNum, ImsMediaSubType nDataType = ImsMediaSubType::MEDIASUBTYPE_UNDEFINED);
    void SetCodec(int32_t type);
    void SetPayloadMode(bool mode);

private:
    void Decode_PH_AMR(uint8_t* pData, uint32_t nDataSize, uint32_t nTimestamp,
        bool bMark, uint32_t nSeqNum);

private:
    int32_t mCodecType;
    bool mOctetAligned;
    uint8_t mPayload[MAX_AUDIO_PAYLOAD_SIZE];
    ImsMediaBitReader mBitReader;
    ImsMediaBitWriter mBitWriter;
    uint32_t mPrevCMR;
    std::mutex mMutexExit;
};

#endif