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

enum ImsMediaResult {
    IMS_MEDIA_OK = 0,
    IMS_MEDIA_ERROR_UNKNOWN,
    IMS_MEDIA_ERROR_INVALID_REQUEST,
    IMS_MEDIA_ERROR_INVALID_ARGUMENT,
};

enum ImsMediaEventType {
    EVENT_NOTIFY_ERROR,
    EVENT_NOTIFY_FIRST_MEDIA_PACKET_RECEIVED,
    EVENT_NOTIFY_HEADER_EXTENSION_RECEIVED,
    EVENT_NOTIFY_MEDIA_INACITIVITY,
    EVENT_NOTIFY_PACKET_LOSS,
    EVENT_NOTIFY_JITTER,
};

enum ImsMediaNotifyType {
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

enum ImsMediaStreamType {
    STREAM_MODE_RTP_TX,
    STREAM_MODE_RTP_RX,
    STREAM_MODE_RTCP,
};

enum ImsMediaType {
    IMS_MEDIA_AUDIO = 0,
    IMS_MEDIA_VIDEO,
    IMS_MEDIA_TEXT,
};

enum ImsMediaProtocolType {
    RTP = 0,
    RTCP,
};

enum ImsMediaSubType {
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
    MEDIASUBTYPE_BITSTREAM_EVRC,
    MEDIASUBTYPE_BITSTREAM_EVRC_B,
    MEDIASUBTYPE_BITSTREAM_AMR_WB,
    MEDIASUBTYPE_BITSTREAM_AMR,
    //rtt bitstream of t.140 format
    MEDIASUBTYPE_BITSTREAM_T140,
    //rtt bitstream of t.140 redundant format
    MEDIASUBTYPE_BITSTREAM_T140_RED,
    MEDIASUBTYPE_PCM_DATA,
    MEDIASUBTYPE_PCM_NO_DATA,
    // Jitter Buffer GetData not ready
    MEDIASUBTYPE_NOT_READY,
    MEDIASUBTYPE_BITSTREAM_CODECCONFIG,
    MEDIASUBTYPE_MAX
};

enum ImsMediaVoiceMsgRequest {
    OPEN_SESSION = 101,
    CLOSE_SESSION,
    MODIFY_SESSION,
    ADD_CONFIG,
    CONFIRM_CONFIG,
    DELETE_CONFIG,
    START_DTMF,
    STOP_DTMF,
    SEND_HEADER_EXTENSION,
    SET_MEDIA_QUALITY_THRESHOLD,
};

enum ImsMediaVoiceMsgResponse {
    OPEN_SUCCESS = 201,
    OPEN_FAILURE,
    MODIFY_SESSION_RESPONSE,
    ADD_CONFIG_RESPONSE,
    CONFIRM_CONFIG_RESPONSE,
    SESSION_CHANGED_IND,
    FIRST_MEDIA_PACKET_IND,
    RTP_HEADER_EXTENSION_IND,
    MEDIA_INACITIVITY_IND,
    PACKET_LOSS_IND,
    JITTER_IND,
};

enum ImsMediaResponse {
    RESPONSE_FAIL = 0,
    RESPONSE_SUCCESS,
};

struct EventParamOpenSession {
public:
    AudioConfig* mConfig;
    int rtpFd;
    int rtcpFd;
    EventParamOpenSession(int rtp, int rtcp, AudioConfig* config)
        : mConfig(config), rtpFd(rtp), rtcpFd(rtcp) {
    }
};

struct EventParamDtmf {
public:
    char digit;
    int volume;
    int duration;

    EventParamDtmf(char dig, int v, int d) {
        digit = dig;
        volume = v;
        duration = d;
    }
};

enum eAudioCodecType {
    AUDIO_CODEC_NONE = 0,
    AUDIO_EVRC,
    AUDIO_EVRC_B,
    AUDIO_AMR,
    AUDIO_AMR_WB,
    AUDIO_G711_PCMU,
    AUDIO_G711_PCMA,
    AUDIO_AAC,
    AUDIO_EVS,
    AUDIO_MAX,
};

enum eRTPPyaloadHeaderMode {
    // evrc mode
    RTPPAYLOADHEADER_MODE_EVRC_BUNDLE = 0,
    RTPPAYLOADHEADER_MODE_EVRC_COMPACT = 1, // evrc encoder should generate fixed rate stream
    RTPPAYLOADHEADER_MODE_EVRC_FREEHEADER = 2,
    // amr mode
    RTPPAYLOADHEADER_MODE_AMR_OCTETALIGNED = 0, // octet aligned mode
    RTPPAYLOADHEADER_MODE_AMR_EFFICIENT = 1, // efficient mode
    // h.264 mode
    RTPPAYLOADHEADER_MODE_H264_SINGLE_NAL_UNIT = 0, // packet mode 0
    RTPPAYLOADHEADER_MODE_H264_NON_INTERLEAVED = 1, // packet mode 1
    // evs mode
    RTPPAYLOADHEADER_MODE_EVS_COMPACT = 0, // EVS compact format 0
    RTPPAYLOADHEADER_MODE_EVS_HEADER_FULL = 1, // EVS header-full format 1
    RTPPAYLOADHEADER_MODE_MAX
};

enum eIPVersion {
    IPV4,
    IPV6,
};

enum StreamState {
    STATE_NULL,
    STATE_CREATED,
    STATE_RUN,
};

enum eSocketOpt {
    SOCKET_OPT_BASE = 0,
    SOCKET_OPT_IP_QOS = 1,
};

struct tRtpHeaderExtensionInfo {
    uint16_t nDefinedByProfile;
    uint16_t nLength;
    uint16_t nExtensionData;
};

#define MAX_IP_LEN 128
#define MAX_REMOTE_POINT 40

class RtpAddress {
public:
    RtpAddress(const char* ip = NULL, uint32_t p = 0) {
        memset(this->ipAddress, 0, MAX_IP_LEN);
        if (ip != NULL) {
            std::strncpy(ipAddress, ip, MAX_IP_LEN);
        }
        port = p;
    }
    ~RtpAddress() {
    }
    RtpAddress(const RtpAddress& address) : port(address.port) {
        memset(this->ipAddress, 0, MAX_IP_LEN);
        std::strncpy(this->ipAddress, address.ipAddress, MAX_IP_LEN);
    }
    RtpAddress& operator=(const RtpAddress& address) {
        memset(this->ipAddress, 0, MAX_IP_LEN);
        std::strncpy(this->ipAddress, address.ipAddress, MAX_IP_LEN);
        this->port = address.port;
        return *this;
    }
    bool operator==(const RtpAddress& address) {
        return (std::strcmp(this->ipAddress, address.ipAddress) == 0
            && this->port == address.port);
    }
    bool operator!=(const RtpAddress& address) {
        return (std::strcmp(this->ipAddress, address.ipAddress) != 0
            || this->port != address.port);
    }
    char ipAddress[MAX_IP_LEN];
    uint32_t port;
};

#endif