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

#ifndef IMSMEDIA_VIDEO_UTIL_H_INCLUDED
#define IMSMEDIA_VIDEO_UTIL_H_INCLUDED

#include <stdint.h>
#include <ImsMediaDefine.h>

#define MAX_CONFIG_LEN              256
#define MAX_CONFIG_INDEX            3
#define MAX_VIDEO_WIDTH             1920
#define MAX_VIDEO_HEIGHT            1920
#define MAX_WAIT_RESTART            1000
#define MAX_WAIT_CAMERA             1000
#define MAX_RTP_PAYLOAD_BUFFER_SIZE (MAX_VIDEO_WIDTH * MAX_VIDEO_HEIGHT * 3 >> 1)

enum kVideoResolution
{
    /* video resolu tion is not defined */
    kVideoResolutionInvalid,
    kVideoResolutionSqcifLandscape,  // 128x92
    kVideoResolutionSqcifPortrait,   // 92x128
    kVideoResolutionQcifLandscape,   // 176x144
    kVideoResolutionQcifPortrait,    // 144x176
    kVideoResolutionQvgaLandscape,   // 320x240
    kVideoResolutionQvgaPortrait,    // 240x320
    kVideoResolutionSifLandscape,    // 352x240
    kVideoResolutionSifPortrait,     // 240x352
    kVideoResolutionCifLandscape,    // 352x288
    kVideoResolutionCifPortrait,     // 288x352
    kVideoResolutionVgaLandscape,    // 640x480
    kVideoResolutionVgaPortrait,     // 480x640
    kVideoResolutionHdLandscape,     // 1280x720
    kVideoResolutionHdPortrait,      // 720x1280
    kVideoResolutionFhdLandscape,    // 1920x1280
    kVideoResolutionFhdPortrait,     // 1280x1920
};

enum kConfigFrameType
{
    kConfigSps = 0,
    kConfigPps = 1,
    kConfigVps = 2,
};

struct tCodecConfig
{
    uint32_t nWidth;
    uint32_t nHeight;
    uint32_t nProfile;
    uint32_t nLevel;
};

enum kRtcpFeedbackType
{
    kRtcpFeedbackNone = 0,
    kRtpFbNack = 1,   // Generic NACK
    kRtpFbTmmbr = 3,  // Temoporary Maximum Media Stream Bitrate Request
    kRtpFbTmmbn = 4,  // Temoporary Maximum Media Stream Bitrate Notification
    kPsfbBoundary = 10,
    kPsfbPli = 11,   // Picture Loss Indication
    kPsfbSli = 12,   // Slice Loss Indication
    kPsfbRrsi = 13,  // Reference Picture Selection Indication
    kPsfbFir = 14,   // Full Intra Request - same as "fast video update"
    kPsfbTstr = 15,  // Temporal-Spatial Tradeoff Request - used for changing framerate
    kPsfbTstn = 16,  // Temporal-Spatial Tradeoff Noficiation
    kPsfbVbcm = 17,  // Video Back Channel Message
};

enum kNackRequestType
{
    kRequestNackNone = 0,
    kRequestInitialNack,
    kRequestSecondNack,
    kRequestPli,
};

struct NackParams
{
public:
    NackParams() :
            nFLP(0),
            nBLP(0),
            nSecNACKCnt(0),
            bNACKReport(false)
    {
    }
    NackParams(const NackParams& p)
    {
        nFLP = p.nFLP;
        nBLP = p.nBLP;
        nSecNACKCnt = p.nSecNACKCnt;
        bNACKReport = p.bNACKReport;
    }
    NackParams(uint16_t f, uint16_t b, uint16_t cnt, bool r) :
            nFLP(f),
            nBLP(b),
            nSecNACKCnt(cnt),
            bNACKReport(r)
    {
    }
    uint16_t nFLP;
    uint16_t nBLP;
    uint16_t nSecNACKCnt;
    bool bNACKReport;
};

enum kCameraFacing
{
    kCameraFacingFront = 0,
    kCameraFacingRear,
};

struct LostPktEntry
{
public:
    LostPktEntry() :
            nLostPktSeqNum(0),
            nReqTime(0),
            nNACKReqType(0)
    {
    }
    LostPktEntry(uint16_t seq, uint32_t time, uint32_t type) :
            nLostPktSeqNum(seq),
            nReqTime(time),
            nNACKReqType(type)
    {
    }
    uint16_t nLostPktSeqNum;
    uint32_t nReqTime;
    uint32_t nNACKReqType;
};

struct InternalRequestEventParam
{
public:
    InternalRequestEventParam() :
            type(0),
            value(0)
    {
    }
    InternalRequestEventParam(uint32_t t, uint32_t v) :
            type(t),
            value(v)
    {
    }
    InternalRequestEventParam(uint32_t t, const NackParams& params) :
            type(t),
            nackParams(params)
    {
    }
    uint32_t type;
    union
    {
        uint32_t value;
        NackParams nackParams;
    };
};

/**
 * @brief Utility class for video codec operation.
 */
class ImsMediaVideoUtil
{
public:
    ImsMediaVideoUtil();
    ~ImsMediaVideoUtil();
    static int32_t ConvertCodecType(int32_t type);
    static uint32_t GetResolutionFromSize(uint32_t nWidth, uint32_t nHeight);
    static bool ModifyAvcSpropParameterSet(const uint8_t* inSpropparam, uint8_t* outSpropparam,
            uint32_t nProfile, uint32_t nLevel);
    static ImsMediaResult ParseAvcSpropParam(const char* szSpropparam, tCodecConfig* pInfo);
    static ImsMediaResult ParseHevcSpropParam(const char* szSpropparam, tCodecConfig* pInfo);
    static bool ParseAvcSps(uint8_t* pbBuffer, uint32_t nBufferSize, tCodecConfig* pInfo);
    static bool ParseHevcSps(uint8_t* pbBuffer, uint32_t nBufferSize, tCodecConfig* pInfo);
};

#endif  // IMSMEDIA_VIDEOUTIL_H_INCLUDED