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

#ifndef IMS_MEDIA_HAL_H
#define IMS_MEDIA_HAL_H

#include <string>

#define MAX_RTP_CONFIGS 40

namespace ImsMediaHal {
/** The Media stack state */
enum MediaStackState {
    /** The media stack is not initialized */
    UNINITIALIZED = 0,
    /** The media stack is initialized and ready */
    READY = 1,
    /** The media stack is in unrecoverable error condition */
    ERROR_FATAL = 2,
};

/** The RTP session state */
enum RtpSessionState {
    /** The RTP session is opened and media flow is not started */
    OPEN = 0,
    /** The RTP session has active media flow */
    ACTIVE = 1,
    /** The RTP session is suspended */
    SUSPENDED = 2,
    /** The RTP session is closed */
    CLOSED = 3,
};

enum RtpError {
    /** Success */
    NO_ERROR = 0,
    /** Invalid parameters passed in the request */
    INVALID_PARAM = 1,
    /** The RTP stack is not ready to handle the request */
    NOT_READY = 2,
    /** Unable to handle the request due to memory allocation failure */
    NO_MEMORY = 3,
    /**
     * Unable to handle the request due to no sufficient resources such as
     * Audio output, audio output, codec
     */
    NO_RESOURCES = 4,
    /** The requested port number is not available */
    PORT_UNAVAILABLE = 5,
    /** The request is not supported by the implementation */
    REQUEST_NOT_SUPPORTED = 6,
};

struct RtpResponseInfo{
    /** Serial number for which the response is sent */
    int serial;
    /** Response error */
    RtpError error;
};

/** RTP media flow direction */
enum MediaDirection {
    /** No media flow in either direction */
    NO_FLOW = 0,
    /** Device sends outgoing media and drops incoming media */
    TRANSMIT_ONLY = 1,
    /** Device receives the downlink media and does not transmit any uplink media */
    RECEIVE_ONLY = 2,
    /** Device sends and receive media in both directions */
    TRANSMIT_RECEIVE = 3,
};

struct MediaJitterBuffer {
    /** Minimum (default) jitter buffer size in milliseconds unit */
    int minJitterBufferSizeMillis;
    /** Maximum jitter buffer size allowed in milliseconds unit*/
    int maxJitterBufferSizeMillis;
    /**
     * The duration in millisecond during which the network status is good to
     * reduce the jitter buffer size
     */
    int jitterBufferAdjustTimerMillis;
    /**
     * The duration in milliseconds that how the jitter buffer size increase or
     * decrease at a time.
     */
    int bufferStepSizeMillis;
};

/** DTMF parameters for the session, see RFC 2833 */
struct DtmfParams{
    /**
     * Dynamic payload type number to be used for DTMF RTP packets. The values is
     * in the range from 96 to 127 chosen during the session establishment. The PT
     * value of the RTP header of all DTMF packets shall be set with this value.
     */
    char payloadTypeNumber;
    /** Sampling rate in kHz */
    char samplingRateKHz;
};

/** Speech codec types, See 3gpp spec 26.103 */
enum CodecType {
    /** Adaptive Multi-Rate */
    AMR = 1 << 0,
    /** Adaptive Multi-Rate Wide Band */
    AMR_WB = 1 << 1,
    /** Enhanced Voice Services */
    EVS = 1 << 2,
    /** G.711 A-law i.e. Pulse Code Modulation using A-law */
    PCMA = 1 << 3,
    /** G.711 μ-law i.e. Pulse Code Modulation using μ-law */
    PCMU = 1 << 4,
};

/** EVS Speech codec bandwidths, See 3gpp spec 26.441 Table 1 */
enum EvsBandwidth {
    NONE = 0,
    NARROW_BAND = 1 << 0,
    WIDE_BAND = 1 << 1,
    SUPER_WIDE_BAND = 1 << 2,
    FULL_BAND = 1 << 3,
};

/** AMR codec mode to represent the bit rate. See 3ggp Specs 26.976 & 26.071 */
enum AmrMode {
    /** 4.75 kbps for AMR / 6.6 kbps for AMR-WB */
    AMR_MODE_0 = 0,
    /** 5.15 kbps for AMR / 8.855 kbps for AMR-WB */
    AMR_MODE_1 = 1,
    /** 5.9 kbps for AMR / 12.65 kbps for AMR-WB */
    AMR_MODE_2 = 2,
    /** 6.7 kbps for AMR / 14.25 kbps for AMR-WB */
    AMR_MODE_3 = 3,
    /** 7.4 kbps for AMR / 15.85 kbps for AMR-WB */
    AMR_MODE_4 = 4,
    /** 7.95 kbps for AMR / 18.25 kbps for AMR-WB */
    AMR_MODE_5 = 5,
    /** 10.2 kbps for AMR / 19.85 kbps for AMR-WB */
    AMR_MODE_6 = 6,
    /** 12.2 kbps for AMR / 23.05 kbps for AMR-WB */
    AMR_MODE_7 = 7,
    /** Silence frame for AMR / 23.85 kbps for AMR-WB */
    AMR_MODE_8 = 8,
};

/** EVS codec mode to represent the bit rate. See 3ggp Spec 26.952 Table 5.1 */
enum EvsMode {
    /** 6.6 kbps for EVS AMR-WB IO */
    EVS_MODE_0 = 0,
    /** 8.855 kbps for AMR-WB IO */
    EVS_MODE_1 = 1,
    /** 12.65 kbps for AMR-WB IO */
    EVS_MODE_2 = 2,
    /** 14.25 kbps for AMR-WB IO */
    EVS_MODE_3 = 3,
    /** 15.85 kbps for AMR-WB IO */
    EVS_MODE_4 = 4,
    /** 18.25 kbps for AMR-WB IO */
    EVS_MODE_5 = 5,
    /** 19.85 kbps for AMR-WB IO */
    EVS_MODE_6 = 6,
    /** 23.05 kbps for AMR-WB IO */
    EVS_MODE_7 = 7,
    /** 23.85 kbps for AMR-WB IO */
    EVS_MODE_8 = 8,
    /** 5.9 kbps for EVS primary */
    EVS_MODE_9 = 9,
    /** 7.2 kbps for EVS primary */
    EVS_MODE_10 = 10,
    /** 8.0 kbps for EVS primary */
    EVS_MODE_11 = 11,
    /** 9.6 kbps for EVS primary */
    EVS_MODE_12 = 12,
    /** 13.2 kbps for EVS primary */
    EVS_MODE_13 = 13,
    /** 16.4 kbps for EVS primary */
    EVS_MODE_14 = 14,
    /** 24.4 kbps for EVS primary */
    EVS_MODE_15 = 15,
    /** 32.0 kbps for EVS primary */
    EVS_MODE_16 = 16,
    /** 48.0 kbps for EVS primary */
    EVS_MODE_17 = 17,
    /** 64.0 kbps for EVS primary */
    EVS_MODE_18 = 18,
    /** 96.0 kbps for EVS primary */
    EVS_MODE_19 = 19,
    /** 128.0 kbps for EVS primary */
    EVS_MODE_20 = 20,
};

struct AmrParams {
    /** mode-set: AMR codec mode to represent the bit rate */
    AmrMode amrMode;
    /**
     * octet-align: If it's set to true then all fields in the AMR/AMR-WB header
     * shall be aligned to octet boundaries by adding padding bits.
     */
    bool octetAligned;
    /**
     * max-red: It’s the maximum duration in milliseconds that elapses between the
     * primary (first) transmission of a frame and any redundant transmission that
     * the sender will use. This parameter allows a receiver to have a bounded delay
     * when redundancy is used. Allowed values are between 0 (no redundancy will be
     * used) and 65535. If the parameter is omitted, no limitation on the use of
     * redundancy is present. See RFC 4867
     */
    int maxRedundancyMillis;
};

struct EvsParams {
    /** mode-set: EVS codec mode set to represent the bit rate */
    EvsMode evsModeSet;
    /**
     * ch-aw-recv: Channel aware mode for the receive direction. Permissible values
     * are -1, 0, 2, 3, 5, and 7. If -1, channel-aware mode is disabled in the
     * session for the receive direction. If 0 or not present, partial redundancy
     * (channel-aware mode) is not used at the start of the session for the receive
     * direction. If positive (2, 3, 5, or 7), partial redundancy (channel-aware
     * mode) is used at the start of the session for the receive direction using the
     * value as the offset, See 3GPP TS 26.445 section 4.4.5
     */
    char channelAwareMode;
    /**
     * hf-only: Header full only is used for the outgoing packets. If it's true then
     * the session shall support header full format only else the session could
     * support both header full format and compact format.
     */
    bool useHeaderFullOnlyOnTx;
    /**
     * hf-only: Header full only used on the incoming packets. If it's true then the
     * session shall support header full format only else the session could support
     * both header full format and compact format.
     */
    bool useHeaderFullOnlyOnRx;
};

/**
 * RTP Control Protocol Extended Reports (RTCP XR) Blocks,
 * See RFC 3611 section 4
 */
enum RtcpXrReportBlockType
{
    /** Disable RTCP XR */
    RTCPXR_NONE = 0,
    /** Loss RLE Report Block */
    RTCPXR_LOSS_RLE_REPORT_BLOCK                  = 1 << 0,
    /** Duplicate RLE Report Block */
    RTCPXR_DUPLICATE_RLE_REPORT_BLOCK             = 1 << 1,
    /** Packet Receipt Times Report Block */
    RTCPXR_PACKET_RECEIPT_TIMES_REPORT_BLOCK      = 1 << 2,
    /** Receiver Reference Time Report Block */
    RTCPXR_RECEIVER_REFERENCE_TIME_REPORT_BLOCK   = 1 << 3,
    /** DLRR Report Block */
    RTCPXR_DLRR_REPORT_BLOCK                      = 1 << 4,
    /** Statistics Summary Report Block */
    RTCPXR_STATISTICS_SUMMARY_REPORT_BLOCK        = 1 << 5,
    /** VoIP Metrics Report Block */
    RTCPXR_VOIP_METRICS_REPORT_BLOCK              = 1 << 6,
};

/** RTP Header Extensions, see RFC 8285 */
struct RtpHeaderExtension {
    /** Local identifier */
    int localId;
    /** Extension data char */
    char data;
};

struct MediaQualityThreshold {
    /** Timer in milliseconds for monitoring RTP inactivity */
    int rtpInactivityTimerMillis;
    /** Timer in milliseconds for monitoring RTCP inactivity */
    int rtcpInactivityTimerMillis;
    /** Duration in milliseconds for monitoring the RTP packet loss rate */
    int rtpPacketLossDurationMillis;
    /**
     * Packet loss rate in percentage of (total number of packets lost) /
     * (total number of packets expected) during rtpPacketLossDurationMs
     */
    int rtpPacketLossRate;
    /** Duration in milliseconds for monitoring the jitter for RTP traffic */
    int jitterDurationMillis;
    /** RTP jitter threshold in milliseconds */
    int rtpJitterMillis;
};

enum MediaProtocolType {
   /** Real Time Protocol, see RFC 3550 */
   RTP = 0,
   /** Real Time Control Protocol, see RFC 3550 */
   RTCP = 1,
};

struct SpeechCodec {
    /** Codec type */
    CodecType codecType;
    /** Codec bandwidth in case of EVS codec */
    int bandwidth;
};

union CodecSpecificParams {
    AmrParams amr;
    EvsParams evs;
};

struct CodecParams {
    /** Negotiated codec and bandwidth */
    SpeechCodec codec;
    /**
     * Static or dynamic payload type number negotiated through the SDP for
     * the incoming RTP packets. This value shall be matched with the PT value
     * of the incoming RTP header. Values 0 to 127, see RFC 3551 section 6
     */
    int rxPayloadTypeNumber;
    /**
     * Static or dynamic payload type number negotiated through the SDP for
     * the outgoing RTP packets. This value shall be set to the PT value
     * of the outgoing RTP header. Values 0 to 127, see RFC 3551 section 6
     */
    int txPayloadTypeNumber;
    /** Sampling rate in kHz*/
    char samplingRateKHz;
    /**
     * cmr: Codec mode request is used to request the speech codec encoder of the
     * other party to set the frame type index of speech mode via RTP header, See RFC
     * 4867 section 4.3.1. Allowed values are -1, 0 and 1.
     */
    char txCodecModeRequest;
    /** dtx: Whether discontinuous transmission is enabled or not */
    bool dtxEnabled;
    /** Codec specific parameters */
    CodecSpecificParams codecSpecificParams;
};

struct RtpSessionParams {
    /**
     * ptime: Recommended length of time in milliseconds represented by the media
     * in each packet, see RFC 4566
     */
    char pTimeMillis;
    /**
     * maxptime: Maximum amount of media that can be encapsulated in each packet
     * represented in milliseconds, see RFC 4566
     */
    char maxPtimeMillis;
    /** Maximum Rtp transfer unit in bytes */
    int maxMtuBytes;
    /** dscp: Differentiated Services Field Code Point value, see RFC 2474 */
    int dscp;
    /** DTMF payload and clock rate */
    DtmfParams dtmfParams;
    /** Negotiated codec parameters */
    CodecParams codecParams;
    /** Jitter buffer parameters */
    MediaJitterBuffer jitterBufferParams;
};

struct RtpAddress {
    /** Point to point IP address */
    std::string ipAddress;
    /** UDP port number used for the RTP traffic */
    int portNumber;
};

struct RtcpConfig {
    /** Canonical name that will be sent to all session participants */
    std::string canonicalName;
    /** Port for sending outgoing RTCP packets */
    int transmitPort;
    /** Port where incoming RTCP packets are received */
    int receivePort;
    /**
     * Transmit interval in seconds. Value 0 indicates that RTCP reports
     * should not be reported.
     */
    int transmitIntervalSec;
    /** Bitmask of RTCP-XR blocks to enable as in RtcpXrReportBlockType */
    int rtcpXrBlocks;
};

//import android.hardware.radio.data.QosEvsBandwidth;
struct RtpConfig {
    /** Media flow direction */
    MediaDirection direction;
    /** IP address and port number of the other party */
    RtpAddress remoteAddress;
    /** Negotiated session parameters */
    RtpSessionParams sessionParams;
    /** RTCP configuration */
    RtcpConfig rtcpConfig;
    /** Downlink bandwidth allocated by network in the dedicated bearer */
    //QosEvsBandwidth downlink;
    /** Uplink bandwidth allocated by network in the dedicated bearer */
    //QosEvsBandwidth uplink;
};

struct LocalEndPoint {
    /** IP address/port on the IMS network where the socket needs to be open */
    RtpAddress localAddress;
    /** The logical modem ID, returned by IRadioConfig.getPhoneCapability() */
    int modemId;
};

struct RtpSession {
    /** Unique identifier of the session */
    int sessionId;
    /** Local RTP address and logical modem identity */
    LocalEndPoint localEndPoint;
    /** RTP session state */
    RtpSessionState sessionState;
    /** List of remote configurations associated with this session */
    RtpConfig remoteConfigs[MAX_RTP_CONFIGS];
};

struct OpenSessionParams {
    int mRtpFd;
    int mRtcpFd;
    RtpConfig mRtpConfig;
};
}
#endif