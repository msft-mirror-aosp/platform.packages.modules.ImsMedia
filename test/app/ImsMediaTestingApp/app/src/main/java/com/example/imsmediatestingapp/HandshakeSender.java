package com.example.imsmediatestingapp;

import android.util.Log;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

/**
 * The HandshakeSender is used to send the DeviceInfo and confirmation message during the handshake
 * process, via DatagramPackets.
 */
public class HandshakeSender implements Runnable {

    private final InetAddress remoteAddress;
    private final int remotePort;
    private byte[] data;
    private int dataLength;
    DatagramSocket sendSocket;
    private final String tag = "HandshakeSender";

    public HandshakeSender(InetAddress remoteAddress, int remotePort) {
        this.remoteAddress = remoteAddress;
        this.remotePort = remotePort;
    }

    @Override
    public void run() {
        try {
            sendSocket = new DatagramSocket();
            sendSocket.setReuseAddress(true);

            DatagramPacket packet = new DatagramPacket(data, dataLength, remoteAddress, remotePort);
            sendSocket.send(packet);

            close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void setData(Object data) {
        if (data instanceof String) {
            String stringData = (String) data;
            byte[] stringBytes = serializeData(stringData);
            this.data = stringBytes;
            this.dataLength = stringBytes.length;

            Log.d(tag, "Data set as type String");
        } else if(data instanceof DeviceInfo) {
            DeviceInfo deviceInfoData = (DeviceInfo) data;
            byte[] deviceInfoBytes = serializeData(deviceInfoData);
            this.data = deviceInfoBytes;
            this.dataLength = deviceInfoBytes.length;

            Log.d(tag, "Data set as type DeviceInfo");
        } else {
            Log.e(tag, "Data set was was not type String or DeviceInfo");
        }
    }

    public void close() {
        sendSocket.close();
    }

    public byte[] serializeData(Object data) {
        ByteArrayOutputStream byteOutputStream = new ByteArrayOutputStream();
        try {
            ObjectOutputStream objectOutputStream = new ObjectOutputStream(byteOutputStream);
            objectOutputStream.writeObject(data);
            objectOutputStream.reset();
            objectOutputStream.close();
            return byteOutputStream.toByteArray();
        } catch (IOException e) {
            Log.e("DeviceInfo", e.getLocalizedMessage());
        }
        return null;
    }

}
