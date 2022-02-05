package com.example.imsmediatestingapp;

import android.content.SharedPreferences;
import android.util.Log;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;

public class DatagramReceiver implements Runnable {

    private static final int MAX_UDP_DATAGRAM_LEN = 65527;
    private final String HANDSHAKE_PORT_PREF = "HANDSHAKE_PORT_OPEN";
    private boolean running = true;
    DatagramSocket socket = null;
    SharedPreferences.Editor editor;

    public DatagramReceiver(SharedPreferences preferences) {
        editor = preferences.edit();
    }

    public void run() {
        DeviceInfo deviceInfo = new DeviceInfo();
        DatagramPacket packet = new DatagramPacket(deviceInfo.getBytes(),
            deviceInfo.getBytes().length);

        try {
            socket = new DatagramSocket();
            socket.setReuseAddress(true);
            editor.putBoolean(HANDSHAKE_PORT_PREF, true);
            editor.apply();
            Log.d("Port", socket.getLocalPort() + " has been opened.");

            while (running) {
                Log.d("", "Port " + socket.getLocalPort() + " is waiting to receive a packet.");
                socket.receive(packet);
                deviceInfo = deserializePacket(packet.getData());
                Log.d("Device Info:", deviceInfo.toString());
            }
        } catch (Throwable e) {
            System.out.println(e.getLocalizedMessage());
        }
    }

    private DeviceInfo deserializePacket(byte[] data) {
        DeviceInfo di = null;
        ByteArrayInputStream byteArrayInputStream = new ByteArrayInputStream(data);
        try {
            ObjectInputStream objectInputStream = new ObjectInputStream(byteArrayInputStream);
            di = (DeviceInfo) objectInputStream.readObject();

        } catch (IOException | ClassNotFoundException e) {
            e.printStackTrace();
        }
        return di;
    }

    public int getBoundSocket() {
        return socket.getLocalPort();
    }

    public void close() {
        Log.d("", "Closing the datagram socket on port: " + socket.getLocalPort());
        running = false;
        socket.close();
        editor.putBoolean(HANDSHAKE_PORT_PREF, false);
        editor.apply();
    }

    public void kill() {
        running = false;
    }

}
