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
    m_AudioTrack = NULL;
    m_MediaCodec = NULL;

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

void ImsMediaVoiceRenderer::SetCodec(eAudioCodecType eCodecType) {
    IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO,
        "[ImsMediaVoiceRenderer::SetCodec] eCodecType[%d]", eCodecType);
    m_nCodecType = eCodecType;
}

void ImsMediaVoiceRenderer::SetCodecMode(uint32_t mode) {
    IMLOGD1("[SetAMRMode] mode[%d]", mode);
    m_amr_mode = mode;
#ifdef DEBUG_PCM_DUMP
    g_amr_mode_tx = m_amr_mode;
#endif
}

bool ImsMediaVoiceRenderer::Start() {
    uint32_t nAudioTrackStatus;
    size_t nMinBufferSize;
    status_t err;
#ifdef DEBUG_PCM_DUMP
        memset(bPCMDump, 0x00, PCM_DUMP_SIZE); // init
        IMLOGE1("[Start] dump[%s]", bPCMDump);
        g_pPCMDump_tx = bPCMDump;
        g_iStoredDumpSize_tx = 0;

        if ((fPCMDump = fopen(
            "/data/user_de/0/com.android.imsstack.imsmedia/voicefile_dl.dat", "w")) == NULL) {
            IMLOGE0("file open error !");
            exit(1);
        }

        static char header[32];
        unsigned int header_size = 0;

        if (m_nCodecType == AUDIO_AMR) {
            std::strcpy(header, "#!AMR");
            header_size = 6;
        } else if (m_nCodecType == AUDIO_AMR_WB) {
            std::strcpy(header, "#!AMR-WB");
            header_size = 9;
        }

        fwrite(header, 1, header_size, fPCMDump);
#endif
    char kMimeType[128] = {'\0'};
    int samplingRate = 8000;
    if (m_nCodecType == AUDIO_AMR) {
        sprintf(kMimeType, "audio/amr");
    } else if (m_nCodecType == AUDIO_AMR_WB) {
        samplingRate = 16000;
        sprintf(kMimeType, "audio/amr-wb");
    }

    AudioTrack::getMinFrameCount(&nMinBufferSize, AUDIO_STREAM_VOICE_CALL, samplingRate);
    IMLOGD1("[Start] nMinBufferSize[%d]", nMinBufferSize);
    m_AudioTrack = new AudioTrack(AUDIO_STREAM_VOICE_CALL, samplingRate, AUDIO_FORMAT_PCM,
        AUDIO_CHANNEL_OUT_MONO, AMR_WB_NOTI_COUNT[m_amr_mode] + 12,
        (audio_output_flags_t)AUDIO_OUTPUT_FLAG_NONE, NULL, NULL,
        AMR_WB_NOTI_COUNT[m_amr_mode] + 12);

    nAudioTrackStatus = m_AudioTrack->initCheck();

    if (nAudioTrackStatus != 0) {
        IMLOGD1("[Start] Init fail [%d]", nAudioTrackStatus);
        return false;
    }

    sp<ALooper> looper = new ALooper;
    looper->setName("ImsMediaVoiceRenderer");
    looper->start();
    IMLOGD1("[Start] Creating codec[%s]", kMimeType);

    m_MediaCodec = MediaCodec::CreateByType(looper, kMimeType, false);
    if (m_MediaCodec == NULL) {
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

    err = m_MediaCodec->configure(format, NULL, NULL, 0);
    if (err != NO_ERROR) {
        IMLOGE2("[Start] unable to configure[%s] codec - err[%d]", kMimeType, err);
        m_MediaCodec->release();
        return err;
    }

    if (m_AudioTrack != NULL) {
        m_AudioTrack->start();
    }

    err = m_MediaCodec->start();
    if (err != NO_ERROR) {
        IMLOGE1("[Start] unable to start codec - err[%d]", err);
        m_MediaCodec->release();
        return false;
    }

    IMLOGD0("[Start] exit");
    return true;
}

void ImsMediaVoiceRenderer::Stop() {
    IMLOGD0("[Stop] enter");
    if (m_MediaCodec != NULL) {
        m_MediaCodec->stop();
        m_MediaCodec->release();
        m_MediaCodec.clear();
        m_MediaCodec = NULL;
    }
    m_AudioTrack->stop();
    m_AudioTrack->flush();
    m_AudioTrack.clear();
#ifdef DEBUG_PCM_DUMP
    if (fwrite(bPCMDump, sizeof(uint8_t), g_iStoredDumpSize_tx, fPCMDump) == -1) {
        IMLOGE0("[Stop] dump - file write error ! ");
    }

    IMLOGE1("[Stop] dump[%d]", g_iStoredDumpSize_tx);

    fclose(fPCMDump);
#endif
    m_AudioTrack = NULL;
    IMLOGD0("[Stop] exit ");
}

bool ImsMediaVoiceRenderer::onDataFrame(uint8_t* buffer, uint32_t nSize) {
    if (nSize == 0) return false;
    if (m_MediaCodec == NULL) return false;

    static int kTimeout = 250000;   // be responsive on signal
    status_t err;
    size_t bufIndex, offset, size;
    int64_t ptsUsec;
    uint32_t flags;

    //process input buffer
    Vector<sp<MediaCodecBuffer>> inputBuffers;
    err = m_MediaCodec->getInputBuffers(&inputBuffers);
    if (err == NO_ERROR) {
        err = m_MediaCodec->dequeueInputBuffer(&bufIndex, kTimeout);

        if (err == NO_ERROR) {
            memcpy(inputBuffers[bufIndex]->data(), buffer, nSize);
            IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO,
                "[onDataFrame] queue input buffer size[%d]", nSize);

            err = m_MediaCodec->queueInputBuffer(bufIndex, 0, nSize,
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
    err = m_MediaCodec->getOutputBuffers(&outputBuffers);
    if (err != NO_ERROR) {
        IMLOGE1("[onDataFrame] Unable to get output buffers - err[%d]", err);
        return false;
    }

    err = m_MediaCodec->dequeueOutputBuffer(&bufIndex, &offset, &size, &ptsUsec,
            &flags, kTimeout);

    switch (err) {
        case NO_ERROR:
            // got a buffer
            if ((flags & MediaCodec::BUFFER_FLAG_CODECCONFIG) != 0) {
                IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO,
                    "Got codec config buffer (%zu bytes)", size);
            }

            if (size != 0) {
                IMLOGD_PACKET3(IM_PACKET_LOG_AUDIO,
                    "Got data in buffer %zu, size=%zu, pts=%" PRId64,
                    bufIndex, size, ptsUsec);

                if (ptsUsec == 0) {
                    ptsUsec = ImsMediaTimer::GetTimeInMicroSeconds() / 1000;
                }

                //call encoding here
                if (m_AudioTrack) {
                    m_AudioTrack->write(outputBuffers[bufIndex]->data(), size, true);
                }
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

            err = m_MediaCodec->releaseOutputBuffer(bufIndex);

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
                //m_MediaCodec->getOutputFormat(&newFormat);
            }
            break;
        case android::INFO_OUTPUT_BUFFERS_CHANGED:   // INFO_OUTPUT_BUFFERS_CHANGED
            // Not expected for an encoder; handle it anyway.
            IMLOGD0("Decoder buffers changed");
            err = m_MediaCodec->getOutputBuffers(&outputBuffers);
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