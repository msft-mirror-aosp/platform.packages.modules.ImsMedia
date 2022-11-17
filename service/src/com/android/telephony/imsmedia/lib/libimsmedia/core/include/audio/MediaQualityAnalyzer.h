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

#ifndef MEDIA_QUALITY_ANALYZER_H_INCLUDED
#define MEDIA_QUALITY_ANALYZER_H_INCLUDED

#include <MediaQuality.h>
#include <ImsMediaDefine.h>
#include <ImsMediaTimer.h>
#include <RtcpXrEncoder.h>
#include <BaseSessionCallback.h>
#include <AudioConfig.h>
#include <list>

class MediaQualityAnalyzer
{
public:
    MediaQualityAnalyzer();
    virtual ~MediaQualityAnalyzer();

    /**
     * @brief A function to invoke when timer expires in certain duration.
     *
     * @param hTimer timer handler
     * @param pUserData instance who invoke timer
     */
    static void onTimer(hTimerHandler hTimer, void* pUserData);

    /**
     * @brief Set the session callback to send the event
     */
    void setCallback(BaseSessionCallback* callback);

    /**
     * @brief Sets the audio codec type
     * @param config The AudioConfig to set
     */
    void setConfig(AudioConfig* config);

    /**
     * @brief Set the jitter notification threshold. If the receiving packet jitter exceed
     * the threshold, notification will be send
     *
     * @param duration The monitoring duration in sec unit for jitter
     * @param threshold The threshold of jitter in milliseconds unit
     */
    void setJitterThreshold(const int32_t duration, const int32_t threshold);

    /**
     * @brief Set the packet loss notification threshold. If the receiving packet loss rate exceed
     * the threshold, notification will be send
     *
     * @param duration The monitoring duration in sec unit for loss rate
     * @param threshold The threshold of pakcet loss rate in percentage unit
     */
    void setPacketLossThreshold(const int32_t duration, const int32_t threshold);

    /**
     * @brief Check the audio config has different codec values
     *
     * @param config The AudioConfig to compare
     */
    bool isSameConfig(AudioConfig* config);

    /**
     * @brief Set timer to trigger for calculating statistics from collected datas
     *
     * @param duration timer duration in seconds unit.
     */
    void startTimer(const int32_t duration);

    /**
     * @brief Stop timer and calculating statistics from collected datas
     */
    void stopTimer();

    /**
     * @brief Collect information of sending or receiving the rtp or the rtcp packet datas.
     *
     * @param streamType The stream type. Tx, Rx, Rtcp.
     * @param packet The packet data struct.
     */
    void collectInfo(const int32_t streamType, RtpPacket* packet);

    /**
     * @brief Collect optional information of sending or receiving the rtp or rtcp packet datas.
     *
     * @param optionType The optional type to collect. The TTL or the Round Trip delay.
     * @param seq The sequence number of the packet to collect.
     * @param value The optional value to collect.
     */
    void collectOptionalInfo(const int32_t optionType, const int32_t seq, const int32_t value);

    /**
     * @brief Collects Rtp status determined from the jitter buffer.
     *
     * @param seq The packet sequence number to collect.
     * @param status The status of the packet. Check in @link{kRtpPacketStatus}
     */
    void collectRxRtpStatus(const int32_t seq, kRtpPacketStatus status);

    /**
     * @brief Collects jitter buffer size.
     *
     * @param currSize The current size of the jitter buffer.
     * @param maxSize The maximum jitter buffer size.
     */
    void collectJitterBufferSize(const int32_t currSize, const int32_t maxSize);

    /**
     * @brief It is invoked when the timer expired
     */
    void processTimer();

    /**
     * @brief generate  Rtcp-Xr report blocks with given report block enabled in bitmask type
     *
     * @param nReportBlocks The bitmask of report block to creates
     * @param data The byte array of total report blocks
     * @param size The size of total report blocks together
     * @return true The report block is not zero and data is valid
     * @return false The report block is zero or got error during create the report block
     */
    bool getRtcpXrReportBlock(const uint32_t nReportBlocks, uint8_t* data, uint32_t& size);

private:
    void reset();
    void createCallQualityReport();
    void clearRxPacketList(const int32_t seq = -1);
    void clearTxPacketList(const int32_t seq = -1);
    void clearLostPacketList(const int32_t seq = -1);
    uint32_t getCallQuality(double lossRate);
    int32_t convertAudioCodecType(const int32_t codec, const int32_t bandwidth);

    BaseSessionCallback* mCallback;
    std::unique_ptr<RtcpXrEncoder> mRtcpXrEncoder;
    /** The interval of timer in milliseconds  unit */
    int32_t mTimerDuration;
    /** The counter increased when the timer expires */
    uint32_t mTimerCount;
    hTimerHandler mTimerHandler;
    /** The list of the packets received ordered by arrival time */
    std::list<RtpPacket*> mListRxPacket;
    /** The list of the lost packets object */
    std::list<LostPktEntry*> mListLostPacket;
    /** The list of the packets sent */
    std::list<RtpPacket*> mListTxPacket;
    /** The ssrc of the receiving Rtp stream to identify */
    int32_t mSSRC;
    /** The codec type of the audio session retrieved from the AudioConfig.h */
    int32_t mCodecType;
    /** The codec attribute of the audio session, it could be bandwidth in evs codec */
    int32_t mCodecAttribute;
    /** The begin of the rx rtp packet sequence number for Rtcp-Xr report */
    int32_t mBeginSeq;
    /** The end of the rx rtp packet sequence number for Rtcp-Xr report */
    int32_t mEndSeq;
    /** The media quality structure to report */
    MediaQuality* mMediaQuality;
    /** The sum of the relative jitter of rx packet for call quality */
    int64_t mCallQualitySumRelativeJitter;
    /** The sum of the round trip delay of the session for call quality */
    uint64_t mSumRoundTripTime;
    /** The number of the round trip delay of the session for call quality */
    uint32_t mCountRoundTripTime;
    /** The current jitter buffer size in milliseconds unit */
    uint32_t mCurrentBufferSize;
    /** The maximum jitter buffer size in milliseconds unit */
    uint32_t mMaxBufferSize;
    /** The number of rx packet received for call quality calculation */
    uint32_t mCallQualityNumRxPacket;
    /** The number of lost rx packet for call quality calculation */
    uint32_t mCallQualityNumLostPacket;
    /** The jitter threshold to check */
    int32_t mJitterThreshold;
    /** The monitoring time to check jitter threshold in sec unit */
    int32_t mJitterDuration;
    /** The packet loss rate threshold in percentage unit */
    int32_t mPacketLossThreshold;
    /** The monitoring time of packet loss rate in sec unit */
    int32_t mPacketLossDuration;
    /** The number of received packet to check packet loss notification */
    uint32_t mNumRxPacket;
    /** The number of lost packet to check packet loss notification */
    uint32_t mNumLostPacket;
    /** The cumulated jitter value when any rx packet received */
    double mJitterRxPacket;
};

#endif