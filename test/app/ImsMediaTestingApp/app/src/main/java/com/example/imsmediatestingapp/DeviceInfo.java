package com.example.imsmediatestingapp;

import android.hardware.radio.ims.media.AmrMode;
import android.hardware.radio.ims.media.EvsBandwidth;
import android.hardware.radio.ims.media.EvsMode;
import androidx.annotation.NonNull;
import com.example.imsmediatestingapp.MainActivity.AudioCodec;
import com.example.imsmediatestingapp.MainActivity.VideoCodec;
import java.io.Serializable;
import java.net.InetAddress;
import java.util.List;
import java.util.Locale;
import java.util.StringJoiner;

/**
 * The DeviceInfo class stores the information about a device's connection details, so it can be
 * quickly and easily sent through DatagramPackets between devices. Uses the Builder pattern to
 * more easily create and change variables.
 */
public class DeviceInfo implements Serializable {
    private final InetAddress inetAddress;
    private final List<AudioCodec> audioCodecs;
    private final List<AmrMode> amrModes;
    private final List<EvsBandwidth> evsBandwidths;
    private final List<EvsMode> evsModes;
    private final List<VideoCodec> videoCodecs;
    private final int handshakePort;
    private final int rtpPort;
    private final int rtcpPort;

    private DeviceInfo(InetAddress inetSocketAddress,
        List<AudioCodec> audioCodecs, List<AmrMode> amrModes, List<EvsBandwidth> evsBandwidths,
        List<EvsMode> evsModes, List<VideoCodec> videoCodecs, int handshakePort, int rtpPort,
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

    public List<AudioCodec> getAudioCodecs() {
        return audioCodecs;
    }

    public List<VideoCodec> getVideoCodecs() {
        return videoCodecs;
    }

    public List<EvsMode> getEvsModes() {
        return evsModes;
    }

    public List<AmrMode> getAmrModes() {
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
        private List<AudioCodec> audioCodecs;
        private List<AmrMode> amrModes;
        private List<EvsBandwidth> evsBandwidths;
        private List<EvsMode> evsModes;
        private List<VideoCodec> videoCodecs;
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
        public DeviceInfo.Builder setAudioCodecs(List<AudioCodec> audioCodecs) {
            this.audioCodecs = audioCodecs;
            return this;
        }

        public DeviceInfo.Builder setAmrModes(List<AmrMode> amrModes) {
            this.amrModes = amrModes;
            return this;
        }

        public DeviceInfo.Builder setEvsBandwidths(List<EvsBandwidth> evsBandwidths) {
            this.evsBandwidths = evsBandwidths;
            return this;
        }

        public DeviceInfo.Builder setEvsModes(List<EvsMode> evsModes) {
            this.evsModes = evsModes;
            return this;
        }

        public DeviceInfo.Builder setVideoCodecs(List<VideoCodec> videoCodecs) {
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
