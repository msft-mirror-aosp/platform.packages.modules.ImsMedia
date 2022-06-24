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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ImsMediaDefine.h>
#include <ImsMediaTimer.h>
#include <ImsMediaTrace.h>
#include <ImsMediaAudioFmt.h>
#include <ImsMediaAudioSource.h>
#include <utils/Errors.h>
#include <thread>

#define AAUDIO_TIMEOUT_NANO   100 * 1000000L
#define NUM_FRAMES_PER_SEC    50
#define DEFAULT_SAMPLING_RATE 8000

using namespace android;

ImsMediaAudioSource::ImsMediaAudioSource()
{
    mAudioStream = NULL;
    mCodec = NULL;
    mFormat = NULL;
    mUplinkCB = NULL;
    mUplinkCBClient = NULL;
    mCodecType = -1;
    mMode = 0;
    mPtime = 0;
    mSamplingRate = DEFAULT_SAMPLING_RATE;
    mBufferSize = 0;
    mEvsBandwidth = kEvsBandwidthNone;
    mEvsBitRate = 0;
    mEvsChAwOffset = 0;
}

ImsMediaAudioSource::~ImsMediaAudioSource() {}

void ImsMediaAudioSource::SetUplinkCallback(void* pClient, AudioUplinkCB pUplinkCB)
{
    std::lock_guard<std::mutex> guard(mMutexUplink);
    mUplinkCBClient = pClient;
    mUplinkCB = pUplinkCB;
}

void ImsMediaAudioSource::SetCodec(int32_t type)
{
    IMLOGD1("[SetCodec] type[%d]", type);
    mCodecType = type;
}

void ImsMediaAudioSource::SetCodecMode(uint32_t mode)
{
    IMLOGD1("[SetCodecMode] mode[%d]", mode);
    mMode = mode;
}

void ImsMediaAudioSource::SetEvsBitRate()
{
    mEvsBitRate = ImsMediaAudioFmt::ConvertEVSModeToBitRate(mMode);
    IMLOGD1("[SetEvsBitRate] EvsBitRate[%d]", mEvsBitRate);
}

void ImsMediaAudioSource::SetSamplingRate(int32_t samplingRate)
{
    mSamplingRate = samplingRate;
}

void ImsMediaAudioSource::SetEvsChAwOffset(int32_t offset)
{
    mEvsChAwOffset = offset;
}

void ImsMediaAudioSource::SetPtime(uint32_t time)
{
    IMLOGD1("[SetPtime] Ptime[%d]", time);
    mPtime = time;
}

void ImsMediaAudioSource::SetEvsBandwidth(kEvsBandwidth evsBandwidth)
{
    mEvsBandwidth = evsBandwidth;
}

bool ImsMediaAudioSource::Start()
{
    char kMimeType[128] = {'\0'};
    auto codecResult = 0;
    int amrBitrate;

    if (mCodecType == kAudioCodecAmr)
    {
        sprintf(kMimeType, "audio/3gpp");
        amrBitrate = ImsMediaAudioFmt::ConvertAmrModeToBitrate(mMode);
    }
    else if (mCodecType == kAudioCodecAmrWb)
    {
        sprintf(kMimeType, "audio/amr-wb");
        amrBitrate = ImsMediaAudioFmt::ConvertAmrWbModeToBitrate(mMode);
    }
    else if (mCodecType == kAudioCodecEvs)
    {
        sprintf(kMimeType, "audio/evs");
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
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_BIT_RATE, amrBitrate);

        mCodec = AMediaCodec_createEncoderByType(kMimeType);

        if (mCodec == NULL)
        {
            IMLOGE1("[Start] unable to create %s codec instance", kMimeType);
            AMediaFormat_delete(mFormat);
            mFormat = NULL;
            return false;
        }

        IMLOGD0("[Start] configure codec");
        codecResult = AMediaCodec_configure(
                mCodec, mFormat, NULL, NULL, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
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
    auto audioResult = AAudioStream_requestStart(mAudioStream);
    if (audioResult != AAUDIO_OK)
    {
        IMLOGE1("[Start] Error start stream[%s]", AAudio_convertResultToText(audioResult));
        if (mCodecType == kAudioCodecAmr || mCodecType == kAudioCodecAmrWb)
        {
            AMediaCodec_delete(mCodec);
            mCodec = NULL;
            AMediaFormat_delete(mFormat);
            mFormat = NULL;
        }
        return false;
    }

    aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STARTING;
    aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
    audioResult = AAudioStream_waitForStateChange(
            mAudioStream, inputState, &nextState, 10 * AAUDIO_TIMEOUT_NANO);
    if (audioResult != AAUDIO_OK)
    {
        IMLOGE1("[Start] Error start stream[%s]", AAudio_convertResultToText(audioResult));
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

    // start audio read thread
    StartThread();

    // start encoder output thread
    if (mCodecType == kAudioCodecAmr || mCodecType == kAudioCodecAmrWb)
    {
        std::thread t1(&ImsMediaAudioSource::processOutputBuffer, this);
        t1.detach();
        IMLOGD0("[Start] exit");
    }
    return true;
}

void ImsMediaAudioSource::Stop()
{
    IMLOGD0("[Stop] Enter");
    mMutexUplink.lock();
    StopThread();
    mMutexUplink.unlock();
    mConditionExit.reset();
    mConditionExit.wait_timeout(100);

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
        mAudioStream = NULL;
    }

    if (mCodec != NULL)
    {
        AMediaCodec_stop(mCodec);
        AMediaCodec_delete(mCodec);
        mCodec = NULL;
    }

    if (mFormat != NULL)
    {
        AMediaFormat_delete(mFormat);
        mFormat = NULL;
    }

    IMLOGD0("[Stop] Exit");
}

bool ImsMediaAudioSource::ProcessCMR(uint32_t mode)
{
    (void)mode;
    return false;
    // do nothing
}

void ImsMediaAudioSource::audioErrorCallback(
        AAudioStream* stream, void* userData, aaudio_result_t error)
{
    if (stream == NULL || userData == NULL)
        return;

    aaudio_stream_state_t streamState = AAudioStream_getState(stream);
    IMLOGW2("[errorCallback] error[%s], state[%d]", AAudio_convertResultToText(error), streamState);

    if (streamState == AAUDIO_STREAM_STATE_DISCONNECTED)
    {
        // Handle stream restart on a separate thread
        std::thread streamRestartThread(&ImsMediaAudioSource::restartAudioStream,
                reinterpret_cast<ImsMediaAudioSource*>(userData));
        streamRestartThread.detach();
    }
}

void* ImsMediaAudioSource::run()
{
    IMLOGD0("[run] enter");
    uint32_t nNextTime = ImsMediaTimer::GetTimeInMilliSeconds();
    int16_t buffer[PCM_BUFFER_SIZE];

    for (;;)
    {
        uint32_t nCurrTime;
        mMutexUplink.lock();
        if (IsThreadStopped())
        {
            IMLOGD0("[run] terminated");
            mMutexUplink.unlock();
            break;
        }
        mMutexUplink.unlock();

        if (mAudioStream != NULL &&
                AAudioStream_getState(mAudioStream) == AAUDIO_STREAM_STATE_STARTED)
        {
            aaudio_result_t readSize = AAudioStream_read(mAudioStream, buffer, mBufferSize, 0);
            if (readSize > 0)
            {
                IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO, "[run] nReadSize[%d]", readSize);
                queueInputBuffer(buffer, readSize * sizeof(uint16_t));
            }
        }

        nNextTime += mPtime;
        nCurrTime = ImsMediaTimer::GetTimeInMilliSeconds();
        IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO, "[run] nCurrTime[%u]", nCurrTime);
        if (nNextTime > nCurrTime)
            ImsMediaTimer::Sleep(nNextTime - nCurrTime);
    }

    return NULL;
}

void ImsMediaAudioSource::openAudioStream()
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
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setChannelCount(builder, 1);
    AAudioStreamBuilder_setSampleRate(builder, mSamplingRate);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_EXCLUSIVE);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setUsage(builder, AAUDIO_USAGE_VOICE_COMMUNICATION);
    AAudioStreamBuilder_setErrorCallback(builder, audioErrorCallback, this);
    AAudioStreamBuilder_setPrivacySensitive(builder, true);

    int numFramesPerSec = 0;
    mPtime == 0 ? numFramesPerSec = NUM_FRAMES_PER_SEC : numFramesPerSec = 1000 / mPtime;

    // open stream
    result = AAudioStreamBuilder_openStream(builder, &mAudioStream);
    AAudioStreamBuilder_delete(builder);

    if (result == AAUDIO_OK && mAudioStream != NULL)
    {
        mBufferSize = AAudioStream_getFramesPerBurst(mAudioStream);
        IMLOGD3("[openAudioStream] samplingRate[%d], framesPerBurst[%d], "
                "performanceMode[%d]",
                AAudioStream_getSampleRate(mAudioStream), mBufferSize,
                AAudioStream_getPerformanceMode(mAudioStream));
        // Set the buffer size to the burst size - this will give us the minimum
        // possible latency
        AAudioStream_setBufferSizeInFrames(mAudioStream, mBufferSize);
    }
    else
    {
        IMLOGE1("[openAudioStream] Failed to openStream. Error[%s]",
                AAudio_convertResultToText(result));
        mAudioStream = NULL;
    }
}

void ImsMediaAudioSource::restartAudioStream()
{
    std::lock_guard<std::mutex> guard(mMutexUplink);
    if (mAudioStream == NULL)
        return;

    AAudioStream_requestStop(mAudioStream);
    AAudioStream_close(mAudioStream);
    mAudioStream = NULL;
    openAudioStream();

    if (mAudioStream == NULL)
        return;

    aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STARTING;
    aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
    aaudio_result_t result = AAudioStream_requestStart(mAudioStream);
    if (result != AAUDIO_OK)
    {
        IMLOGE1("[restartAudioStream] Error start stream[%s]", AAudio_convertResultToText(result));
        return;
    }

    result = AAudioStream_waitForStateChange(
            mAudioStream, inputState, &nextState, 10 * AAUDIO_TIMEOUT_NANO);

    if (result != AAUDIO_OK)
    {
        IMLOGE1("[restartAudioStream] Error start stream[%s]", AAudio_convertResultToText(result));
        return;
    }

    IMLOGD1("[restartAudioStream] start stream state[%s]",
            AAudio_convertStreamStateToText(nextState));
}

void ImsMediaAudioSource::queueInputBuffer(int16_t* buffer, uint32_t size)
{
    ssize_t index = AMediaCodec_dequeueInputBuffer(mCodec, 0);
    if (index >= 0)
    {
        size_t bufferSize = 0;
        uint8_t* inputBuffer = AMediaCodec_getInputBuffer(mCodec, index, &bufferSize);
        if (inputBuffer != NULL)
        {
            memcpy(inputBuffer, buffer, size);
            IMLOGD_PACKET2(IM_PACKET_LOG_AUDIO,
                    "[queueInputBuffer] queue input buffer index[%d], size[%d]", index, size);

            auto err = AMediaCodec_queueInputBuffer(
                    mCodec, index, 0, size, ImsMediaTimer::GetTimeInMicroSeconds(), 0);
            if (err != AMEDIA_OK)
            {
                IMLOGE1("[queueInputBuffer] Unable to queue input buffers - err[%d]", err);
            }
        }
    }
}

void ImsMediaAudioSource::processOutputBuffer()
{
    static int kTimeout = 100000;  // be responsive on signal
    uint32_t nNextTime = ImsMediaTimer::GetTimeInMilliSeconds();

    for (;;)
    {
        uint32_t nCurrTime;
        mMutexUplink.lock();
        if (IsThreadStopped())
        {
            IMLOGD0("[processOutputBuffer] terminated");
            mMutexUplink.unlock();
            break;
        }
        mMutexUplink.unlock();

        AMediaCodecBufferInfo info;
        auto index = AMediaCodec_dequeueOutputBuffer(mCodec, &info, kTimeout);

        if (index >= 0)
        {
            IMLOGD_PACKET5(IM_PACKET_LOG_AUDIO,
                    "[processOutputBuffer] index[%d], size[%d], offset[%d], time[%ld],\
flags[%d]",
                    index, info.size, info.offset, info.presentationTimeUs, info.flags);

            if (info.size > 0)
            {
                size_t buffCapacity;
                uint8_t* buf = AMediaCodec_getOutputBuffer(mCodec, index, &buffCapacity);
                mUplinkCB(mUplinkCBClient, buf, info.size, info.presentationTimeUs, info.flags);
            }

            AMediaCodec_releaseOutputBuffer(mCodec, index, false);
        }
        else if (index == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED)
        {
            IMLOGD0("[processOutputBuffer] Encoder output buffer changed");
        }
        else if (index == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED)
        {
            if (mFormat != NULL)
            {
                AMediaFormat_delete(mFormat);
            }
            mFormat = AMediaCodec_getOutputFormat(mCodec);
            IMLOGD1("[processOutputBuffer] Encoder format changed, format[%s]",
                    AMediaFormat_toString(mFormat));
        }
        else if (index == AMEDIACODEC_INFO_TRY_AGAIN_LATER)
        {
            IMLOGD0("[processOutputBuffer] no output buffer");
        }
        else
        {
            IMLOGD1("[processOutputBuffer] unexpected index[%d]", index);
        }

        nNextTime += mPtime;
        nCurrTime = ImsMediaTimer::GetTimeInMilliSeconds();
        IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO, "[processOutputBuffer] nCurrTime[%u]", nCurrTime);
        if (nNextTime > nCurrTime)
            ImsMediaTimer::Sleep(nNextTime - nCurrTime);
    }

    mConditionExit.signal();
}
