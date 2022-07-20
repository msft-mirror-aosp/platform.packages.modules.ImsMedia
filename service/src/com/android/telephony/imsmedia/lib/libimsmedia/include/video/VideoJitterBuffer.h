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

#ifndef VIDEO_JITTER_BUFFER_H_INCLUDEED
#define VIDEO_JITTER_BUFFER_H_INCLUDEED

#include <ImsMediaDefine.h>
#include <BaseJitterBuffer.h>
#include <ImsMediaVideoUtil.h>
#include <mutex>
#include <list>

class VideoJitterBuffer : public BaseJitterBuffer
{
public:
    VideoJitterBuffer();
    virtual ~VideoJitterBuffer();
    /**
     * @brief Set the Jitter Buffer Size
     *
     * @param nInit initial size of jitter buffer in milliseconds unit
     * @param nMin minimum size of jitter buffer in milliseconds unit
     * @param nMax maximum size of jitter buffer in milliseconds unit
     */
    virtual void SetJitterBufferSize(uint32_t nInit, uint32_t nMin, uint32_t nMax);
    virtual uint32_t GetCount();
    virtual void Reset();
    virtual void Delete();
    virtual void Add(ImsMediaSubType subtype, uint8_t* pbBuffer, uint32_t nBufferSize,
            uint32_t nTimeStamp, bool bMark, uint32_t nSeqNum, ImsMediaSubType nDataType);
    virtual bool Get(ImsMediaSubType* pImsMediaSubType, uint8_t** ppData, uint32_t* pnDataSize,
            uint32_t* pnTimeStamp, bool* pbMark, uint32_t* pnSeqNum, uint32_t* pnChecker = NULL);
    void SetCodecType(uint32_t type);
    void SetFramerate(uint32_t framerate);
    void SetVideoDropPFrame(bool value);
    void SetRWT(uint32_t nRWT);

private:
    bool CheckHeader(uint8_t* pbBuffer);
    void CheckValidIDR(DataEntry* pIDREntry);
    void InitLostPktList();
    void RemoveLossPacket(uint16_t seqNum, bool bRemOldPkt = false);
    void CheckLossPacket(uint16_t seqNum, uint16_t nLastRecvPkt);
    bool CalcLostPacket(
            uint16_t nLostPkt, uint16_t* nSecondNACKPkt, uint16_t* nPLIPkt, bool* bPLIPkt);
    bool UpdateLostPktList(LostPktEntry* pTempEntry, uint16_t nLostPkt, uint16_t* nSecondNACKPkt,
            uint16_t* nPLIPkt, bool* bPLIPkt);
    void RequestNack(uint16_t nLossGap, uint16_t nFLP, uint16_t nSecondNACKPkt, bool bNACK = true);
    void RequestPicLostMSG(uint32_t eType);

private:
    std::mutex mMutex;
    uint32_t mCodecType;
    uint32_t mFramerate;
    uint32_t mFrameInterval;
    uint32_t mSavedFrameNum;
    uint32_t mMarkedFrameNum;
    uint32_t mLastPlayedTime;
    uint32_t mNumPlayedPacket;
    uint32_t mLastAddedTimestamp;
    uint32_t mLastAddedSeqNum;
    uint32_t mRWT;
    std::list<LostPktEntry*> mLostPktList;
    bool mDropPFrame;
    bool mIDRFrameRemoved;
    uint32_t mIDRCheckCnt;
    uint32_t mFIRTimeStamp;
};
#endif
