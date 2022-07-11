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

#include <stdio.h>
#include <sys/time.h>
// #include <sys/timeb.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>

#include <ImsMediaDefine.h>
#include <ImsMediaTrace.h>
#include <ImsMediaTimer.h>
#include <ImsMediaAudioFmt.h>
#include <ImsMediaAudioPlayer.h>
#include <utils/Errors.h>

#define AAUDIO_TIMEOUT_NANO   100 * 1000000L
#define DEFAULT_SAMPLING_RATE 8000

using namespace android;

ImsMediaAudioPlayer::ImsMediaAudioPlayer()
{
    mAudioStream = NULL;
    mCodec = NULL;
    mSamplingRate = DEFAULT_SAMPLING_RATE;
    mEvsCodecHeaderMode = kRtpPyaloadHeaderModeEvsHeaderFull;
}

ImsMediaAudioPlayer::~ImsMediaAudioPlayer() {}

void ImsMediaAudioPlayer::SetCodec(int32_t type)
{
    IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO, "[SetCodec] type[%d]", type);
    mCodecType = type;
}

void ImsMediaAudioPlayer::SetEvsBitRate(uint32_t mode)
{
    mEvsBitRate = ImsMediaAudioFmt::ConvertEVSModeToBitRate(mode);
    IMLOGD1("[SetEvsBitRate] EvsBitRate[%d]", mEvsBitRate);
}

void ImsMediaAudioPlayer::SetEvsChAwOffset(int32_t offset)
{
    mEvsChAwOffset = offset;
}

void ImsMediaAudioPlayer::SetSamplingRate(int32_t samplingRate)
{
    mSamplingRate = samplingRate;
}

void ImsMediaAudioPlayer::SetEvsBandwidth(kEvsBandwidth evsBandwidth)
{
    mEvsBandwidth = evsBandwidth;
}

void ImsMediaAudioPlayer::SetEvsPayloadHeaderMode(kRtpPyaloadHeaderMode EvsPayloadHeaderMode)
{
    mEvsCodecHeaderMode = EvsPayloadHeaderMode;
}

bool ImsMediaAudioPlayer::Start()
{
    char kMimeType[128] = {'\0'};
    auto codecResult = 0;

    if (mCodecType == kAudioCodecAmr)
    {
        sprintf(kMimeType, "audio/3gpp");
    }
    else if (mCodecType == kAudioCodecAmrWb)
    {
        sprintf(kMimeType, "audio/amr-wb");
    }
    else if (mCodecType == kAudioCodecEvs)
    {
        sprintf(kMimeType, "audio/evs");
    }
    else
    {
        return false;
    }

    openAudioStream();

    if (mAudioStream == NULL)
    {
        IMLOGE0("[Start] create audio stream failed");
        return false;
    }

    IMLOGD1("[Start] Creating codec[%s]", kMimeType);

    if (mCodecType == kAudioCodecAmr || mCodecType == kAudioCodecAmrWb)
    {
        mFormat = AMediaFormat_new();
        AMediaFormat_setString(mFormat, AMEDIAFORMAT_KEY_MIME, kMimeType);
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_SAMPLE_RATE, mSamplingRate);
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_CHANNEL_COUNT, 1);

        mCodec = AMediaCodec_createDecoderByType(kMimeType);

        if (mCodec == NULL)
        {
            IMLOGE1("[Start] unable to create %s codec instance", kMimeType);
            AMediaFormat_delete(mFormat);
            mFormat = NULL;
            return false;
        }

        IMLOGD0("[Start] configure codec");
        codecResult = AMediaCodec_configure(mCodec, mFormat, NULL, NULL, 0);

        if (codecResult != AMEDIA_OK)
        {
            IMLOGE2("[Start] unable to configure[%s] codec - err[%d]", kMimeType, codecResult);
            AMediaCodec_delete(mCodec);
            mCodec = NULL;
            AMediaFormat_delete(mFormat);
            mFormat = NULL;
            return false;
        }
    }

    aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STARTING;
    aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
    auto result = AAudioStream_requestStart(mAudioStream);

    if (result != AAUDIO_OK)
    {
        IMLOGE1("[Start] Error start stream[%s]", AAudio_convertResultToText(result));
        if (mCodecType == kAudioCodecAmr || mCodecType == kAudioCodecAmrWb)
        {
            AMediaCodec_delete(mCodec);
            mCodec = NULL;
            AMediaFormat_delete(mFormat);
            mFormat = NULL;
        }
        return false;
    }

    result = AAudioStream_waitForStateChange(
            mAudioStream, inputState, &nextState, AAUDIO_TIMEOUT_NANO);

    if (result != AAUDIO_OK)
    {
        IMLOGE1("[Start] Error start stream[%s]", AAudio_convertResultToText(result));
        if (mCodecType == kAudioCodecAmr || mCodecType == kAudioCodecAmrWb)
        {
            AMediaCodec_delete(mCodec);
            mCodec = NULL;
            AMediaFormat_delete(mFormat);
            mFormat = NULL;
        }
        return false;
    }

    IMLOGD1("[Start] start stream state[%s]", AAudio_convertStreamStateToText(nextState));

    if (mCodecType == kAudioCodecAmr || mCodecType == kAudioCodecAmrWb)
    {
        codecResult = AMediaCodec_start(mCodec);
        if (codecResult != AMEDIA_OK)
        {
            IMLOGE1("[Start] unable to start codec - err[%d]", codecResult);
            AMediaCodec_delete(mCodec);
            mCodec = NULL;
            AMediaFormat_delete(mFormat);
            mFormat = NULL;
            return false;
        }
    }

    IMLOGD0("[Start] exit");
    return true;
}

void ImsMediaAudioPlayer::Stop()
{
    IMLOGD0("[Stop] enter");
    std::lock_guard<std::mutex> guard(mMutex);
    if ((mCodecType == kAudioCodecAmr || mCodecType == kAudioCodecAmrWb) && (mCodec != NULL))
    {
        AMediaCodec_stop(mCodec);
        AMediaCodec_delete(mCodec);
        mCodec = NULL;
    }

    if ((mCodecType == kAudioCodecAmr || mCodecType == kAudioCodecAmrWb) && (mFormat != NULL))
    {
        AMediaFormat_delete(mFormat);
        mFormat = NULL;
    }

    aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STOPPING;
    aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
    aaudio_result_t result = AAudioStream_requestStop(mAudioStream);
    result = AAudioStream_waitForStateChange(
            mAudioStream, inputState, &nextState, AAUDIO_TIMEOUT_NANO);

    if (result != AAUDIO_OK)
    {
        IMLOGE1("[Stop] Error stop stream[%s]", AAudio_convertResultToText(result));
    }

    IMLOGD1("[Stop] Stop stream state[%s]", AAudio_convertStreamStateToText(nextState));

    if (mAudioStream != NULL)
    {
        AAudioStream_close(mAudioStream);
    }

    mAudioStream = NULL;
    IMLOGD0("[Stop] exit ");
}

bool ImsMediaAudioPlayer::onDataFrame(uint8_t* buffer, uint32_t size)
{
    std::lock_guard<std::mutex> guard(mMutex);
    if ((mCodecType == kAudioCodecAmr || mCodecType == kAudioCodecAmrWb) &&
            (size == 0 || mCodec == NULL || mAudioStream == NULL || buffer == NULL ||
                    AAudioStream_getState(mAudioStream) != AAUDIO_STREAM_STATE_STARTED))
    {
        return false;
    }

    static int kTimeout = 100000;  // be responsive on signal
    auto index = AMediaCodec_dequeueInputBuffer(mCodec, kTimeout);

    if (index >= 0)
    {
        size_t bufferSize = 0;
        uint8_t* inputBuffer = AMediaCodec_getInputBuffer(mCodec, index, &bufferSize);
        if (inputBuffer != NULL)
        {
            memcpy(inputBuffer, buffer, size);
            IMLOGD_PACKET2(IM_PACKET_LOG_AUDIO,
                    "[onDataFrame] queue input buffer index[%d], size[%d]", index, size);

            auto err = AMediaCodec_queueInputBuffer(
                    mCodec, index, 0, size, ImsMediaTimer::GetTimeInMicroSeconds(), 0);
            if (err != AMEDIA_OK)
            {
                IMLOGE1("[onDataFrame] Unable to queue input buffers - err[%d]", err);
            }
        }
    }
    else
    {
        IMLOGE1("[onDataFrame] Unable to get input buffers - err[%d]", index);
    }

    AMediaCodecBufferInfo info;
    index = AMediaCodec_dequeueOutputBuffer(mCodec, &info, kTimeout);

    if (index >= 0)
    {
        IMLOGD_PACKET5(IM_PACKET_LOG_AUDIO,
                "[onDataFrame] index[%d], size[%d], offset[%d], time[%ld], flags[%d]", index,
                info.size, info.offset, info.presentationTimeUs, info.flags);

        if (info.size > 0)
        {
            size_t buffCapacity;
            uint8_t* buf = AMediaCodec_getOutputBuffer(mCodec, index, &buffCapacity);
            if (buf != NULL && buffCapacity > 0)
            {
                memcpy(mBuffer, buf, info.size);
                // call audio write
                AAudioStream_write(mAudioStream, mBuffer, info.size / 2, 0);
            }
        }

        AMediaCodec_releaseOutputBuffer(mCodec, index, false);
    }
    else if (index == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED)
    {
        IMLOGD0("[onDataFrame] output buffer changed");
    }
    else if (index == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED)
    {
        if (mFormat != NULL)
        {
            AMediaFormat_delete(mFormat);
        }
        mFormat = AMediaCodec_getOutputFormat(mCodec);
        IMLOGD1("[onDataFrame] format changed, format[%s]", AMediaFormat_toString(mFormat));
    }
    else if (index == AMEDIACODEC_INFO_TRY_AGAIN_LATER)
    {
        IMLOGD0("[onDataFrame] no output buffer");
    }
    else
    {
        IMLOGD1("[onDataFrame] unexpected index[%d]", index);
    }

    return false;
}

void ImsMediaAudioPlayer::openAudioStream()
{
    AAudioStreamBuilder* builder = NULL;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);

    if (result != AAUDIO_OK)
    {
        IMLOGE1("[openAudioStream] Error creating stream builder[%s]",
                AAudio_convertResultToText(result));
        return;
    }

    // setup builder
    AAudioStreamBuilder_setInputPreset(builder, AAUDIO_INPUT_PRESET_VOICE_COMMUNICATION);
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setChannelCount(builder, 1);
    AAudioStreamBuilder_setSampleRate(builder, mSamplingRate);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_EXCLUSIVE);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setUsage(builder, AAUDIO_USAGE_VOICE_COMMUNICATION);
    AAudioStreamBuilder_setErrorCallback(builder, audioErrorCallback, this);

    // open stream
    result = AAudioStreamBuilder_openStream(builder, &mAudioStream);
    AAudioStreamBuilder_delete(builder);

    if (result != AAUDIO_OK)
    {
        IMLOGE1("[openAudioStream] Failed to openStream. Error[%s]",
                AAudio_convertResultToText(result));
        if (mAudioStream != NULL)
        {
            AAudioStream_close(mAudioStream);
        }
        mAudioStream = NULL;
    }
}

void ImsMediaAudioPlayer::restartAudioStream()
{
    std::lock_guard<std::mutex> guard(mMutex);

    if (mAudioStream == NULL)
    {
        return;
    }

    AAudioStream_requestStop(mAudioStream);
    AAudioStream_close(mAudioStream);

    mAudioStream = NULL;
    openAudioStream();

    if (mAudioStream == NULL)
    {
        return;
    }

    aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STARTING;
    aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
    aaudio_result_t result = AAudioStream_requestStart(mAudioStream);
    if (result != AAUDIO_OK)
    {
        IMLOGE1("[restartAudioStream] Error start stream[%s]", AAudio_convertResultToText(result));
        return;
    }

    result = AAudioStream_waitForStateChange(
            mAudioStream, inputState, &nextState, 3 * AAUDIO_TIMEOUT_NANO);

    if (result != AAUDIO_OK)
    {
        IMLOGE1("[restartAudioStream] Error start stream[%s]", AAudio_convertResultToText(result));
        return;
    }

    IMLOGD1("[restartAudioStream] start stream state[%s]",
            AAudio_convertStreamStateToText(nextState));
}

void ImsMediaAudioPlayer::audioErrorCallback(
        AAudioStream* stream, void* userData, aaudio_result_t error)
{
    if (stream == NULL || userData == NULL)
        return;

    aaudio_stream_state_t streamState = AAudioStream_getState(stream);
    IMLOGW2("[errorCallback] error[%s], state[%d]", AAudio_convertResultToText(error), streamState);

    if (streamState == AAUDIO_STREAM_STATE_DISCONNECTED)
    {
        // Handle stream restart on a separate thread
        std::thread streamRestartThread(&ImsMediaAudioPlayer::restartAudioStream,
                reinterpret_cast<ImsMediaAudioPlayer*>(userData));
        streamRestartThread.detach();
    }
}
