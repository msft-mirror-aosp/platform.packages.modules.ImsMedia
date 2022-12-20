/**
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License") {
}

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

#include <MediaQualityAnalyzer.h>
#include <ImsMediaTimer.h>
#include <ImsMediaTrace.h>
#include <ImsMediaAudioUtil.h>
#include <RtcpXrEncoder.h>
#include <AudioConfig.h>
#include <stdlib.h>

#define DEFAULT_PARAM                -1
#define DEFAULT_INACTIVITY_TIME      5
#define CALL_QUALITY_MONITORING_TIME 5
#define MAX_NUM_PACKET_STORED        500
#define DELETE_ALL                   65536

MediaQualityAnalyzer::MediaQualityAnalyzer()
{
    mMediaQuality = NULL;
    mCodecType = 0;
    mCodecAttribute = 0;
    mCallback = NULL;
    std::unique_ptr<RtcpXrEncoder> analyzer(new RtcpXrEncoder());
    mRtcpXrEncoder = std::move(analyzer);
    mJitterThreshold = 0;
    mJitterDuration = 0;
    mPacketLossThreshold = 0;
    mPacketLossDuration = 0;
    mTimerHandler = NULL;
    reset();
}

MediaQualityAnalyzer::~MediaQualityAnalyzer()
{
    if (mTimerHandler != NULL)
    {
        MediaQuality* quality = new MediaQuality(*mMediaQuality);
        mCallback->SendEvent(kAudioCallQualityChangedInd, reinterpret_cast<uint64_t>(quality));

        IMLOGD0("[stopTimer]");
        ImsMediaTimer::TimerStop(mTimerHandler, NULL);
    }

    reset();

    if (mMediaQuality != NULL)
    {
        delete mMediaQuality;
    }
}

void MediaQualityAnalyzer::onTimer(hTimerHandler hTimer, void* pUserData)
{
    (void)hTimer;

    if (pUserData != NULL)
    {
        MediaQualityAnalyzer* analyzer = reinterpret_cast<MediaQualityAnalyzer*>(pUserData);
        analyzer->processTimer();
    }
}

void MediaQualityAnalyzer::setConfig(AudioConfig* config)
{
    mCodecType = config->getCodecType();
    mCodecAttribute = config->getEvsParams().getEvsBandwidth();

    IMLOGD2("[setCodecType] type[%d], bandwidth[%d]", mCodecType, mCodecAttribute);

    if (mCodecType == AudioConfig::CODEC_AMR)
    {
        mRtcpXrEncoder->setSamplingRate(8);
    }
    else
    {
        mRtcpXrEncoder->setSamplingRate(16);
    }
}

void MediaQualityAnalyzer::setCallback(BaseSessionCallback* callback)
{
    mCallback = callback;
}

void MediaQualityAnalyzer::setJitterThreshold(const int32_t duration, const int32_t threshold)
{
    IMLOGD2("[setJitterThreshold] duration[%d], threshold[%d]", duration, threshold);
    mJitterThreshold = threshold;
    mJitterDuration = duration;

    // reset the counter
    mJitterRxPacket = 0;
}

void MediaQualityAnalyzer::setPacketLossThreshold(const int32_t duration, const int32_t threshold)
{
    IMLOGD2("[setPacketLossThreshold] duration[%d], threshold[%d]", duration, threshold);
    mPacketLossThreshold = threshold;
    mPacketLossDuration = duration;

    // reset the counter
    mNumRxPacket = 0;
    mNumLostPacket = 0;
}

bool MediaQualityAnalyzer::isSameConfig(AudioConfig* config)
{
    return (mCodecType == config->getCodecType() &&
            mCodecAttribute == config->getEvsParams().getEvsBandwidth());
}

void MediaQualityAnalyzer::startTimer(const int32_t duration)
{
    mTimerDuration = duration;

    if (duration != 0 && mTimerHandler == NULL)
    {
        IMLOGD1("[startTimer] duration[%d]", duration);
        mTimerHandler = ImsMediaTimer::TimerStart(mTimerDuration, true, onTimer, this);

        mMediaQuality->setCodecType(convertAudioCodecType(
                mCodecType, ImsMediaAudioUtil::FindMaxEvsBandwidthFromRange(mCodecAttribute)));
    }
}

void MediaQualityAnalyzer::stopTimer()
{
    MediaQuality* quality = new MediaQuality(*mMediaQuality);
    mCallback->SendEvent(kAudioCallQualityChangedInd, reinterpret_cast<uint64_t>(quality));

    reset();

    if (mTimerHandler != NULL)
    {
        IMLOGD0("[stopTimer]");
        ImsMediaTimer::TimerStop(mTimerHandler, NULL);
        mTimerHandler = NULL;
    }
}

void MediaQualityAnalyzer::collectInfo(const int32_t streamType, RtpPacket* packet)
{
    if (packet == NULL)
    {
        return;
    }

    if (streamType == kStreamRtpTx)
    {
        mListTxPacket.push_back(packet);

        if (mListTxPacket.size() >= MAX_NUM_PACKET_STORED)
        {
            RtpPacket* pPacket = mListTxPacket.front();
            mListTxPacket.pop_front();
            delete pPacket;
        }

        mMediaQuality->setNumRtpPacketsTransmitted(
                mMediaQuality->getNumRtpPacketsTransmitted() + 1);
        IMLOGD_PACKET1(IM_PACKET_LOG_RTP, "[collectInfo] tx list size[%d]", mListTxPacket.size());
    }
    else if (streamType == kStreamRtpRx)
    {
        if (mSSRC != DEFAULT_PARAM && mSSRC != packet->ssrc)
        {
            IMLOGW0("[collectInfo] ssrc changed");
            // TODO: check point, no codec update but ssrc changed
            stopTimer();
            startTimer(mTimerDuration);
        }

        // for call qualty report
        mMediaQuality->setNumRtpPacketsReceived(mMediaQuality->getNumRtpPacketsReceived() + 1);
        mCallQualitySumRelativeJitter += packet->jitter;

        if (mMediaQuality->getMaxRelativeJitter() < packet->jitter)
        {
            mMediaQuality->setMaxRelativeJitter(packet->jitter);
        }

        mMediaQuality->setAverageRelativeJitter(
                mCallQualitySumRelativeJitter / mMediaQuality->getNumRtpPacketsReceived());

        switch (packet->rtpDataType)
        {
            case kRtpDataTypeNoData:
                mMediaQuality->setNumNoDataFrames(mMediaQuality->getNumNoDataFrames() + 1);
                break;
            case kRtpDataTypeSid:
                mMediaQuality->setNumRtpSidPacketsReceived(
                        mMediaQuality->getNumRtpSidPacketsReceived() + 1);
                break;
            default:
            case kRtpDataTypeNormal:
                break;
        }

        // for loss rate, jitter check
        if (mSSRC == DEFAULT_PARAM)  // stream is reset
        {
            mJitterRxPacket = std::abs(packet->jitter);
            // update rtcp-xr params
            mRtcpXrEncoder->setSsrc(packet->ssrc);
        }
        else
        {
            mJitterRxPacket =
                    mJitterRxPacket + (double)(std::abs(packet->jitter) - mJitterRxPacket) * 0.0625;
        }

        mSSRC = packet->ssrc;
        mNumRxPacket++;
        mListRxPacket.push_back(packet);

        if (mListRxPacket.size() >= MAX_NUM_PACKET_STORED)
        {
            RtpPacket* pPacket = mListRxPacket.front();
            mListRxPacket.pop_front();
            delete pPacket;
        }

        IMLOGD_PACKET3(IM_PACKET_LOG_RTP, "[collectInfo] seq[%d], jitter[%d], rx list size[%d]",
                packet->seqNum, packet->jitter, mListRxPacket.size());
    }
    else if (streamType == kStreamRtcp)
    {
        /** TODO: add implementation later */
    }
}

void MediaQualityAnalyzer::collectOptionalInfo(
        const int32_t optionType, const int32_t seq, const int32_t value)
{
    IMLOGD_PACKET3(IM_PACKET_LOG_RTP, "[collectOptionalInfo] optionType[%d], seq[%d], value[%d]",
            optionType, seq, value);

    if (optionType == kTimeToLive)
    {
        // TODO : pass data to rtcp-xr
    }
    else if (optionType == kRoundTripDelay)
    {
        mSumRoundTripTime += value;
        mCountRoundTripTime++;
        mMediaQuality->setAverageRoundTripTime(mSumRoundTripTime / mCountRoundTripTime);
        mRtcpXrEncoder->setRoundTripDelay(value);
    }
    else if (optionType == kReportPacketLossGap)
    {
        LostPktEntry* entry = new LostPktEntry(seq, value);
        mListLostPacket.push_back(entry);

        for (int32_t i = 0; i < value; i++)
        {
            // for rtcp xr
            mRtcpXrEncoder->stackRxRtpStatus(kRtpStatusLost, 0);

            // for call quality report
            mMediaQuality->setNumRtpPacketsNotReceived(
                    mMediaQuality->getNumRtpPacketsNotReceived() + 1);
            mCallQualityNumLostPacket++;

            // for loss checking
            mNumLostPacket++;
        }

        IMLOGD_PACKET3(IM_PACKET_LOG_RTP,
                "[collectOptionalInfo] lost packet seq[%d], value[%d], list size[%d]", seq, value,
                mListLostPacket.size());
    }
}

void MediaQualityAnalyzer::collectRxRtpStatus(const int32_t seq, kRtpPacketStatus status)
{
    bool found = false;

    if (mListRxPacket.empty())
    {
        return;
    }

    for (std::list<RtpPacket*>::reverse_iterator rit = mListRxPacket.rbegin();
            rit != mListRxPacket.rend(); ++rit)
    {
        RtpPacket* packet = *rit;

        if (packet->seqNum == seq)
        {
            packet->status = status;
            packet->delay = ImsMediaTimer::GetTimeInMilliSeconds() - packet->delay;
            mRtcpXrEncoder->stackRxRtpStatus(packet->status, packet->delay);
            IMLOGD_PACKET3(IM_PACKET_LOG_RTP, "[collectRxRtpStatus] seq[%d], status[%d], delay[%u]",
                    seq, packet->status, packet->delay);
            found = true;
            break;
        }
    }

    if (!found)
    {
        IMLOGW1("[collectRxRtpStatus] no rtp packet found seq[%d]", seq);
        return;
    }

    switch (status)
    {
        case kRtpStatusNormal:
            mMediaQuality->setNumVoiceFrames(mMediaQuality->getNumVoiceFrames() + 1);
            mCallQualityNumRxPacket++;
            break;
        case kRtpStatusLate:
        case kRtpStatusDiscarded:
            mMediaQuality->setNumDroppedRtpPackets(mMediaQuality->getNumDroppedRtpPackets() + 1);
            mCallQualityNumRxPacket++;
            break;
        case kRtpStatusDuplicated:
            mMediaQuality->setNumRtpDuplicatePackets(
                    mMediaQuality->getNumRtpDuplicatePackets() + 1);
            mCallQualityNumRxPacket++;
            break;
        default:
            break;
    }

    if (mBeginSeq == -1)
    {
        mBeginSeq = seq;
        mEndSeq = seq;
    }
    else
    {
        if (USHORT_SEQ_ROUND_COMPARE(seq, mEndSeq))
        {
            mEndSeq = seq;
        }
    }
}

void MediaQualityAnalyzer::collectJitterBufferSize(const int32_t currSize, const int32_t maxSize)
{
    IMLOGD_PACKET2(IM_PACKET_LOG_RTP, "[collectJitterBufferSize] current size[%d], max size[%d]",
            currSize, maxSize);

    mCurrentBufferSize = currSize;
    mMaxBufferSize = maxSize;

    mRtcpXrEncoder->setJitterBufferStatus(currSize, maxSize);
}

void MediaQualityAnalyzer::processTimer()
{
    ++mTimerCount;
    IMLOGD_PACKET1(IM_PACKET_LOG_RTP, "[processTimer] count[%d]", mTimerCount);

    if (mTimerCount == DEFAULT_INACTIVITY_TIME && mMediaQuality->getNumRtpPacketsReceived() == 0)
    {
        mMediaQuality->setRtpInactivityDetected(true);
        MediaQuality* mediaQuality = new MediaQuality(*mMediaQuality);
        mCallback->SendEvent(kAudioCallQualityChangedInd, reinterpret_cast<uint64_t>(mediaQuality));
    }

    mMediaQuality->setCallDuration(mMediaQuality->getCallDuration() + 1000);

    if (mTimerCount % CALL_QUALITY_MONITORING_TIME == 0)
    {
        double lossRate = 0;

        mCallQualityNumRxPacket == 0 ? lossRate = 0
                                     : lossRate = (double)mCallQualityNumLostPacket /
                        (mCallQualityNumLostPacket + mCallQualityNumRxPacket) * 100;

        int32_t quality = getCallQuality(lossRate);

        IMLOGD3("[processTimer] lost[%d], received[%d], quality[%d]", mCallQualityNumLostPacket,
                mCallQualityNumRxPacket, quality);

        if (mMediaQuality->getDownlinkCallQualityLevel() != quality)
        {
            mMediaQuality->setDownlinkCallQualityLevel(quality);
            MediaQuality* mediaQuality = new MediaQuality(*mMediaQuality);
            mCallback->SendEvent(
                    kAudioCallQualityChangedInd, reinterpret_cast<uint64_t>(mediaQuality));
        }

        mCallQualityNumLostPacket = 0;
        mCallQualityNumRxPacket = 0;
    }

    // check packet loss
    if (mPacketLossDuration != 0 && (mTimerCount % mPacketLossDuration) == 0)
    {
        double lossRate = 0;

        if (mNumLostPacket > 0)
        {
            lossRate = (double)mNumLostPacket / (mNumRxPacket + mNumLostPacket) * 100;
        }

        IMLOGD1("[processTimer] lossRate[%lf]", lossRate);

        if (lossRate >= mPacketLossThreshold && mCallback != NULL)
        {
            mCallback->SendEvent(kImsMediaEventPacketLoss, lossRate);
        }

        mNumRxPacket = 0;
        mNumLostPacket = 0;
    }

    // check average jitter
    if (mJitterDuration != 0 && mTimerCount % mJitterDuration == 0)
    {
        IMLOGD1("[processTimer] Jitter[%lf]", mJitterRxPacket);

        if (mJitterRxPacket >= mJitterThreshold && mCallback != NULL)
        {
            mCallback->SendEvent(kImsMediaEventNotifyJitter, mJitterRxPacket);
        }
    }
}

bool MediaQualityAnalyzer::getRtcpXrReportBlock(
        const uint32_t rtcpXrReport, uint8_t* data, uint32_t& size)
{
    IMLOGD1("[getRtcpXrReportBlock] rtcpXrReport[%d]", rtcpXrReport);

    if (rtcpXrReport == 0 || mRtcpXrEncoder == NULL)
    {
        return false;
    }

    if (mRtcpXrEncoder->createRtcpXrReport(rtcpXrReport, &mListRxPacket, &mListLostPacket,
                mBeginSeq, mEndSeq, data, size) == false)
    {
        IMLOGE0("[getRtcpXrReportBlock] error createRtcpXrReport");
        return false;
    }

    mBeginSeq = mEndSeq + 1;
    clearPacketList(mListRxPacket, mEndSeq);
    clearPacketList(mListTxPacket, mEndSeq);
    clearLostPacketList(mEndSeq);
    return true;
}

MediaQuality MediaQualityAnalyzer::getMediaQuality()
{
    return *mMediaQuality;
}

uint32_t MediaQualityAnalyzer::getRxPacketSize()
{
    return mListRxPacket.size();
}

uint32_t MediaQualityAnalyzer::getTxPacketSize()
{
    return mListTxPacket.size();
}

uint32_t MediaQualityAnalyzer::getLostPacketSize()
{
    return mListLostPacket.size();
}

void MediaQualityAnalyzer::reset()
{
    mSSRC = DEFAULT_PARAM;
    mBeginSeq = -1;
    mEndSeq = -1;

    if (mMediaQuality != NULL)
    {
        delete mMediaQuality;
    }

    mMediaQuality = new MediaQuality();
    mCallQualitySumRelativeJitter = 0;
    mSumRoundTripTime = 0;
    mCountRoundTripTime = 0;
    mCurrentBufferSize = 0;
    mMaxBufferSize = 0;
    mCallQualityNumRxPacket = 0;
    mCallQualityNumLostPacket = 0;
    clearPacketList(mListRxPacket, DELETE_ALL);
    clearPacketList(mListTxPacket, DELETE_ALL);
    clearLostPacketList(DELETE_ALL);
    mTimerCount = 0;
    mNumRxPacket = 0;
    mNumLostPacket = 0;
    mJitterRxPacket = 0.0;
}

void MediaQualityAnalyzer::clearPacketList(std::list<RtpPacket*>& list, const int32_t seq)
{
    RtpPacket* packet = NULL;
    std::list<RtpPacket*>::iterator iter;

    for (iter = list.begin(); iter != list.end();)
    {
        packet = *iter;
        // do not remove the packet seq is larger than target seq
        if (packet->seqNum > seq)
        {
            iter++;
            continue;
        }

        iter = list.erase(iter);
        delete packet;
    }
}

void MediaQualityAnalyzer::clearLostPacketList(const int32_t seq)
{
    LostPktEntry* packet = NULL;
    std::list<LostPktEntry*>::iterator iter;

    for (iter = mListLostPacket.begin(); iter != mListLostPacket.end();)
    {
        packet = *iter;
        // do not remove the lost packet entry seq is larger than target seq
        if (packet->seqNum > seq)
        {
            iter++;
            continue;
        }

        iter = mListLostPacket.erase(iter);
        delete packet;
    }
}

uint32_t MediaQualityAnalyzer::getCallQuality(const double lossRate)
{
    if (lossRate < 1.0f)
    {
        return MediaQuality::kCallQualityExcellent;
    }
    else if (lossRate < 3.0f)
    {
        return MediaQuality::kCallQualityGood;
    }
    else if (lossRate < 5.0f)
    {
        return MediaQuality::kCallQualityFair;
    }
    else if (lossRate < 8.0f)
    {
        return MediaQuality::kCallQualityPoor;
    }
    else
    {
        return MediaQuality::kCallQualityBad;
    }
}

int32_t MediaQualityAnalyzer::convertAudioCodecType(const int32_t codec, const int32_t bandwidth)
{
    switch (codec)
    {
        default:
            return MediaQuality::AUDIO_QUALITY_NONE;
        case AudioConfig::CODEC_AMR:
            return MediaQuality::AUDIO_QUALITY_AMR;
        case AudioConfig::CODEC_AMR_WB:
            return MediaQuality::AUDIO_QUALITY_AMR_WB;
        case AudioConfig::CODEC_EVS:
        {
            switch (bandwidth)
            {
                default:
                case EvsParams::EVS_BAND_NONE:
                    break;
                case EvsParams::EVS_NARROW_BAND:
                    return MediaQuality::AUDIO_QUALITY_EVS_NB;
                case EvsParams::EVS_WIDE_BAND:
                    return MediaQuality::AUDIO_QUALITY_EVS_WB;
                case EvsParams::EVS_SUPER_WIDE_BAND:
                    return MediaQuality::AUDIO_QUALITY_EVS_SWB;
                case EvsParams::EVS_FULL_BAND:
                    return MediaQuality::AUDIO_QUALITY_EVS_FB;
            }
        }
    }

    return MediaQuality::AUDIO_QUALITY_NONE;
}