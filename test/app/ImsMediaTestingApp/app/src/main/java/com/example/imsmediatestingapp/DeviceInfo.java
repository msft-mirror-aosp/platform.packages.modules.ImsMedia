package com.example.imsmediatestingapp;

import android.annotation.SuppressLint;
import android.util.Log;
import androidx.annotation.NonNull;
import com.example.imsmediatestingapp.MainActivity.AudioCodec;
import com.example.imsmediatestingapp.MainActivity.EvsBandwidth;
import com.example.imsmediatestingapp.MainActivity.VideoCodec;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.net.InetAddress;
import java.util.Arrays;
import java.util.List;

public class DeviceInfo implements Serializable {

    private InetAddress ipAddress;
    private List<AudioCodec> audioCodecs = Arrays
        .asList(AudioCodec.AMR_NB, AudioCodec.AMR_WB, AudioCodec.EVS, AudioCodec.PCMA,
            AudioCodec.PCMU);
    private List<EvsBandwidth> evsBandwithds = Arrays
        .asList(EvsBandwidth.NARROW_BAND, EvsBandwidth.WIDE_BAND, EvsBandwidth.SUPER_WIDE_BAND,
            EvsBandwidth.FULL_BAND);
    private List<VideoCodec> videoCodecs = Arrays.asList(VideoCodec.H264, VideoCodec.HEVC);
    private int handshakePort;
    private int rtpPort;
    private int rtcpPort;

    public DeviceInfo(int handshakePort, InetAddress ipAddress, List<AudioCodec> audioCodecs,
        List<VideoCodec> videoCodecs) {
        this.handshakePort = handshakePort;
        this.ipAddress = ipAddress;
        this.audioCodecs = audioCodecs;
        this.videoCodecs = videoCodecs;
    }

    public DeviceInfo() {

    }

    public byte[] getBytes() {
        ByteArrayOutputStream byteOutputStream = new ByteArrayOutputStream();
        try {
            ObjectOutputStream objectOutputStream = new ObjectOutputStream(byteOutputStream);
            objectOutputStream.writeObject(this);
            objectOutputStream.flush();
            objectOutputStream.close();
            return byteOutputStream.toByteArray();
        } catch (IOException e) {
            Log.e("", e.getLocalizedMessage());
        }
        return null;
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

    public void setRtpPort(int rtpPort) {
        this.rtpPort = rtpPort;
    }

    public int getRtcpPort() {
        return rtcpPort;
    }

    public void setRtcpPort(int rtcpPort) {
        this.rtcpPort = rtcpPort;
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
        for (EvsBandwidth bandwidth : evsBandwithds) {
            sb.append(bandwidth.toString());
        }

        return sb.toString();
    }

}
