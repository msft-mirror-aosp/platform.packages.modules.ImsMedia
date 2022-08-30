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

#include <AudioJitterBuffer.h>
#include <ImsMediaDataQueue.h>
#include <ImsMediaTimer.h>
#include <ImsMediaTrace.h>

#if 1
#define AUDIO_JITTER_BUFFER_SIZE       5
#define AUDIO_G711_JITTER_BUFFER_SIZE  6
#define AUDIO_JITTER_BUFFER_MIN_SIZE   3
#define AUDIO_JITTER_BUFFER_MAX_SIZE   9
#define AUDIO_JITTER_BUFFER_START_SIZE 4
#else
#define AUDIO_JITTER_BUFFER_SIZE      3
#define AUDIO_G711_JITTER_BUFFER_SIZE 4
#endif
#if defined(FEATURE_IMS_MEDIA_ATT) || defined(FEATURE_IMS_MEDIA_CMCC)
#define JITTERBUFFER_UPDATE_DURATION 9
#else
#define JITTERBUFFER_UPDATE_DURATION 0
#endif
#define CODECFILTER_AUDIO_FMC_MAX_SEQUENCE          0xffff
#define CODECFILTER_AUDIO_PACKETRECEIVE_COUNTER_MAX 50
#define CODECFILTER_AUDIO_PACKETINSERT_COUNTER_MAX  5
#define CODECFILTER_AUDIO_SKIP_READCOUNTER          100

AudioJitterBuffer::AudioJitterBuffer()
{
    mInitJitterBufferSize = AUDIO_JITTER_BUFFER_START_SIZE;
    mMinJitterBufferSize = AUDIO_JITTER_BUFFER_MIN_SIZE;
    mMaxJitterBufferSize = AUDIO_JITTER_BUFFER_MAX_SIZE;
    AudioJitterBuffer::Reset();
    mBufferIgnoreSIDPacket = false;
    mBufferImprovement = false;
}

AudioJitterBuffer::~AudioJitterBuffer() {}

void AudioJitterBuffer::Reset()
{
    mNextJitterBufferSize = mCurrJitterBufferSize;
    mLastPlayedSeqNum = 0;
    mLastPlayedTimestamp = 0;
    mIsReceivedFirst = false;
    mDtxOn = false;
    mSIDCount = 0;
    mWaiting = false;
    mDeleteCount = 0;
    mBaseTS = 0;
    mBaseAT = 0;
    mUpdateJitterBufferSize = 0;
    mCheckUpdateJitterPacketCnt = 0;
    mFourceToUpdateJitterBuffer = false;
    mNeedToUpdateBasePacket = false;
    mCanNotGetCount = 0;
    mNullDataCount = 0;
    BaseJitterBuffer::Reset();
    mJitterAnalyser.Reset();
    mJitterAnalyser.SetMinMaxJitterBufferSize(mMinJitterBufferSize, mMaxJitterBufferSize);
    mCurrPlayingSeq = 0;
    mBufferUpdateDuration = 0;
}

void AudioJitterBuffer::SetJitterBufferSize(uint32_t nInit, uint32_t nMin, uint32_t nMax)
{
    IMLOGD3("[SetJitterBufferSize] %02x, %02x, %02x", nInit, nMin, nMax);
    if (nMin > 0)
    {
        mMinJitterBufferSize = nMin;
        mMinJitterBufferSize = mMinJitterBufferSize;
    }
    if (nMax > 0)
    {
        mMaxJitterBufferSize = nMax;
        mMaxJitterBufferSize = mMaxJitterBufferSize;
    }
    if (nInit > 0)
    {
        if (nInit < mMinJitterBufferSize)
            nInit = mMinJitterBufferSize;
        if (nInit > mMaxJitterBufferSize)
            nInit = mMaxJitterBufferSize;
        mInitJitterBufferSize = nInit;
        mCurrJitterBufferSize = mInitJitterBufferSize;
        mNextJitterBufferSize = mInitJitterBufferSize;
    }

    mJitterAnalyser.SetMinMaxJitterBufferSize(mMinJitterBufferSize, mMaxJitterBufferSize);
}

void AudioJitterBuffer::SetJitterOptions(
        uint32_t nReduceTH, uint32_t nStepSize, double zValue, bool bIgnoreSID, bool bImprovement)
{
    // do it later
    mBufferIgnoreSIDPacket = bIgnoreSID;
    mBufferImprovement = bImprovement;
    mJitterAnalyser.SetJitterOptions(nReduceTH, nStepSize, zValue, bImprovement);
}

void AudioJitterBuffer::Add(ImsMediaSubType subtype, uint8_t* pbBuffer, uint32_t nBufferSize,
        uint32_t nTimestamp, bool bMark, uint32_t nSeqNum, ImsMediaSubType nDataType)
{
    // do it later
    (void)nDataType;

    DataEntry currEntry;
    memset(&currEntry, 0, sizeof(DataEntry));
    currEntry.subtype = subtype;
    currEntry.pbBuffer = pbBuffer;
    currEntry.nBufferSize = nBufferSize;
    currEntry.nTimestamp = nTimestamp;
    currEntry.bMark = bMark;
    currEntry.nSeqNum = nSeqNum;
    currEntry.bHeader = true;
    currEntry.bValid = true;
    currEntry.nInputTime = ImsMediaTimer::GetTimeInMilliSeconds() - (bMark * 20);

    // fix non-voice issue when min buffer size is 10 (VT case)
    if (mBufferImprovement == false && mCanNotGetCount > 500)
    {
        IMLOGD0("[Add] Refreshed");
        // CollectJitterStatus(RTP_PACKET_STATUS_ANAL_JITTER_RESET, nSeqNum);
        Reset();
    }
    else if (mBufferImprovement)
    {
        if ((mCanNotGetCount > mMaxJitterBufferSize) && (mCanNotGetCount > mMinJitterBufferSize))
        {
            IMLOGD0("[Add] Refreshed");
            // CollectJitterStatus(RTP_PACKET_STATUS_ANAL_JITTER_RESET, nSeqNum);
            Reset();
        }
    }

    if (mBufferIgnoreSIDPacket == false)
    {
        // first packet delay compensation
        if ((mBaseTS == 0 && mBaseAT == 0) || (mNeedToUpdateBasePacket == true))
        {
            mBaseTS = currEntry.nTimestamp;
            mBaseAT = currEntry.nInputTime;
            mJitterAnalyser.BasePacketChange(mBaseTS, mBaseAT);
            // fixed bug for network delay
            mNeedToUpdateBasePacket = false;
        }
        else if (mBaseTS > currEntry.nTimestamp || mBaseAT > currEntry.nInputTime)
        {
            // rounding case (more consider case)
            mBaseTS = currEntry.nTimestamp;
            mBaseAT = currEntry.nInputTime;
            mJitterAnalyser.BasePacketChange(mBaseTS, mBaseAT);
        }
        else
        {
            // update case
            if (currEntry.nTimestamp - mBaseTS > currEntry.nInputTime - mBaseAT)
            {
                mBaseTS = currEntry.nTimestamp;
                mBaseAT = currEntry.nInputTime;
                mJitterAnalyser.BasePacketChange(mBaseTS, mBaseAT);
            }
            else
            {  // compensation case
                uint32_t temp = currEntry.nInputTime;
                IMLOGD_PACKET2(IM_PACKET_LOG_JITTER, "Before compensation[%u], seqNum[%d]", temp,
                        currEntry.nSeqNum);
                currEntry.nInputTime = mBaseAT + (currEntry.nTimestamp - mBaseTS);
                IMLOGD_PACKET2(IM_PACKET_LOG_JITTER, "After compensation[%u], delay[%u]",
                        currEntry.nInputTime, temp - currEntry.nInputTime);
            }
        }

        mJitterAnalyser.OnInputData(
                nTimestamp, bMark, nSeqNum, ImsMediaTimer::GetTimeInMilliSeconds());
    }
    else if ((mBufferIgnoreSIDPacket == true) && !IsSID(currEntry.pbBuffer, currEntry.nBufferSize))
    {
        // first packet delay compensation
        if ((mBaseTS == 0 && mBaseAT == 0) || (mNeedToUpdateBasePacket == true))
        {
            mBaseTS = currEntry.nTimestamp;
            mBaseAT = currEntry.nInputTime;
            mJitterAnalyser.BasePacketChange(mBaseTS, mBaseAT);
            // fixed bug for network delay
            mNeedToUpdateBasePacket = false;
        }
        else if (mBaseTS > currEntry.nTimestamp || mBaseAT > currEntry.nInputTime)
        {
            // rounding case (more consider case)
            mBaseTS = currEntry.nTimestamp;
            mBaseAT = currEntry.nInputTime;
            mJitterAnalyser.BasePacketChange(mBaseTS, mBaseAT);
        }
        else
        {
            // update case
            if (currEntry.nTimestamp - mBaseTS > currEntry.nInputTime - mBaseAT)
            {
                mBaseTS = currEntry.nTimestamp;
                mBaseAT = currEntry.nInputTime;
                mJitterAnalyser.BasePacketChange(mBaseTS, mBaseAT);
            }
            else
            {  // compensation case
                uint32_t temp = currEntry.nInputTime;
                IMLOGD_PACKET2(IM_PACKET_LOG_JITTER, "Before compensation[%u], seqNum[%d]", temp,
                        currEntry.nSeqNum);
                currEntry.nInputTime = mBaseAT + (currEntry.nTimestamp - mBaseTS);
                IMLOGD_PACKET2(IM_PACKET_LOG_JITTER, "After compensation[%u], delay[%u]",
                        currEntry.nInputTime, temp - currEntry.nInputTime);
            }
        }

        mJitterAnalyser.OnInputData(
                nTimestamp, bMark, nSeqNum, ImsMediaTimer::GetTimeInMilliSeconds());
    }

    if (nBufferSize == 0)
    {
        // CollectJitterStatus(RTP_PACKET_STATUS_NODATA_PACKET, currEntry.nSeqNum);
        return;
    }

    std::lock_guard<std::mutex> guard(mMutex);

    IMLOGD_PACKET7(IM_PACKET_LOG_JITTER,
            "[Add] seq[%d], bMark[%d], nTS[%d], nbuffer[%d] subType[%d] QueueSize[%d] "
            "ninputTime[%u] ",
            nSeqNum, bMark, nTimestamp, nBufferSize, subtype, mDataQueue.GetCount() + 1,
            currEntry.nInputTime);

    if (mDataQueue.GetCount() == 0)
    {  // jitter buffer is empty
        mDataQueue.Add(&currEntry);
    }
    else
    {
        DataEntry* pEntry;
        mDataQueue.GetLast(&pEntry);

        if (pEntry == NULL)
            return;
        // current data is the latest data
        if (USHORT_SEQ_ROUND_COMPARE(nSeqNum, pEntry->nSeqNum))
        {
            mDataQueue.Add(&currEntry);
            if (nSeqNum == pEntry->nSeqNum)
            {
                // for RTCP-XR report to duplicated packet
                // because real audio does not start.
                // CollectJitterStatus(RTP_PACKET_STATUS_DUPLICATE, nSeqNum);
            }
        }
        else
        {
            // find the position of current data and insert current data to the correct position
            uint32_t i;
            bool bIsLateArrival = false;
            mDataQueue.SetReadPosFirst();

            for (i = 0; mDataQueue.GetNext(&pEntry); i++)
            {
                // late arrival packet
                if (!USHORT_SEQ_ROUND_COMPARE(nSeqNum, pEntry->nSeqNum))
                {
                    mDataQueue.InsertAt(i, &currEntry);
                    bIsLateArrival = true;
                    break;
                }
            }

            if (bIsLateArrival == true)
            {
                uint32_t countDup = 0;
                mDataQueue.SetReadPosFirst();  // for RTCP-XR report to duplicated packet

                for (i = 0; mDataQueue.GetNext(&pEntry); i++)
                {
                    if (nSeqNum == pEntry->nSeqNum)
                    {
                        countDup++;
                    }
                }

                if (countDup >= 2)
                {
                    // because real audio does not start.
                    // CollectJitterStatus(RTP_PACKET_STATUS_DUPLICATE, nSeqNum);
                }
            }
        }
    }

    // Count Duplicated SeqNum packets not to delete bundled packets (for maxptime 240 support)
    uint32_t nMaxBundledCount = 0;
    uint32_t nTempBundledSeqNum = 0;
    uint32_t nTempBundledCount = 0;
    DataEntry* pEntry;
    mDataQueue.SetReadPosFirst();

    while (mDataQueue.GetNext(&pEntry) && pEntry != NULL)
    {
        if (nTempBundledSeqNum != pEntry->nSeqNum)
        {
            nTempBundledSeqNum = pEntry->nSeqNum;
            nTempBundledCount = 1;
        }
        else
        {
            nTempBundledCount++;
        }

        if (nMaxBundledCount < nTempBundledCount)
        {
            nMaxBundledCount = nTempBundledCount;  // count maximum
        }
    }

    IMLOGD_PACKET1(IM_PACKET_LOG_JITTER, "[Add] Frames with Duplicated SeqNum Count [%d]",
            nMaxBundledCount);

    if (mBufferImprovement == false)
    {
        while (mDataQueue.GetCount() >
                mMaxJitterBufferSize + mMinJitterBufferSize + nMaxBundledCount - 1)
        {
            // CollectJitterStatus(RTP_PACKET_STATUS_LATE);
            mDataQueue.Delete();
            mDeleteCount++;
            IMLOGD_PACKET0(IM_PACKET_LOG_JITTER, "[Add] Delete - !mIsReceivedFirst");
        }
    }
    else
    {
        if (mIsReceivedFirst == false)
        {
            // 20160524 Do not delete bundled packet (for maxptime 240 support)
            // while (mDataQueue.GetCount() > mMinJitterBufferSize + 1)
            while (mDataQueue.GetCount() > mCurrJitterBufferSize + nMaxBundledCount)
            {
                IMLOGD_PACKET7(IM_PACKET_LOG_JITTER,
                        "[Add] Early Received - seq[%d], bMark[%d], nTS[%d],\
nbuffer[%d] subType[%d] QueueSize[%d] ninputTime[%u]",
                        nSeqNum, bMark, nTimestamp, nBufferSize, subtype, mDataQueue.GetCount() + 1,
                        currEntry.nInputTime);
                // because real audio does not start.
                // CollectJitterStatus(RTP_PACKET_STATUS_XR_DROP);
                mDataQueue.Delete();
                mDeleteCount++;
                IMLOGD_PACKET0(IM_PACKET_LOG_JITTER, "[Add] Delete - mIsReceivedFirst");
            }
        }
        else
        {
            // 20160524 Do not delete bundled packet (for maxptime 240 support)
            // while (mDataQueue.GetCount() > mMaxJitterBufferSize + mMinJitterBufferSize)
            while (mDataQueue.GetCount() >
                    mMaxJitterBufferSize + mMinJitterBufferSize + nMaxBundledCount - 1)
            {
                // CollectJitterStatus(RTP_PACKET_STATUS_LATE);
                mDataQueue.Delete();
                mDeleteCount++;
                IMLOGD_PACKET0(IM_PACKET_LOG_JITTER, "[Add] Delete - !mIsReceivedFirst");
            }
        }
    }
}

bool AudioJitterBuffer::Get(ImsMediaSubType* psubtype, uint8_t** ppData, uint32_t* pnDataSize,
        uint32_t* pnTimestamp, bool* pbMark, uint32_t* pnSeqNum, uint32_t* pnChecker)
{
    if (mNullDataCount < 4)
    {
        mNullDataCount++;
        IMLOGD1("[Get] mNullDataCount is [%d].", mNullDataCount);

        if (psubtype)
            *psubtype = MEDIASUBTYPE_UNDEFINED;
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
        if (pnChecker)
            *pnChecker = 0;

        return false;
    }

    std::lock_guard<std::mutex> guard(mMutex);

    bool bWait = false;
    DataEntry* pEntry = NULL;
    bool bForceToPlay = false;
    mUpdateJitterBufferSize++;  // add code for no dtx
    mCheckUpdateJitterPacketCnt++;
    int32_t nTimediff = 0;

    // determin bWait
    if (mIsReceivedFirst == false)
    {
        bWait = true;
    }
    else if (mDtxOn && mDataQueue.GetCount() > 0)
    {
        mDataQueue.Get(&pEntry);
        if (pEntry == NULL)
            return false;
        if (IsSID(pEntry->pbBuffer, pEntry->nBufferSize) == false)
        {
            IMLOGD_PACKET0(IM_PACKET_LOG_JITTER, "[Get] bWait = True");
            bWait = true;
        }
    }

    // set current jitter buffer size
    // when dtx occur, calculate nextJitterBufferSize.
    if (mBufferImprovement == false)
    {
        if ((mWaiting == false && bWait == true) || mUpdateJitterBufferSize > 2 * 50)
        {
            uint32_t nPrev_jitterbufferSize = mCurrJitterBufferSize;
            mCurrJitterBufferSize = mJitterAnalyser.GetJitterBufferSize(mCurrJitterBufferSize);

            if (mUpdateJitterBufferSize > 2 * 50)
            {  // 2s
                nTimediff = ((int32_t)mCurrJitterBufferSize - (int32_t)nPrev_jitterbufferSize) * 20;
                IMLOGD1("[Get] mFourceToUpdateJitterBuffer set. nTimediff[%d]", nTimediff);
                mFourceToUpdateJitterBuffer = true;
            }
            mUpdateJitterBufferSize = 0;
        }

        mWaiting = bWait;
    }
    else
    {
        if ((mWaiting == false && bWait == true) || mUpdateJitterBufferSize > 500)
        {
            // update mCurrJitterBufferSize
            if (mCheckUpdateJitterPacketCnt > 50 * mBufferUpdateDuration)
            {
                mCurrJitterBufferSize = mJitterAnalyser.GetJitterBufferSize(mCurrJitterBufferSize);
                mCheckUpdateJitterPacketCnt = 0;
            }

            if (mUpdateJitterBufferSize > 500)
            {
                IMLOGD_PACKET0(IM_PACKET_LOG_JITTER, "[Get] mFourceToUpdateJitterBuffer set");
                mFourceToUpdateJitterBuffer = true;
            }

            mUpdateJitterBufferSize = 0;
        }

        mWaiting = bWait;
    }
    // Wait case
    if (bWait)
    {
        if (mDataQueue.Get(&pEntry))
        {
            IMLOGD_PACKET5(IM_PACKET_LOG_JITTER,
                    "[Get] time[%u], nInputTime[%u], nTS[%d], mBaseTS[%u], mBaseAT[%u]",
                    ImsMediaTimer::GetTimeInMilliSeconds(), pEntry->nInputTime, pEntry->nTimestamp,
                    mBaseTS, mBaseAT);
        }

        if (mDataQueue.GetCount() == 0 ||
                ((mDataQueue.Get(&pEntry) &&
                         (ImsMediaTimer::GetTimeInMilliSeconds() - pEntry->nInputTime) <
                                 ((mCurrJitterBufferSize - 1) * 20 + 10)) &&
                        (mDataQueue.GetCount() <= mCurrJitterBufferSize)))
        {
            if (psubtype)
                *psubtype = MEDIASUBTYPE_UNDEFINED;
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
            if (pnChecker)
                *pnChecker = 0;

            if (mDataQueue.GetCount() > 0)
            {
                IMLOGD_PACKET5(IM_PACKET_LOG_JITTER,
                        "[Get] Wait - Seq[%u], CurrJBSize[%u], curr[%u], th[%u], QueueCount[%u]",
                        pEntry->nSeqNum, mCurrJitterBufferSize,
                        (uint32_t)(ImsMediaTimer::GetTimeInMilliSeconds() - pEntry->nInputTime),
                        ((mCurrJitterBufferSize - 1) * 20 + 10), mDataQueue.GetCount());
                mCanNotGetCount++;
            }
            else
            {
                IMLOGD_PACKET0(IM_PACKET_LOG_JITTER, "[Get] Wait - empty");
            }

            return false;
        }

        // the first frame of voice term
        mCurrPlayingTS = pEntry->nTimestamp;
        mCurrPlayingSeq = pEntry->nSeqNum;
    }

    if (mDataQueue.Get(&pEntry) == true && pEntry->nTimestamp != mCurrPlayingTS &&
            ((mCurrPlayingTS - 10) < pEntry->nTimestamp) &&
            (pEntry->nTimestamp < (mCurrPlayingTS + 10)))
    {
        IMLOGD_PACKET0(IM_PACKET_LOG_JITTER, "[Get] CurrPlaying Timestamp Reset!");
        mCurrPlayingTS = pEntry->nTimestamp;
        mCurrPlayingSeq = pEntry->nSeqNum;
    }

    // delete very old data
    while (mDataQueue.GetCount() > 0)
    {
        mDataQueue.Get(&pEntry);

        if (pEntry == NULL)
            return false;

        if (mDeleteCount > 4 && mDataQueue.GetCount() < mCurrJitterBufferSize + 1)
        {
            IMLOGD0("[Get] Resync");
            // mCurrPlayingTS = pEntry->nTimestamp;

            if (mBufferImprovement == false)
            {
                uint32_t nTempBuferSize = (mCurrJitterBufferSize + mMinJitterBufferSize) / 2;

                if (mDataQueue.GetCount() > nTempBuferSize)
                {
                    mCurrPlayingTS = pEntry->nTimestamp;
                    mCurrPlayingSeq = pEntry->nSeqNum;
                }
                else
                {
                    mCurrPlayingTS =
                            pEntry->nTimestamp - (nTempBuferSize - mDataQueue.GetCount()) * 20;
                    mCurrPlayingSeq = 0;
                }
            }
            else
            {
                uint32_t nTempBuferSize =
                        (mCurrJitterBufferSize + AUDIO_JITTER_BUFFER_MIN_SIZE) / 2;
                if (mDataQueue.GetCount() >= nTempBuferSize)
                {
                    mCurrPlayingTS = pEntry->nTimestamp;
                    mCurrPlayingSeq = pEntry->nSeqNum;
                }
                else
                {
                    mCurrPlayingTS =
                            pEntry->nTimestamp - (nTempBuferSize - mDataQueue.GetCount()) * 20;
                    mCurrPlayingSeq = 0;
                }
            }

            mNeedToUpdateBasePacket = true;
            mDeleteCount = 0;
            break;
        }

        if (USHORT_TS_ROUND_COMPARE(pEntry->nTimestamp, mCurrPlayingTS) == false)
        {
            if (IsSID(pEntry->pbBuffer, pEntry->nBufferSize))
            {
                mSIDCount++;
                mDtxOn = true;
                IMLOGD_PACKET0(IM_PACKET_LOG_JITTER, "[Get] Dtx On");
            }
            else
            {
                mSIDCount = 0;
                // mDtxOn = false;
                IMLOGD_PACKET0(IM_PACKET_LOG_JITTER, "[Get] Dtx Off");
            }

            mDeleteCount++;

            if (mDtxOn == true)
            {
                // CollectJitterStatus(RTP_PACKET_STATUS_OK);
                // CollectJitterBufferSize(mCurrJitterBufferSize, mMaxJitterBufferSize);
            }
            else
            {
                // CollectJitterStatus(RTP_PACKET_STATUS_LATE);
            }

            mDataQueue.Delete();
            IMLOGD_PACKET0(
                    IM_PACKET_LOG_JITTER, "[Get] Delete - pEntry->nTimestamp < mCurrPlayingTS)");
        }
        else
        {
            uint32_t timediff = pEntry->nTimestamp - mCurrPlayingTS;
            mDeleteCount = 0;

            // timestamp compensation logic
            if ((timediff > 0) && (timediff < 20))
            {
                IMLOGD2("[Get] Re sync2 - PacketTS[%d], PlayTS[%d]", pEntry->nTimestamp,
                        mCurrPlayingTS);
                bForceToPlay = true;
            }

            break;
        }

        IMLOGD4("[Get]  D [ %d / %u / %u / %d ]", pEntry->nSeqNum, pEntry->nTimestamp,
                mCurrPlayingTS, mDataQueue.GetCount());
    }

    // decrease jitter buffer
    if (mDtxOn && mSIDCount > 4 && mDataQueue.GetCount() > mCurrJitterBufferSize)
    {
        mDataQueue.Get(&pEntry);
        if (pEntry && IsSID(pEntry->pbBuffer, pEntry->nBufferSize))
        {
            IMLOGD_PACKET5(IM_PACKET_LOG_JITTER,
                    "[Get] Delete SID - seq[%d], bMark[%d], nTS[%d], curTS[%d], size[%d]",
                    pEntry->nSeqNum, pEntry->bMark, pEntry->nTimestamp, mCurrPlayingTS,
                    mDataQueue.GetCount());

            mSIDCount++;
            mDtxOn = true;
            IMLOGD_PACKET0(IM_PACKET_LOG_JITTER, "[Get] Dtx On");

            // CollectJitterStatus(RTP_PACKET_STATUS_OK);
            // CollectJitterBufferSize(mCurrJitterBufferSize, mMaxJitterBufferSize);
            mDataQueue.Delete();
            bForceToPlay = true;
        }
    }

    // add condition in case of changing Seq# & TS
    if ((mCanNotGetCount > 10) && (mDataQueue.Get(&pEntry) == true) &&
            (pEntry->nTimestamp - mCurrPlayingTS > TS_ROUND_QUARD))
    {
        IMLOGD4("[Get] TS changing case [ %d / %u / %u / %d ]", pEntry->nSeqNum, pEntry->nTimestamp,
                mCurrPlayingTS, mDataQueue.GetCount());
        bForceToPlay = true;
    }

    if (mFourceToUpdateJitterBuffer == true)
    {
        // removing delete packet in min JitterBuffer size
        if (mDataQueue.GetCount() > mCurrJitterBufferSize + 1)
        {
            mDataQueue.Get(&pEntry);

            if (pEntry)
            {
                IMLOGD_PACKET5(IM_PACKET_LOG_JITTER,
                        "[Get] Delete Packets - seq[%d], bMark[%d], nTS[%d], curTS[%d], size[%d]",
                        pEntry->nSeqNum, pEntry->bMark, pEntry->nTimestamp, mCurrPlayingTS,
                        mDataQueue.GetCount());

                if (IsSID(pEntry->pbBuffer, pEntry->nBufferSize))
                {
                    mSIDCount++;
                    mDtxOn = true;
                    IMLOGD_PACKET0(IM_PACKET_LOG_JITTER, "[Get] Dtx On");
                }
                else
                {
                    mSIDCount = 0;
                    mDtxOn = false;
                    IMLOGD_PACKET0(IM_PACKET_LOG_JITTER, "[Get] Dtx Off");
                }

                if (mDtxOn == true)
                {
                    // CollectJitterStatus(RTP_PACKET_STATUS_OK);
                    // CollectJitterBufferSize(mCurrJitterBufferSize, mMaxJitterBufferSize);
                }
                else
                {
                    // CollectJitterStatus(RTP_PACKET_STATUS_DROP);
                }

                mDataQueue.Delete();
                bForceToPlay = true;
            }
        }

        if (mBufferImprovement == false)
        {
            // increase case
            if ((mDataQueue.GetCount() < 2) || (mDataQueue.GetCount() < mCurrJitterBufferSize))
            {
                IMLOGD3("[Get] increase queue size[%d], curr buffer[%d], nTimediff[%d]",
                        mDataQueue.GetCount(), mCurrJitterBufferSize, nTimediff);

                if (nTimediff > 0)
                {
                    mCurrPlayingTS -= nTimediff;
                    mCurrPlayingSeq -= nTimediff / 20;
                    IMLOGD1("[Get] increase queue size [%d]", nTimediff / 20);
                }
            }

            // reset
            mFourceToUpdateJitterBuffer = false;
            mUpdateJitterBufferSize = 0;
        }
        else
        {
            mFourceToUpdateJitterBuffer = false;
            mUpdateJitterBufferSize = 0;
            if ((mDataQueue.GetCount() < 2) ||
                    (mDataQueue.GetCount() < mCurrJitterBufferSize - mMinJitterBufferSize))
            {
                IMLOGD_PACKET0(IM_PACKET_LOG_JITTER, "[Get] increase packet");
                return false;
            }
        }
    }

    // get datas
    if (mDataQueue.Get(&pEntry))
    {
        IMLOGD_PACKET3(IM_PACKET_LOG_JITTER,
                "[Get] Determin - nDataQueue[%d], curTS[%d], pEntry->nTS[%d]",
                mDataQueue.GetCount(), mCurrPlayingTS, pEntry->nTimestamp);
    }
    else
    {
        IMLOGD_PACKET2(IM_PACKET_LOG_JITTER, "[Get] Determin - nDataQueue[%d], curTS[%d]", 0,
                mCurrPlayingTS);
    }

    if (mDataQueue.Get(&pEntry) == true && pEntry != NULL &&
            (pEntry->nTimestamp == mCurrPlayingTS || bForceToPlay ||
                    (pEntry->nTimestamp < TS_ROUND_QUARD && mCurrPlayingTS > 0xFFFF)))
    {
        if (psubtype)
            *psubtype = pEntry->subtype;
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

        mIsReceivedFirst = true;

        if (IsSID(pEntry->pbBuffer, pEntry->nBufferSize))
        {
            mSIDCount++;
            mDtxOn = true;
            IMLOGD_PACKET0(IM_PACKET_LOG_JITTER, "[Get] Dtx On");
        }
        else
        {
            mSIDCount = 0;
            mDtxOn = false;
            IMLOGD_PACKET0(IM_PACKET_LOG_JITTER, "[Get] Dtx Off");
        }

        IMLOGD_PACKET7(IM_PACKET_LOG_JITTER,
                "[Get] OK - dtx mode[%d], curTS[%d], seq[%d], Mark[%d], TS[%d], buffer[%d], "
                "size[%d]",
                mDtxOn, mCurrPlayingTS, pEntry->nSeqNum, pEntry->bMark, pEntry->nTimestamp,
                pEntry->nBufferSize, mDataQueue.GetCount());

        mCurrPlayingTS = pEntry->nTimestamp + 20;
        mCurrPlayingSeq = pEntry->nSeqNum + 1;
        // Analyse packet lossRate
        mJitterAnalyser.LossPacketRateAnalyser();
        mCanNotGetCount = 0;
        // CollectJitterStatus(RTP_PACKET_STATUS_OK);
        // CollectJitterBufferSize(mCurrJitterBufferSize, mMaxJitterBufferSize);
        return true;
    }
    else
    {
        // check EVS redundancy
        {
            if ((mCodecType == kAudioCodecEvs) && (mDtxOn == false) && (mRedundancyOffSet > 0) &&
                    (mDataQueue.GetCount() > 0))
            {
                // check partial redundancy packet
                bool ret = false;
                ret = CheckPartialRedundancyFrame(
                        psubtype, ppData, pnDataSize, pnTimestamp, pbMark, pnSeqNum, pnChecker);

                if (ret == true)
                {
                    mCurrPlayingTS += 20;
                    mCurrPlayingSeq += 1;
                    return true;
                }
            }
        }

        if (mDtxOn == false)
        {
            mJitterAnalyser.OnFrameLoss();
            mCanNotGetCount++;
        }

        if (psubtype)
            *psubtype = MEDIASUBTYPE_UNDEFINED;
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

        IMLOGD_PACKET2(IM_PACKET_LOG_JITTER, "[Get] fail - dtx mode[%d], curTS[%d]", mDtxOn,
                mCurrPlayingTS);

        mCurrPlayingTS += 20;
        mCurrPlayingSeq += 1;
        return false;
    }

    return false;
}

bool AudioJitterBuffer::CheckPartialRedundancyFrame(ImsMediaSubType* psubtype, uint8_t** ppData,
        uint32_t* pnDataSize, uint32_t* pnTimestamp, bool* pbMark, uint32_t* pnSeqNum,
        uint32_t* pnChecker)
{
    DataEntry* pEntry;
    uint32_t nFindPartialRedundancyFrameSeq = mCurrPlayingSeq + mRedundancyOffSet;
    bool nFindPartialFrame = false;

    // 1. find redundancy Frame from DataQueue using CAM offset(mRedundancyOffSet)
    for (uint32_t i = 0; i < mDataQueue.GetCount(); i++)
    {
        mDataQueue.GetAt(i, &pEntry);
        if ((pEntry != NULL) && (pEntry->nSeqNum == nFindPartialRedundancyFrameSeq))
        {
            if (psubtype)
                *psubtype = pEntry->subtype;
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
            nFindPartialFrame = true;
            break;
        }
    }

    // 2. check bitrate using dataSize (13.2kbps : data size -> 33 byte)
    if ((nFindPartialFrame != true) || (pEntry->nBufferSize != 33))
    {
        IMLOGD1("[CheckPartialRedundancyFrame] not found or not adjust CAM -- PartialFrame[%d]",
                nFindPartialFrame);
        return false;
    }

    // 3. check PartialRedundancyFrame using provided QCT code.
    // need to porting QCT code.
    int16_t nPartialFrameOffset = 0;
    int16_t nPartialFlag = 0;

    // 4. if PartialRedundancyFrame is useable, send ppData and set nChecker variable to
    // 1, after that, return true. if not, return false.
    if ((nPartialFlag == 1) && (mRedundancyOffSet == (uint32_t)nPartialFrameOffset))
    {
        IMLOGD2("[CheckPartialRedundancyFrame] adjust CAM -- redundancyOffSet[%d], adjust seq[%d]",
                mRedundancyOffSet, pEntry->nSeqNum);
        if (pnChecker)
            *pnChecker = 1;
        return true;
    }
    else
    {
        IMLOGD3("[CheckPartialRedundancyFrame] partial frame at seq[%d], flag[%d], offset[%d]",
                pEntry->nSeqNum, nPartialFlag, nPartialFrameOffset);
        return false;
    }

    return false;
}

bool AudioJitterBuffer::IsSID(uint8_t* pbBuffer, uint32_t nBufferSize)
{
    // remove warning
    (void)pbBuffer;
    switch (mCodecType)
    {
        case kAudioCodecAmr:
        case kAudioCodecAmrWb:
            if (nBufferSize == 5)
                return true;
            else
                return false;
        case kAudioCodecEvs:
            if ((nBufferSize == 6) || (nBufferSize == 5))
                return true;
            else
                return false;
        case kAudioCodecPcmu:
        case kAudioCodecPcma:
            return false;
        default:
            IMLOGE1("[IsSID] DTX detect method is not defined for[%u] codec", mCodecType);
            return false;
    }
}