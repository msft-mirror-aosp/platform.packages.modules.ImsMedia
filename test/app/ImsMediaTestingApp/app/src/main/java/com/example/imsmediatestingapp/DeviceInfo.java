package com.example.imsmediatestingapp;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothClass.Device;
import android.util.Log;
import androidx.annotation.NonNull;
import com.example.imsmediatestingapp.MainActivity.AudioCodec;
import com.example.imsmediatestingapp.MainActivity.EvsBandwidth;
import com.example.imsmediatestingapp.MainActivity.VideoCodec;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.util.Arrays;
import java.util.List;

/**
 * The DeviceInfo class stores the information about a device's connection details, so it can be
 * quickly and easily sent through DatagramPackets between devices.
 */
public class DeviceInfo implements Serializable {

    private InetAddress ipAddress;
    private List<AudioCodec> audioCodecs = Arrays.asList(AudioCodec.values());
    private List<EvsBandwidth> evsBandwidths = Arrays.asList(EvsBandwidth.values());
    private List<VideoCodec> videoCodecs = Arrays.asList(VideoCodec.values());
    private int handshakePort = -1;
    private int rtpPort = -1;
    private int rtcpPort = -1;

    public DeviceInfo(int handshakePort, InetAddress ipAddress, List<AudioCodec> audioCodecs,
        List<VideoCodec> videoCodecs) {
        this.handshakePort = handshakePort;
        this.ipAddress = ipAddress;
        this.audioCodecs = audioCodecs;
        this.videoCodecs = videoCodecs;
    }

    public DeviceInfo() {

    }

    public DeviceInfo(InetAddress ipAddress, int handshakePort, int rtpPort, int rtcpPort) {
        this.ipAddress = ipAddress;
        this.handshakePort = handshakePort;
        this.rtpPort = rtpPort;
        this.rtcpPort = rtcpPort;
    }

    public DeviceInfo(int handshakePort, InetAddress ipAddress) {
        this.handshakePort = handshakePort;
        this.ipAddress = ipAddress;
    }

    @NonNull
    public String toString() {
        @SuppressLint("DefaultLocale") String toString = String.format(
            "IP Address: %s\nHandshake Port: %d\nRTP Port: %d\nRTCP Port: %d\nAudio Codecs: %s\n"
                + "Video Codecs: %s\nEVS Bandwidths: %s",
            ipAddress.getHostAddress(), handshakePort, rtpPort, rtcpPort, getAudioCodecsAsString(),
            getVideoCodecsAsString(), getEvsBandwidthsCodecsAsString());
        return toString;
    }

    public int getHandshakePort() {
        return handshakePort;
    }

    public InetAddress getIpAddress() {
        return ipAddress;
    }

    public void setHandshakePort(int handshakePort) {
        this.handshakePort = handshakePort;
    }

    public void setIpAddress(InetAddress ipAddress) {
        this.ipAddress = ipAddress;
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

    public void setAudioCodecs(List<AudioCodec> audioCodecs) {
        this.audioCodecs = audioCodecs;
    }

    public List<VideoCodec> getVideoCodecs() {
        return videoCodecs;
    }

    public void setVideoCodecs(List<VideoCodec> videoCodecs) {
        this.videoCodecs = videoCodecs;
    }

    public void setRtpPort(int rtpPort) {
        this.rtpPort = rtpPort;
    }

    public void setRtcpPort(int rtcpPort) {
        this.rtcpPort = rtcpPort;
    }

    public String getAudioCodecsAsString() {
        StringBuilder sb = new StringBuilder("Audio Codecs: ");
        for (AudioCodec codec : audioCodecs) {
            sb.append(codec.toString());
        }

        return sb.toString();
    }

    public String getVideoCodecsAsString() {
        StringBuilder sb = new StringBuilder("Video Codecs: ");
        for (VideoCodec codec : videoCodecs) {
            sb.append(codec.toString());
        }

        return sb.toString();
    }

    public String getEvsBandwidthsCodecsAsString() {
        StringBuilder sb = new StringBuilder("EVS Bandwidths: ");
        for (EvsBandwidth bandwidth : evsBandwidths) {
            sb.append(bandwidth.toString());
        }

        return sb.toString();
    }

}
