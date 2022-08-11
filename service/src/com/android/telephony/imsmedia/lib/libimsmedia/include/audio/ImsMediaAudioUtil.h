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

#ifndef AUDIO_AMRFMT_H_INCLUDED
#define AUDIO_AMRFMT_H_INCLUDED

#include <ImsMediaDefine.h>
#include <stdint.h>

#define IMSAMR_FRAME_BYTES              34

#define IMSAMR_CLASS_A_BITS_BAD         0
#define IMSAMR_CLASS_A_BITS_SID         39

#define IMSAMR_CLASS_A_BITS_475         42
#define IMSAMR_CLASS_B_BITS_475         53
#define IMSAMR_CLASS_C_BITS_475         0

#define IMSAMR_CLASS_A_BITS_515         49
#define IMSAMR_CLASS_B_BITS_515         54
#define IMSAMR_CLASS_C_BITS_515         0

#define IMSAMR_CLASS_A_BITS_590         55
#define IMSAMR_CLASS_B_BITS_590         63
#define IMSAMR_CLASS_C_BITS_590         0

#define IMSAMR_CLASS_A_BITS_670         58
#define IMSAMR_CLASS_B_BITS_670         76
#define IMSAMR_CLASS_C_BITS_670         0

#define IMSAMR_CLASS_A_BITS_740         61
#define IMSAMR_CLASS_B_BITS_740         87
#define IMSAMR_CLASS_C_BITS_740         0

#define IMSAMR_CLASS_A_BITS_795         75
#define IMSAMR_CLASS_B_BITS_795         84
#define IMSAMR_CLASS_C_BITS_795         0

#define IMSAMR_CLASS_A_BITS_102         65
#define IMSAMR_CLASS_B_BITS_102         99
#define IMSAMR_CLASS_C_BITS_102         40

#define IMSAMR_CLASS_A_BITS_122         81
#define IMSAMR_CLASS_B_BITS_122         103
#define IMSAMR_CLASS_C_BITS_122         60

/* for AMRWB starts */
#define IMSAMRWB_CLASS_A_BITS_BAD       0   // doubt
#define IMSAMRWB_CLASS_A_BITS_SID       35  // doubt

#define IMSAMRWB_CLASS_A_BITS_660       54
#define IMSAMRWB_CLASS_B_BITS_660       78
#define IMSAMRWB_CLASS_C_BITS_660       0

#define IMSAMRWB_CLASS_A_BITS_885       64
#define IMSAMRWB_CLASS_B_BITS_885       113
#define IMSAMRWB_CLASS_C_BITS_885       0

#define IMSAMRWB_CLASS_A_BITS_1265      72
#define IMSAMRWB_CLASS_B_BITS_1265      181
#define IMSAMRWB_CLASS_C_BITS_1265      0

#define IMSAMRWB_CLASS_A_BITS_1425      72
#define IMSAMRWB_CLASS_B_BITS_1425      213
#define IMSAMRWB_CLASS_C_BITS_1425      0

#define IMSAMRWB_CLASS_A_BITS_1585      72
#define IMSAMRWB_CLASS_B_BITS_1585      245
#define IMSAMRWB_CLASS_C_BITS_1585      0

#define IMSAMRWB_CLASS_A_BITS_1825      72
#define IMSAMRWB_CLASS_B_BITS_1825      293
#define IMSAMRWB_CLASS_C_BITS_1825      0

#define IMSAMRWB_CLASS_A_BITS_1985      72
#define IMSAMRWB_CLASS_B_BITS_1985      325
#define IMSAMRWB_CLASS_C_BITS_1985      0

#define IMSAMRWB_CLASS_A_BITS_2305      72
#define IMSAMRWB_CLASS_B_BITS_2305      389
#define IMSAMRWB_CLASS_C_BITS_2305      0

#define IMSAMRWB_CLASS_A_BITS_2385      72
#define IMSAMRWB_CLASS_B_BITS_2385      405
#define IMSAMRWB_CLASS_C_BITS_2385      0

#define EVS_COMPACT_PRIMARY_PAYLOAD_NUM 13
#define EVS_COMPACT_AMRWBIO_PAYLOAD_NUM 10
#define EVS_COMPACT_PAYLOAD_MAX_NUM     32

#define AUDIO_STOP_TIMEOUT              1000

enum kImsAudioFrameEntype
{
    kImsAudioFrameGsmSid = 0,        /* GSM HR, FR or EFR : silence descriptor   */
    kImsAudioFrameGsmSpeechGood,     /* GSM HR, FR or EFR : good speech frame    */
    kImsAudioFrameGsmBfi,            /* GSM HR, FR or EFR : bad frame indicator  */
    kImsAudioFrameGsmInvalidSid,     /* GSM HR            : invalid SID frame    */
    kImsAudioFrameAmrSpeechGood,     /* AMR : good speech frame              */
    kImsAudioFrameAmrSpeechDegraded, /* AMR : degraded speech frame          */
    kImsAudioFrameAmrOnSet,          /* AMR : onset                          */
    kImsAudioFrameAmrSpeechBad,      /* AMR : bad speech frame               */
    kImsAudioFrameAmrSidFirst,       /* AMR : first silence descriptor       */
    kImsAudioFrameAmrSidUpdate,      /* AMR : successive silence descriptor  */
    kImsAudioFrameAmrSidBad,         /* AMR : bad silence descriptor frame   */
    kImsAudioFrameAmrNoData,         /* AMR : Nothing to Transmit     */
    kImsAudioFrameAmrSpeechLost,     /* downlink speech lost           */
    kImsAudioFrameMax
};

enum kImsAudioAmrMode
{
    kImsAudioAmrMode475 = 0,  /* 4.75 kbit/s                             */
    kImsAudioAmrMode515 = 1,  /* 5.15 kbit/s                             */
    kImsAudioAmrMode590 = 2,  /* 5.90 kbit/s                             */
    kImsAudioAmrMode670 = 3,  /* 6.70 kbit/s                             */
    kImsAudioAmrMode740 = 4,  /* 7.40 kbit/s                             */
    kImsAudioAmrMode795 = 5,  /* 7.95 kbit/s                             */
    kImsAudioAmrMode1020 = 6, /* 10.20 kbit/s                            */
    kImsAudioAmrMode1220 = 7, /* 12.20 kbit/s, also used for GSM EFR     */
    kImsAudioAmrModeSID = 8,  /* AMR SID */
    /* 9~13: for future use */
    kImsAudioAmrModeSPL = 14,    /* Speech Lost frame  */
    kImsAudioAmrModeNoData = 15, /* No Data */
    kImsAudioAmrModeEVRC0 = 0,   /* Indicates vocoder data was blanked. */
    kImsAudioAmrModeEVRC8,       /* Indicates rate 1/8 vocoder data. */
    kImsAudioAmrModeEVRC4,       /* Indicates rate 1/4 vocoder data. */
    kImsAudioAmrModeEVRC2,       /* Indicates rate 1/2 vocoder data. */
    kImsAudioAmrModeEVRC1,       /* Indicates rate 1 vocoder data. */
    kImsAudioAmrModeEVRCERASURE, /* Indicates frame erasure */
    kImsAudioAmrModeEVRCERR,     /* Indicates invalid vocoder data. */
    kImsAudioAmrModeMax
};

enum kImsAudioAmrWbMode
{
    kImsAudioAmrWbMode660 = 0,  /* 6.60 kbit/s */
    kImsAudioAmrWbMode885 = 1,  /* 8.85 kbit/s */
    kImsAudioAmrWbMode1265 = 2, /* 12.65 kbit/s */
    kImsAudioAmrWbMode1425 = 3, /* 14.25 kbit/s */
    kImsAudioAmrWbMode1585 = 4, /* 15.85 kbit/s */
    kImsAudioAmrWbMode1825 = 5, /* 18.25 kbit/s */
    kImsAudioAmrWbMode1985 = 6, /* 19.85 kbit/s */
    kImsAudioAmrWbMode2305 = 7, /* 23.05 kbit/s */
    kImsAudioAmrWbMode2385 = 8, /* 23.85 kbit/s */
    kImsAudioAmrWbModeSID = 9,  /* AMRWB SID */
    /* 10~13: for future use */
    kImsAudioAmrWbModeSPL = 14,    /* AMRWB Speech Lost frame */
    kImsAudioAmrWbModeNoData = 15, /* AMRWB No Data */
    kImsAudioAmrWbModeMax
};

enum kImsAudioEvsPrimaryMode
{
    kImsAudioEvsPrimaryMode00280 = 0,  /* 2.8 kbps, Special case */
    kImsAudioEvsPrimaryMode00720 = 1,  /* 7.2 kbps, EVS Primary */
    kImsAudioEvsPrimaryMode00800 = 2,  /* 8 kbps, EVS Primary */
    kImsAudioEvsPrimaryMode00960 = 3,  /* 9.6 kbps, EVS Primary */
    kImsAudioEvsPrimaryMode01320 = 4,  /* 13.20 kbps, EVS Primary */
    kImsAudioEvsPrimaryMode01640 = 5,  /* 16.4 kbps, EVS Primary */
    kImsAudioEvsPrimaryMode02440 = 6,  /* 24.4 kbps, EVS Primary */
    kImsAudioEvsPrimaryMode03200 = 7,  /* 32 kbps, EVS Primary */
    kImsAudioEvsPrimaryMode04800 = 8,  /* 48 kbps, EVS Primary */
    kImsAudioEvsPrimaryMode06400 = 9,  /* 64 kbps, EVS Primary */
    kImsAudioEvsPrimaryMode09600 = 10, /* 96 kbps, EVS Primary */
    kImsAudioEvsPrimaryMode12800 = 11, /* 128 kbps, EVS Primary */
    kImsAudioEvsPrimaryModeSID = 12,   /* 2.4 kbps, EVS Primary SID */
    /* 13 is remaind for future use.. */
    kImsAudioEvsPrimaryModeSpeechLost = 14, /* SPEECH LOST */
    kImsAudioEvsPrimaryModeNoData = 15,     /* NO DATA */
    kImsAudioEvsPrimaryModeMax
};

enum kImsAudioEvsAmrWbIoMode
{
    kImsAudioEvsAmrWbIoMode660 = 0,  /* 6.60 kbit/s */
    kImsAudioEvsAmrWbIoMode885 = 1,  /* 8.85 kbit/s */
    kImsAudioEvsAmrWbIoMode1265 = 2, /* 12.65 kbit/s */
    kImsAudioEvsAmrWbIoMode1425 = 3, /* 14.25 kbit/s */
    kImsAudioEvsAmrWbIoMode1585 = 4, /* 15.85 kbit/s */
    kImsAudioEvsAmrWbIoMode1825 = 5, /* 18.25 kbit/s */
    kImsAudioEvsAmrWbIoMode1985 = 6, /* 19.85 kbit/s */
    kImsAudioEvsAmrWbIoMode2305 = 7, /* 23.05 kbit/s */
    kImsAudioEvsAmrWbIoMode2385 = 8, /* 23.85 kbit/s */
    kImsAudioEvsAmrWbIoModeSID = 9,  /* AMRWB SID */
    /* 10~13: for future use */
    kImsAudioEvsAmrWbIoModeSPL = 14,    /* AMRWB Speech Lost frame */
    kImsAudioEvsAmrWbIoModeNoData = 15, /* AMRWB No Data */
    kImsAudioEvsAmrWbIoModeMax
};

class ImsMediaAudioUtil
{
public:
    static int32_t ConvertCodecType(int32_t type);
    static int32_t ConvertEvsCodecMode(int32_t evsMode);
    static void AmrFmtFraming(kImsAudioFrameEntype eFrameType, kImsAudioAmrMode eRate,
            uint8_t* pRawData, uint8_t* pEncodedData);
    static void AmrFmtDeframing(kImsAudioFrameEntype eFrameType, kImsAudioAmrMode eRate,
            uint8_t* pRawData, uint8_t* pEncodedData);
    static void AmrWbFmtFraming(kImsAudioFrameEntype eFrameType, kImsAudioAmrWbMode eRate,
            uint8_t* pRawData, uint8_t* pEncodedData);
    static void AmrFmtSwap(uint8_t* pSrc, uint8_t* pDst, uint32_t nNumOfByte);
    static void AmrFmtWbDeframing(kImsAudioFrameEntype eFrameType, kImsAudioAmrWbMode eRate,
            uint8_t* pRawData, uint8_t* pEncodedData);
    static uint32_t ConvertAmrModeToLen(uint32_t mode);
    static uint32_t ConvertAmrModeToBitLen(uint32_t mode);
    static uint32_t ConvertLenToAmrMode(uint32_t nLen);
    static void ConvertEvsBandwidthToStr(kEvsBandwidth bandwidth, char* nBandwidth);
    static uint32_t ConvertAmrWbModeToLen(uint32_t mode);
    static uint32_t ConvertAmrWbModeToBitLen(uint32_t mode);
    static uint32_t ConvertLenToAmrWbMode(uint32_t nLen);
    static uint32_t ConvertLenToEVSAudioMode(uint32_t nLen);
    static uint32_t ConvertLenToEVSAMRIOAudioMode(uint32_t nLen);
    static uint32_t ConvertEVSAudioModeToBitLen(uint32_t mode);
    static uint32_t ConvertEVSAMRIOAudioModeToBitLen(uint32_t mode);
    static uint32_t ConvertAmrModeToBitrate(int mode);
    static uint32_t ConvertAmrWbModeToBitrate(int mode);
    static uint32_t GetBitrateEVS(int mode);
    static kRtpPyaloadHeaderMode ConvertEVSPayloadMode(
            uint32_t nDataSize, kEvsCodecMode* pEVSCodecMode, uint32_t* pEVSCompactId);
    static kEvsBandwidth FindMaxEVSBandwidth(uint32_t nEVSBandwidthSet);
    static kEvsBitrate FindMaxEVSBitrate(uint32_t nEVSBitrateSet, kEvsCodecMode kEvsCodecMode);
    static kEvsCodecMode CheckEVSCodecMode(uint32_t nAudioFrameLength);
    static int32_t ConvertEVSModeToBitRate(int32_t EvsModeToBitRate);
    static kEvsBandwidth FindMaxEvsBandwidthFromRange(int32_t EvsBandwidthRange);
};

#endif  // AUDIO_AMRFMT_H_INCLUDED
