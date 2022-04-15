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
//#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <ImsMediaDefine.h>
#include <ImsMediaTrace.h>
#include <ImsMediaTimer.h>
#include <ImsMediaAudioFmt.h>
#include <ImsMediaVoiceRenderer.h>
#include <utils/Errors.h>
#include <gui/Surface.h>
#include <mediadrm/ICrypto.h>
#include <media/MediaCodecBuffer.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/ALooper.h>

#define NANOS_PER_SECOND 1000000000L
#define AAUDIO_NANOS_PER_MILLISECOND 1000000L

using android::status_t;
using android::ALooper;
using android::AMessage;
using android::AString;
using android::Vector;
using android::MediaCodecBuffer;

#ifdef ANDROID_VOICE_INTERFACE_V2
#include <system/audio.h>
#endif

#ifdef CALL_AUDIOSYSTEM_SETPHONE_STATE
#include <AudioSystem.h>
#endif

using namespace android;

#define AMR_BUFFER_SIZE         31
#define AMRWB_BUFFER_SIZE       62

#define SAMPLING_RATE 8000
#define LOCAL_FRAME_COUNT   371 //480 //371 //4096            // SKT
//#define LOCAL_FRAME_COUNT   0 //371 //480 //371 //4096    // LGT
#define PCM_NOTI_COUNT             160
//#define AMR_NOTI_COUNT             31
//#define AMRWB_NOTI_COUNT           62

#ifdef DEBUG_PCM_DUMP
uint8_t *g_pPCMDump_tx; // location pointer for PCM Dump
int32_t g_iStoredDumpSize_tx; // the size of stored PCM Dump size
uint32_t g_amr_mode_tx;
#endif

ImsMediaVoiceRenderer::ImsMediaVoiceRenderer() {
    mAudioStream = NULL;
    mMediaCodec = NULL;

#ifdef CALL_AUDIOSYSTEM_SETPHONE_STATE
    AudioSystem::setCallModeInfo(2);
    AudioSystem::setPhoneState(AUDIO_MODE_VOIP_CALL);
#endif
}

ImsMediaVoiceRenderer::~ImsMediaVoiceRenderer() {
#ifdef CALL_AUDIOSYSTEM_SETPHONE_STATE
    AudioSystem::setCallModeInfo(0);
    AudioSystem::setPhoneState(AUDIO_MODE_NORMAL);
#endif
}

void ImsMediaVoiceRenderer::SetCodec(int32_t type) {
    IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO,
        "[SetCodec] type[%d]", type);
    mCodecType = type;
}

void ImsMediaVoiceRenderer::SetCodecMode(uint32_t mode) {
    IMLOGD1("[SetCodecMode] mode[%d]", mode);
    mCodecMode = mode;
#ifdef DEBUG_PCM_DUMP
    g_amr_mode_tx = mCodecMode;
#endif
}

bool ImsMediaVoiceRenderer::Start() {
    AAudioStreamBuilder *builder = NULL;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);

    if (result != AAUDIO_OK) {
        IMLOGE1("[Start] Error creating stream builder: %s", AAudio_convertResultToText(result));
        return false;
    }

    status_t err;

    char kMimeType[128] = {'\0'};
    int samplingRate = 8000;
    if (mCodecType == AUDIO_AMR) {
        sprintf(kMimeType, "audio/3gpp");
    } else if (mCodecType == AUDIO_AMR_WB) {
        samplingRate = 16000;
        sprintf(kMimeType, "audio/amr-wb");
    }

    //setup builder
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setChannelCount(builder, 1);
    AAudioStreamBuilder_setSampleRate(builder, samplingRate);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_EXCLUSIVE);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);

    //open stream
    result = AAudioStreamBuilder_openStream(builder, &mAudioStream);
    AAudioStreamBuilder_delete(builder);

    if (result == AAUDIO_OK && mAudioStream != NULL) {
        int bufferSize = AAudioStream_getFramesPerBurst(mAudioStream);
        IMLOGD3("[Start] samplingRate[%d], framesPerBurst[%d], framesPerData[%d]",
            AAudioStream_getSampleRate(mAudioStream), bufferSize,
            AAudioStream_getFramesPerDataCallback(mAudioStream));
        switch (AAudioStream_getPerformanceMode(mAudioStream)) {
            case AAUDIO_PERFORMANCE_MODE_LOW_LATENCY:
                IMLOGD0("[Start] getPerformanceMode : low latency");
                break;
            case AAUDIO_PERFORMANCE_MODE_POWER_SAVING:
                IMLOGD0("[Start] getPerformanceMode : power saving");
                break;
            default:
                break;
        }
        // Set the buffer size to the burst size - this will give us the minimum possible latency
        AAudioStream_setBufferSizeInFrames(mAudioStream, bufferSize);
    } else {
        IMLOGE1("[Start] Failed to openStream. Error[%s]", AAudio_convertResultToText(result));
        return false;
    }

    sp<ALooper> looper = new ALooper;
    looper->setName("ImsMediaVoiceRenderer");
    looper->start();
    IMLOGD1("[Start] Creating codec[%s]", kMimeType);

    mMediaCodec = MediaCodec::CreateByType(looper, kMimeType, false);
    if (mMediaCodec == NULL) {
        IMLOGE1("[Start] unable to create %s codec instance", kMimeType);
        return false;
    }

    constexpr char KEY_MIME[] = "mime";
    constexpr char KEY_SAMPLE_RATE[] = "sample-rate";
    constexpr char KEY_CHANNEL_COUNT[] = "channel-count";

    sp<AMessage> format = new AMessage();
    format->setString(KEY_MIME, kMimeType);
    format->setInt32(KEY_SAMPLE_RATE, samplingRate);
    format->setInt32(KEY_CHANNEL_COUNT, 1);

    err = mMediaCodec->configure(format, NULL, NULL, 0);
    if (err != NO_ERROR) {
        IMLOGE2("[Start] unable to configure[%s] codec - err[%d]", kMimeType, err);
        mMediaCodec->release();
        return false;
    }

    aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STARTING;
    aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
    int64_t timeoutNanos = 100 * AAUDIO_NANOS_PER_MILLISECOND;
    result = AAudioStream_requestStart(mAudioStream);
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

    IMLOGD0("[Start] exit");
    return true;
}

void ImsMediaVoiceRenderer::Stop() {
    IMLOGD0("[Stop] enter");
    if (mMediaCodec != NULL) {
        mMediaCodec->stop();
        mMediaCodec->release();
        mMediaCodec.clear();
        mMediaCodec = NULL;
    }

    aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STOPPING;
    aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
    int64_t timeoutNanos = 100 * AAUDIO_NANOS_PER_MILLISECOND;
    aaudio_result_t result = AAudioStream_requestStop(mAudioStream);
    result = AAudioStream_waitForStateChange(mAudioStream, inputState, &nextState, timeoutNanos);
    if (result != AAUDIO_OK) {
        IMLOGE1("[Stop] Error stop stream[%s]", AAudio_convertResultToText(result));
    }
    IMLOGD1("[Stop] Stop stream state[%s]", AAudio_convertStreamStateToText(nextState));
    AAudioStream_close(mAudioStream);
    mAudioStream = NULL;
    IMLOGD0("[Stop] exit ");
}

bool ImsMediaVoiceRenderer::onDataFrame(uint8_t* buffer, uint32_t nSize) {
    if (nSize == 0) return false;
    if (mMediaCodec == NULL) return false;
    if (mAudioStream == NULL || AAudioStream_getState(mAudioStream) != AAUDIO_STREAM_STATE_STARTED)
        return false;

    static int kTimeout = 250000;   // be responsive on signal
    status_t err;
    size_t bufIndex, offset, size;
    int64_t ptsUsec;
    uint32_t flags;

    //process input buffer
    Vector<sp<MediaCodecBuffer>> inputBuffers;
    err = mMediaCodec->getInputBuffers(&inputBuffers);
    if (err == NO_ERROR) {
        err = mMediaCodec->dequeueInputBuffer(&bufIndex, kTimeout);

        if (err == NO_ERROR) {
            memcpy(inputBuffers[bufIndex]->data(), buffer, nSize);
            IMLOGD_PACKET2(IM_PACKET_LOG_AUDIO,
                "[onDataFrame] queue input buffer index[%d], size[%d]", bufIndex, nSize);

            err = mMediaCodec->queueInputBuffer(bufIndex, 0, nSize,
                ImsMediaTimer::GetTimeInMicroSeconds(),
                MediaCodec::BUFFER_FLAG_SYNCFRAME);

            if (err != NO_ERROR) {
                IMLOGE1("[onDataFrame] Unable to get input buffers - err[%d]", err);
            }
        }
    } else {
        IMLOGE1("[onDataFrame] Unable to get input buffers - err[%d]", err);
    }

    //process output buffer
    Vector<sp<MediaCodecBuffer>> outputBuffers;
    err = mMediaCodec->getOutputBuffers(&outputBuffers);
    if (err != NO_ERROR) {
        IMLOGE1("[onDataFrame] Unable to get output buffers - err[%d]", err);
        return false;
    }

    err = mMediaCodec->dequeueOutputBuffer(&bufIndex, &offset, &size, &ptsUsec,
            &flags, kTimeout);

    switch (err) {
        case NO_ERROR:
            // got a buffer
            if ((flags & MediaCodec::BUFFER_FLAG_CODECCONFIG) != 0) {
                IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO,
                    "[onDataFrame] Got codec config buffer[%zd])", size);
            }

            if (size != 0) {
                IMLOGD_PACKET3(IM_PACKET_LOG_AUDIO,
                    "[onDataFrame] write audio data in buffer[%zu], size[%zu], pts[%" PRId64 "]",
                    bufIndex, size, ptsUsec);

                memcpy(mBuffer, outputBuffers[bufIndex]->data(), size);
                //call audio write
                AAudioStream_write(mAudioStream, mBuffer, size / 2, 0);
#ifdef DEBUG_PCM_DUMP
                if ((g_iStoredDumpSize_tx + size + 1)< PCM_DUMP_SIZE) {
                    unsigned char packet_header;
                    packet_header = (unsigned char)g_amr_mode_tx;
                    packet_header <<= 3;
                    packet_header |= 0x04;
                    memcpy(g_pPCMDump_tx, &packet_header, 1);
                    g_pPCMDump_tx += 1;
                    g_iStoredDumpSize_tx += 1;
                    memcpy(g_pPCMDump_tx, outputBuffers[bufIndex]->data(), size);
                    g_pPCMDump_tx += size;
                    g_iStoredDumpSize_tx += size;
                    IMLOGD1("[AudioDnLinkCb] g_iStoredDumpSize[%d]", g_iStoredDumpSize_tx);
                }
#endif
            }

            err = mMediaCodec->releaseOutputBuffer(bufIndex);

            if (err != NO_ERROR) {
                IMLOGE1("Unable to release output buffer - err[%d]", err);
            }

            if ((flags & MediaCodec::BUFFER_FLAG_EOS) != 0) {
                // Not expecting EOS from SurfaceFlinger.  Go with it.
                IMLOGD0("Received end-of-stream");
            }
            return true;
            break;
        case -EAGAIN:                       // INFO_TRY_AGAIN_LATER
            IMLOGD_PACKET0(IM_PACKET_LOG_AUDIO, "Got -EAGAIN, looping");
            break;
        case android::INFO_FORMAT_CHANGED:    // INFO_OUTPUT_FORMAT_CHANGED
            {
                // Format includes CSD, which we must provide to muxer.
                IMLOGD0("Decoder format changed");
                //sp<AMessage> newFormat;
                //mMediaCodec->getOutputFormat(&newFormat);
            }
            break;
        case android::INFO_OUTPUT_BUFFERS_CHANGED:   // INFO_OUTPUT_BUFFERS_CHANGED
            // Not expected for an encoder; handle it anyway.
            IMLOGD0("Decoder buffers changed");
            err = mMediaCodec->getOutputBuffers(&outputBuffers);
            if (err != NO_ERROR) {
                IMLOGE1("Unable to get new output buffers - err[%d]", err);
                return false;
            }
            break;
        case INVALID_OPERATION:
            IMLOGD0("dequeueOutputBuffer returned INVALID_OPERATION");
            return false;
        default:
            IMLOGE1("Got weird result[%d] from dequeueOutputBuffer", err);
            return false;
    }

    return false;
}