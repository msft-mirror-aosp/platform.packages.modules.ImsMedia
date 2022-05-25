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

VideoJitterBuffer::VideoJitterBuffer() :
        BaseJitterBuffer()
{
    mCodecType = kVideoCodecAvc;
    mFramerate = 15;
    mFrameInterval = 1000 / mFramerate;
    mMaxSaveFrameNum = DEFAULT_MAX_SAVE_FRAME_NUM;
    mSavedFrameNum = 0;
    mMarkedFrameNum = 0;
    // for NACk
    InitLostPktList();
    mRWT = 0;
    mLastPlayedTime = 0;
    mLastPlayedSeqNum = 0;
    mLastAddedTimestamp = 0;
    mLastAddedSeqNum = 0;
    mDropPFrame = false;
    mIDRFrameRemoved = false;
    mIDRCheckCnt = DEFAULT_IDR_FRAME_CHECK_INTRERVAL;
    mFIRTimeStamp = 0;
}

VideoJitterBuffer::~VideoJitterBuffer()
{
    InitLostPktList();
}

void VideoJitterBuffer::SetJitterBufferSize(uint32_t nInit, uint32_t nMin, uint32_t nMax)
{
    BaseJitterBuffer::SetJitterBufferSize(nInit, nMin, nMax);
    mMaxSaveFrameNum = mMaxJitterBufferSize * 20 / mFrameInterval;
    mIDRCheckCnt = DEFAULT_VIDEO_JITTER_IDR_WAIT_DELAY / mFrameInterval;
    mFIRTimeStamp = 0;
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
    mNumPlayedPacket = 0;
    mLastPlayedTimestamp = 0;
    mLastAddedTimestamp = 0;
    mLastAddedSeqNum = 0;
    InitLostPktList();
    mRWT = 0;
    mIDRFrameRemoved = false;
}

void VideoJitterBuffer::SetVideoDropPFrame(bool value)
{
    IMLOGD1("[SetVideoDropPFrame] %u", value);
    mDropPFrame = value;
}

void VideoJitterBuffer::SetRWT(uint32_t nRWT)
{
    IMLOGD2("[SetRWT] RWT[%u] -> RWT[%u]", mRWT, nRWT);
    mRWT = nRWT;
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
    std::lock_guard<std::mutex> guard(mMutex);
    uint32_t nSavedIdrFrame = 0;

    // check validation
    if (mNewInputData)
    {
        mSavedFrameNum = 0;
        mMarkedFrameNum = 0;
        bFoundHeader = false;
        uint16_t nLastRecvSeq = 0;  // for NACK Message
        mDataQueue.SetReadPosFirst();

        for (nIndex = 0; mDataQueue.GetNext(&pEntry); nIndex++)
        {
            IMLOGD_PACKET8(IM_PACKET_LOG_JITTER,
                    "[Get] queue[%u/%u] bValid[%u], Seq[%u], Mark[%u], Header[%u], TS[%u], "
                    "Size[%u]",
                    nIndex, mDataQueue.GetCount(), pEntry->bValid, pEntry->nSeqNum, pEntry->bMark,
                    pEntry->bHeader, pEntry->nTimestamp, pEntry->nBufferSize);

            // TODO: Check NACK / PLI MSG
            if (mRWT > 0 && nLastTimeStamp != (uint64_t)-1)
            {
                // CheckLossPacket(pEntry->nSeqNum, nLastRecvSeq);
            }

            nLastRecvSeq = pEntry->nSeqNum;
            // End Check NACK / PLI MSG

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
                // CheckValidIDR(pEntry);
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
                if (mDropPFrame == true)
                {
                    mIDRFrameRemoved = true;
                }

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
                        break;
                }

                mSavedFrameNum--;

                // remove the packets from NACK / PLI checkList
                if (mLostPktList.size() > 0)
                {
                    // RemoveLossPacket(nDeleteSeqNum, true);
                }
            }

            /*
            // Send PLI
            if (bFrameDeleted == true) {
                RequestPicLostMSG(kPsfbPli);
            }
            */
        }

        if (mSavedFrameNum >= mMaxSaveFrameNum)
        {
            mDataQueue.Get(&pEntry);

            if (pEntry == NULL)
                return false;

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
            // uint32_t nTSDiff = pEntry->nTimestamp - mLastPlayedTimestamp;
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
                nThreshold = 66;  // 15fps

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
        if (pEntry->eDataType == MEDIASUBTYPE_VIDEO_IDR_FRAME)
        {
            mIDRFrameRemoved = false;
        }

        // remove non-idr frame until new idr frame is arrived
        if (mIDRFrameRemoved == true && pEntry->eDataType == MEDIASUBTYPE_VIDEO_NON_IDR_FRAME)
        {
            uint32_t nDeleteTimeStamp = pEntry->nTimestamp;
            uint32_t nDeleteSeqNum = pEntry->nSeqNum;

            while (nDeleteTimeStamp == pEntry->nTimestamp)
            {
                IMLOGD_PACKET1(
                        IM_PACKET_LOG_JITTER, "[Get] Delete - nonIDR, Seq[%u]", pEntry->nSeqNum);
                nDeleteSeqNum = pEntry->nSeqNum;
                mDataQueue.Delete();
                mDataQueue.Get(&pEntry);
                if (pEntry == NULL)
                    break;
            }

            // TODO : remove the packets from NACK / PLI checkList
            if (mLostPktList.size() > 0)
            {
                // RemoveLossPacket(nDeleteSeqNum, true);
            }

            mSavedFrameNum--;
            return false;
        }

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

        if (mNumPlayedPacket != 0 && pEntry->nSeqNum - mLastPlayedSeqNum > 1)
        {
            IMLOGD2("[Get] Lost Seq[%u], Count[%u]", mLastPlayedSeqNum + 1,
                    pEntry->nSeqNum - mLastPlayedSeqNum - 1);
        }

        mLastPlayedSeqNum = pEntry->nSeqNum;

        IMLOGD_PACKET7(IM_PACKET_LOG_JITTER,
                "[Get] Seq[%u], Mark[%u], TS[%u], Size[%u], SavedFrame[%u], MarkedFrame[%u], "
                "queue[%u]",
                pEntry->nSeqNum, pEntry->bMark, pEntry->nTimestamp, pEntry->nBufferSize,
                mSavedFrameNum, mMarkedFrameNum, mDataQueue.GetCount());

        mNumPlayedPacket++;
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
        return;
    if (pIDREntry->bValid == false)
    {
        IMLOGD2("[CheckValidIDR] mFIRTimeStamp[%u] -> nTimestamp[%u]", mFIRTimeStamp,
                pIDREntry->nTimestamp);

        if (pIDREntry->nTimestamp == mFIRTimeStamp)
            return;

        RequestPicLostMSG(kPsfbFir);
        mFIRTimeStamp = pIDREntry->nTimestamp;
        return;
    }
}

void VideoJitterBuffer::Delete()
{
    DataEntry* pEntry;
    std::lock_guard<std::mutex> guard(mMutex);
    mDataQueue.Get(&pEntry);
    if (pEntry == NULL)
        return;
    IMLOGD_PACKET2(IM_PACKET_LOG_JITTER, "[Delete] Seq[%u] / BufferCount[%u]", pEntry->nSeqNum,
            mDataQueue.GetCount());
    mLastPlayedSeqNum = pEntry->nSeqNum;
    mDataQueue.Delete();
    mNewInputData = true;
    // TODO: remove the packets from NACK / PLI checkList
    if (mLostPktList.size() > 0)
    {
        // RemoveLossPacket(mLastPlayedSeqNum, true);
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

void VideoJitterBuffer::RemoveLossPacket(uint16_t nCurrentPkt, bool bRemoveOldPacket)
{
    LostPktEntry* pEntry = NULL;
    while (mLostPktList.size())
    {
        pEntry = mLostPktList.back();
        if (pEntry->nLostPktSeqNum == nCurrentPkt)
        {
            IMLOGD_PACKET1(IM_PACKET_LOG_JITTER, "[RemoveLossPacket] delete Seq[%u]",
                    pEntry->nLostPktSeqNum);
            delete pEntry;
            mLostPktList.pop_back();
            break;
        }
    }

    if (bRemoveOldPacket == false)
        return;

    uint16_t nLostPkt = 0;
    for (std::list<LostPktEntry*>::iterator it = mLostPktList.begin(); it != mLostPktList.end();
            ++it)
    {
        pEntry = *it;
        if (RTCPNACK_SEQ_ROUND_COMPARE(nCurrentPkt, pEntry->nLostPktSeqNum))
            continue;
        if (pEntry->nLostPktSeqNum <= nCurrentPkt)
        {
            IMLOGD_PACKET3(IM_PACKET_LOG_JITTER,
                    "[RemoveLossPacket] delete Seq[%u], nCurrentPkt[%u], bRemoveOldPacket[%u]",
                    pEntry->nLostPktSeqNum, nCurrentPkt, bRemoveOldPacket);

            if (pEntry->nNACKReqType == kRequestNackNone)
                nLostPkt++;
            mLostPktList.erase(it);
        }
    }

    if (nLostPkt > 0)
    {
        RequestNack(nLostPkt, nCurrentPkt, 0, false);
    }
}

void VideoJitterBuffer::CheckLossPacket(uint16_t nSeqPkt, uint16_t nLastRecvPkt)
{
    if (mLostPktList.size() > 0)
    {
        RemoveLossPacket(nSeqPkt);
    }

    // 1. normal case : no packet loss
    if (RTCPNACK_SEQ_INCREASE(nLastRecvPkt) == nSeqPkt)
        return;
    if (nLastRecvPkt == nSeqPkt)
        return;  // same packet should be removed in STAP-A type case.

    // 2. the first lost packet.
    uint16_t nFLP = RTCPNACK_SEQ_INCREASE(nLastRecvPkt);

    // 3. the number of lost packets
    uint16_t nLossGap = RTCPNACK_SEQ_ROUND_COMPARE(nLastRecvPkt, nSeqPkt)
            ? ((0xffff - nLastRecvPkt) + nSeqPkt)
            : (nSeqPkt - nFLP);

    if (nLossGap > 0x000f)
        nLossGap = 0x000f;

    uint16_t nNACKPkt = 0;
    uint16_t nSecondNACKPkt = 0;
    uint16_t nPLIPkt = 0;
    bool bPLIPkt = false;
    bool bSentPLI = false;

    // 4. calc. NACK / PLI packets.
    for (uint32_t index = 0; index < nLossGap; index++)
    {
        if (CalcLostPacket(nFLP + index, &nSecondNACKPkt, &nPLIPkt, &bPLIPkt))
            nNACKPkt++;
        // 5. request PLI Message
        if (bPLIPkt && !bSentPLI)
        {
            IMLOGD1("[CheckLossPacket] nPLI pkt[%u]", nPLIPkt);
            RequestPicLostMSG(kPsfbPli);
            bSentPLI = true;
        }
    }

    if (nPLIPkt > nFLP)
    {
        nFLP = nPLIPkt + 1;
    }

    // 6. return if nFLP is same as the current seq.
    if (nFLP == nSeqPkt)
        return;

    // 7. request NACK Message
    if (nNACKPkt > 0)
    {
        RequestNack(nNACKPkt, nFLP, nSecondNACKPkt);
    }
}

bool VideoJitterBuffer::CalcLostPacket(
        uint16_t nLostPkt, uint16_t* nSecondNACKPkt, uint16_t* nPLIPkt, bool* bPLIPkt)
{
    for (auto& i : mLostPktList)
    {
        if (i->nLostPktSeqNum == nLostPkt)
        {
            return UpdateLostPktList(i, nLostPkt, nSecondNACKPkt, nPLIPkt, bPLIPkt);
        }
    }

    LostPktEntry* pLostPktEntry =
            new LostPktEntry(nLostPkt, ImsMediaTimer::GetTimeInMilliSeconds(), kRequestNackNone);
    mLostPktList.push_back(pLostPktEntry);
    return false;
}

bool VideoJitterBuffer::UpdateLostPktList(LostPktEntry* pEntry, uint16_t nLostPkt,
        uint16_t* nSecondNACKPkt, uint16_t* nPLIPkt, bool* bPLIPkt)
{
    if (pEntry->nNACKReqType == kRequestNackNone)
    {
        if ((ImsMediaTimer::GetTimeInMilliSeconds() - pEntry->nReqTime) < mFrameInterval)
        {
            return false;
        }
        pEntry->nReqTime = ImsMediaTimer::GetTimeInMilliSeconds();
        pEntry->nNACKReqType = kRequestInitialNack;
        IMLOGD_PACKET1(
                IM_PACKET_LOG_JITTER, "[UpdateLostPktList] kRequestInitialNack seq[%u]", nLostPkt);
        return true;
    }

    if ((ImsMediaTimer::GetTimeInMilliSeconds() - pEntry->nReqTime) < mRWT)
        return false;
    if (pEntry->nNACKReqType == kRequestInitialNack)
    {
        (*nSecondNACKPkt)++;
        pEntry->nReqTime = ImsMediaTimer::GetTimeInMilliSeconds();
        pEntry->nNACKReqType = kRequestSecondNack;
        IMLOGD_PACKET1(
                IM_PACKET_LOG_JITTER, "[UpdateLostPktList] kRequestSecondNack seq[%u]", nLostPkt);
        return true;
    }
    else if (pEntry->nNACKReqType == kRequestSecondNack)
    {
        *nPLIPkt = nLostPkt;
        *bPLIPkt = true;
        pEntry->nReqTime = ImsMediaTimer::GetTimeInMilliSeconds();
        pEntry->nNACKReqType = kRequestPli;
        IMLOGD_PACKET1(IM_PACKET_LOG_JITTER, "[UpdateLostPktList] kRequestPli seq[%u]", nLostPkt);
    }
    else if (pEntry->nNACKReqType == kRequestPli)
    {
        *nPLIPkt = nLostPkt;
        pEntry->nReqTime = ImsMediaTimer::GetTimeInMilliSeconds();
    }
    return false;
}

void VideoJitterBuffer::RequestNack(
        uint16_t nLossGap, uint16_t nFLP, uint16_t nSecondNACKPkt, bool bNACK)
{
    uint32_t nBLP = 0x0;  // bitmask of following lost packets
    if (nLossGap > 1)
        nBLP = (0x01 << (nLossGap - 1)) - 1;
    InternalRequestEventParam* pParam = new InternalRequestEventParam(
            kRequestVideoSendNack, NackParams(nFLP, nBLP, nSecondNACKPkt, bNACK));

    IMLOGD0("[RequestNack]");
    mCallback->SendEvent(kRequestVideoSendNack, reinterpret_cast<uint64_t>(pParam));
}

void VideoJitterBuffer::RequestPicLostMSG(uint32_t type)
{
    IMLOGD0("[RequestPicLostMSG]");
    InternalRequestEventParam* pParam = new InternalRequestEventParam(kRequestVideoIdrFrame, type);
    mCallback->SendEvent(kRequestVideoIdrFrame, reinterpret_cast<uint64_t>(pParam));
}
