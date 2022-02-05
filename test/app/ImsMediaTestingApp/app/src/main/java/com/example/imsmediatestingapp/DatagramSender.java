package com.example.imsmediatestingapp;

import android.util.Log;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

public class DatagramSender implements Runnable {

    private String ip;
    private int port;
    private byte[] data;
    private int dataLength;
    DatagramSocket sendSocket = null;
    InetAddress address;

    public DatagramSender(String ip, int port) {
        if (ip.isEmpty()) {
            this.ip = "localhost";
        } else {
            this.ip = ip;
        }

        this.port = port;
    }

    @Override
    public void run() {
        try {
            address = InetAddress.getByName(ip);
            sendSocket = new DatagramSocket();
            sendSocket.setReuseAddress(true);
            Log.d("", "Port: " + sendSocket.getLocalPort() + " has been opened for sending data.");

            DatagramPacket packet = new DatagramPacket(data, dataLength, address, port);
            sendSocket.send(packet);
            Log.d("", "Packet was sent to: " + ip + " at port: " + port);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void setData(byte[] data, int len) {
        this.data = data;
        this.dataLength = len;
    }

    public void close() {
        sendSocket.close();
    }

    private DatagramPacket createPacket(byte[] data) {
        return new DatagramPacket(data, data.length);
    }
}
