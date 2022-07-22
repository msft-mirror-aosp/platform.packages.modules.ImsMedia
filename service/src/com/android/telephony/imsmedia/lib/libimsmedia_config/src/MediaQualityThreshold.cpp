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

#include <MediaQualityThreshold.h>

namespace android
{

namespace telephony
{

namespace imsmedia
{

MediaQualityThreshold::MediaQualityThreshold()
{
    mRtpInactivityTimerMillis = 0;
    mRtcpInactivityTimerMillis = 0;
    mRtpPacketLossDurationMillis = 0;
    mRtpPacketLossRate = 0;
    mJitterDurationMillis = 0;
    mRtpJitterMillis = 0;
}

MediaQualityThreshold::MediaQualityThreshold(Parcel& in)
{
    mRtpInactivityTimerMillis = in.readInt32();
    mRtcpInactivityTimerMillis = in.readInt32();
    mRtpPacketLossDurationMillis = in.readInt32();
    mRtpPacketLossRate = in.readInt32();
    mJitterDurationMillis = in.readInt32();
    mRtpJitterMillis = in.readInt32();
}

MediaQualityThreshold::~MediaQualityThreshold() {}

MediaQualityThreshold& MediaQualityThreshold::operator=(const MediaQualityThreshold& threshold)
{
    mRtpInactivityTimerMillis = threshold.mRtpInactivityTimerMillis;
    mRtcpInactivityTimerMillis = threshold.mRtcpInactivityTimerMillis;
    mRtpPacketLossDurationMillis = threshold.mRtpPacketLossDurationMillis;
    mRtpPacketLossRate = threshold.mRtpPacketLossRate;
    mJitterDurationMillis = threshold.mJitterDurationMillis;
    mRtpJitterMillis = threshold.mRtpJitterMillis;
    return *this;
}

bool MediaQualityThreshold::operator==(const MediaQualityThreshold& threshold) const
{
    return (mRtpInactivityTimerMillis == threshold.mRtpInactivityTimerMillis &&
            mRtcpInactivityTimerMillis == threshold.mRtcpInactivityTimerMillis &&
            mRtpPacketLossDurationMillis == threshold.mRtpPacketLossDurationMillis &&
            mRtpPacketLossRate == threshold.mRtpPacketLossRate &&
            mJitterDurationMillis == threshold.mJitterDurationMillis &&
            mRtpJitterMillis == threshold.mRtpJitterMillis);
}

bool MediaQualityThreshold::operator!=(const MediaQualityThreshold& threshold) const
{
    return (mRtpInactivityTimerMillis != threshold.mRtpInactivityTimerMillis ||
            mRtcpInactivityTimerMillis != threshold.mRtcpInactivityTimerMillis ||
            mRtpPacketLossDurationMillis != threshold.mRtpPacketLossDurationMillis ||
            mRtpPacketLossRate != threshold.mRtpPacketLossRate ||
            mJitterDurationMillis != threshold.mJitterDurationMillis ||
            mRtpJitterMillis != threshold.mRtpJitterMillis);
}

status_t MediaQualityThreshold::writeToParcel(Parcel* out) const
{
    status_t err;
    err = out->writeInt32(mRtpInactivityTimerMillis);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = out->writeInt32(mRtcpInactivityTimerMillis);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = out->writeInt32(mRtpPacketLossDurationMillis);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = out->writeInt32(mRtpPacketLossRate);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = out->writeInt32(mJitterDurationMillis);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = out->writeInt32(mRtpJitterMillis);
    if (err != NO_ERROR)
    {
        return err;
    }

    return NO_ERROR;
}

status_t MediaQualityThreshold::readFromParcel(const Parcel* in)
{
    status_t err;
    err = in->readInt32(&mRtpInactivityTimerMillis);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mRtcpInactivityTimerMillis);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mRtpPacketLossDurationMillis);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mRtpPacketLossRate);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mJitterDurationMillis);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&mRtpJitterMillis);
    if (err != NO_ERROR)
    {
        return err;
    }

    return NO_ERROR;
}

void MediaQualityThreshold::setRtpInactivityTimerMillis(int32_t time)
{
    mRtpInactivityTimerMillis = time;
}

int32_t MediaQualityThreshold::getRtpInactivityTimerMillis()
{
    return mRtpInactivityTimerMillis;
}

void MediaQualityThreshold::setRtcpInactivityTimerMillis(int32_t time)
{
    mRtcpInactivityTimerMillis = time;
}

int32_t MediaQualityThreshold::getRtcpInactivityTimerMillis()
{
    return mRtcpInactivityTimerMillis;
}

void MediaQualityThreshold::setRtpPacketLossDurationMillis(int32_t time)
{
    mRtpPacketLossDurationMillis = time;
}

int32_t MediaQualityThreshold::getRtpPacketLossDurationMillis()
{
    return mRtpPacketLossDurationMillis;
}

void MediaQualityThreshold::setRtpPacketLossRate(int32_t rate)
{
    mRtpPacketLossRate = rate;
}

int32_t MediaQualityThreshold::getRtpPacketLossRate()
{
    return mRtpPacketLossRate;
}

void MediaQualityThreshold::setJitterDurationMillis(int32_t rate)
{
    mJitterDurationMillis = rate;
}

int32_t MediaQualityThreshold::getJitterDurationMillis()
{
    return mJitterDurationMillis;
}

void MediaQualityThreshold::setRtpJitterMillis(int32_t jitter)
{
    mRtpJitterMillis = jitter;
}

int32_t MediaQualityThreshold::getRtpJitterMillis()
{
    return mRtpJitterMillis;
}

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android