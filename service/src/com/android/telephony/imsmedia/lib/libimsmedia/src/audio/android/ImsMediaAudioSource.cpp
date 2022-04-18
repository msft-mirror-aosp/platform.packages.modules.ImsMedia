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
#include <gui/Surface.h>
#include <mediadrm/ICrypto.h>
#include <media/MediaCodecBuffer.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/ALooper.h>
#include <thread>

#define NANOS_PER_SECOND 1000000000L
#define AAUDIO_NANOS_PER_MILLISECOND 1000000L
#define NUM_FRAMES_PER_SEC 50

using android::status_t;
using android::ALooper;
using android::AMessage;
using android::AString;
using android::Vector;
using android::MediaCodecBuffer;

using namespace android;

#define AMR_BUFFER_SIZE 31
#define AMRWB_BUFFER_SIZE 60
#define DEFAULT_SAMPLING_RATE 8000

ImsMediaAudioSource::ImsMediaAudioSource() {
    mUplinkCB = NULL;
    mAudioStream = NULL;
    mBufferSize = 0;
    mSamplingRate = DEFAULT_SAMPLING_RATE;
}

ImsMediaAudioSource::~ImsMediaAudioSource() {
}

void ImsMediaAudioSource::SetUplinkCallback(void* pClient, AudioUplinkCB pUplinkCB) {
    std::lock_guard<std::mutex> guard(mMutexUplink);
    mUplinkCBClient = pClient;
    mUplinkCB = pUplinkCB;
}

void ImsMediaAudioSource::SetCodec(int32_t type) {
    IMLOGD1("[SetCodec] type[%d]", type);
    mCodecType = type;
}

void ImsMediaAudioSource::SetCodecMode(uint32_t mode) {
    IMLOGD1("[SetCodecMode] mode[%d]", mode);
    mMode = mode;
}

void ImsMediaAudioSource::SetPtime(uint32_t time) {
    IMLOGD1("[SetCodecMode] Ptime[%d]", time);
    mPtime = time;
}

bool ImsMediaAudioSource::Start() {
    status_t err;
    char kMimeType[128] = {'\0'};
    if (mCodecType == AUDIO_G711_PCMU || mCodecType == AUDIO_G711_PCMA) {
    } else if (mCodecType == AUDIO_AMR) {
        sprintf(kMimeType, "audio/3gpp");
    } else if (mCodecType == AUDIO_AMR_WB) {
        mSamplingRate = 16000;
        sprintf(kMimeType, "audio/amr-wb");
    }

    openAudioStream();

    if (mAudioStream == NULL) {
        IMLOGE0("[Start] create audio stream failed");
        return false;
    }

    sp<ALooper> looper = new ALooper;
    looper->setName("ImsMediaAudioSource");
    looper->start();
    IMLOGD1("[Start] Creating codec[%s]", kMimeType);

    mMediaCodec = MediaCodec::CreateByType(looper, kMimeType, true);
    if (mMediaCodec == NULL) {
        IMLOGE1("[Start] unable to create %s codec instance", kMimeType);
        return false;
    }

    constexpr char KEY_MIME[] = "mime";
    constexpr char KEY_SAMPLE_RATE[] = "sample-rate";
    constexpr char KEY_CHANNEL_COUNT[] = "channel-count";
    constexpr char KEY_BIT_RATE[] = "bitrate";

    sp<AMessage> format = new AMessage();
    format->setString(KEY_MIME, kMimeType);
    format->setInt32(KEY_SAMPLE_RATE, mSamplingRate);
    format->setInt32(KEY_CHANNEL_COUNT, 1);
    if (mCodecType == AUDIO_AMR) {
        format->setInt32(KEY_BIT_RATE, ImsMediaAudioFmt::GetBitrateAmr(mMode));
    } else if (mCodecType == AUDIO_AMR_WB) {
        format->setInt32(KEY_BIT_RATE, ImsMediaAudioFmt::GetBitrateAmrWb(mMode));
    }

    err = mMediaCodec->configure(format, NULL, NULL, MediaCodec::CONFIGURE_FLAG_ENCODE);
    if (err != NO_ERROR) {
        IMLOGE2("[Start] unable to configure[%s] codec - err[%d]", kMimeType, err);
        mMediaCodec->release();
        return false;
    }

    aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STARTING;
    aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
    int64_t timeoutNanos = 300 * AAUDIO_NANOS_PER_MILLISECOND;
    aaudio_result_t result = AAudioStream_requestStart(mAudioStream);
    if (result != AAUDIO_OK) {
        IMLOGE1("[Start] Error start stream[%s]", AAudio_convertResultToText(result));
        mMediaCodec->release();
        return false;
    }

    result = AAudioStream_waitForStateChange(mAudioStream, inputState, &nextState, timeoutNanos);
    if (result != AAUDIO_OK) {
        IMLOGE1("[Start] Error start stream[%s]", AAudio_convertResultToText(result));
        mMediaCodec->release();
        return false;
    }

    IMLOGD1("[Start] start stream state[%s]", AAudio_convertStreamStateToText(nextState));

    err = mMediaCodec->start();
    if (err != NO_ERROR) {
        IMLOGE1("[Start] unable to start codec - err[%d]", err);
        mMediaCodec->release();
        return false;
    }

    StartThread();

    std::thread t1(&ImsMediaAudioSource::processOutputBuffer, this);
    t1.detach();
    IMLOGD0("[Start] exit");
    return true;
}

void ImsMediaAudioSource::Stop() {
    IMLOGD0("[Stop] Enter");
    std::lock_guard<std::mutex> guard(mMutexUplink);
    StopThread();
    aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STOPPING;
    aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
    int64_t timeoutNanos = 100 * AAUDIO_NANOS_PER_MILLISECOND;
    aaudio_result_t result = AAudioStream_requestStop(mAudioStream);
    result = AAudioStream_waitForStateChange(mAudioStream, inputState, &nextState, timeoutNanos);
    if (result != AAUDIO_OK) {
        IMLOGE1("[Stop] Error stop stream[%s]", AAudio_convertResultToText(result));
    }

    IMLOGD1("[Stop] Stop stream state[%s]",
        AAudio_convertStreamStateToText(nextState));

    AAudioStream_close(mAudioStream);
    mAudioStream = NULL;

    if (mMediaCodec != NULL) {
        mMediaCodec->stop();
        mMediaCodec->release();
        mMediaCodec = NULL;
    }
    IMLOGD0("[Stop] Exit");
}

bool ImsMediaAudioSource::ProcessCMR(uint32_t mode) {
    (void)mode;
    return false;
    // do nothing
}

aaudio_data_callback_result_t ImsMediaAudioSource::uplinkCallback(AAudioStream *stream,
    void *userData, void *audioData, int32_t numFrames) {
    (void)stream;
    if (userData == NULL || audioData == NULL) return AAUDIO_CALLBACK_RESULT_STOP;
    IMLOGD1("[uplinkCallback] size[%d]", numFrames);
    ImsMediaAudioSource *voice = reinterpret_cast<ImsMediaAudioSource*>(userData);
    voice->queueInputBuffer(reinterpret_cast<uint16_t*>(audioData), numFrames);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void ImsMediaAudioSource::errorCallback(AAudioStream *stream, void *userData,
    aaudio_result_t error) {
    if (stream == NULL || userData == NULL) return;

    aaudio_stream_state_t streamState = AAudioStream_getState(stream);
    IMLOGW2("[errorCallback] error[%s], state[%d]", AAudio_convertResultToText(error), streamState);

    if (streamState == AAUDIO_STREAM_STATE_DISCONNECTED) {
        // Handle stream restart on a separate thread
        std::thread streamRestartThread(&ImsMediaAudioSource::restartAudioStream,
            reinterpret_cast<ImsMediaAudioSource*>(userData));
        streamRestartThread.detach();
    }
}

void* ImsMediaAudioSource::run() {
    IMLOGD0("[run] enter");
    uint32_t nNextTime = ImsMediaTimer::GetTimeInMilliSeconds();

    for (;;) {
        uint32_t nCurrTime;
        mMutexUplink.lock();
        if (IsThreadStopped()) {
            IMLOGD0("[run] terminated");
            mMutexUplink.unlock();
            break;
        }
        mMutexUplink.unlock();

        if (mAudioStream != NULL &&
            AAudioStream_getState(mAudioStream) == AAUDIO_STREAM_STATE_STARTED) {
            aaudio_result_t readSize = AAudioStream_read(mAudioStream, mBuffer, mBufferSize, 0);
            IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO, "[run] nReadSize[%d]", readSize);
            queueInputBuffer(mBuffer, readSize);
        }

        nNextTime += mPtime;
        nCurrTime = ImsMediaTimer::GetTimeInMilliSeconds();
        IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO, "[run] nCurrTime[%u]", nCurrTime);
        if (nNextTime > nCurrTime) ImsMediaTimer::Sleep(nNextTime - nCurrTime);
    }

    return NULL;
}

void ImsMediaAudioSource::openAudioStream() {
    AAudioStreamBuilder *builder = NULL;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        IMLOGE1("[openAudioStream] Error creating stream builder[%s]",
            AAudio_convertResultToText(result));
        return;
    }

    //setup builder
    AAudioStreamBuilder_setInputPreset(builder, AAUDIO_INPUT_PRESET_VOICE_COMMUNICATION);
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setChannelCount(builder, 1);
    AAudioStreamBuilder_setSampleRate(builder, mSamplingRate);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_EXCLUSIVE);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setUsage(builder, AAUDIO_USAGE_VOICE_COMMUNICATION);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, this);

    int numFramesPerSec = 0;
    mPtime == 0 ? numFramesPerSec = NUM_FRAMES_PER_SEC : numFramesPerSec = 1000 / mPtime;

    //open stream
    result = AAudioStreamBuilder_openStream(builder, &mAudioStream);
    AAudioStreamBuilder_delete(builder);

    if (result == AAUDIO_OK && mAudioStream != NULL) {
        mBufferSize = AAudioStream_getFramesPerBurst(mAudioStream);
        IMLOGD3("[openAudioStream] samplingRate[%d], framesPerBurst[%d], framesPerData[%d]",
            AAudioStream_getSampleRate(mAudioStream), mBufferSize,
            AAudioStream_getFramesPerDataCallback(mAudioStream));
        switch (AAudioStream_getPerformanceMode(mAudioStream)) {
            case AAUDIO_PERFORMANCE_MODE_LOW_LATENCY:
                IMLOGD0("[openAudioStream] getPerformanceMode : low latency");
                break;
            case AAUDIO_PERFORMANCE_MODE_POWER_SAVING:
                IMLOGD0("[openAudioStream] getPerformanceMode : power saving");
                break;
            default:
                break;
        }
        // Set the buffer size to the burst size - this will give us the minimum possible latency
        AAudioStream_setBufferSizeInFrames(mAudioStream, mBufferSize);
    } else {
        IMLOGE1("[openAudioStream] Failed to openStream. Error[%s]",
            AAudio_convertResultToText(result));
    }
}

void ImsMediaAudioSource::restartAudioStream() {
    std::lock_guard<std::mutex> guard(mMutexUplink);
    if (mAudioStream == NULL) return;

    AAudioStream_requestStop(mAudioStream);
    AAudioStream_close(mAudioStream);
    openAudioStream();

    if (mAudioStream == NULL) return;

    aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STARTING;
    aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
    int64_t timeoutNanos = 300 * AAUDIO_NANOS_PER_MILLISECOND;
    aaudio_result_t result = AAudioStream_requestStart(mAudioStream);
    if (result != AAUDIO_OK) {
        IMLOGE1("[restartAudioStream] Error start stream[%s]",
            AAudio_convertResultToText(result));
        return;
    }

    result = AAudioStream_waitForStateChange(mAudioStream, inputState, &nextState, timeoutNanos);
    if (result != AAUDIO_OK) {
        IMLOGE1("[restartAudioStream] Error start stream[%s]",
            AAudio_convertResultToText(result));
        return;
    }

    IMLOGD1("[restartAudioStream] start stream state[%s]",
        AAudio_convertStreamStateToText(nextState));
}

void ImsMediaAudioSource::queueInputBuffer(uint16_t* buffer, int32_t nSize) {
    std::lock_guard<std::mutex> guard(mMutexUplink);
    static int kTimeout = 100000;   // be responsive on signal
    status_t err;
    size_t bufIndex;

    if (nSize <= 0 || buffer == NULL || mMediaCodec == NULL) {
        return;
    }

    int byteSize = nSize * sizeof(buffer[0]);

    //process input buffer
    Vector<sp<MediaCodecBuffer>> inputBuffers;
    err = mMediaCodec->getInputBuffers(&inputBuffers);
    if (err == NO_ERROR) {
        err = mMediaCodec->dequeueInputBuffer(&bufIndex, kTimeout);

        if (err == NO_ERROR) {
            memcpy(inputBuffers[bufIndex]->data(), buffer, byteSize);
            IMLOGD_PACKET2(IM_PACKET_LOG_AUDIO,
                "[queueInputBuffer] queue input buffer index[%d], size[%d]", bufIndex, byteSize);

            err = mMediaCodec->queueInputBuffer(bufIndex, 0, byteSize,
                ImsMediaTimer::GetTimeInMicroSeconds(),
                MediaCodec::BUFFER_FLAG_SYNCFRAME);

            if (err != NO_ERROR) {
                IMLOGE1("[queueInputBuffer] Unable to get input buffers - err[%d]", err);
            }
        }
    } else {
        IMLOGE1("[queueInputBuffer] Unable to get input buffers - err[%d]", err);
    }
}

void ImsMediaAudioSource::processOutputBuffer() {
    size_t bufIndex, offset, size;
    static int kTimeout = 100000;   // be responsive on signal
    int64_t ptsUsec;
    uint32_t flags;
    status_t err;
    uint32_t nNextTime = ImsMediaTimer::GetTimeInMilliSeconds();

    for (;;) {
        uint32_t nCurrTime;
        mMutexUplink.lock();
        if (IsThreadStopped() || mAudioStream == NULL || mMediaCodec == NULL) {
            IMLOGD0("[processOutputBuffer] terminated");
            mMutexUplink.unlock();
            break;
        }
        mMutexUplink.unlock();

        //process output buffer
        Vector<sp<MediaCodecBuffer>> outputBuffers;
        err = mMediaCodec->getOutputBuffers(&outputBuffers);

        if (err == NO_ERROR) {
            err = mMediaCodec->dequeueOutputBuffer(&bufIndex, &offset, &size, &ptsUsec,
                    &flags, kTimeout);

            switch (err) {
                case NO_ERROR:
                    // got a buffer
                    if ((flags & MediaCodec::BUFFER_FLAG_CODECCONFIG) != 0) {
                        IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO,
                            "[processOutputBuffer] Got codec config buffer (%zu bytes)", size);
                    }

                    if (size != 0) {
                        IMLOGD_PACKET3(IM_PACKET_LOG_AUDIO,
                            "[processOutputBuffer] Got data in buffer[%zu], size[%zu], pts=%ld",
                            bufIndex, size, ptsUsec);

                        if (ptsUsec == 0) {
                            ptsUsec = ImsMediaTimer::GetTimeInMicroSeconds() / 1000;
                        }

                        //call encoding here
                        mUplinkCB(mUplinkCBClient, outputBuffers[bufIndex]->data(), size,
                            ptsUsec, flags);
                    }

                    err = mMediaCodec->releaseOutputBuffer(bufIndex);

                    if ((flags & MediaCodec::BUFFER_FLAG_EOS) != 0) {
                        IMLOGD_PACKET0(IM_PACKET_LOG_AUDIO,
                            "[processOutputBuffer] Received end-of-stream");
                    }
                    break;
                case -EAGAIN:                       // INFO_TRY_AGAIN_LATER
                    IMLOGD_PACKET0(IM_PACKET_LOG_AUDIO, "Got -EAGAIN, looping");
                    break;
                case android::INFO_FORMAT_CHANGED:    // INFO_OUTPUT_FORMAT_CHANGED
                    {
                        // Format includes CSD, which we must provide to muxer.
                        IMLOGD0("[processOutputBuffer] Encoder format changed");
                        //sp<AMessage> newFormat;
                        //mMediaCodec->getOutputFormat(&newFormat);
                    }
                    break;
                case android::INFO_OUTPUT_BUFFERS_CHANGED:   // INFO_OUTPUT_BUFFERS_CHANGED
                    // Not expected for an encoder; handle it anyway.
                    IMLOGD0("[processOutputBuffer] Encoder buffers changed");
                    err = mMediaCodec->getOutputBuffers(&outputBuffers);
                    if (err != NO_ERROR) {
                        IMLOGW1("[processOutputBuffer] Unable to get new output buffers - err[%d]",
                            err);
                    }
                    break;
                case INVALID_OPERATION:
                    IMLOGW0("[processOutputBuffer] dequeueOutputBuffer returned INVALID_OPERATION");
                    break;
                default:
                    IMLOGE1("[processOutputBuffer] Got weird result[%d] from dequeueOutputBuffer",
                        err);
                    break;
            }

            nNextTime += mPtime;
            nCurrTime = ImsMediaTimer::GetTimeInMilliSeconds();
            IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO, "[processOutputBuffer] nCurrTime[%u]", nCurrTime);
            if (nNextTime > nCurrTime) ImsMediaTimer::Sleep(nNextTime - nCurrTime);
        }
    }
}