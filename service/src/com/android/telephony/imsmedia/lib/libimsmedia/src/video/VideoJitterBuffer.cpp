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

#include <VideoJitterBuffer.h>
#include <ImsMediaDataQueue.h>
#include <ImsMediaTrace.h>
#include <ImsMediaVideoUtil.h>
#include <ImsMediaTimer.h>

#define DEFAULT_MAX_SAVE_FRAME_NUM                  5
#define DEFAULT_IDR_FRAME_CHECK_INTRERVAL           3
#define DEFAULT_VIDEO_JITTER_MAX_DELAY              300
#define DEFAULT_VIDEO_JITTER_IDR_WAIT_DELAY         200
#define CODECFILTER_AUDIO_FMC_MAX_SEQUENCE          0xffff
#define CODECFILTER_AUDIO_PACKETRECEIVE_COUNTER_MAX 50
#define CODECFILTER_AUDIO_PACKETINSERT_COUNTER_MAX  5
#define CODECFILTER_AUDIO_SKIP_READCOUNTER          100
#define RTCPNACK_SEQ_INCREASE(seq)                  (seq == 0xffff ? 0 : seq + 1)
#define RTCPNACK_SEQ_ROUND_COMPARE(a, b)            ((a > b) && (a > 0xfff0) && (b < 0x000f))
#define DEFAULT_BITRATE_MONITORING_TIME             1000

VideoJitterBuffer::VideoJitterBuffer() :
        BaseJitterBuffer()
{
    // base member valuable
    mCallback = NULL;
    mCodecType = kVideoCodecAvc;
    mInitJitterBufferSize = 0;
    mMinJitterBufferSize = 0;
    mMaxJitterBufferSize = 0;
    mNewInputData = false;
    mLastPlayedSeqNum = 0;
    mLastPlayedTimestamp = 0;

    mFramerate = 15;
    mFrameInterval = 1000 / mFramerate;
    mMaxSaveFrameNum = DEFAULT_MAX_SAVE_FRAME_NUM;
    mSavedFrameNum = 0;
    mMarkedFrameNum = 0;
    InitLostPktList();
    mResponseWaitTime = 0;
    mLastPlayedTime = 0;
    mNumAddedPacket = 0;
    mNumLossPacket = 0;
    mAccumulatedPacketSize = 0;
    mLastAddedTimestamp = 0;
    mLastAddedSeqNum = 0;
    mIDRCheckCnt = DEFAULT_IDR_FRAME_CHECK_INTRERVAL;
    mFirTimeStamp = 0;
    mIncomingBitrate = 0;
    mLossDuration = 0;
    mLossRateThreshold = 0;
    mCountTimerExpired = 0;
    mTimer = NULL;
}

VideoJitterBuffer::~VideoJitterBuffer()
{
    InitLostPktList();

    if (mTimer != NULL)
    {
        IMLOGD0("[~VideoJitterBuffer] stop timer");
        ImsMediaTimer::TimerStop(mTimer, NULL);
        mTimer = NULL;
    }
}

void VideoJitterBuffer::SetJitterBufferSize(uint32_t nInit, uint32_t nMin, uint32_t nMax)
{
    BaseJitterBuffer::SetJitterBufferSize(nInit, nMin, nMax);
    mMaxSaveFrameNum = mMaxJitterBufferSize * 20 / mFrameInterval;
    mIDRCheckCnt = DEFAULT_VIDEO_JITTER_IDR_WAIT_DELAY / mFrameInterval;
    mFirTimeStamp = 0;
    IMLOGD2("[SetJitterBufferSize] maxSaveFrameNum[%u], IDRCheckCnt[%d]", mMaxSaveFrameNum,
            mIDRCheckCnt);
}

void VideoJitterBuffer::SetCodecType(uint32_t type)
{
    mCodecType = type;
}

void VideoJitterBuffer::SetFramerate(uint32_t framerate)
{
    mFramerate = framerate;
    mFrameInterval = 1000 / mFramerate;
    IMLOGD2("[SetFramerate] framerate[%u], frameInterval[%d]", mFramerate, mFrameInterval);
}

void VideoJitterBuffer::InitLostPktList()
{
    IMLOGD0("[InitLostPktList]");

    while (mLostPktList.size())
    {
        LostPktEntry* entry = mLostPktList.front();

        if (entry != NULL)
        {
            delete entry;
        }

        mLostPktList.pop_front();
    }

    mLostPktList.clear();
}

void VideoJitterBuffer::Reset()
{
    BaseJitterBuffer::Reset();
    mSavedFrameNum = 0;
    mMarkedFrameNum = 0;
    mLastPlayedTime = 0;
    mNumAddedPacket = 0;
    mNumLossPacket = 0;
    mAccumulatedPacketSize = 0;
    mLastPlayedTimestamp = 0;
    mLastAddedTimestamp = 0;
    mLastAddedSeqNum = 0;
    InitLostPktList();
    mResponseWaitTime = 0;
}

void VideoJitterBuffer::StartTimer(uint32_t time, uint32_t rate)
{
    IMLOGD2("[SetRtpPacketLossRate] time[%d], rate[%u]", time, rate);
    mLossDuration = time;
    mLossRateThreshold = rate;

    if (mTimer == NULL)
    {
        IMLOGD0("[StartTimer] timer start");
        mTimer = ImsMediaTimer::TimerStart(DEFAULT_BITRATE_MONITORING_TIME, true, OnTimer, this);
    }
}

void VideoJitterBuffer::StopTimer()
{
    if (mTimer != NULL)
    {
        IMLOGD0("[StopTimer] stop timer");
        ImsMediaTimer::TimerStop(mTimer, NULL);
        mTimer = NULL;
    }
}

void VideoJitterBuffer::SetResponseWaitTime(const uint32_t time)
{
    IMLOGD1("[SetResponseWaitTime] time[%u]", time);
    mResponseWaitTime = time;
}

void VideoJitterBuffer::Add(ImsMediaSubType subtype, uint8_t* pbBuffer, uint32_t nBufferSize,
        uint32_t nTimestamp, bool bMark, uint32_t nSeqNum, ImsMediaSubType eDataType)
{
    DataEntry currEntry;
    memset(&currEntry, 0, sizeof(DataEntry));

    if (subtype == MEDIASUBTYPE_REFRESHED)
    {
        Reset();
        return;
    }

    currEntry.pbBuffer = pbBuffer;
    currEntry.nBufferSize = nBufferSize;
    currEntry.nTimestamp = nTimestamp;
    currEntry.bMark = bMark;
    currEntry.nSeqNum = nSeqNum;
    currEntry.bHeader = CheckHeader(pbBuffer);
    currEntry.bValid = false;
    currEntry.subtype = subtype;
    currEntry.eDataType = eDataType;

    std::lock_guard<std::mutex> guard(mMutex);

    if (currEntry.bHeader == true)
    {
        if (eDataType == MEDIASUBTYPE_VIDEO_CONFIGSTRING)
        {
            currEntry.bMark = true;
        }

        if (eDataType == MEDIASUBTYPE_VIDEO_SEI_FRAME)
        {
            IMLOGD1("[Add] ignore SEI frame seq[%d]", nSeqNum);
            return;
        }
    }

    IMLOGD_PACKET6(IM_PACKET_LOG_JITTER,
            "[Add] eDataType[%u], Seq[%u], Mark[%u], Header[%u], TS[%u], Size[%u]",
            currEntry.eDataType, nSeqNum, bMark, currEntry.bHeader, nTimestamp, nBufferSize);

    // very old frame, don't add this frame, nothing to do
    if ((!USHORT_SEQ_ROUND_COMPARE(nSeqNum, mLastPlayedSeqNum)) && (mLastPlayedTime != 0))
    {
        IMLOGE2("[Add] Receive very old frame!!! Drop Packet. Seq[%u], LastPlayedSeqNum[%u]",
                nSeqNum, mLastPlayedSeqNum);
    }
    else if (mDataQueue.GetCount() == 0)
    {  // jitter buffer is empty
        mDataQueue.Add(&currEntry);
        mNumAddedPacket++;
        mAccumulatedPacketSize += nBufferSize;
        IMLOGD_PACKET4(IM_PACKET_LOG_JITTER,
                "[Add] queue[%u] Seq[%u], LastPlayedSeqNum[%u], LastAddedTimestamp[%u]",
                mDataQueue.GetCount(), nSeqNum, mLastPlayedSeqNum, mLastAddedTimestamp);
        mLastAddedTimestamp = nTimestamp;
        mLastAddedSeqNum = nSeqNum;
    }
    else
    {
        DataEntry* pEntry;
        mDataQueue.GetLast(&pEntry);

        if (pEntry == NULL)
        {
            return;
        }

        if (USHORT_SEQ_ROUND_COMPARE(nSeqNum, pEntry->nSeqNum))
        {
            // current data is the latest data
            if (nSeqNum == pEntry->nSeqNum && nBufferSize == pEntry->nBufferSize)
            {
                IMLOGD1("[Add] drop duplicate Seq[%u]", nSeqNum);
                return;
            }

            mDataQueue.Add(&currEntry);
            mNumAddedPacket++;
            mAccumulatedPacketSize += nBufferSize;
            IMLOGD_PACKET4(IM_PACKET_LOG_JITTER,
                    "[Add] queue[%u] Seq[%u], LastPlayedSeqNum[%u], LastAddedTimestamp[%u]",
                    mDataQueue.GetCount(), nSeqNum, mLastPlayedSeqNum, mLastAddedTimestamp);
        }
        else
        {
            // find the position of current data and insert current data to the correct position
            uint32_t i;
            mDataQueue.SetReadPosFirst();

            for (i = 0; mDataQueue.GetNext(&pEntry); i++)
            {
                if (nSeqNum == pEntry->nSeqNum && nBufferSize == pEntry->nBufferSize)
                {
                    IMLOGD1("[Add] drop duplicate Seq[%u]", nSeqNum);
                    return;
                }

                if (!USHORT_SEQ_ROUND_COMPARE(nSeqNum, pEntry->nSeqNum))
                {
                    mDataQueue.InsertAt(i, &currEntry);
                    break;
                }
            }
        }

        // Remove mark of packets with same Timestamp and less SeqNum
        mDataQueue.SetReadPosFirst();

        while (mDataQueue.GetNext(&pEntry))
        {
            if (pEntry->nSeqNum != nSeqNum && pEntry->nTimestamp == nTimestamp &&
                    pEntry->bMark == true && pEntry->eDataType != MEDIASUBTYPE_VIDEO_CONFIGSTRING &&
                    USHORT_SEQ_ROUND_COMPARE(nSeqNum, pEntry->nSeqNum))
            {
                IMLOGD_PACKET6(IM_PACKET_LOG_JITTER,
                        "[Add] Remove marker of Seq/TS/Mark[%u/%u/%u], pEntry "
                        "Seq/TS/Mark[%u/%u/%u]",
                        pEntry->nSeqNum, pEntry->nTimestamp, pEntry->bMark, currEntry.nSeqNum,
                        currEntry.nTimestamp, currEntry.bMark);
                pEntry->bMark = false;
                continue;
            }
            else if (!USHORT_SEQ_ROUND_COMPARE(nSeqNum, pEntry->nSeqNum))
            {
                break;
            }
        }

        mLastAddedTimestamp = nTimestamp;
        mLastAddedSeqNum = nSeqNum;
    }

    mNewInputData = true;
}

bool VideoJitterBuffer::Get(ImsMediaSubType* subtype, uint8_t** ppData, uint32_t* pnDataSize,
        uint32_t* pnTimestamp, bool* pbMark, uint32_t* pnSeqNum, uint32_t* pnChecker)
{
    (void)pnChecker;

    DataEntry* pEntry = NULL;
    uint32_t nIndex = 0;
    uint32_t nHeaderIndex = 0;
    uint16_t nHeaderSeq = 0;
    uint64_t nHeaderTimestamp = -1;
    uint32_t nMarkIndex = 0;
    uint16_t nMarkSeq = 0;
    uint64_t nLastTimeStamp = -1;
    bool bValidPacket = false;
    bool bFoundHeader = false;
    uint32_t nSavedIdrFrame = 0;

    std::lock_guard<std::mutex> guard(mMutex);

    // check validation
    if (mNewInputData)
    {
        mSavedFrameNum = 0;
        mMarkedFrameNum = 0;
        bFoundHeader = false;
        uint16_t nLastRecvSeq = 0;  // for NACK generation
        mDataQueue.SetReadPosFirst();

        for (nIndex = 0; mDataQueue.GetNext(&pEntry); nIndex++)
        {
            IMLOGD_PACKET8(IM_PACKET_LOG_JITTER,
                    "[Get] queue[%u/%u] bValid[%u], Seq[%u], Mark[%u], Header[%u], TS[%u], "
                    "Size[%u]",
                    nIndex, mDataQueue.GetCount(), pEntry->bValid, pEntry->nSeqNum, pEntry->bMark,
                    pEntry->bHeader, pEntry->nTimestamp, pEntry->nBufferSize);

            if (mResponseWaitTime > 0 && nLastTimeStamp != (uint64_t)-1)
            {
                CheckPacketLoss(pEntry->nSeqNum, nLastRecvSeq);
            }

            nLastRecvSeq = pEntry->nSeqNum;

            if (pEntry->nTimestamp != nLastTimeStamp || nLastTimeStamp == (uint64_t)-1)
            {
                if (pEntry->eDataType != MEDIASUBTYPE_VIDEO_CONFIGSTRING)
                {
                    if (pEntry->eDataType == MEDIASUBTYPE_VIDEO_IDR_FRAME)
                    {
                        nSavedIdrFrame++;
                    }

                    mSavedFrameNum++;
                    nLastTimeStamp = pEntry->nTimestamp;
                }
            }

            if (pEntry->bMark)
            {
                mMarkedFrameNum++;
            }

            if (nSavedIdrFrame == mIDRCheckCnt && pEntry->eDataType == MEDIASUBTYPE_VIDEO_IDR_FRAME)
            {
                /** TODO: improve this logic later */
                CheckValidIDR(pEntry);
            }

            IMLOGD_PACKET3(IM_PACKET_LOG_JITTER,
                    "[Get] SavedFrameNum[%u], mMarkedFrameNum[%u], nLastTimeStamp[%u]",
                    mSavedFrameNum, mMarkedFrameNum, nLastTimeStamp);

            if (pEntry->bHeader)
            {
                if (pEntry->bMark)
                {
                    pEntry->bValid = true;

                    if (pEntry->nTimestamp > nHeaderTimestamp)
                    {
                        bFoundHeader = false;  // make Header false only for new frame
                    }
                }
                else
                {
                    // Check Header with new timestamp. Otherwise(duplicated header with
                    // same timestamp), do not update Header Information
                    if (bFoundHeader == false || pEntry->nTimestamp < nHeaderTimestamp ||
                            nHeaderTimestamp == (uint64_t)-1)
                    {
                        nHeaderTimestamp = pEntry->nTimestamp;
                        nHeaderIndex = nIndex;
                        nHeaderSeq = pEntry->nSeqNum;
                        bFoundHeader = true;

                        IMLOGD_PACKET3(IM_PACKET_LOG_JITTER,
                                "[Get] New Header Found at [%u] - Seq[%u], Timestamp[%u]",
                                nHeaderIndex, nHeaderSeq, nHeaderTimestamp);
                    }
                }
            }

            if (bFoundHeader)
            {
                IMLOGD_PACKET4(IM_PACKET_LOG_JITTER,
                        "[Get] bFoundHeader[%u] - Check Valid of Seq[%u ~ %u], "
                        "nHeaderTimestamp[%u]",
                        bFoundHeader, nHeaderSeq, pEntry->nSeqNum, nHeaderTimestamp);

                if (pEntry->bMark)
                {
                    nMarkIndex = nIndex;
                    nMarkSeq = pEntry->nSeqNum;

                    // make sure type of 16bit unsigned int sequence number
                    if (nMarkIndex - nHeaderIndex == nMarkSeq - nHeaderSeq)
                    {
                        uint32_t i;

                        for (i = nHeaderIndex; i <= nMarkIndex; i++)
                        {
                            DataEntry* pValidEntry;
                            mDataQueue.GetAt(i, &pValidEntry);

                            if (pValidEntry == NULL)
                            {
                                return false;
                            }

                            pValidEntry->bValid = true;

                            IMLOGD_PACKET3(IM_PACKET_LOG_JITTER,
                                    "[Get] Validation Check for Seq[%u] true :: nHeaderIndex[%u] "
                                    "to nMarkIndex[%u]",
                                    pValidEntry->nSeqNum, nHeaderIndex, nMarkIndex);
                        }
                    }

                    bFoundHeader = false;
                }
            }
        }

        if (mSavedFrameNum > mMaxSaveFrameNum)
        {
            IMLOGD_PACKET2(IM_PACKET_LOG_JITTER,
                    "[Get] Delete - SavedFrameNum[%u], nMaxFrameNum[%u]", mSavedFrameNum,
                    mMaxSaveFrameNum);

            mDataQueue.Get(&pEntry);

            if (pEntry == NULL)
                return false;

            if (pEntry->bValid == false)
            {
                uint32_t nDeleteTimeStamp = pEntry->nTimestamp;
                uint32_t nDeleteSeqNum = pEntry->nSeqNum;

                while (nDeleteTimeStamp == pEntry->nTimestamp)
                {
                    IMLOGD_PACKET7(IM_PACKET_LOG_JITTER,
                            "[Get] Delete - Seq[%u], Count[%u], bValid[%u], eDataType[%u], "
                            "bHeader[%u], TimeStamp[%u], Size[%u]",
                            pEntry->nSeqNum, mDataQueue.GetCount(), pEntry->bValid,
                            pEntry->eDataType, pEntry->bHeader, pEntry->nTimestamp,
                            pEntry->nBufferSize);

                    nDeleteSeqNum = pEntry->nSeqNum;
                    mDataQueue.Delete();
                    mDataQueue.Get(&pEntry);  // next packet

                    if (pEntry == NULL)
                    {
                        break;
                    }
                }

                mSavedFrameNum--;

                // remove the packets from NACK / PLI checkList
                if (mLostPktList.size() > 0)
                {
                    RemovePacketFromLostList(nDeleteSeqNum, true);
                }
            }
        }

        if (mSavedFrameNum >= mMaxSaveFrameNum)
        {
            mDataQueue.Get(&pEntry);

            if (pEntry == NULL)
            {
                return false;
            }

            mLastPlayedSeqNum = pEntry->nSeqNum - 1;
        }

        mNewInputData = false;
    }

    if (mSavedFrameNum >= (mMaxSaveFrameNum / 2) && mDataQueue.Get(&pEntry) == true &&
            pEntry->bValid && (mLastPlayedSeqNum == 0 || pEntry->nSeqNum <= mLastPlayedSeqNum + 1))
    {
        IMLOGD_PACKET4(IM_PACKET_LOG_JITTER,
                "[Get] bValid[%u], LastPlayedTS[%u], Seq[%u], LastPlayedSeq[%u]", pEntry->bValid,
                mLastPlayedTimestamp, pEntry->nSeqNum, mLastPlayedSeqNum);

        uint32_t nCurrTime = ImsMediaTimer::GetTimeInMilliSeconds();
        uint32_t nTimeDiff = 0;

        if (mLastPlayedTimestamp == 0 || mLastPlayedTime == 0)
        {
            bValidPacket = true;
        }
        else if (pEntry->nTimestamp == mLastPlayedTimestamp)
        {
            bValidPacket = true;
        }
        else
        {
            nTimeDiff = nCurrTime - mLastPlayedTime;
            uint32_t nThreshold;

            // this is optimized in 15fps
            if (mSavedFrameNum <= 2 && mMarkedFrameNum <= 1)
            {
                nThreshold = nTimeDiff;
            }
            else if (mSavedFrameNum <= 3)
            {
                nThreshold = nTimeDiff / 2;
            }
            else
            {
                nThreshold = nTimeDiff / ((mSavedFrameNum + 2) / 2);
            }

            if (nThreshold > 66)
            {
                nThreshold = 66;  // 15fps
            }

            if (nTimeDiff >= nThreshold || (mLastPlayedTimestamp > pEntry->nTimestamp) ||
                    (mLastPlayedTime > nCurrTime))
            {
                bValidPacket = true;
            }
            else
            {
                IMLOGD_PACKET3(IM_PACKET_LOG_JITTER,
                        "[Get] bValidPacket[%u], nTimeDiff[%u], nThreshold[%u]", bValidPacket,
                        nTimeDiff, nThreshold);

                bValidPacket = false;
            }
        }

        if (bValidPacket)
        {
            mLastPlayedTimestamp = pEntry->nTimestamp;
            mLastPlayedTime = nCurrTime;
        }

        mNewInputData = true;
    }
    else
    {
        bValidPacket = false;
    }

    if (bValidPacket && pEntry != NULL)
    {
        if (subtype)
            *subtype = pEntry->subtype;
        if (ppData)
            *ppData = pEntry->pbBuffer;
        if (pnDataSize)
            *pnDataSize = pEntry->nBufferSize;
        if (pnTimestamp)
            *pnTimestamp = pEntry->nTimestamp;
        if (pbMark)
            *pbMark = pEntry->bMark;
        if (pnSeqNum)
            *pnSeqNum = pEntry->nSeqNum;

        mLastPlayedSeqNum = pEntry->nSeqNum;

        IMLOGD_PACKET7(IM_PACKET_LOG_JITTER,
                "[Get] Seq[%u], Mark[%u], TS[%u], Size[%u], SavedFrame[%u], MarkedFrame[%u], "
                "queue[%u]",
                pEntry->nSeqNum, pEntry->bMark, pEntry->nTimestamp, pEntry->nBufferSize,
                mSavedFrameNum, mMarkedFrameNum, mDataQueue.GetCount());
        return true;
    }
    else
    {
        if (subtype)
            *subtype = MEDIASUBTYPE_UNDEFINED;
        if (ppData)
            *ppData = NULL;
        if (pnDataSize)
            *pnDataSize = 0;
        if (pnTimestamp)
            *pnTimestamp = 0;
        if (pbMark)
            *pbMark = false;
        if (pnSeqNum)
            *pnSeqNum = 0;
        IMLOGD_PACKET3(IM_PACKET_LOG_JITTER,
                "[Get] false - SavedFrame[%u], MarkedFrame[%u], queue[%u]", mSavedFrameNum,
                mMarkedFrameNum, mDataQueue.GetCount());
        return false;
    }
}

void VideoJitterBuffer::CheckValidIDR(DataEntry* pIDREntry)
{
    if (pIDREntry == NULL)
    {
        return;
    }

    if (pIDREntry->bValid == false)
    {
        IMLOGD2("[CheckValidIDR] mFirTimeStamp[%u] -> nTimestamp[%u]", mFirTimeStamp,
                pIDREntry->nTimestamp);

        if (pIDREntry->nTimestamp == mFirTimeStamp)
        {
            return;
        }

        RequestToSendPictureLost(kPsfbFir);
        mFirTimeStamp = pIDREntry->nTimestamp;
        return;
    }
}

void VideoJitterBuffer::Delete()
{
    DataEntry* pEntry;
    std::lock_guard<std::mutex> guard(mMutex);
    mDataQueue.Get(&pEntry);

    if (pEntry == NULL)
    {
        return;
    }

    IMLOGD_PACKET2(IM_PACKET_LOG_JITTER, "[Delete] Seq[%u] / BufferCount[%u]", pEntry->nSeqNum,
            mDataQueue.GetCount());
    mLastPlayedSeqNum = pEntry->nSeqNum;
    mDataQueue.Delete();
    mNewInputData = true;

    if (mLostPktList.size() > 0)
    {
        RemovePacketFromLostList(mLastPlayedSeqNum, true);
    }
}

uint32_t VideoJitterBuffer::GetCount()
{
    return mDataQueue.GetCount();
}

bool VideoJitterBuffer::CheckHeader(uint8_t* pbBuffer)
{
    bool ret = false;
    switch (mCodecType)
    {
        case kVideoCodecAvc:
            // check start code
            if ((pbBuffer[0] == 0x00 && pbBuffer[1] == 0x00 && pbBuffer[2] == 0x00) ||
                    (pbBuffer[0] == 0x00 && pbBuffer[1] == 0x00 && pbBuffer[2] == 0x01))
            {
                ret = true;
            }
            else
            {
                ret = false;
            }
            break;
        case kVideoCodecHevc: /* [HEVC] check H.265 start code */
            // check start code
            if ((pbBuffer[0] == 0x00 && pbBuffer[1] == 0x00 && pbBuffer[2] == 0x00) ||
                    (pbBuffer[0] == 0x00 && pbBuffer[1] == 0x00 && pbBuffer[2] == 0x01))
            {
                ret = true;
            }
            else
            {
                ret = false;
            }
            break;
        default:
            ret = true;
            IMLOGE1("[CheckHeader] Invalid video codec type %u", mCodecType);
    }

    IMLOGD_PACKET5(IM_PACKET_LOG_JITTER, "[CheckHeader] ret[%u] %02X %02X %02X %02X ", ret,
            pbBuffer[0], pbBuffer[1], pbBuffer[2], pbBuffer[3]);

    return ret;
}

void VideoJitterBuffer::RemovePacketFromLostList(uint16_t seqNum, bool bRemoveOldPacket)
{
    LostPktEntry* pEntry = NULL;
    std::list<LostPktEntry*>::iterator it = mLostPktList.begin();

    while (it != mLostPktList.end())
    {
        pEntry = *it;

        if (pEntry == NULL)
        {
            mLostPktList.erase(it++);
            continue;
        }

        if (bRemoveOldPacket && pEntry->nSeqNum < seqNum)
        {
            IMLOGD_PACKET3(IM_PACKET_LOG_JITTER,
                    "[RemovePacketFromLostList] delete lost seq[%u], target seq[%u], "
                    "bRemoveOldPacket[%u]",
                    pEntry->nSeqNum, seqNum, bRemoveOldPacket);

            it = mLostPktList.erase(it);
            delete pEntry;
        }
        else if (pEntry->nSeqNum == seqNum)
        {
            IMLOGD_PACKET1(IM_PACKET_LOG_JITTER, "[RemovePacketFromLostList] remove lost seq[%u]",
                    pEntry->nSeqNum);

            it = mLostPktList.erase(it);
            delete pEntry;
        }
        else
        {
            it++;
        }
    }
}

void VideoJitterBuffer::CheckPacketLoss(uint16_t seqNum, uint16_t nLastRecvPkt)
{
    if (mLostPktList.size() > 0)
    {
        RemovePacketFromLostList(seqNum);
    }

    // normal case : no packet loss
    if (RTCPNACK_SEQ_INCREASE(nLastRecvPkt) == seqNum)
    {
        return;
    }

    if (nLastRecvPkt == seqNum)
    {
        return;  // same packet should be removed in STAP-A type case.
    }

    // the first lost packet.
    uint16_t PID = RTCPNACK_SEQ_INCREASE(nLastRecvPkt);

    // the number of lost packets
    uint16_t nLossGap = RTCPNACK_SEQ_ROUND_COMPARE(nLastRecvPkt, seqNum)
            ? ((0xffff - nLastRecvPkt) + seqNum)
            : (seqNum - PID);

    if (nLossGap > 0x000f)
    {
        nLossGap = 0x000f;
    }

    uint16_t countNackPacketNum = 0;
    uint16_t countSecondNack = 0;
    uint16_t nPLIPkt = 0;
    bool bPLIPkt = false;
    bool bSentPLI = false;

    for (uint32_t index = 0; index < nLossGap; index++)
    {
        if (UpdateLostPacketList(PID + index, &countSecondNack, &nPLIPkt, &bPLIPkt))
        {
            countNackPacketNum++;
        }

        // request PLI Message
        if (bPLIPkt && !bSentPLI)
        {
            IMLOGD1("[CheckPacketLoss] nPLI pkt[%u]", nPLIPkt);
            RequestToSendPictureLost(kPsfbPli);
            bSentPLI = true;
        }
    }

    if (nPLIPkt > PID)
    {
        PID = nPLIPkt + 1;
    }

    // return if PID is same as the current seq.
    if (PID == seqNum)
    {
        return;
    }

    // request NACK Message
    if (countNackPacketNum > 0)
    {
        RequestSendNack(countNackPacketNum, PID, countSecondNack);
    }
}

bool VideoJitterBuffer::UpdateLostPacketList(
        uint16_t lostSeq, uint16_t* countSecondNack, uint16_t* nPLIPkt, bool* bPLIPkt)
{
    for (auto& pLostPktEntry : mLostPktList)
    {
        if (pLostPktEntry != NULL && pLostPktEntry->nSeqNum == lostSeq)
        {
            return UpdateNackStatus(pLostPktEntry, lostSeq, countSecondNack, nPLIPkt, bPLIPkt);
        }
    }

    IMLOGD_PACKET2(IM_PACKET_LOG_JITTER, "[UpdateLostPacketList] add lost seq[%u], queue size[%d]",
            lostSeq, mLostPktList.size());

    LostPktEntry* pLostPktEntry =
            new LostPktEntry(lostSeq, ImsMediaTimer::GetTimeInMilliSeconds(), kRequestSendNackNone);
    mLostPktList.push_back(pLostPktEntry);
    mNumLossPacket++;
    return false;
}

bool VideoJitterBuffer::UpdateNackStatus(LostPktEntry* pEntry, uint16_t lostSeq,
        uint16_t* countSecondNack, uint16_t* nPLIPkt, bool* bPLIPkt)
{
    /**
     * Send initial NACK if there is error in decoding frame due to packet loss
     */
    if (pEntry->nNACKReqType == kRequestSendNackNone)
    {
        if ((ImsMediaTimer::GetTimeInMilliSeconds() - pEntry->nReqTime) < mFrameInterval)
        {
            return false;
        }

        pEntry->nReqTime = ImsMediaTimer::GetTimeInMilliSeconds();
        pEntry->nNACKReqType = kRequestInitialNack;
        IMLOGD_PACKET1(IM_PACKET_LOG_JITTER, "[UpdateNackStatus] initial NACK, seq[%u]", lostSeq);
        return true;
    }

    if ((ImsMediaTimer::GetTimeInMilliSeconds() - pEntry->nReqTime) < mResponseWaitTime)
    {
        return false;
    }

    /**
     * Send Second NACK if there is first packet still not arrived within RWT duration
     */
    if (pEntry->nNACKReqType == kRequestInitialNack)
    {
        (*countSecondNack)++;
        pEntry->nReqTime = ImsMediaTimer::GetTimeInMilliSeconds();
        pEntry->nNACKReqType = kRequestSecondNack;
        IMLOGD_PACKET1(IM_PACKET_LOG_JITTER, "[UpdateNackStatus] second NACK, seq[%u]", lostSeq);
        return true;
    }
    else if (pEntry->nNACKReqType == kRequestSecondNack)
    {
        /**
         * Send PLI if the recovery picture does not arrived within two RWT duration
         */
        *nPLIPkt = lostSeq;
        *bPLIPkt = true;
        pEntry->nReqTime = ImsMediaTimer::GetTimeInMilliSeconds();
        pEntry->nNACKReqType = kRequestPli;
        IMLOGD_PACKET1(IM_PACKET_LOG_JITTER, "[UpdateNackStatus] request PLI seq[%u]", lostSeq);
    }
    else if (pEntry->nNACKReqType == kRequestPli)
    {
        *nPLIPkt = lostSeq;
        pEntry->nReqTime = ImsMediaTimer::GetTimeInMilliSeconds();
    }

    return false;
}

void VideoJitterBuffer::RequestSendNack(
        uint16_t nLossGap, uint16_t PID, uint16_t countSecondNack, bool bNACK)
{
    uint32_t BLP = 0x0;  // bitmask of following lost packets

    if (nLossGap > 1)
    {
        BLP = (0x01 << (nLossGap - 1)) - 1;
    }

    InternalRequestEventParam* pParam = new InternalRequestEventParam(
            kRequestVideoSendNack, NackParams(PID, BLP, countSecondNack, bNACK));

    IMLOGD0("[RequestSendNack]");
    mCallback->SendEvent(kRequestVideoSendNack, reinterpret_cast<uint64_t>(pParam));
}

void VideoJitterBuffer::RequestToSendPictureLost(uint32_t type)
{
    IMLOGD0("[RequestToSendPictureLost]");
    InternalRequestEventParam* pParam =
            new InternalRequestEventParam(kRequestVideoSendPictureLost, type);
    mCallback->SendEvent(kRequestVideoSendPictureLost, reinterpret_cast<uint64_t>(pParam));
}

void VideoJitterBuffer::OnTimer(hTimerHandler hTimer, void* pUserData)
{
    (void)hTimer;
    VideoJitterBuffer* jitter = reinterpret_cast<VideoJitterBuffer*>(pUserData);

    if (jitter != NULL)
    {
        jitter->ProcessTimer();
    }
}

void VideoJitterBuffer::ProcessTimer()
{
    mCountTimerExpired++;

    /** calculate loss rate */
    if (mLossDuration != 0 &&
            mCountTimerExpired >= (mLossDuration / DEFAULT_BITRATE_MONITORING_TIME))
    {
        double lossRate = mNumLossPacket * 100 / (mNumAddedPacket + mNumLossPacket);

        IMLOGD3("[ProcessTimer] rate[%lf], lossPackets[%d], addedPackets[%d]", lossRate,
                mNumLossPacket, mNumAddedPacket);

        if (lossRate >= mLossRateThreshold)
        {
            /** TODO: request send TMMBR */
            /** TODO: request send loss indication event */
        }

        mNumLossPacket = 0;
        mNumAddedPacket = 0;
        mCountTimerExpired = 0;
    }

    /** calculate bitrate */
    mIncomingBitrate = mAccumulatedPacketSize * 8;

    IMLOGD_PACKET1(IM_PACKET_LOG_JITTER, "[ProcessTimer] IncomingBitrate[%d]", mIncomingBitrate);
    mAccumulatedPacketSize = 0;
}