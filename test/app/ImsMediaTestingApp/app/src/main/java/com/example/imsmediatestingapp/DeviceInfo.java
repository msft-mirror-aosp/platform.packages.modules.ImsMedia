package com.example.imsmediatestingapp;

import android.hardware.radio.ims.media.AmrMode;
import android.hardware.radio.ims.media.CodecType;
import android.hardware.radio.ims.media.EvsBandwidth;
import android.hardware.radio.ims.media.EvsMode;
import androidx.annotation.NonNull;
import com.example.imsmediatestingapp.MainActivity.VideoCodecEnum;
import java.io.Serializable;
import java.net.InetAddress;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;
import java.util.StringJoiner;

/**
 * The DeviceInfo class stores the information about a device's connection details, so it can be
 * quickly and easily sent through DatagramPackets between devices. Uses the Builder pattern to
 * more easily create and change variables.
 */
public class DeviceInfo implements Serializable {
    private final InetAddress inetAddress;
    private final Set<Integer> audioCodecs;
    private final Set<Integer> amrModes;
    private final Set<Integer> evsBandwidths;
    private final Set<Integer> evsModes;
    private final Set<VideoCodecEnum> videoCodecs;
    private final int handshakePort;
    private final int rtpPort;
    private final int rtcpPort;

    private DeviceInfo(InetAddress inetSocketAddress,
        Set<Integer> audioCodecs, Set<Integer> amrModes, Set<Integer> evsBandwidths,
        Set<Integer> evsModes, Set<VideoCodecEnum> videoCodecs, int handshakePort, int rtpPort,
        int rtcpPort) {
        this.inetAddress = inetSocketAddress;
        this.audioCodecs = audioCodecs;
        this.amrModes = amrModes;
        this.evsBandwidths = evsBandwidths;
        this.evsModes = evsModes;
        this.videoCodecs = videoCodecs;
        this.handshakePort = handshakePort;
        this.rtpPort = rtpPort;
        this.rtcpPort = rtcpPort;
    }

    @NonNull
    public String toString() {
        return String.format(Locale.US,
            "IP Address: %s\nHandshake Port: %d\nRTP Port: %d\nRTCP Port: %d\nSelected Audio "
                + "Codecs: %s\nSelected AMR Modes: %s\nSelected EVS Bandwidths: %s\nSelected EVS "
                + "Modes: %s\nSelected Video Codecs: %s",
            inetAddress.getHostName(), handshakePort, rtpPort, rtcpPort, getAudioCodecsToString(),
            getAmrModesToString(), getEvsBandwidthsToString(), getEvsModesToString(),
            getVideoCodecsToString());
    }

    public int getHandshakePort() {
        return handshakePort;
    }

    public int getRtpPort() {
        return rtpPort;
    }

    public int getRtcpPort() {
        return rtcpPort;
    }

    public Set<Integer> getAudioCodecs() {
        return audioCodecs;
    }

    public Set<Integer> getEvsBandwidths() {
        return evsBandwidths;
    }

    public Set<VideoCodecEnum> getVideoCodecs() {
        return videoCodecs;
    }

    public Set<Integer> getEvsModes() {
        return evsModes;
    }

    public Set<Integer> getAmrModes() {
        return amrModes;
    }

    public InetAddress getInetAddress() {
        return inetAddress;
    }

    private String getAudioCodecsToString() {
        StringJoiner joiner = new StringJoiner(",");
        audioCodecs.forEach(item -> joiner.add(item.toString()));
        return joiner.toString();
    }

    private String getAmrModesToString() {
        StringJoiner joiner = new StringJoiner(",");
        amrModes.forEach(item -> joiner.add(item.toString()));
        return joiner.toString();
    }

    private String getEvsBandwidthsToString() {
        StringJoiner joiner = new StringJoiner(",");
        evsBandwidths.forEach(item -> joiner.add(item.toString()));
        return joiner.toString();
    }

    private String getEvsModesToString() {
        StringJoiner joiner = new StringJoiner(",");
        evsModes.forEach(item -> joiner.add(item.toString()));
        return joiner.toString();
    }

    private String getVideoCodecsToString() {
        StringJoiner joiner = new StringJoiner(",");
        videoCodecs.forEach(item -> joiner.add(item.toString()));
        return joiner.toString();
    }

    public static final class Builder {
        private InetAddress inetAddress;
        private Set<Integer> audioCodecs = new HashSet<>(Arrays.asList(CodecType.AMR,
            CodecType.AMR_WB, CodecType.EVS, CodecType.PCMA, CodecType.PCMU));
        private Set<Integer> amrModes = new HashSet<>(Arrays.asList(AmrMode.AMR_MODE_0,
            AmrMode.AMR_MODE_1, AmrMode.AMR_MODE_2, AmrMode.AMR_MODE_3, AmrMode.AMR_MODE_4,
            AmrMode.AMR_MODE_5, AmrMode.AMR_MODE_6, AmrMode.AMR_MODE_7, AmrMode.AMR_MODE_8));
        private Set<Integer> evsBandwidths = new HashSet<>(Arrays.asList(EvsBandwidth.NONE,
            EvsBandwidth.NARROW_BAND, EvsBandwidth.WIDE_BAND, EvsBandwidth.SUPER_WIDE_BAND,
            EvsBandwidth.FULL_BAND));
        private Set<Integer> evsModes = new HashSet<>(Arrays.asList(EvsMode.EVS_MODE_0,
            EvsMode.EVS_MODE_1, EvsMode.EVS_MODE_2, EvsMode.EVS_MODE_3, EvsMode.EVS_MODE_4,
            EvsMode.EVS_MODE_5, EvsMode.EVS_MODE_6, EvsMode.EVS_MODE_7, EvsMode.EVS_MODE_8,
            EvsMode.EVS_MODE_9, EvsMode.EVS_MODE_10, EvsMode.EVS_MODE_11, EvsMode.EVS_MODE_12,
            EvsMode.EVS_MODE_13, EvsMode.EVS_MODE_14, EvsMode.EVS_MODE_15, EvsMode.EVS_MODE_16,
            EvsMode.EVS_MODE_17, EvsMode.EVS_MODE_18, EvsMode.EVS_MODE_19, EvsMode.EVS_MODE_20));
        private Set<VideoCodecEnum> videoCodecs;
        private int handshakePort;
        private int rtpPort;
        private int rtcpPort;

        public Builder() {
        }

        @NonNull
        public DeviceInfo.Builder setInetAddress(InetAddress inetAddress) {
            this.inetAddress = inetAddress;
            return this;
        }

        @NonNull
        public DeviceInfo.Builder setAudioCodecs(Set<Integer> audioCodecs) {
            if(!audioCodecs.isEmpty()) { this.audioCodecs = audioCodecs; }
            return this;
        }

        public DeviceInfo.Builder setAmrModes(Set<Integer> amrModes) {
            if(!amrModes.isEmpty()) { this.amrModes = amrModes; }
            return this;
        }

        public DeviceInfo.Builder setEvsBandwidths(Set<Integer> evsBandwidths) {
            if(!evsBandwidths.isEmpty()) { this.evsBandwidths = evsBandwidths; }
            return this;
        }

        public DeviceInfo.Builder setEvsModes(Set<Integer> evsModes) {
            if(!evsModes.isEmpty()) { this.evsModes = evsModes; }
            return this;
        }

        public DeviceInfo.Builder setVideoCodecs(Set<VideoCodecEnum> videoCodecs) {
            this.videoCodecs = videoCodecs;
            return this;
        }

        @NonNull
        public DeviceInfo.Builder setHandshakePort(int handshakePort) {
            this.handshakePort = handshakePort;
            return this;
        }

        @NonNull
        public DeviceInfo.Builder setRtpPort(int rtpPort) {
            this.rtpPort = rtpPort;
            return this;
        }

        @NonNull
        public DeviceInfo.Builder setRtcpPort(int rtcpPort) {
            this.rtcpPort = rtcpPort;
            return this;
        }

        @NonNull
        public DeviceInfo build() {
            return new DeviceInfo(inetAddress, audioCodecs, amrModes, evsBandwidths, evsModes,
                videoCodecs, handshakePort, rtpPort, rtcpPort);
        }
    }
}
