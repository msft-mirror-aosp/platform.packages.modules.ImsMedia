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

#include <MediaQuality.h>

namespace android
{

namespace telephony
{

namespace imsmedia
{

#define DEFAULT_PARAM (-1)

MediaQuality::MediaQuality()
{
    mDownlinkCallQualityLevel = 0;
    mUplinkCallQualityLevel = 0;
    mCallDuration = 0;
    mNumRtpPacketsTransmitted = 0;
    mNumRtpPacketsReceived = 0;
    mNumRtpPacketsTransmittedLost = 0;
    mNumRtpPacketsNotReceived = 0;
    mAverageRelativeJitter = 0;
    mMaxRelativeJitter = 0;
    mAverageRoundTripTime = 0;
    mCodecType = DEFAULT_PARAM;
    mRtpInactivityDetected = false;
    mRxSilenceDetected = false;
    mTxSilenceDetected = false;
    mNumVoiceFrames = 0;
    mNumNoDataFrames = 0;
    mNumDroppedRtpPackets = 0;
    mMinPlayoutDelayMillis = 0;
    mMaxPlayoutDelayMillis = 0;
    mNumRtpSidPacketsReceived = 0;
    mNumRtpDuplicatePackets = 0;
}

MediaQuality::MediaQuality(const MediaQuality& quality)
{
    mDownlinkCallQualityLevel = quality.mDownlinkCallQualityLevel;
    mUplinkCallQualityLevel = quality.mUplinkCallQualityLevel;
    mCallDuration = quality.mCallDuration;
    mNumRtpPacketsTransmitted = quality.mNumRtpPacketsTransmitted;
    mNumRtpPacketsReceived = quality.mNumRtpPacketsReceived;
    mNumRtpPacketsTransmittedLost = quality.mNumRtpPacketsTransmittedLost;
    mNumRtpPacketsNotReceived = quality.mNumRtpPacketsNotReceived;
    mAverageRelativeJitter = quality.mAverageRelativeJitter;
    mMaxRelativeJitter = quality.mMaxRelativeJitter;
    mAverageRoundTripTime = quality.mAverageRoundTripTime;
    mCodecType = quality.mCodecType;
    mRtpInactivityDetected = quality.mRtpInactivityDetected;
    mRxSilenceDetected = quality.mRxSilenceDetected;
    mTxSilenceDetected = quality.mTxSilenceDetected;
    mNumVoiceFrames = quality.mNumVoiceFrames;
    mNumNoDataFrames = quality.mNumNoDataFrames;
    mNumDroppedRtpPackets = quality.mNumDroppedRtpPackets;
    mMinPlayoutDelayMillis = quality.mMinPlayoutDelayMillis;
    mMaxPlayoutDelayMillis = quality.mMaxPlayoutDelayMillis;
    mNumRtpSidPacketsReceived = quality.mNumRtpSidPacketsReceived;
    mNumRtpDuplicatePackets = quality.mNumRtpDuplicatePackets;
}

MediaQuality::~MediaQuality() {}

MediaQuality& MediaQuality::operator=(const MediaQuality& quality)
{
    if (this != &quality)
    {
        mDownlinkCallQualityLevel = quality.mDownlinkCallQualityLevel;
        mUplinkCallQualityLevel = quality.mUplinkCallQualityLevel;
        mCallDuration = quality.mCallDuration;
        mNumRtpPacketsTransmitted = quality.mNumRtpPacketsTransmitted;
        mNumRtpPacketsReceived = quality.mNumRtpPacketsReceived;
        mNumRtpPacketsTransmittedLost = quality.mNumRtpPacketsTransmittedLost;
        mNumRtpPacketsNotReceived = quality.mNumRtpPacketsNotReceived;
        mAverageRelativeJitter = quality.mAverageRelativeJitter;
        mMaxRelativeJitter = quality.mMaxRelativeJitter;
        mAverageRoundTripTime = quality.mAverageRoundTripTime;
        mCodecType = quality.mCodecType;
        mRtpInactivityDetected = quality.mRtpInactivityDetected;
        mRxSilenceDetected = quality.mRxSilenceDetected;
        mTxSilenceDetected = quality.mTxSilenceDetected;
        mNumVoiceFrames = quality.mNumVoiceFrames;
        mNumNoDataFrames = quality.mNumNoDataFrames;
        mNumDroppedRtpPackets = quality.mNumDroppedRtpPackets;
        mMinPlayoutDelayMillis = quality.mMinPlayoutDelayMillis;
        mMaxPlayoutDelayMillis = quality.mMaxPlayoutDelayMillis;
        mNumRtpSidPacketsReceived = quality.mNumRtpSidPacketsReceived;
        mNumRtpDuplicatePackets = quality.mNumRtpDuplicatePackets;
    }
    return *this;
}

bool MediaQuality::operator==(const MediaQuality& quality) const
{
    return (mDownlinkCallQualityLevel == quality.mDownlinkCallQualityLevel &&
            mUplinkCallQualityLevel == quality.mUplinkCallQualityLevel &&
            mCallDuration == quality.mCallDuration &&
            mNumRtpPacketsTransmitted == quality.mNumRtpPacketsTransmitted &&
            mNumRtpPacketsReceived == quality.mNumRtpPacketsReceived &&
            mNumRtpPacketsTransmittedLost == quality.mNumRtpPacketsTransmittedLost &&
            mNumRtpPacketsNotReceived == quality.mNumRtpPacketsNotReceived &&
            mAverageRelativeJitter == quality.mAverageRelativeJitter &&
            mMaxRelativeJitter == quality.mMaxRelativeJitter &&
            mAverageRoundTripTime == quality.mAverageRoundTripTime &&
            mCodecType == quality.mCodecType &&
            mRtpInactivityDetected == quality.mRtpInactivityDetected &&
            mRxSilenceDetected == quality.mRxSilenceDetected &&
            mTxSilenceDetected == quality.mTxSilenceDetected &&
            mNumVoiceFrames == quality.mNumVoiceFrames &&
            mNumNoDataFrames == quality.mNumNoDataFrames &&
            mNumDroppedRtpPackets == quality.mNumDroppedRtpPackets &&
            mMinPlayoutDelayMillis == quality.mMinPlayoutDelayMillis &&
            mMaxPlayoutDelayMillis == quality.mMaxPlayoutDelayMillis &&
            mNumRtpSidPacketsReceived == quality.mNumRtpSidPacketsReceived &&
            mNumRtpDuplicatePackets == quality.mNumRtpDuplicatePackets);
}

bool MediaQuality::operator!=(const MediaQuality& quality) const
{
    return (mDownlinkCallQualityLevel != quality.mDownlinkCallQualityLevel ||
            mUplinkCallQualityLevel != quality.mUplinkCallQualityLevel ||
            mCallDuration != quality.mCallDuration ||
            mNumRtpPacketsTransmitted != quality.mNumRtpPacketsTransmitted ||
            mNumRtpPacketsReceived != quality.mNumRtpPacketsReceived ||
            mNumRtpPacketsTransmittedLost != quality.mNumRtpPacketsTransmittedLost ||
            mNumRtpPacketsNotReceived != quality.mNumRtpPacketsNotReceived ||
            mAverageRelativeJitter != quality.mAverageRelativeJitter ||
            mMaxRelativeJitter != quality.mMaxRelativeJitter ||
            mAverageRoundTripTime != quality.mAverageRoundTripTime ||
            mCodecType != quality.mCodecType ||
            mRtpInactivityDetected != quality.mRtpInactivityDetected ||
            mRxSilenceDetected != quality.mRxSilenceDetected ||
            mTxSilenceDetected != quality.mTxSilenceDetected ||
            mNumVoiceFrames != quality.mNumVoiceFrames ||
            mNumNoDataFrames != quality.mNumNoDataFrames ||
            mNumDroppedRtpPackets != quality.mNumDroppedRtpPackets ||
            mMinPlayoutDelayMillis != quality.mMinPlayoutDelayMillis ||
            mMaxPlayoutDelayMillis != quality.mMaxPlayoutDelayMillis ||
            mNumRtpSidPacketsReceived != quality.mNumRtpSidPacketsReceived ||
            mNumRtpDuplicatePackets != quality.mNumRtpDuplicatePackets);
}

status_t MediaQuality::writeToParcel(Parcel* parcel) const
{
    status_t err;
    err = parcel->writeInt32(mDownlinkCallQualityLevel);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = parcel->writeInt32(mUplinkCallQualityLevel);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = parcel->writeInt32(mCallDuration);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = parcel->writeInt32(mNumRtpPacketsTransmitted);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = parcel->writeInt32(mNumRtpPacketsReceived);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = parcel->writeInt32(mNumRtpPacketsTransmittedLost);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = parcel->writeInt32(mNumRtpPacketsNotReceived);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = parcel->writeInt32(mAverageRelativeJitter);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = parcel->writeInt32(mMaxRelativeJitter);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = parcel->writeInt32(mAverageRoundTripTime);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = parcel->writeInt32(mCodecType);
    if (err != NO_ERROR)
    {
        return err;
    }

    int32_t value = 0;
    mRtpInactivityDetected ? value = 1 : value = 0;
    err = parcel->writeInt32(value);
    if (err != NO_ERROR)
    {
        return err;
    }

    mRxSilenceDetected ? value = 1 : value = 0;
    err = parcel->writeInt32(value);
    if (err != NO_ERROR)
    {
        return err;
    }

    mTxSilenceDetected ? value = 1 : value = 0;
    err = parcel->writeInt32(value);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = parcel->writeInt32(mNumVoiceFrames);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = parcel->writeInt32(mNumNoDataFrames);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = parcel->writeInt32(mNumDroppedRtpPackets);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = parcel->writeInt64(mMinPlayoutDelayMillis);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = parcel->writeInt64(mMaxPlayoutDelayMillis);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = parcel->writeInt32(mNumRtpSidPacketsReceived);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = parcel->writeInt32(mNumRtpDuplicatePackets);
    if (err != NO_ERROR)
    {
        return err;
    }

    return NO_ERROR;
}

status_t MediaQuality::readFromParcel(const Parcel* in)
{
    status_t err;
    err = in->readInt32(&mDownlinkCallQualityLevel);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mUplinkCallQualityLevel);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mCallDuration);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mNumRtpPacketsTransmitted);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mNumRtpPacketsReceived);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mNumRtpPacketsTransmittedLost);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mNumRtpPacketsNotReceived);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mAverageRelativeJitter);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mMaxRelativeJitter);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mAverageRoundTripTime);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mCodecType);
    if (err != NO_ERROR)
    {
        return err;
    }

    int32_t value = 0;
    err = in->readInt32(&value);
    if (err != NO_ERROR)
    {
        return err;
    }

    value == 0 ? mRtpInactivityDetected = false : mRtpInactivityDetected = true;

    err = in->readInt32(&value);
    if (err != NO_ERROR)
    {
        return err;
    }

    value == 0 ? mRxSilenceDetected = false : mRxSilenceDetected = true;

    err = in->readInt32(&value);
    if (err != NO_ERROR)
    {
        return err;
    }

    value == 0 ? mTxSilenceDetected = false : mTxSilenceDetected = true;

    err = in->readInt32(&mNumVoiceFrames);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mNumNoDataFrames);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mNumDroppedRtpPackets);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt64(&mMinPlayoutDelayMillis);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt64(&mMaxPlayoutDelayMillis);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mNumRtpSidPacketsReceived);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mNumRtpDuplicatePackets);
    if (err != NO_ERROR)
    {
        return err;
    }

    return NO_ERROR;
}

int MediaQuality::getDownlinkCallQualityLevel()
{
    return mDownlinkCallQualityLevel;
}

void MediaQuality::setDownlinkCallQualityLevel(const int level)
{
    mDownlinkCallQualityLevel = level;
}

int MediaQuality::getUplinkCallQualityLevel()
{
    return mUplinkCallQualityLevel;
}

void MediaQuality::setUplinkCallQualityLevel(const int level)
{
    mUplinkCallQualityLevel = level;
}

int MediaQuality::getCallDuration()
{
    return mCallDuration;
}

void MediaQuality::setCallDuration(const int duration)
{
    mCallDuration = duration;
}

int MediaQuality::getNumRtpPacketsTransmitted()
{
    return mNumRtpPacketsTransmitted;
}

void MediaQuality::setNumRtpPacketsTransmitted(const int num)
{
    mNumRtpPacketsTransmitted = num;
}

int MediaQuality::getNumRtpPacketsReceived()
{
    return mNumRtpPacketsReceived;
}

void MediaQuality::setNumRtpPacketsReceived(const int num)
{
    mNumRtpPacketsReceived = num;
}

int MediaQuality::getNumRtpPacketsTransmittedLost()
{
    return mNumRtpPacketsTransmittedLost;
}

void MediaQuality::setNumRtpPacketsTransmittedLost(const int num)
{
    mNumRtpPacketsTransmittedLost = num;
}

int MediaQuality::getNumRtpPacketsNotReceived()
{
    return mNumRtpPacketsNotReceived;
}

void MediaQuality::setNumRtpPacketsNotReceived(const int num)
{
    mNumRtpPacketsNotReceived = num;
}

int MediaQuality::getAverageRelativeJitter()
{
    return mAverageRelativeJitter;
}

void MediaQuality::setAverageRelativeJitter(const int jitter)
{
    mAverageRelativeJitter = jitter;
}

int MediaQuality::getMaxRelativeJitter()
{
    return mMaxRelativeJitter;
}

void MediaQuality::setMaxRelativeJitter(const int jitter)
{
    mMaxRelativeJitter = jitter;
}

int MediaQuality::getAverageRoundTripTime()
{
    return mAverageRoundTripTime;
}

void MediaQuality::setAverageRoundTripTime(const int time)
{
    mAverageRoundTripTime = time;
}

int MediaQuality::getCodecType()
{
    return mCodecType;
}

void MediaQuality::setCodecType(const int type)
{
    mCodecType = type;
}

bool MediaQuality::getRtpInactivityDetected()
{
    return mRtpInactivityDetected;
}

void MediaQuality::setRtpInactivityDetected(const bool detected)
{
    mRtpInactivityDetected = detected;
}

bool MediaQuality::getRxSilenceDetected()
{
    return mRxSilenceDetected;
}

void MediaQuality::setRxSilenceDetected(const bool detected)
{
    mRxSilenceDetected = detected;
}

bool MediaQuality::getTxSilenceDetected()
{
    return mTxSilenceDetected;
}

void MediaQuality::setTxSilenceDetected(const bool detected)
{
    mTxSilenceDetected = detected;
}

int MediaQuality::getNumVoiceFrames()
{
    return mNumVoiceFrames;
}

void MediaQuality::setNumVoiceFrames(const int num)
{
    mNumVoiceFrames = num;
}

int MediaQuality::getNumNoDataFrames()
{
    return mNumNoDataFrames;
}

void MediaQuality::setNumNoDataFrames(const int num)
{
    mNumNoDataFrames = num;
}

int MediaQuality::getNumDroppedRtpPackets()
{
    return mNumDroppedRtpPackets;
}

void MediaQuality::setNumDroppedRtpPackets(const int num)
{
    mNumDroppedRtpPackets = num;
}

int64_t MediaQuality::getMinPlayoutDelayMillis()
{
    return mMinPlayoutDelayMillis;
}

void MediaQuality::setMinPlayoutDelayMillis(const int64_t delay)
{
    mMinPlayoutDelayMillis = delay;
}

int64_t MediaQuality::getMaxPlayoutDelayMillis()
{
    return mMaxPlayoutDelayMillis;
}

void MediaQuality::setMaxPlayoutDelayMillis(const int64_t delay)
{
    mMaxPlayoutDelayMillis = delay;
}

int MediaQuality::getNumRtpSidPacketsReceived()
{
    return mNumRtpSidPacketsReceived;
}

void MediaQuality::setNumRtpSidPacketsReceived(const int num)
{
    mNumRtpSidPacketsReceived = num;
}

int MediaQuality::getNumRtpDuplicatePackets()
{
    return mNumRtpDuplicatePackets;
}

void MediaQuality::setNumRtpDuplicatePackets(const int num)
{
    mNumRtpDuplicatePackets = num;
}

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android