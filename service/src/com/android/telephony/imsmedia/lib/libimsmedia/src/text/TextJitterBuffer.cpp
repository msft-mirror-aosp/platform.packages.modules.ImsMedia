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

#include <TextJitterBuffer.h>
#include <ImsMediaTrace.h>

/** Maximum Jitter Buffer size to save in frame unit */
#define TEXT_JITTER_MAX_SAVEPACKET_NUM 3
/** Maximum Jitter Buffer size to save redundant packets*/
#define TEXT_JITTER_MAX_WAITING_TIME   1000

TextJitterBuffer::TextJitterBuffer() :
        BaseJitterBuffer()
{
    mSavedFrameNum = 0;
    mMarkedFrameNum = 0;
    mLastPlayedTime = 0;
    mLastTimestamp = 0;
    mCurrPlayingTimestamp = 0;
    mIsFirstPkt = false;
}

TextJitterBuffer::~TextJitterBuffer() {}

void TextJitterBuffer::Reset() {}

void TextJitterBuffer::Add(ImsMediaSubType subtype, uint8_t* buffer, uint32_t size,
        uint32_t timestamp, bool mark, uint32_t seqNum, ImsMediaSubType dataType)
{
    (void)dataType;
    DataEntry currEntry;
    memset(&currEntry, 0, sizeof(DataEntry));
    currEntry.subtype = subtype;
    currEntry.pbBuffer = buffer;
    currEntry.nBufferSize = size;
    currEntry.nTimestamp = timestamp;
    currEntry.bMark = mark;
    currEntry.nSeqNum = seqNum;
    currEntry.bHeader = true;
    currEntry.bValid = true;

    IMLOGD_PACKET5(IM_PACKET_LOG_JITTER,
            "[Add] Seq[%u], bMark[%u], TimeStamp[%u], Size[%u], LastPlayedSeqNum[%u]", seqNum, mark,
            timestamp, size, mLastPlayedSeqNum);

    std::lock_guard<std::mutex> guard(mMutex);

    if (!USHORT_SEQ_ROUND_COMPARE(seqNum, mLastPlayedSeqNum) || (seqNum == mLastPlayedSeqNum))
    {
        IMLOGD_PACKET2(IM_PACKET_LOG_JITTER,
                "[Add] Receive Old or already played data. Seq[%u], LastPlayedSeqNum[%u]", seqNum,
                mLastPlayedSeqNum);

        if (mIsFirstPkt == false)
            return;

        IMLOGD1("[Add] first packet received, Seq[%u]", seqNum);
        mIsFirstPkt = false;
    }

    if (mDataQueue.GetCount() == 0)  // jitter buffer is empty
    {
        mDataQueue.Add(&currEntry);
    }
    else
    {
        DataEntry* pEntry;

        mDataQueue.GetLast(&pEntry);

        if (pEntry == NULL)
        {
            return;
        }

        if (USHORT_SEQ_ROUND_COMPARE(seqNum, pEntry->nSeqNum))  // current data is the latest data
        {
            IMLOGD_PACKET1(
                    IM_PACKET_LOG_JITTER, "[Add] current data is the latest data. Seq[%u]", seqNum);
            mDataQueue.Add(&currEntry);
        }
        else  // find the position of current data and insert current data to the correct position
        {
            uint32_t i;
            mDataQueue.SetReadPosFirst();

            for (i = 0; mDataQueue.GetNext(&pEntry); i++)
            {
                if (seqNum == pEntry->nSeqNum)
                {
                    IMLOGD_PACKET1(
                            IM_PACKET_LOG_JITTER, "[Add] Redundancy received. SeqNum[%u]", seqNum);
                    break;
                }
                else if (!USHORT_SEQ_ROUND_COMPARE(seqNum, pEntry->nSeqNum))
                {
                    IMLOGD_PACKET2(IM_PACKET_LOG_JITTER, "[Add] InsertAt[%u]. Seq[%u]", i, seqNum);
                    mDataQueue.InsertAt(i, &currEntry);
                    break;
                }
            }
        }
    }

    mDataCount++;
    mNewInputData = true;
    mIsFirstPkt = false;
}

bool TextJitterBuffer::Get(ImsMediaSubType* subtype, uint8_t** data, uint32_t* dataSize,
        uint32_t* timestamp, bool* mark, uint32_t* seqNum, uint32_t* checker)
{
    (void)checker;
    std::lock_guard<std::mutex> guard(mMutex);
    DataEntry* pEntry;

    if (mDataQueue.Get(&pEntry) == true && pEntry != NULL)
    {
        if (subtype)
            *subtype = pEntry->subtype;
        if (data)
            *data = pEntry->pbBuffer;
        if (dataSize)
            *dataSize = pEntry->nBufferSize;
        if (timestamp)
            *timestamp = pEntry->nTimestamp;
        if (mark)
            *mark = pEntry->bMark;
        if (seqNum)
            *seqNum = pEntry->nSeqNum;

        IMLOGD_PACKET6(IM_PACKET_LOG_JITTER,
                "[Get] OK - m_nCurrPlayingTS[%u], Seq[%u], bMark[%u], TimeStamp[%u], Size[%u], "
                "queueCount[%u]",
                mCurrPlayingTimestamp, pEntry->nSeqNum, pEntry->bMark, pEntry->nTimestamp,
                pEntry->nBufferSize, mDataQueue.GetCount());

        return true;
    }
    else
    {
        if (subtype)
            *subtype = MEDIASUBTYPE_UNDEFINED;
        if (data)
            *data = NULL;
        if (dataSize)
            *dataSize = 0;
        if (timestamp)
            *timestamp = 0;
        if (mark)
            *mark = false;
        if (seqNum)
            *seqNum = 0;
        if (mDataQueue.GetCount() == 0)
            mDataCount = 0;

        IMLOGD_PACKET0(IM_PACKET_LOG_JITTER, "[Get] fail");

        return false;
    }
}