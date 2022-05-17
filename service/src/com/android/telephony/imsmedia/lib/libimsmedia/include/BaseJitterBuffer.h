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

#ifndef BASE_JITTER_BUFFER_H
#define BASE_JITTER_BUFFER_H

#include <ImsMediaDataQueue.h>
#include <mutex>

#define SEQ_ROUND_QUARD 655  // 1% of FFFF
#define USHORT_SEQ_ROUND_COMPARE(a, b)                                              \
    (((a >= b) && ((b >= SEQ_ROUND_QUARD) || ((a <= 0xffff - SEQ_ROUND_QUARD)))) || \
            ((a <= SEQ_ROUND_QUARD) && (b >= 0xffff - SEQ_ROUND_QUARD)))
#define TS_ROUND_QUARD 3000
#define USHORT_TS_ROUND_COMPARE(a, b)                                             \
    (((a >= b) && ((b >= TS_ROUND_QUARD) || ((a <= 0xffff - TS_ROUND_QUARD)))) || \
            ((a <= TS_ROUND_QUARD) && (b >= 0xffff - TS_ROUND_QUARD)))

/*!
 *    @class        BaseJitterBuffer
 */
class BaseJitterBuffer
{
public:
    BaseJitterBuffer();
    virtual ~BaseJitterBuffer();
    virtual void SetCodecType(uint32_t type);
    virtual void SetJitterBufferSize(uint32_t nInit, uint32_t nMin, uint32_t nMax);
    virtual void SetJitterOptions(uint32_t nReduceTH, uint32_t nStepSize, double zValue,
            bool bIgnoreSID, bool bImprovement) = 0;
    virtual uint32_t GetCount();
    virtual void Reset();
    virtual void Delete();
    virtual void Add(ImsMediaSubType subtype, uint8_t* pbBuffer, uint32_t nBufferSize,
            uint32_t nTimestamp, bool bMark, uint32_t nSeqNum,
            ImsMediaSubType nDataType = ImsMediaSubType::MEDIASUBTYPE_UNDEFINED) = 0;
    virtual bool Get(ImsMediaSubType* psubtype, uint8_t** ppData, uint32_t* pnDataSize,
            uint32_t* pnTimestamp, bool* pbMark, uint32_t* pnSeqNum,
            uint32_t* pnChecker = NULL) = 0;

private:
    BaseJitterBuffer(const BaseJitterBuffer& objRHS);
    BaseJitterBuffer& operator=(const BaseJitterBuffer& objRHS);

protected:
    uint32_t mCodecType;
    ImsMediaDataQueue mDataQueue;
    std::mutex mMutex;
    uint32_t mInitJitterBufferSize;
    uint32_t mMinJitterBufferSize;
    uint32_t mMaxJitterBufferSize;
    bool mNewInputData;
    uint32_t mDataCount;
    uint16_t mLastPlayedSeqNum;
    uint32_t mLastPlayedTimestamp;
    uint32_t mMaxSavePacketNum;
    const void* mGraph;
    uint64_t mNumOfHandedPacket;
};

#endif