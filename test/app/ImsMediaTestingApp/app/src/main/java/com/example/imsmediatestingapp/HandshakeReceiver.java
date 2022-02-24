package com.example.imsmediatestingapp;

import android.content.SharedPreferences;
import android.util.Log;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;

/**
 *  The HandshakeReceiver handles and stores information about incoming packets during the
 *  handshake process.
 */
public class HandshakeReceiver implements Runnable {

    private static final int MAX_UDP_DATAGRAM_LEN = 65527;
    private final String HANDSHAKE_PORT_PREF = "HANDSHAKE_PORT_OPEN";
    private final String CONFIRMATION_MESSAGE = "CONNECTED";
    private final String LOG_PREFIX = "DatagramReceiver";
    private boolean running = true;
    private boolean isConfirmationReceived = false;
    private boolean isHandshakeReceived = false;
    private DeviceInfo receivedDeviceInfo;
    DatagramSocket socket = null;
    SharedPreferences.Editor editor;

    public HandshakeReceiver(SharedPreferences preferences) {
        editor = preferences.edit();
    }

    public void run() {
        DeviceInfo deviceInfo = null;
        String confirmation = null;
        byte[] buffer = new byte[MAX_UDP_DATAGRAM_LEN];
        DatagramPacket packet = new DatagramPacket(buffer, buffer.length);

        try {
            socket = new DatagramSocket();
            socket.setReuseAddress(true);

            editor.putBoolean(HANDSHAKE_PORT_PREF, true).apply();

            while (running) {
                socket.receive(packet);
                Object dataReceived = deserializePacket(packet.getData());

                if(dataReceived instanceof DeviceInfo && !isHandshakeReceived) {
                    deviceInfo = (DeviceInfo) dataReceived;
                    if (verifyHandshakePacket(deviceInfo)) {
                        isHandshakeReceived = true;
                        receivedDeviceInfo = deviceInfo;
                        Log.d(LOG_PREFIX, "RECEIVED: Device Info");
                    }

                } else if(dataReceived instanceof String && !isConfirmationReceived) {
                    confirmation = (String) dataReceived;
                    if(verifyConfirmationPacket(confirmation)) {
                        isConfirmationReceived = true;
                        Log.d(LOG_PREFIX, "RECEIVED: Confirmation");
                        running = false;

                    }
                }

            }
        } catch (Throwable e) {
            System.out.println(e.getLocalizedMessage());
        }
    }

    private <T> T deserializePacket(byte[] data) {
        DeviceInfo deviceInfo = null;
        String confirmationMessage = null;
        ByteArrayInputStream byteArrayInputStream = new ByteArrayInputStream(data);
        try {
            ObjectInputStream objectInputStream = new ObjectInputStream(byteArrayInputStream);
            Object readObject = objectInputStream.readObject();
            if(readObject instanceof DeviceInfo) {
                deviceInfo = (DeviceInfo) readObject;
                return (T) deviceInfo;

            } else if(readObject instanceof String) {
                confirmationMessage = (String) readObject;
                return (T) confirmationMessage;
            }

        } catch (IOException | ClassNotFoundException e) {
            e.printStackTrace();
            Log.d("", new String(data));
            return (T) new String(data);
        }
        return null;
    }

    public int getBoundSocket() {
        return socket.getLocalPort();
    }

    public void close() {
        Log.d("", "Closing the socket on port: " + socket.getLocalPort());
        running = false;
        socket.close();
        editor.putBoolean(HANDSHAKE_PORT_PREF, false).apply();
    }

    public void kill() {
        running = false;
    }

    private boolean verifyHandshakePacket(DeviceInfo deviceInfo) {
        if(deviceInfo.getHandshakePort() == -1 || deviceInfo.getRtpPort() == -1
            || deviceInfo.getRtcpPort() == -1) {
            Log.d("", "One or more of the ports sent in the handshake have not been opened.");
            return false;
        }

        return true;
    }

    private boolean verifyConfirmationPacket(String confirmation) {
        return confirmation.equals(CONFIRMATION_MESSAGE);
    }

    public boolean isConfirmationReceived() {
        return isConfirmationReceived;
    }

    public boolean isHandshakeReceived() {
        return isHandshakeReceived;
    }

    public DeviceInfo getReceivedDeviceInfo() {
        return receivedDeviceInfo;
    }
}
