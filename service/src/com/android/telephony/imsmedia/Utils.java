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

package com.android.telephony.imsmedia;

import android.hardware.radio.ims.media.CodecParams;
import android.hardware.radio.ims.media.CodecSpecificParams;
import android.hardware.radio.ims.media.DtmfParams;
import android.hardware.radio.ims.media.RtpAddress;
import android.hardware.radio.ims.media.RtpSessionParams;
import android.net.InetAddresses;
import android.os.Handler;
import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.support.annotation.VisibleForTesting;
import android.telephony.ims.RtpHeaderExtension;
import android.telephony.imsmedia.AmrParams;
import android.telephony.imsmedia.AudioConfig;
import android.telephony.imsmedia.EvsParams;
import android.telephony.imsmedia.MediaQualityThreshold;
import android.telephony.imsmedia.RtcpConfig;
import android.telephony.imsmedia.RtpConfig;
import com.android.telephony.imsmedia.ImsMediaController.OpenSessionCallback;
import java.net.InetSocketAddress;

/**
 * Class consists of utility methods and sub classes
 *
 * @hide
 */
public final class Utils {

    static final int UNUSED = -1;

    /** Class to encapsulate open session parameters */
    static final class OpenSessionParams {
        private final ParcelFileDescriptor rtpFd;
        private final ParcelFileDescriptor rtcpFd;
        private final RtpConfig rtpConfig;
        private final OpenSessionCallback callback;

        OpenSessionParams(final ParcelFileDescriptor rtpFd,
                final ParcelFileDescriptor rtcpFd,
                final RtpConfig rtpConfig,
                final OpenSessionCallback callback) {
            this.rtpFd = rtpFd;
            this.rtcpFd = rtcpFd;
            this.rtpConfig = rtpConfig;
            this.callback = callback;
        }

        ParcelFileDescriptor getRtpFd() {
            return rtpFd;
        }

        ParcelFileDescriptor getRtcpFd() {
            return rtcpFd;
        }

        RtpConfig getRtpConfig() {
            return rtpConfig;
        }

        OpenSessionCallback getCallback() {
            return callback;
        }
    }

    static void sendMessage(final Handler handler, final int command) {
        final Message msg = handler.obtainMessage(command);
        msg.sendToTarget();
    }

    static void sendMessage(final Handler handler, final int command, final Object argument) {
        final Message msg = handler.obtainMessage(command, argument);
        msg.sendToTarget();
    }

    static void sendMessage(final Handler handler,
            final int command, final int arg1, final int arg2) {
        final Message msg = handler.obtainMessage(command, arg1, arg2);
        msg.sendToTarget();
    }

    static void sendMessage(final Handler handler, final int command,
            final int arg1, final int arg2, final Object object) {
        final Message msg = handler.obtainMessage(command, arg1, arg2, object);
        msg.sendToTarget();
    }

    private static RtpAddress buildRtpAddress(final AudioConfig audioConfig) {
        final RtpAddress addr = new RtpAddress();

        addr.ipAddress = audioConfig.getRemoteRtpAddress().getAddress().getHostAddress();
        addr.portNumber = audioConfig.getRemoteRtpAddress().getPort();

        return addr;
    }

    private static DtmfParams buildDtmfParams(final AudioConfig audioConfig) {
        final DtmfParams dtmfParams = new DtmfParams();

        dtmfParams.payloadTypeNumber = audioConfig.getDtmfPayloadTypeNumber();
        dtmfParams.samplingRateKHz = audioConfig.getDtmfSamplingRateKHz();

        return dtmfParams;
    }

    private static android.hardware.radio.ims.media.AmrParams
           buildAmrParams(final AudioConfig audioConfig) {
        final android.hardware.radio.ims.media.AmrParams amrParams =
                new android.hardware.radio.ims.media.AmrParams();

        amrParams.amrMode = audioConfig.getAmrParams().getAmrMode();
        amrParams.octetAligned = audioConfig.getAmrParams().getOctetAligned();
        amrParams.maxRedundancyMillis = audioConfig.getAmrParams().getMaxRedundancyMillis();

        return amrParams;
    }

    private static android.hardware.radio.ims.media.EvsParams
            buildEvsParams(final AudioConfig audioConfig) {
        final android.hardware.radio.ims.media.EvsParams evsParams =
                new android.hardware.radio.ims.media.EvsParams();

        evsParams.bandwidth = audioConfig.getEvsParams().getEvsBandwidth();
        evsParams.evsMode = audioConfig.getEvsParams().getEvsMode();
        evsParams.channelAwareMode = audioConfig.getEvsParams().getChannelAwareMode();
        evsParams.useHeaderFullOnlyOnTx =
                audioConfig.getEvsParams().getUseHeaderFullOnlyOnTx();
        evsParams.useHeaderFullOnlyOnRx =
                audioConfig.getEvsParams().getUseHeaderFullOnlyOnRx();

        return evsParams;
    }

    private static CodecParams buildCodecParams(final AudioConfig audioConfig) {
        final CodecParams codecParams = new CodecParams();

        codecParams.codecType = audioConfig.getCodecType();
        codecParams.rxPayloadTypeNumber = audioConfig.getRxPayloadTypeNumber();
        codecParams.txPayloadTypeNumber = audioConfig.getTxPayloadTypeNumber();
        codecParams.samplingRateKHz = audioConfig.getSamplingRateKHz();
        codecParams.txCodecModeRequest = audioConfig.getTxCodecModeRequest();
        codecParams.dtxEnabled = audioConfig.getDtxEnabled();

        if (audioConfig.getCodecType() == AudioConfig.CODEC_AMR
              || audioConfig.getCodecType() == AudioConfig.CODEC_AMR_WB) {
            codecParams.codecSpecificParams = new CodecSpecificParams();
            codecParams.codecSpecificParams.setAmr(buildAmrParams(audioConfig));
        } else if (audioConfig.getCodecType() == AudioConfig.CODEC_EVS) {
            codecParams.codecSpecificParams = new CodecSpecificParams();
            codecParams.codecSpecificParams.setEvs(buildEvsParams(audioConfig));
        }

        return codecParams;
    }

    private static RtpSessionParams buildSessionParams(final AudioConfig audioConfig) {
        final RtpSessionParams sessionParams = new RtpSessionParams();

        sessionParams.pTimeMillis = audioConfig.getPtimeMillis();
        sessionParams.maxPtimeMillis = audioConfig.getMaxPtimeMillis();
        sessionParams.maxMtuBytes = audioConfig.getMaxMtuBytes();
        sessionParams.dscp = audioConfig.getDscp();
        sessionParams.dtmfParams = buildDtmfParams(audioConfig);
        sessionParams.codecParams = buildCodecParams(audioConfig);

        return sessionParams;
    }

    private static android.hardware.radio.ims.media.RtcpConfig
            buildRtcpConfig(final AudioConfig audioConfig) {
        final android.hardware.radio.ims.media.RtcpConfig rtcpConfig =
                new android.hardware.radio.ims.media.RtcpConfig();

        rtcpConfig.canonicalName = audioConfig.getRtcpConfig().getCanonicalName();
        rtcpConfig.transmitPort = audioConfig.getRtcpConfig().getTransmitPort();
        rtcpConfig.transmitIntervalSec = audioConfig.getRtcpConfig().getIntervalSec();
        rtcpConfig.rtcpXrBlocks = audioConfig.getRtcpConfig().getRtcpXrBlockTypes();

        return rtcpConfig;
    }

    /** Converts {@link AudioConfig} to HAL RtpConfig */
    public static android.hardware.radio.ims.media.RtpConfig convertToRtpConfig(
            final AudioConfig audioConfig) {
        final android.hardware.radio.ims.media.RtpConfig rtpConfig;

        if (audioConfig == null) {
            rtpConfig = null;
        } else {
            rtpConfig = new android.hardware.radio.ims.media.RtpConfig();
            rtpConfig.direction = audioConfig.getMediaDirection();
            rtpConfig.accessNetwork = audioConfig.getAccessNetwork();
            rtpConfig.remoteAddress = buildRtpAddress(audioConfig);
            rtpConfig.sessionParams = buildSessionParams(audioConfig);
            rtpConfig.rtcpConfig = buildRtcpConfig(audioConfig);
        }

        return rtpConfig;
    }

    private static RtcpConfig buildRtcpConfig(
            final android.hardware.radio.ims.media.RtpConfig rtpConfig) {
        final RtcpConfig rtcpConfig;

        if (rtpConfig.rtcpConfig == null) {
            rtcpConfig = null;
        } else {
            rtcpConfig = new RtcpConfig.Builder()
                    .setCanonicalName(rtpConfig.rtcpConfig.canonicalName)
                    .setTransmitPort(rtpConfig.rtcpConfig.transmitPort)
                    .setIntervalSec(rtpConfig.rtcpConfig.transmitIntervalSec)
                    .setRtcpXrBlockTypes(rtpConfig.rtcpConfig.rtcpXrBlocks)
                    .build();
        }

        return rtcpConfig;
    }

    private static EvsParams buildEvsParams(
            final android.hardware.radio.ims.media.RtpConfig rtpConfig) {
        final EvsParams evsParams;

        if (rtpConfig == null
                || rtpConfig.sessionParams == null
                || rtpConfig.sessionParams.codecParams == null
                || rtpConfig.sessionParams.codecParams.codecSpecificParams == null
                || rtpConfig.sessionParams.codecParams.codecSpecificParams.getTag()
                    != CodecSpecificParams.evs) {
            evsParams = null;
        } else {
            final android.hardware.radio.ims.media.EvsParams evs =
                    rtpConfig.sessionParams.codecParams.codecSpecificParams.getEvs();
            evsParams = new EvsParams.Builder()
                .setEvsbandwidth(evs.bandwidth)
                .setEvsMode(evs.evsMode)
                .setChannelAwareMode(evs.channelAwareMode)
                .setHeaderFullOnlyOnTx(evs.useHeaderFullOnlyOnTx)
                .setHeaderFullOnlyOnRx(evs.useHeaderFullOnlyOnRx)
                .build();
        }

        return evsParams;
    }

    private static AmrParams buildAmrParams(
            final android.hardware.radio.ims.media.RtpConfig rtpConfig) {
        final AmrParams amrParams;

        if (rtpConfig == null
                || rtpConfig.sessionParams == null
                || rtpConfig.sessionParams.codecParams == null
                || rtpConfig.sessionParams.codecParams.codecSpecificParams == null
                || rtpConfig.sessionParams.codecParams.codecSpecificParams.getTag()
                    != CodecSpecificParams.amr) {
            amrParams = null;
        } else {
            final android.hardware.radio.ims.media.AmrParams amr =
                    rtpConfig.sessionParams.codecParams.codecSpecificParams.getAmr();
            amrParams = new AmrParams.Builder()
                .setAmrMode(amr.amrMode)
                .setOctetAligned(amr.octetAligned)
                .setMaxRedundancyMillis(amr.maxRedundancyMillis)
                .build();
        }

        return amrParams;
    }

    private static InetSocketAddress buildRtpAddress(
            final android.hardware.radio.ims.media.RtpConfig rtpConfig) {
        final InetSocketAddress rtpAddress;

        if (rtpConfig.remoteAddress == null) {
            rtpAddress = null;
        } else {
            rtpAddress = new InetSocketAddress(
                     InetAddresses.parseNumericAddress(rtpConfig.remoteAddress.ipAddress),
                     rtpConfig.remoteAddress.portNumber);
        }

        return rtpAddress;
    }

    /** Converts HAL RtpConfig to AudioConfig */
    public static AudioConfig convertToAudioConfig(
            final android.hardware.radio.ims.media.RtpConfig rtpConfig) {
        final AudioConfig audioConfig;

        if (rtpConfig == null) {
            audioConfig = null;
        } else {
            audioConfig = new AudioConfig.Builder()
                    .setMediaDirection(rtpConfig.direction)
                    .setAccessNetwork(rtpConfig.accessNetwork)
                    .setRemoteRtpAddress(buildRtpAddress(rtpConfig))
                    .setRtcpConfig(buildRtcpConfig(rtpConfig))
                    .setEvsParams(buildEvsParams(rtpConfig))
                    .setAmrParams(buildAmrParams(rtpConfig))
                    .build();

            /** Populate session related parameters if present */
            if (rtpConfig.sessionParams != null) {
                audioConfig.setMaxMtuBytes(rtpConfig.sessionParams.maxMtuBytes);
                audioConfig.setDscp(rtpConfig.sessionParams.dscp);
                audioConfig.setPtimeMillis(rtpConfig.sessionParams.pTimeMillis);
                audioConfig.setMaxPtimeMillis(rtpConfig.sessionParams.maxPtimeMillis);

                /** Populate DTMF parameter */
                final DtmfParams dtmfParams = rtpConfig.sessionParams.dtmfParams;
                if (dtmfParams != null) {
                    audioConfig.setDtmfPayloadTypeNumber(dtmfParams.payloadTypeNumber);
                    audioConfig.setDtmfSamplingRateKHz(dtmfParams.samplingRateKHz);
                }

                /** Populate codec parameters */
                final CodecParams codecParams = rtpConfig.sessionParams.codecParams;
                if (codecParams != null) {
                    audioConfig.setCodecType(codecParams.codecType);
                    audioConfig.setRxPayloadTypeNumber(codecParams.rxPayloadTypeNumber);
                    audioConfig.setTxPayloadTypeNumber(codecParams.txPayloadTypeNumber);
                    audioConfig.setSamplingRateKHz(codecParams.samplingRateKHz);
                    audioConfig.setTxCodecModeRequest(codecParams.txCodecModeRequest);
                    audioConfig.setDtxEnabled(codecParams.dtxEnabled);
                }
            }
        }

        return audioConfig;
    }

    /** Converts {@link MediaQualityThreshold} to HAL MediaQualityThreshold */
    public static android.hardware.radio.ims.media.MediaQualityThreshold
            convertMediaQualityThreshold(final MediaQualityThreshold in) {
        final android.hardware.radio.ims.media.MediaQualityThreshold out;

        if (in == null) {
            out =  null;
        } else {
            out = new android.hardware.radio.ims.media.MediaQualityThreshold();
            out.rtpInactivityTimerMillis = in.getRtpInactivityTimerMillis();
            out.rtcpInactivityTimerMillis = in.getRtcpInactivityTimerMillis();
            out.rtpPacketLossDurationMillis = in.getPacketLossPeriodMillis();
            out.rtpPacketLossRate = in.getPacketLossThreshold();
            out.jitterDurationMillis = in.getJitterPeriodMillis();
            out.rtpJitterMillis = in.getJitterThresholdMillis();
        }

        return out;
    }

    /** Converts HAL MediaQualityThreshold to {@link MediaQualityThreshold} */
    public static MediaQualityThreshold convertMediaQualityThreshold(
            final android.hardware.radio.ims.media.MediaQualityThreshold in) {
        return (in == null) ? null : new MediaQualityThreshold.Builder()
                .setRtpInactivityTimerMillis(in.rtpInactivityTimerMillis)
                .setRtcpInactivityTimerMillis(in.rtcpInactivityTimerMillis)
                .setPacketLossPeriodMillis(in.rtpPacketLossDurationMillis)
                .setPacketLossThreshold(in.rtpPacketLossRate)
                .setJitterPeriodMillis(in.jitterDurationMillis)
                .setJitterThresholdMillis(in.rtpJitterMillis)
                .build();
    }

    public static android.hardware.radio.ims.media.RtpHeaderExtension
            convertRtpHeaderExtension(final RtpHeaderExtension in) {
        final android.hardware.radio.ims.media.RtpHeaderExtension out;

        if (in == null) {
            out = null;
        } else {
            out = new android.hardware.radio.ims.media.RtpHeaderExtension();
            out.localId = in.getLocalIdentifier();
            out.data = in.getExtensionData();
        }

        return out;
    }

    public static RtpHeaderExtension convertRtpHeaderExtension(
            final android.hardware.radio.ims.media.RtpHeaderExtension in) {
        return (in == null) ? null : new RtpHeaderExtension(in.localId, in.data);
    }
}
