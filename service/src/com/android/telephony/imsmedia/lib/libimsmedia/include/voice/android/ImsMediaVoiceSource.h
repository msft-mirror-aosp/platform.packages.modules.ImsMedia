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

#ifndef AUDIO_RECORD_H_INCLUDED
#define AUDIO_RECORD_H_INCLUDED

#include <ImsMediaDefine.h>
#include <ImsMediaAudioDefine.h>
#include <mutex>
#include <IImsMediaThread.h>
#include <ImsMediaCondition.h>
#include <media/AudioRecord.h>
#include <media/stagefright/MediaCodec.h>

using android::sp;
using android::AudioRecord;
using android::MediaCodec;

typedef struct  WAV_HEADER
{
    /* RIFF Chunk Descriptor */
    uint8_t         RIFF[4];        // RIFF Header Magic header
    uint32_t        ChunkSize;      // RIFF Chunk Size
    uint8_t         WAVE[4];        // WAVE Header
    /* "fmt" sub-chunk */
    uint8_t         fmt[4];         // FMT header
    uint32_t        Subchunk1Size;  // Size of the fmt chunk
    uint16_t        AudioFormat;    // Audio format 1=PCM
    uint16_t        NumOfChan;      // Number of channels 1=Mono 2=Sterio
    uint32_t        SamplesPerSec;  // Sampling Frequency in Hz
    uint32_t        bytesPerSec;    // bytes per second
    uint16_t        blockAlign;     // 2=16-bit mono, 4=16-bit stereo
    uint16_t        bitsPerSample;  // Number of bits per sample
    /* "data" sub-chunk */
    uint8_t         Subchunk2ID[4]; // "data"  string
    uint32_t        Subchunk2Size;  // Sampled data length
} wav_hdr;

typedef void (*AudioUplinkCB)(void* pClient, uint8_t* pBitstream, uint32_t nSize,
    int64_t pstUsec, uint32_t flag);

class ImsMediaVoiceSource : public IImsMediaThread {
public:
    std::mutex m_mutexUplink;
    sp<AudioRecord> m_AudioRecord;
    sp<MediaCodec> m_MediaCodec;
    AudioUplinkCB m_pUplinkCB;
    void*    m_pUplinkCBClient;
    int32_t m_nCodecType;
    uint32_t m_nMode;
    uint8_t m_pbBuffer[PCM_BUFFER_SIZE]; // read buffer size
    android::content::AttributionSourceState mSource;
#ifdef DEBUG_PCM_DUMP
private:
    FILE *fPCMDump; // file pointer for PCM Dump
    uint8_t bPCMDump[PCM_DUMP_SIZE];  // buffer of PCM Dump
#endif
    FILE *mWavFile;
    wav_hdr mWavHeader;

private:
    void queueInputBuffer(uint8_t* buffer, int32_t nSize);
    void processOutputBuffer();

public:
    ImsMediaVoiceSource();
    virtual ~ImsMediaVoiceSource();
    void SetAttributionSource(android::content::AttributionSourceState& source);
    void SetUplinkCallback(void* pClient, AudioUplinkCB pDnlinkCB);
    void SetCodec(int32_t type);
    void SetCodecMode(uint32_t mode);
    bool Start();
    void Stop();
    bool ProcessCMR(uint32_t mode);
    virtual void* run();

};

#endif
