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

#ifndef IMS_MEDIA_DEFINE_H
#define IMS_MEDIA_DEFINE_H

#include <RtpConfig.h>
#include <AudioConfig.h>
#include <string.h>

using namespace android::telephony::imsmedia;

enum ImsMediaResult
{
    RESULT_SUCCESS = 0,
    RESULT_INVALID_PARAM,
    RESULT_NOT_READY,
    RESULT_NO_MEMORY,
    RESULT_NO_RESOURCES,
    RESULT_PORT_UNAVAILABLE,
    RESULT_NOT_SUPPORTED,
};

enum ImsMediaEventType
{
    EVENT_NOTIFY_ERROR,
    EVENT_NOTIFY_FIRST_MEDIA_PACKET_RECEIVED,
    EVENT_NOTIFY_HEADER_EXTENSION_RECEIVED,
    EVENT_NOTIFY_MEDIA_INACITIVITY,
    EVENT_NOTIFY_PACKET_LOSS,
    EVENT_NOTIFY_JITTER,
};

enum ImsMediaNotifyType
{
    RECV_AUDIO_RX_PACKET_RECEIVED = 0,
    SEND_AUDIO_TX_FIRST_PACKET_SENT = 1,
    ERROR_RTP_TIMEOUT_NO_AUDIO_RX_PACKET,
    ERROR_SOCKET,
    DTMF_KEY_0 = 600,
    DTMF_KEY_1,
    DTMF_KEY_2,
    DTMF_KEY_3,
    DTMF_KEY_4,
    DTMF_KEY_5,
    DTMF_KEY_6,
    DTMF_KEY_7,
    DTMF_KEY_8,
    DTMF_KEY_9,
    DTMF_KEY_STAR,
    DTMF_KEY_POUND,
    DTMF_KEY_A,
    DTMF_KEY_B,
    DTMF_KEY_C,
    DTMF_KEY_D,
};

enum ImsMediaStreamType
{
    STREAM_MODE_RTP_TX,
    STREAM_MODE_RTP_RX,
    STREAM_MODE_RTCP,
};

enum ImsMediaType
{
    IMS_MEDIA_AUDIO = 0,
    IMS_MEDIA_VIDEO,
    IMS_MEDIA_TEXT,
};

enum ImsMediaProtocolType
{
    RTP = 0,
    RTCP,
};

enum kEvsBandwidth
{
    kEvsBandwidthNone = 0,
    kEvsBandwidthNB = 1,
    kEvsBandwidthWB = 2,
    kEvsBandwidthSWB = 4,
    kEvsBandwidthFB = 8,
};

enum kEvsBitrate
{
    /* 6.6 kbps, AMR-IO */
    kEvsAmrIoModeBitrate00660 = 0,
    /* 8.85 kbps, AMR-IO */
    kEvsAmrIoModeBitrate00885 = 1,
    /* 12.65 kbps, AMR-IO */
    kEvsAmrIoModeBitrate01265 = 2,
    /* 14.25 kbps, AMR-IO */
    kEvsAmrIoModeBitrate01425 = 3,
    /* 15.85 kbps, AMR-IO */
    kEvsAmrIoModeBitrate01585 = 4,
    /* 18.25 kbps, AMR-IO */
    kEvsAmrIoModeBitrate01825 = 5,
    /* 19.85 kbps, AMR-IO */
    kEvsAmrIoModeBitrate01985 = 6,
    /* 23.05 kbps, AMR-IO */
    kEvsAmrIoModeBitrate02305 = 7,
    /* 23.85 kbps, AMR-IO */
    kEvsAmrIoModeBitrate02385 = 8,
    /* 5.9 kbps, EVS Primary - SC-VBR 2.8kbps, 7.2kbps, 8kbps*/
    kEvsPrimaryModeBitrate00590 = 9,
    /* 7.2 kbps, EVS Primary */
    kEvsPrimaryModeBitrate00720 = 10,
    /* 8 kbps, EVS Primary */
    kEvsPrimaryModeBitrate00800 = 11,
    /* 9.6 kbps, EVS Primary */
    kEvsPrimaryModeBitrate00960 = 12,
    /* 13.20 kbps, EVS Primary */
    kEvsPrimaryModeBitrate01320 = 13,
    /* 16.4 kbps, EVS Primary */
    kEvsPrimaryModeBitrate01640 = 14,
    /* 24.4 kbps, EVS Primary */
    kEvsPrimaryModeBitrate02440 = 15,
    /* 32 kbps, EVS Primary */
    kEvsPrimaryModeBitrate03200 = 16,
    /* 48 kbps, EVS Primary */
    kEvsPrimaryModeBitrate04800 = 17,
    /* 64 kbps, EVS Primary */
    kEvsPrimaryModeBitrate06400 = 18,
    /* 96 kbps, EVS Primary */
    kEvsPrimaryModeBitrate09600 = 19,
    /* 128 kbps, EVS Primary */
    kEvsPrimaryModeBitrate12800 = 20,
    /* 2.4 kbps, EVS Primary */
    kEvsPrimaryModeBitrateSID = 21,
    /* SPEECH LOST */
    kEvsPrimaryModeBitrateSpeechLost = 22,
    /* NO DATA */
    kEvsPrimaryModeBitrateNoData = 23,
};

enum kEvsCodecMode
{
    kEvsCodecModePrimary = 0,  // EVS PRIMARY mode 0
    kEvsCodecModeAmrIo = 1,    // EVS AMR-WB IO mode 1
    kEvsCodecModeMax = 0x7FFFFFFF
};

enum kEvsCmrCodeType
{
    kEvsCmrCodeTypeNb = 0,      // 000
    kEvsCmrCodeTypeAmrIO = 1,   // 001
    kEvsCmrCodeTypeWb = 2,      // 010
    kEvsCmrCodeTypeSwb = 3,     // 011
    kEvsCmrCodeTypeFb = 4,      // 100
    kEvsCmrCodeTypeWbCha = 5,   // 101
    kEvsCmrCodeTypeSwbCha = 6,  // 110
    kEvsCmrCodeTypeNoReq = 7,   // 111
};

enum kEvsCmrCodeDefine
{
    kEvsCmrCodeDefine59 = 0,      // 0000
    kEvsCmrCodeDefine72 = 1,      // 0001
    kEvsCmrCodeDefine80 = 2,      // 0010
    kEvsCmrCodeDefine96 = 3,      // 0011
    kEvsCmrCodeDefine132 = 4,     // 0100
    kEvsCmrCodeDefine164 = 5,     // 0101
    kEvsCmrCodeDefine244 = 6,     // 0110
    kEvsCmrCodeDefine320 = 7,     // 0111
    kEvsCmrCodeDefine480 = 8,     // 1000
    kEvsCmrCodeDefine640 = 9,     // 1001
    kEvsCmrCodeDefine960 = 10,    // 1010
    kEvsCmrCodeDefine1280 = 11,   // 1011
    kEvsCmrCodeDefineNoReq = 15,  // 1111

    // Ch-A
    kEvsCmrCodeDefineChaOffset2 = 0,   // 0000
    kEvsCmrCodeDefineChaOffset3 = 1,   // 0001
    kEvsCmrCodeDefineChaOffset5 = 2,   // 0010
    kEvsCmrCodeDefineChaOffset7 = 3,   // 0011
    kEvsCmrCodeDefineChaOffsetH2 = 4,  // 0100
    kEvsCmrCodeDefineChaOffsetH3 = 5,  // 0101
    kEvsCmrCodeDefineChaOffsetH5 = 6,  // 0110
    kEvsCmrCodeDefineChaOffsetH7 = 7,  // 0111

    // AMR WB-IO
    kEvsCmrCodeDefineAmrIo660 = 0,   // 0000
    kEvsCmrCodeDefineAmrIo885 = 1,   // 0001
    kEvsCmrCodeDefineAmrIo1265 = 2,  // 0010
    kEvsCmrCodeDefineAmrIo1425 = 3,  // 0011
    kEvsCmrCodeDefineAmrIo1585 = 4,  // 0100
    kEvsCmrCodeDefineAmrIo1825 = 5,  // 0101
    kEvsCmrCodeDefineAmrIo1985 = 6,  // 0110
    kEvsCmrCodeDefineAmrIo2305 = 7,  // 0111
    kEvsCmrCodeDefineAmrIo2385 = 8,  // 1000

    kEvsCmrCodeDefineENUM_MAX = 0x7FFFFFFF
};

enum ImsMediaSubType
{
    MEDIASUBTYPE_UNDEFINED,
    // rtp payload header + encoded bitstream
    MEDIASUBTYPE_RTPPAYLOAD,
    // rtp packet
    MEDIASUBTYPE_RTPPACKET,
    // rtcp packet
    MEDIASUBTYPE_RTCPPACKET,
    // rtcp packet
    MEDIASUBTYPE_RTCPPACKET_BYE,
    // raw yuv or pcm data
    MEDIASUBTYPE_RAWDATA,
    MEDIASUBTYPE_RAWDATA_ROT90,
    MEDIASUBTYPE_RAWDATA_ROT90_FLIP,
    MEDIASUBTYPE_RAWDATA_ROT270,
    MEDIASUBTYPE_RAWDATA_CROP_ROT90,
    MEDIASUBTYPE_RAWDATA_CROP_ROT90_FLIP,
    MEDIASUBTYPE_RAWDATA_CROP_ROT270,
    MEDIASUBTYPE_RAWDATA_CROP,
    // dtmf packet with start bit set
    MEDIASUBTYPE_DTMFSTART,
    // dtmf payload
    MEDIASUBTYPE_DTMF_PAYLOAD,
    // dtmf packet with end bit set
    MEDIASUBTYPE_DTMFEND,
    // EVRC-B
    MEDIASUBTYPE_DTXSTART,
    // encoded bitstream of h.263 codec
    MEDIASUBTYPE_BITSTREAM_H263,
    // encoded bitstream of mpeg 4 codec
    MEDIASUBTYPE_BITSTREAM_MPEG4,
    // encoded bitstream of h.264 codec
    MEDIASUBTYPE_BITSTREAM_H264,
    // encoded bitstream of hevc codec
    MEDIASUBTYPE_BITSTREAM_HEVC,
    // encoded bitstream of pcmu
    MEDIASUBTYPE_BITSTREAM_G711_PCMU,
    // encoded bitstream of pcma
    MEDIASUBTYPE_BITSTREAM_G711_PCMA,
    MEDIASUBTYPE_BITSTREAM_AMR_WB,
    MEDIASUBTYPE_BITSTREAM_AMR,
    MEDIASUBTYPE_REFRESHED,
    // rtt bitstream of t.140 format
    MEDIASUBTYPE_BITSTREAM_T140,
    // rtt bitstream of t.140 redundant format
    MEDIASUBTYPE_BITSTREAM_T140_RED,
    MEDIASUBTYPE_PCM_DATA,
    MEDIASUBTYPE_PCM_NO_DATA,
    // Jitter Buffer GetData not ready
    MEDIASUBTYPE_NOT_READY,
    MEDIASUBTYPE_BITSTREAM_CODECCONFIG,
    MEDIASUBTYPE_MAX
};

enum ImsMediaAudioMsgRequest
{
    kAudioOpenSession = 101,
    kAudioCloseSession,
    kAudioModifySession,
    kAudioAddConfig,
    kAudioDeleteConfig,
    kAudioConfirmConfig,
    kAudioSendDtmf,
    kAudioSendHeaderExtension,
    kAudioSetMediaQualityThreshold,
};

enum ImsMediaAudioMsgResponse
{
    kAudioOpenSessionSuccess = 201,
    kAudioOpenSessionFailure,
    kAudioModifySessionResponse,
    kAudioAddConfigResponse,
    kAudioConfirmConfigResponse,
    kAudioSessionChangedInd,
    kAudioFirstMediaPacketInd,
    kAudioRtpHeaderExtensionInd,
    kAudioMediaInactivityInd,
    kAudioPacketLossInd,
    kAudioJitterInd,
};

struct EventParamOpenSession
{
public:
    void* mConfig;
    int rtpFd;
    int rtcpFd;
    EventParamOpenSession(int rtp, int rtcp, void* config) :
            mConfig(config),
            rtpFd(rtp),
            rtcpFd(rtcp)
    {
    }
};

struct EventParamDtmf
{
public:
    char digit;
    int duration;

    EventParamDtmf(char dig, int d)
    {
        digit = dig;
        duration = d;
    }
};

enum kAudioCodecType
{
    kAudioCodecNone = 0,
    kAudioCodecAmr,
    kAudioCodecAmrWb,
    kAudioCodecPcmu,
    kAudioCodecPcma,
    kAudioCodecEvs,
};

enum kRtpPyaloadHeaderMode
{
    // Amr mode
    kRtpPyaloadHeaderModeAmrOctetAligned = 0,  // octet aligned mode
    kRtpPyaloadHeaderModeAmrEfficient = 1,     // efficient mode
                                               // Video packetization mode
    kRtpPyaloadHeaderModeSingleNalUnit = 0,    // packet mode 0
    kRtpPyaloadHeaderModeNonInterleaved = 1,   // packet mode 1
                                               // Evs mode
    kRtpPyaloadHeaderModeEvsCompact = 0,       // EVS compact format 0
    kRtpPyaloadHeaderModeEvsHeaderFull = 1,    // EVS header-full format 1
    kRtpPyaloadHeaderModeMax
};

enum eIPVersion
{
    IPV4,
    IPV6,
};

enum StreamState
{
    kStreamStateIdle,
    kStreamStateCreated,
    kStreamStateRunning,
    /**
     * Video state wait surface in stating
     */
    kStreamStateWaitSurface,
};

enum SessionState
{
    /** The state that the session is created but graph is not created */
    kSessionStateOpen,
    /** The state that the session is created and the Rtp StreamGraphs are running */
    kSessionStateActive,
    /** The state that the session is created and the Rtp StreamGraphs is not running */
    kSessionStateSuspended,
    /** The state that the session is closed */
    kSessionStateClosed,
};

enum eSocketOpt
{
    SOCKET_OPT_BASE = 0,
    SOCKET_OPT_IP_QOS = 1,
};

struct tRtpHeaderExtensionInfo
{
    uint16_t nDefinedByProfile;
    uint16_t nLength;
    uint16_t nExtensionData;
};

#define MAX_IP_LEN       128
#define MAX_REMOTE_POINT 40

class RtpAddress
{
public:
    RtpAddress(const char* ip = NULL, uint32_t p = 0)
    {
        memset(this->ipAddress, 0, MAX_IP_LEN);
        if (ip != NULL)
        {
            std::strncpy(ipAddress, ip, MAX_IP_LEN);
        }
        port = p;
    }
    ~RtpAddress() {}
    RtpAddress(const RtpAddress& address) :
            port(address.port)
    {
        memset(this->ipAddress, 0, MAX_IP_LEN);
        std::strncpy(this->ipAddress, address.ipAddress, MAX_IP_LEN);
    }
    RtpAddress& operator=(const RtpAddress& address)
    {
        memset(this->ipAddress, 0, MAX_IP_LEN);
        std::strncpy(this->ipAddress, address.ipAddress, MAX_IP_LEN);
        this->port = address.port;
        return *this;
    }
    bool operator==(const RtpAddress& address)
    {
        return (std::strcmp(this->ipAddress, address.ipAddress) == 0 && this->port == address.port);
    }
    bool operator!=(const RtpAddress& address)
    {
        return (std::strcmp(this->ipAddress, address.ipAddress) != 0 || this->port != address.port);
    }
    char ipAddress[MAX_IP_LEN];
    uint32_t port;
};

#endif
