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
#include <ImsMediaDefine.h>
#include <ImsMediaTimer.h>
#include <ImsMediaTrace.h>
#include <ImsMediaAudioFmt.h>
#include <ImsMediaVoiceSource.h>
#include <utils/Errors.h>
#include <gui/Surface.h>
#include <mediadrm/ICrypto.h>
#include <media/MediaCodecBuffer.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/ALooper.h>
#include <thread>

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
#include <media/AudioSystem.h>
#endif

using namespace android;

#define AMR_BUFFER_SIZE 31
#define AMRWB_BUFFER_SIZE 60
#define SAMPLING_RATE 8000
#ifdef DEBUG_PCM_DUMP
uint8_t *g_pPCMDump; // location pointer for PCM Dump
int32_t g_iStoredDumpSize; // the size of stored PCM Dump size
uint32_t g_amr_mode;
#endif

ImsMediaVoiceSource::ImsMediaVoiceSource() {
    m_pUplinkCB = NULL;
    m_AudioRecord = NULL;
    mWavFile = NULL;
#ifdef CALL_AUDIOSYSTEM_SETPHONE_STATE
    AudioSystem::setCallModeInfo(2);
    AudioSystem::setPhoneState(AUDIO_MODE_VOIP_CALL);
#endif
}

ImsMediaVoiceSource::~ImsMediaVoiceSource() {
#ifdef CALL_AUDIOSYSTEM_SETPHONE_STATE
    AudioSystem::setCallModeInfo(0);
    AudioSystem::setPhoneState(AUDIO_MODE_NORMAL);
#endif
}

void ImsMediaVoiceSource::SetUplinkCallback(void* pClient, AudioUplinkCB pUplinkCB) {
    std::lock_guard<std::mutex> guard(m_mutexUplink);
    m_pUplinkCBClient = pClient;
    m_pUplinkCB = pUplinkCB;
}

void ImsMediaVoiceSource::SetCodec(int32_t type) {
    IMLOGD1("[SetCodec] type[%d]", type);
    m_nCodecType = type;
}

void ImsMediaVoiceSource::SetCodecMode(uint32_t mode) {
    IMLOGD1("[SetCodecMode] mode[%d]", mode);
    m_nMode = mode;
#ifdef DEBUG_PCM_DUMP
    g_amr_mode = m_nMode;
#endif
}

void ImsMediaVoiceSource::SetAttributionSource(android::content::AttributionSourceState& source) {
    mSource = source;
}

bool ImsMediaVoiceSource::Start() {
    size_t nMinBufferSize;
    status_t err;
    IMLOGD0("[Start] enter");
#ifdef DEBUG_PCM_DUMP
    memset(bPCMDump, 0x00, PCM_DUMP_SIZE); // init
    IMLOGE1("[Start] dump[%s]", bPCMDump);
    g_pPCMDump = bPCMDump;
    g_iStoredDumpSize = 0;

    if ((fPCMDump = fopen(
        "/data/user_de/0/com.android.telephony.imsmedia/voicefile.wav","w")) == NULL) {
        IMLOGE0("file open error !");
    }

    static char header[32];
    unsigned int header_size = 0;
    fwrite(header, 1, header_size, fPCMDump);
#endif
    //Set AMR Codec Mode
    //setAudioSystemAmrModeParameter();
    //Set DTX Mode
    //AudioSystem::setParameters(0,String8("dtx_on=true"));

    char kMimeType[128] = {'\0'};
    int samplingRate = 8000;
    if (m_nCodecType == AUDIO_G711_PCMU || m_nCodecType == AUDIO_G711_PCMA) {
    } else if (m_nCodecType == AUDIO_AMR) {
        sprintf(kMimeType, "audio/amr");
    } else if (m_nCodecType == AUDIO_AMR_WB) {
        samplingRate = 16000;
        sprintf(kMimeType, "audio/amr-wb");
    }

    AudioRecord::getMinFrameCount(&nMinBufferSize, samplingRate, AUDIO_FORMAT_PCM,
        AUDIO_CHANNEL_IN_MONO);
    IMLOGD1("[Start] nMinBufferSize[%d]", nMinBufferSize);

    /*
     * inputSource:        Select the audio input to record from (e.g. AUDIO_SOURCE_DEFAULT).
     * sampleRate:         Data sink sampling rate in Hz.  Zero means to use the source sample rate.
     * format:             Audio format (e.g AUDIO_FORMAT_PCM_16_BIT for signed
     *                     16 bits per sample).
     * channelMask:        Channel mask, such that audio_is_input_channel(channelMask) is true.
     * client:             The attribution source of the owner of the record
     * frameCount:         Minimum size of track PCM buffer in frames. This defines the
     *                     application's contribution to the
     *                     latency of the track.  The actual size selected by the AudioRecord could
     *                     be larger if the requested size is not compatible with current audio HAL
     *                     latency.  Zero means to use a default value.
     * cbf:                Callback function. If not null, this function is called periodically
     *                     to consume new data in TRANSFER_CALLBACK mode
     *                     and inform of marker, position updates, etc.
     * user:               Context for use by the callback receiver.
     * notificationFrames: The callback function is called each time notificationFrames PCM
     *                     frames are ready in record track output buffer.
     * sessionId:          Not yet supported.
     * transferType:       How data is transferred from AudioRecord.
     * flags:              See comments on audio_input_flags_t in <system/audio.h>
     * pAttributes:        If not NULL, supersedes inputSource for use case selection.
     * threadCanCallJava:  Not present in parameter list, and so is fixed at false.
     */
    IMLOGD1("[Start] client[%s]", mSource.toString().c_str());
    m_AudioRecord = new AudioRecord(mSource);

    //m_AudioRecord = new AudioRecord(
    err = m_AudioRecord->set(AUDIO_SOURCE_VOICE_COMMUNICATION,
        samplingRate,
        AUDIO_FORMAT_PCM_16_BIT,
        AUDIO_CHANNEL_IN_MONO);

    if (err != NO_ERROR) {
        IMLOGE1("[Start] err[%d]", err);
        m_AudioRecord.clear();
        m_AudioRecord = NULL;
        //return false;
        //test read pcm file
        mWavFile = fopen("/data/user_de/0/com.android.telephony.imsmedia/voicesample1.wav","r");
        if (mWavFile == NULL) {
            IMLOGE0("[Start] Unable to open wave file");
            return false;
        }

        int headerSize = sizeof(wav_hdr);
        size_t bytesRead = fread(&mWavHeader, 1, headerSize, mWavFile);
        if (bytesRead > 0) {
            //print header info
            IMLOGD4("[Start] chunksize[%d] SamplingRate[%d], Format[%d], bitsPerSample[%d]",
                mWavHeader.ChunkSize, mWavHeader.SamplesPerSec,
                mWavHeader.AudioFormat, mWavHeader.bitsPerSample);
        }
    }

    sp<ALooper> looper = new ALooper;
    looper->setName("ImsMediaVoiceSource");
    looper->start();
    IMLOGD1("[Start] Creating codec[%s]", kMimeType);

    m_MediaCodec = MediaCodec::CreateByType(looper, kMimeType, true);
    if (m_MediaCodec == NULL) {
        IMLOGE1("[Start] unable to create %s codec instance", kMimeType);
        return false;
    }

    constexpr char KEY_MIME[] = "mime";
    constexpr char KEY_SAMPLE_RATE[] = "sample-rate";
    constexpr char KEY_CHANNEL_COUNT[] = "channel-count";
    constexpr char KEY_BIT_RATE[] = "bitrate";

    sp<AMessage> format = new AMessage();
    format->setString(KEY_MIME, kMimeType);
    format->setInt32(KEY_SAMPLE_RATE, samplingRate);
    format->setInt32(KEY_CHANNEL_COUNT, 1);
    if (m_nCodecType == AUDIO_AMR) {
        format->setInt32(KEY_BIT_RATE, ImsMediaAudioFmt::GetBitrateAmr(m_nMode));
    } else if (m_nCodecType == AUDIO_AMR_WB) {
        format->setInt32(KEY_BIT_RATE, ImsMediaAudioFmt::GetBitrateAmrWb(m_nMode));
    }

    err = m_MediaCodec->configure(format, NULL, NULL, MediaCodec::CONFIGURE_FLAG_ENCODE);
    if (err != NO_ERROR) {
        IMLOGE2("[Start] unable to configure[%s] codec - err[%d]", kMimeType, err);
        m_MediaCodec->release();
        return false;
    }

    if (m_AudioRecord != NULL) {
        m_AudioRecord->start();
    }

    err = m_MediaCodec->start();
    if (err != NO_ERROR) {
        IMLOGE1("[Start] unable to start codec - err[%d]", err);
        m_MediaCodec->release();
        return false;
    }

    StartThread();

    std::thread t1(&ImsMediaVoiceSource::processOutputBuffer, this);
    t1.detach();
    IMLOGD0("[Start] exit");
    return true;
}

void ImsMediaVoiceSource::Stop() {
    IMLOGD0("[Stop] Enter");
    m_mutexUplink.lock();
    StopThread();
    m_AudioRecord->stop();
    m_AudioRecord.clear();
    m_AudioRecord = NULL;
    if (m_MediaCodec != NULL) {
        m_MediaCodec->stop();
        m_MediaCodec->release();
        m_MediaCodec = NULL;
    }
    m_mutexUplink.unlock();
#ifdef DEBUG_PCM_DUMP
    if (fwrite(bPCMDump, sizeof(uint8_t), g_iStoredDumpSize, fPCMDump) == -1) {
        IMLOGD("file write error !");
    }

    IMLOGE1("[Stop] dumpsize[%d]", g_iStoredDumpSize);
    fclose(fPCMDump);
#endif
    if (mWavFile != NULL) {
        fclose(mWavFile);
    }
    IMLOGD0("[Stop] Exit");
}

bool ImsMediaVoiceSource::ProcessCMR(uint32_t mode) {
    (void)mode;
    return false;
    // do nothing
}

void* ImsMediaVoiceSource::run() {
    IMLOGD0("[run] enter");
    uint32_t nNextTime = ImsMediaTimer::GetTimeInMilliSeconds();

    for (;;) {
        uint32_t nCurrTime;
        m_mutexUplink.lock();
        if (IsThreadStopped()) {
            IMLOGD0("[run] terminated");
            m_mutexUplink.unlock();
            break;
        }
        m_mutexUplink.unlock();

        int32_t readSize = 0;
        //get PCM
        if (m_AudioRecord != NULL) {
            readSize = m_AudioRecord->read(m_pbBuffer, PCM_BUFFER_SIZE, false);
        } else {
            uint16_t bytePerSample = mWavHeader.bitsPerSample / 8;
            readSize = fread(m_pbBuffer, bytePerSample, 1, mWavFile);
        }

        queueInputBuffer(m_pbBuffer, readSize);

        nNextTime += 20;
        nCurrTime = ImsMediaTimer::GetTimeInMilliSeconds();
        IMLOGD1("[run] nCurrTime[%u]", nCurrTime);
        if (nNextTime > nCurrTime) ImsMediaTimer::Sleep(nNextTime - nCurrTime);
    }

    return NULL;
}

void ImsMediaVoiceSource::queueInputBuffer(uint8_t* buffer, int32_t nSize) {
    IMLOGD0("[processUplinkThread] enter");
    std::lock_guard<std::mutex> guard(m_mutexUplink);
    static int kTimeout = 100000;   // be responsive on signal
    status_t err;
    size_t bufIndex;

    IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO, "[processUplinkThread] size[%d]", nSize);

    if (nSize <= 0 || buffer == NULL || m_MediaCodec == NULL) {
        return;
    }

    //process input buffer
    Vector<sp<MediaCodecBuffer>> inputBuffers;
    err = m_MediaCodec->getInputBuffers(&inputBuffers);
    if (err == NO_ERROR) {
        err = m_MediaCodec->dequeueInputBuffer(&bufIndex, kTimeout);

        if (err == NO_ERROR) {
            memcpy(inputBuffers[bufIndex]->data(), buffer, nSize);
            IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO,
                "[processUplinkThread] queue input buffer size[%d]", nSize);

            err = m_MediaCodec->queueInputBuffer(bufIndex, 0, nSize,
                ImsMediaTimer::GetTimeInMicroSeconds(),
                MediaCodec::BUFFER_FLAG_SYNCFRAME);

            if (err != NO_ERROR) {
                IMLOGE1("[processUplinkThread] Unable to get input buffers - err[%d]", err);
            }
        }
    } else {
        IMLOGE1("[processUplinkThread] Unable to get input buffers - err[%d]", err);
    }
}

void ImsMediaVoiceSource::processOutputBuffer() {
    IMLOGD0("[processOutputBuffer] enter");
    size_t bufIndex, offset, size;
    static int kTimeout = 100000;   // be responsive on signal
    int64_t ptsUsec;
    uint32_t flags;
    status_t err;

    for (;;) {
        m_mutexUplink.lock();
        if (IsThreadStopped()) {
            IMLOGD0("[processOutputBuffer] terminated");
            m_mutexUplink.unlock();
            break;
        }
        m_mutexUplink.unlock();
        //process output buffer
        Vector<sp<MediaCodecBuffer>> outputBuffers;
        err = m_MediaCodec->getOutputBuffers(&outputBuffers);
        if (err != NO_ERROR) {
            IMLOGE1("[processOutputBuffer] Unable to get output buffers - err[%d]", err);
            continue;
        }

        err = m_MediaCodec->dequeueOutputBuffer(&bufIndex, &offset, &size, &ptsUsec,
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
                        "[processOutputBuffer] Got data in buffer[%zu], size[%zu], pts=%" PRId64,
                        bufIndex, size, ptsUsec);

                    if (ptsUsec == 0) {
                        ptsUsec = ImsMediaTimer::GetTimeInMicroSeconds() / 1000;
                    }

                    //call encoding here
                    m_pUplinkCB(m_pUplinkCBClient, outputBuffers[bufIndex]->data(), size,
                        ptsUsec, flags);
    #ifdef DEBUG_PCM_DUMP
                    memcpy(g_pPCMDump+g_iStoredDumpSize, outputBuffers[bufIndex]->data(), size);
                    g_iStoredDumpSize += readSize;
    #endif
                }
                err = m_MediaCodec->releaseOutputBuffer(bufIndex);

                if (err != NO_ERROR) {
                    IMLOGE1("[processOutputBuffer] Unable to release output buffer - err[%d]", err);
                    continue;
                }

                if ((flags & MediaCodec::BUFFER_FLAG_EOS) != 0) {
                    // Not expecting EOS from SurfaceFlinger.  Go with it.
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
                    //m_MediaCodec->getOutputFormat(&newFormat);
                }
                break;
            case android::INFO_OUTPUT_BUFFERS_CHANGED:   // INFO_OUTPUT_BUFFERS_CHANGED
                // Not expected for an encoder; handle it anyway.
                IMLOGD0("[processOutputBuffer] Encoder buffers changed");
                err = m_MediaCodec->getOutputBuffers(&outputBuffers);
                if (err != NO_ERROR) {
                    IMLOGE1("Unable to get new output buffers - err[%d]", err);
                    continue;
                }
                break;
            case INVALID_OPERATION:
                IMLOGD0("[processOutputBuffer] dequeueOutputBuffer returned INVALID_OPERATION");
                continue;
            default:
                IMLOGE1("[processOutputBuffer] Got weird result[%d] from dequeueOutputBuffer", err);
                continue;
        }
    }
}