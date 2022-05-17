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

typedef enum
{
    IMSVOC_AUDIOFRAME_GSM_SID = 0,        /* GSM HR, FR or EFR : silence descriptor   */
    IMSVOC_AUDIOFRAME_GSM_SPEECHGOOD,     /* GSM HR, FR or EFR : good speech frame    */
    IMSVOC_AUDIOFRAME_GSM_BFI,            /* GSM HR, FR or EFR : bad frame indicator  */
    IMSVOC_AUDIOFRAME_GSM_INVALIDSID,     /* GSM HR            : invalid SID frame    */
    IMSVOC_AUDIOFRAME_AMR_SPEECHGOOD,     /* AMR : good speech frame              */
    IMSVOC_AUDIOFRAME_AMR_SPEECHDEGRADED, /* AMR : degraded speech frame          */
    IMSVOC_AUDIOFRAME_AMR_ONSET,          /* AMR : onset                          */
    IMSVOC_AUDIOFRAME_AMR_SPEECHBAD,      /* AMR : bad speech frame               */
    IMSVOC_AUDIOFRAME_AMR_SIDFIRST,       /* AMR : first silence descriptor       */
    IMSVOC_AUDIOFRAME_AMR_SIDUPDATE,      /* AMR : successive silence descriptor  */
    IMSVOC_AUDIOFRAME_AMR_SIDBAD,         /* AMR : bad silence descriptor frame   */
    IMSVOC_AUDIOFRAME_AMR_NODATA,         /* AMR : Nothing to Transmit     */
    IMSVOC_AUDIOFRAME_AMR_SPEECHLOST,     /* downlink speech lost           */
    IMSVOC_AUDIOFRAME_GSM_FRAME_MAX
} IMSVOC_AUDIOFRAME_ENTYPE;

typedef enum
{
    IMSVOC_AUDIORATE_475 = 0,  /* 4.75 kbit /s                             */
    IMSVOC_AUDIORATE_515 = 1,  /* 5.15 kbit /s                             */
    IMSVOC_AUDIORATE_590 = 2,  /* 5.90 kbit /s                             */
    IMSVOC_AUDIORATE_670 = 3,  /* 6.70 kbit /s                             */
    IMSVOC_AUDIORATE_740 = 4,  /* 7.40 kbit /s                             */
    IMSVOC_AUDIORATE_795 = 5,  /* 7.95 kbit /s                             */
    IMSVOC_AUDIORATE_1020 = 6, /* 10.20 kbit /s                            */
    IMSVOC_AUDIORATE_1220 = 7, /* 12.20 kbit /s, also used for GSM EFR     */
    IMSVOC_AUDIORATE_SID = 8,  /* AMR SID                    */
    /* 9~13: for future use */
    IMSVOC_AUDIORATE_SPL = 14,    /* Speech Lost frame  */
    IMSVOC_AUDIORATE_NODATA = 15, /* No Data                    */
    IMSVOC_AUDIORATE_EVRC0 = 0,   /* Indicates vocoder data was blanked.      */
    IMSVOC_AUDIORATE_EVRC8,       /* Indicates rate 1/8 vocoder data.          */
    IMSVOC_AUDIORATE_EVRC4,       /* Indicates rate 1/4 vocoder data.          */
    IMSVOC_AUDIORATE_EVRC2,       /* Indicates rate 1/2 vocoder data.          */
    IMSVOC_AUDIORATE_EVRC1,       /* Indicates rate 1    vocoder data.          */
    IMSVOC_AUDIORATE_EVRCERASURE, /* Indicates frame erasure                  */
    IMSVOC_AUDIORATE_EVRCERR,     /* Indicates invalid vocoder data.            */
    IMSVOC_AUDIORATE_MAX = 0x7FFFFFFF
} IMSVOC_AUDIORATE_ENTYPE;

typedef enum _IMSVOC_AMRWB_ENTYPE
{
    IMSVOC_AMRWB_MODE_660 = 0,  /* 6.60 kbit /s */
    IMSVOC_AMRWB_MODE_885 = 1,  /* 8.85 kbit /s */
    IMSVOC_AMRWB_MODE_1265 = 2, /* 12.65 kbit /s */
    IMSVOC_AMRWB_MODE_1425 = 3, /* 14.25 kbit /s */
    IMSVOC_AMRWB_MODE_1585 = 4, /* 15.85 kbit /s */
    IMSVOC_AMRWB_MODE_1825 = 5, /* 18.25 kbit /s */
    IMSVOC_AMRWB_MODE_1985 = 6, /* 19.85 kbit /s */
    IMSVOC_AMRWB_MODE_2305 = 7, /* 23.05 kbit /s */
    IMSVOC_AMRWB_MODE_2385 = 8, /* 23.85 kbit /s */
    IMSVOC_AMRWB_MODE_SID = 9,  /* AMRWB SID */
    /* 10~13: for future use */
    IMSVOC_AMRWB_MODE_SPL = 14,     /* AMRWB Speech Lost frame */
    IMSVOC_AMRWB_MODE_NO_DATA = 15, /* AMRWB No Data */
    IMSVOC_AMRWB_MODE_MAX = 0x7FFFFFFF
} IMSVOC_AMRWB_ENTYPE;

typedef enum
{
    IMSVOC_EVS_PRIMARY_MODE_00280 = 0,  /* 2.8 kbps, Special case */
    IMSVOC_EVS_PRIMARY_MODE_00720 = 1,  /* 7.2 kbps, EVS Primary */
    IMSVOC_EVS_PRIMARY_MODE_00800 = 2,  /* 8 kbps, EVS Primary */
    IMSVOC_EVS_PRIMARY_MODE_00960 = 3,  /* 9.6 kbps, EVS Primary */
    IMSVOC_EVS_PRIMARY_MODE_01320 = 4,  /* 13.20 kbps, EVS Primary */
    IMSVOC_EVS_PRIMARY_MODE_01640 = 5,  /* 16.4 kbps, EVS Primary */
    IMSVOC_EVS_PRIMARY_MODE_02440 = 6,  /* 24.4 kbps, EVS Primary */
    IMSVOC_EVS_PRIMARY_MODE_03200 = 7,  /* 32 kbps, EVS Primary */
    IMSVOC_EVS_PRIMARY_MODE_04800 = 8,  /* 48 kbps, EVS Primary */
    IMSVOC_EVS_PRIMARY_MODE_06400 = 9,  /* 64 kbps, EVS Primary */
    IMSVOC_EVS_PRIMARY_MODE_09600 = 10, /* 96 kbps, EVS Primary */
    IMSVOC_EVS_PRIMARY_MODE_12800 = 11, /* 128 kbps, EVS Primary */
    IMSVOC_EVS_PRIMARY_MODE_SID = 12,   /* 2.4 kbps, EVS Primary SID */
    /* 13 is remaind for future use.. */
    IMSVOC_EVS_PRIMARY_MODE_SPEECH_LOST = 14, /* SPEECH LOST */
    IMSVOC_EVS_PRIMARY_MODE_NO_DATA = 15,     /* NO DATA */
    IMSVOC_EVS_PRIMARY_MODE_MAX
} IMSVOC_EVS_PRIMARY_ENTYPE;

typedef enum
{
    IMSVOC_EVS_AMRWB_IO_MODE_660 = 0,  /* 6.60 kbit /s */
    IMSVOC_EVS_AMRWB_IO_MODE_885 = 1,  /* 8.85 kbit /s */
    IMSVOC_EVS_AMRWB_IO_MODE_1265 = 2, /* 12.65 kbit /s */
    IMSVOC_EVS_AMRWB_IO_MODE_1425 = 3, /* 14.25 kbit /s */
    IMSVOC_EVS_AMRWB_IO_MODE_1585 = 4, /* 15.85 kbit /s */
    IMSVOC_EVS_AMRWB_IO_MODE_1825 = 5, /* 18.25 kbit /s */
    IMSVOC_EVS_AMRWB_IO_MODE_1985 = 6, /* 19.85 kbit /s */
    IMSVOC_EVS_AMRWB_IO_MODE_2305 = 7, /* 23.05 kbit /s */
    IMSVOC_EVS_AMRWB_IO_MODE_2385 = 8, /* 23.85 kbit /s */
    IMSVOC_EVS_AMRWB_IO_MODE_SID = 9,  /* AMRWB SID */
    /* 10~13: for future use */
    IMSVOC_EVS_AMRWB_IO_SPL = 14,     /* AMRWB Speech Lost frame */
    IMSVOC_EVS_AMRWB_IO_NO_DATA = 15, /* AMRWB No Data */
    IMSVOC_EVS_AMRWB_IO_MODE_MAX = 0x7FFFFFFF
} IMSVOC_EVS_AMRWB_IO_ENTYPE;

class ImsMediaAudioFmt
{
public:
    static void AmrFmt_Framing(IMSVOC_AUDIOFRAME_ENTYPE eFrameType, IMSVOC_AUDIORATE_ENTYPE eRate,
            uint8_t* pRawData, uint8_t* pEncodedData);
    static void AmrFmt_Deframing(IMSVOC_AUDIOFRAME_ENTYPE eFrameType, IMSVOC_AUDIORATE_ENTYPE eRate,
            uint8_t* pRawData, uint8_t* pEncodedData);
    static void AmrWbFmt_Framing(IMSVOC_AUDIOFRAME_ENTYPE eFrameType, IMSVOC_AMRWB_ENTYPE eRate,
            uint8_t* pRawData, uint8_t* pEncodedData);
    static void AmrFmt_Swap(uint8_t* pSrc, uint8_t* pDst, uint32_t nNumOfByte);
    static void AmrFmtWb_Deframing(IMSVOC_AUDIOFRAME_ENTYPE eFrameType, IMSVOC_AMRWB_ENTYPE eRate,
            uint8_t* pRawData, uint8_t* pEncodedData);
    static uint32_t ConvertAmrModeToLen(uint32_t mode);
    static uint32_t ConvertAmrModeToBitLen(uint32_t mode);
    static uint32_t ConvertLenToAmrMode(uint32_t nLen);
    static uint32_t ConvertAmrWbModeToLen(uint32_t mode);
    static uint32_t ConvertAmrWbModeToBitLen(uint32_t mode);
    static uint32_t ConvertLenToAmrWbMode(uint32_t nLen);
    static uint32_t ConvertLenToEVSAudioRate(uint32_t nLen);
    static uint32_t ConvertLenToEVSAMRIOAudioRate(uint32_t nLen);
    static uint32_t ConvertEVSAudioRateToBitLen(uint32_t mode);
    static uint32_t ConvertEVSAMRIOAudioRateToBitLen(uint32_t mode);
    static uint32_t GetBitrateAmr(int mode);
    static uint32_t GetBitrateAmrWb(int mode);
};

#endif  // AUDIO_AMRFMT_H_INCLUDED