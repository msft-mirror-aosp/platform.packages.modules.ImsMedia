package com.example.imsmediatestingapp;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.telephony.AccessNetworkConstants.AccessNetworkType;
import android.telephony.imsmedia.AmrParams;
import android.telephony.imsmedia.AudioConfig;
import android.telephony.imsmedia.AudioSessionCallback;
import android.telephony.imsmedia.EvsParams;
import android.telephony.imsmedia.ImsAudioSession;
import android.telephony.imsmedia.ImsMediaManager;
import android.telephony.imsmedia.ImsMediaSession;
import android.telephony.imsmedia.MediaQualityThreshold;
import android.telephony.imsmedia.RtcpConfig;
import android.telephony.imsmedia.RtpConfig;
import android.text.format.Formatter;
import android.util.Log;
import android.util.SparseBooleanArray;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

/**
 * The MainActivity is the main and default layout for the application.
 */
public class MainActivity extends AppCompatActivity {

    private final List<AudioCodec> selectedAudioCodecs = new ArrayList<>();
    private final List<VideoCodec> selectedVideoCodecs = new ArrayList<>();
    public static final String PREF_NAME = "preferences";
    private final String HANDSHAKE_PORT_PREF = "HANDSHAKE_PORT_OPEN";
    private final String CONNECTED_PREF = "IS_CONNECTED";
    private final String CONFIRMATION_MESSAGE = "CONNECTED";
    SharedPreferences prefs;
    SharedPreferences.Editor editor;
    Thread waitForHandshakeThread;
    Thread initiateHandshakeThread;
    private ConnectionStatus connectionStatus;

    HandshakeReceiver handshakeReceptionSocket;
    DatagramSocket rtp;
    DatagramSocket rtcp;

    DeviceInfo remoteDeviceInfo;

    private boolean isMediaManagerReady = false;
    private boolean isOpenSessionSent = false;
    AudioConfig audioConfig;
    private ImsAudioSession audioSession;
    int testPortNum = 6000;
    Context context;
    MediaManagerCallback callback;
    RtpAudioSessionCallback sessionCallback;
    Executor executor;
    ImsMediaManager imsMediaManager;

    public enum AudioCodec {
        AMR_NB(0),
        AMR_WB(1),
        EVS(2),
        PCMA(3),
        PCMU(4);

        private final int value;

        AudioCodec(int value) {
            this.value = value;
        }

        public int getValue() {
            return value;
        }
    }

    public enum EvsBandwidth {
        NONE(0),
        NARROW_BAND(1),
        WIDE_BAND(2),
        SUPER_WIDE_BAND(3),
        FULL_BAND(4);

        private final int value;

        EvsBandwidth(int value) {
            this.value = value;
        }

        public int getValue() {
            return value;
        }
    }

    public enum VideoCodec {
        H264(0),
        HEVC(1);

        private final int value;

        VideoCodec(int value) {
            this.value = value;
        }

        public int getValue() {
            return value;
        }
    }

    public enum ConnectionStatus {
        OFFLINE(0),
        AWAITING_CONNECTION(1),
        CONNECTING(2),
        CONNECTED(3),
        ACTIVE_CALL(4);

        private final int value;

        ConnectionStatus(int value) {
            this.value = value;
        }

        public int getValue() {
            return value;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        prefs = getSharedPreferences(PREF_NAME, MODE_PRIVATE);
        editor = prefs.edit();
        editor.putBoolean(CONNECTED_PREF, false);
        editor.putBoolean(HANDSHAKE_PORT_PREF, false);
        editor.apply();

        connectionStatus = ConnectionStatus.OFFLINE;

        askForPermissions();
        styleDeviceInfo();
        styleMainActivity();

        context = getApplicationContext();
        callback = new MediaManagerCallback();
        executor = Executors.newSingleThreadExecutor();
        imsMediaManager = new ImsMediaManager(context, executor, callback);

    }

    @Override
    protected void onStart() {
        super.onStart();
        askForPermissions();
        styleDeviceInfo();
        styleMainActivity();
    }

    @Override
    protected void onResume() {
        super.onResume();
        styleDeviceInfo();
        styleMainActivity();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        rtp.close();
        rtcp.close();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @SuppressLint("NonConstantResourceId")
    @Override
    public boolean onOptionsItemSelected(@NonNull MenuItem item) {
        switch (item.getItemId()) {
            case R.id.settingsButton:
                setContentView(R.layout.settings);
                setupAudioCodecDropDown();
                setupSettingsPage();
                break;

            case R.id.homeButton:
                setContentView(R.layout.activity_main);
                styleDeviceInfo();
                styleMainActivity();
                break;

            default:
                throw new IllegalStateException("Unexpected value: " + item.getItemId());
        }
        return super.onOptionsItemSelected(item);
    }

    private void setupAudioCodecDropDown() {
        ArrayAdapter<AudioCodec> adapter = new ArrayAdapter<>(
            this, android.R.layout.simple_list_item_multiple_choice, AudioCodec.values());
        ListView audio = findViewById(R.id.audioCodecList);
        audio.setAdapter(adapter);

        ArrayAdapter<EvsBandwidth> evsAdaptor = new ArrayAdapter<>(
            this, android.R.layout.simple_list_item_multiple_choice, EvsBandwidth.values());
        ListView evs = findViewById(R.id.evsBandwidthsList);
        evs.setAdapter(evsAdaptor);

        ArrayAdapter<VideoCodec> videoAdaptor = new ArrayAdapter<>(
            this, android.R.layout.simple_list_item_multiple_choice, VideoCodec.values());
        ListView video = findViewById(R.id.videoCodecList);
        video.setAdapter(videoAdaptor);
    }

    public String getOtherDeviceIp() {
        return prefs.getString("OTHER_IP_ADDRESS", "localhost");
    }

    public int getOtherDevicePort() {
        return prefs.getInt("OTHER_HANDSHAKE_PORT", -1);

    }

    public int getOtherDevicePortEditText() {
        EditText portBox = findViewById(R.id.port_number);
        return Integer.parseInt(portBox.getText().toString());
    }

    public String getOtherDeviceIpEditText() {
        EditText ipBox = findViewById(R.id.other_device_ip);
        return ipBox.getText().toString();
    }

    public void setupSettingsPage() {
        EditText ipAddress = findViewById(R.id.other_device_ip);
        EditText portNumber = findViewById(R.id.port_number);
        String ip = prefs.getString("OTHER_IP_ADDRESS", "");
        String portNum = String.valueOf(prefs.getInt("OTHER_HANDSHAKE_PORT", 0));

        ipAddress.setText(ip);
        portNumber.setText(portNum);
    }

    private WifiInfo retrieveNetworkConfig() {
        WifiManager wifiManager = (WifiManager) getApplication()
            .getSystemService(Context.WIFI_SERVICE);
        return wifiManager.getConnectionInfo();
    }

    private String getLocalIpAddress() {
        return Formatter.formatIpAddress(retrieveNetworkConfig().getIpAddress());
    }

    private void getAudioCodecSelections() {
        ListView audioCodecs = findViewById(R.id.audioCodecList);
        SparseBooleanArray selectedCodecs = audioCodecs.getCheckedItemPositions();
        for (int i = 0; i < selectedCodecs.size(); i++) {
            AudioCodec codec = (AudioCodec) audioCodecs.getAdapter()
                .getItem(selectedCodecs.keyAt(i));
            if (selectedCodecs.valueAt(i)) {
                selectedAudioCodecs.add(codec);
            } else {
                selectedAudioCodecs.remove(codec);
            }
        }
    }

    private void getVideoCodecSelections() {
        ListView videoCodecList = findViewById(R.id.videoCodecList);
        SparseBooleanArray selected = videoCodecList.getCheckedItemPositions();
        for (int i = 0; i < selected.size(); i++) {
            VideoCodec codec = (VideoCodec) videoCodecList.getAdapter().getItem(selected.keyAt(i));
            if (selected.valueAt(i)) {
                selectedVideoCodecs.add(codec);
            } else {
                selectedVideoCodecs.remove(codec);
            }
        }
    }

    private void askForPermissions() {
        ActivityResultLauncher<String[]> locationPermissionRequest =
            registerForActivityResult(new ActivityResultContracts
                    .RequestMultiplePermissions(), result -> {
                    Boolean fineLocationGranted = result.getOrDefault(
                        Manifest.permission.ACCESS_FINE_LOCATION, false);
                    Boolean coarseLocationGranted = result.getOrDefault(
                        Manifest.permission.ACCESS_COARSE_LOCATION, false);
                    if (fineLocationGranted != null && fineLocationGranted) {
                        Log.d("", "Precise location permissions have already been granted.");
                    } else if (coarseLocationGranted != null && coarseLocationGranted) {
                        Log.d("", "Only coarse location permissions have been granted."
                            + "\n Precise location permissions are needed to access WiFi "
                            + "information.");
                    } else {
                        Log.e("", "No location permissions have been granted.");
                    }
                }
            );

        locationPermissionRequest.launch(new String[]{
            Manifest.permission.ACCESS_FINE_LOCATION,
            Manifest.permission.ACCESS_COARSE_LOCATION
        });
    }

    private String getIpAddress() {
        TextView ipBox = findViewById(R.id.otherIpAddress);
        return ipBox.getText().toString();
    }

    private String getPortNumber() {
        EditText portNumberBox = findViewById(R.id.port_number);
        return portNumberBox.getText().toString();
    }

    public void openRtpPorts() {
        Executor socketBindingExecutor = Executors.newSingleThreadExecutor();

        Runnable bindSockets = new Runnable() {
            @Override
            public void run() {
                try {
                    rtp = new DatagramSocket();
                    rtp.setReuseAddress(true);

                    rtcp = new DatagramSocket(rtp.getLocalPort() + 1);
                    rtcp.setReuseAddress(true);

                    handshakeReceptionSocket = new HandshakeReceiver(prefs);
                    handshakeReceptionSocket.run();
                } catch (SocketException e) {
                    Log.d("", e.toString());
                }
            }
        };

        socketBindingExecutor.execute(bindSockets);
    }

    public void closePorts() {
        handshakeReceptionSocket.close();
        rtp.close();
        rtcp.close();
    }

    public void styleMainActivity() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                switch (connectionStatus) {

                    case OFFLINE:
                        styleOffline();
                        break;

                    case AWAITING_CONNECTION:
                        styleAwaitingConnection();
                        break;

                    case CONNECTING:
                        break;

                    case CONNECTED:
                        styleConnected();
                        break;

                    case ACTIVE_CALL:
                        styleActiveCall();
                        break;
                }
            }
        });
    }

    private void styleDeviceInfo() {
        styleIpLabel();

        TextView otherIp = findViewById(R.id.otherIpAddress);
        String ip = prefs.getString("OTHER_IP_ADDRESS", "null");
        otherIp.setText(getString(R.string.other_device_ip_label, ip));

        TextView otherPort = findViewById(R.id.otherHandshakePort);
        String portNum = String.valueOf(getOtherDevicePort());
        otherPort.setText(getString(R.string.other_device_port_label, portNum));

    }

    private void styleIpLabel() {
        TextView ipText = findViewById(R.id.ipAddress);

        String ip = getLocalIpAddress();
        ipText.setText(getString(R.string.local_ip_label, ip));
    }

    public void styleOffline() {
        Button allowCallsButton, connectButton, openSessionButton, closeSessionButton;

        allowCallsButton = findViewById(R.id.clientButton);
        allowCallsButton.setText(R.string.allow_calls_button_text);
        allowCallsButton.setBackgroundColor(getResources().getColor(R.color.mint_green));

        connectButton = findViewById(R.id.connectButton);
        connectButton.setEnabled(false);
        connectButton.setAlpha(0.5f);

        openSessionButton = findViewById(R.id.openSessionButton);
        openSessionButton.setEnabled(false);
        openSessionButton.setAlpha(0.5f);

        closeSessionButton = findViewById(R.id.closeSessionButton);
        closeSessionButton.setEnabled(false);
        closeSessionButton.setAlpha(0.5f);

        TextView portText, rtpPortText, rtcpPortText;

        portText = findViewById(R.id.receptionPort);
        portText.setText(getString(R.string.port_closed_label));

        rtpPortText = findViewById(R.id.rtpReceptionPort);
        rtpPortText.setText(getString(R.string.port_closed_label));

        rtcpPortText = findViewById(R.id.rtcpReceptionPort);
        rtcpPortText.setText(getString(R.string.port_closed_label));
        styleDeviceInfo();

    }

    public void styleAwaitingConnection() {
        Button allowCallsButton, connectButton, openSessionButton, closeSessionButton;

        allowCallsButton = findViewById(R.id.clientButton);
        allowCallsButton.setText(R.string.disable_calls_button_text);
        allowCallsButton.setBackgroundColor(getResources().getColor(R.color.coral_red));

        connectButton = findViewById(R.id.connectButton);
        connectButton.setEnabled(true);
        connectButton.setAlpha(1.0f);

        openSessionButton = findViewById(R.id.openSessionButton);
        openSessionButton.setEnabled(false);
        openSessionButton.setAlpha(0.5f);

        closeSessionButton = findViewById(R.id.closeSessionButton);
        closeSessionButton.setEnabled(false);
        closeSessionButton.setAlpha(0.5f);

        TextView handshakePort, localRtp, localRtcpPort, remoteHandshake, remoteRtp, remoteRtcp;

        handshakePort = findViewById(R.id.receptionPort);
        handshakePort.setText(getString(R.string.reception_port_label,
            String.valueOf(handshakeReceptionSocket.getBoundSocket())));

        localRtp = findViewById(R.id.rtpReceptionPort);
        localRtp.setText(getString(R.string.rtp_reception_port_label,
            String.valueOf(rtp.getLocalPort())));

        localRtcpPort = findViewById(R.id.rtcpReceptionPort);
        localRtcpPort.setText(getString(R.string.rtcp_reception_port_label,
            String.valueOf(rtcp.getLocalPort())));

        remoteHandshake = findViewById(R.id.otherHandshakePort);
        remoteHandshake.setText(getString(R.string.reception_port_label,
            String.valueOf(getOtherDevicePort())));

        remoteRtp = findViewById(R.id.remoteRtpPort);
        remoteRtp.setText(getString(R.string.port_closed_label));

        remoteRtcp = findViewById(R.id.remoteRtcpPort);
        remoteRtcp.setText(getString(R.string.port_closed_label));
        styleDeviceInfo();
    }

    public void styleConnected() {
        Button allowCallsButton, connectButton, openSessionButton, closeSessionButton;

        allowCallsButton = findViewById(R.id.clientButton);
        allowCallsButton.setText(R.string.disable_calls_button_text);
        allowCallsButton.setBackgroundColor(getColor(R.color.coral_red));

        connectButton = findViewById(R.id.connectButton);
        connectButton.setText(R.string.disconnect_button_text);
        connectButton.setBackgroundColor(getColor(R.color.coral_red));
        connectButton.setEnabled(true);
        connectButton.setAlpha(1.0f);

        openSessionButton = findViewById(R.id.openSessionButton);
        openSessionButton.setEnabled(true);
        openSessionButton.setAlpha(1.0f);

        closeSessionButton = findViewById(R.id.closeSessionButton);
        closeSessionButton.setEnabled(false);
        closeSessionButton.setAlpha(0.5f);

        TextView handshakePort, localRtp, localRtcpPort, remoteHandshake, remoteRtp, remoteRtcp;

        handshakePort = findViewById(R.id.receptionPort);
        handshakePort.setText(getString(R.string.reception_port_label,
            getString(R.string.connected)));

        localRtp = findViewById(R.id.rtpReceptionPort);
        localRtp.setText(getString(R.string.rtp_reception_port_label,
            String.valueOf(rtp.getLocalPort())));

        localRtcpPort = findViewById(R.id.rtcpReceptionPort);
        localRtcpPort.setText(getString(R.string.rtcp_reception_port_label,
            String.valueOf(rtcp.getLocalPort())));

        remoteHandshake = findViewById(R.id.otherHandshakePort);
        remoteHandshake.setText(getString(R.string.reception_port_label,
            getString(R.string.connected)));

        remoteRtp = findViewById(R.id.remoteRtpPort);
        remoteRtp.setText(getString(R.string.rtp_reception_port_label,
            String.valueOf(remoteDeviceInfo.getRtpPort())));

        remoteRtcp = findViewById(R.id.remoteRtcpPort);
        remoteRtcp.setText(getString(R.string.rtcp_reception_port_label,
            String.valueOf(remoteDeviceInfo.getRtcpPort())));

        styleDeviceInfo();
    }

    public void styleActiveCall() {
        Button allowCallsButton, connectButton, openSessionButton, closeSessionButton;

        allowCallsButton = findViewById(R.id.clientButton);
        allowCallsButton.setText(R.string.disable_calls_button_text);
        allowCallsButton.setBackgroundColor(getResources().getColor(R.color.coral_red));

        connectButton = findViewById(R.id.connectButton);
        connectButton.setEnabled(false);
        connectButton.setAlpha(0.5f);

        openSessionButton = findViewById(R.id.openSessionButton);
        openSessionButton.setEnabled(false);
        openSessionButton.setAlpha(0.5f);

        closeSessionButton = findViewById(R.id.closeSessionButton);
        closeSessionButton.setEnabled(true);
        closeSessionButton.setAlpha(1.0f);

        TextView handshakePort, localRtp, localRtcpPort, remoteHandshake, remoteRtp, remoteRtcp;

        handshakePort = findViewById(R.id.receptionPort);
        handshakePort
            .setText(getString(R.string.reception_port_label, getString(R.string.connected)));

        localRtp = findViewById(R.id.rtpReceptionPort);
        localRtp.setText(getString(R.string.rtp_reception_port_label,
            String.valueOf(rtp.getLocalPort())));

        localRtcpPort = findViewById(R.id.rtcpReceptionPort);
        localRtcpPort.setText(getString(R.string.rtcp_reception_port_label,
            String.valueOf(rtcp.getLocalPort())));

        remoteHandshake = findViewById(R.id.otherHandshakePort);
        remoteHandshake.setText(getString(R.string.reception_port_label,
            getString(R.string.connected)));

        remoteRtp = findViewById(R.id.remoteRtpPort);
        remoteRtp.setText(getString(R.string.rtp_reception_port_label,
            String.valueOf(remoteDeviceInfo.getRtpPort())));

        remoteRtcp = findViewById(R.id.remoteRtcpPort);
        remoteRtcp.setText(getString(R.string.rtcp_reception_port_label,
            String.valueOf(remoteDeviceInfo.getRtcpPort())));
    }

    Runnable handleIncomingHandshake = new Runnable() {
        @Override
        public void run() {
            try {
                while (!handshakeReceptionSocket.isHandshakeReceived()) {
                    if (Thread.currentThread().isInterrupted()) {
                        throw new InterruptedException();
                    }
                }

                remoteDeviceInfo = handshakeReceptionSocket.getReceivedDeviceInfo();

                HandshakeSender handshakeSender = new HandshakeSender(
                    remoteDeviceInfo.getIpAddress(), remoteDeviceInfo.getHandshakePort());
                handshakeSender.setData(createMyDeviceInfo());
                handshakeSender.run();

                while (!handshakeReceptionSocket.isConfirmationReceived()) {
                    if (Thread.currentThread().isInterrupted()) {
                        throw new InterruptedException();
                    }
                }

                handshakeSender = new HandshakeSender(remoteDeviceInfo.getIpAddress(),
                    remoteDeviceInfo.getHandshakePort());
                handshakeSender.setData(CONFIRMATION_MESSAGE);
                handshakeSender.run();
                Log.d("", "accepted handshake initiation from other device");
                editor.putString("OTHER_IP_ADDRESS",
                    remoteDeviceInfo.getIpAddress().getHostName());
                editor.putInt("OTHER_HANDSHAKE_PORT", remoteDeviceInfo.getRtpPort());
                editor.apply();
                updateUI(ConnectionStatus.CONNECTED);
            } catch (InterruptedException e) {
                Log.d("", "" + e.toString());
            }

        }
    };

    Runnable initiateHandshake = new Runnable() {
        @Override
        public void run() {
            try {
                HandshakeSender sender = new HandshakeSender(
                    InetAddress.getByName(getOtherDeviceIp()), getOtherDevicePort());
                DeviceInfo myDeviceInfo = createMyDeviceInfo();
                sender.setData(myDeviceInfo);
                sender.run();

                while (!handshakeReceptionSocket.isHandshakeReceived()) {

                }
                remoteDeviceInfo = handshakeReceptionSocket.getReceivedDeviceInfo();

                sender.setData(CONFIRMATION_MESSAGE);
                sender.run();

                while (!handshakeReceptionSocket.isConfirmationReceived()) {

                }

                Log.d("", "connected to the other device");
                updateUI(ConnectionStatus.CONNECTED);
            } catch (Exception e) {
                Log.e("initiateHandshake(): ", e.toString());
            }
        }
    };

    public DeviceInfo createMyDeviceInfo() {
        try {
            DeviceInfo deviceInfo = new DeviceInfo();
            deviceInfo.setIpAddress(InetAddress.getByName(getLocalIpAddress()));
            deviceInfo.setHandshakePort(handshakeReceptionSocket.getBoundSocket());
            deviceInfo.setRtpPort(rtp.getLocalPort());
            deviceInfo.setRtcpPort(rtcp.getLocalPort());
            return deviceInfo;
        } catch (UnknownHostException e) {
            e.printStackTrace();
        }
        return null;
    }

    public void allowCallsOnClick(View v) {
        Button connectButton = findViewById(R.id.connectButton);
        if (prefs.getBoolean(HANDSHAKE_PORT_PREF, false)) {
            closePorts();
            Log.d("", "Closed handshake, rtp, and rtcp ports.");

            Toast.makeText(getApplicationContext(),
                "Closing ports",
                Toast.LENGTH_SHORT).show();
            updateUI(ConnectionStatus.OFFLINE);
        } else {
            openRtpPorts();
            while (!prefs.getBoolean(HANDSHAKE_PORT_PREF, false)) {
            }
            Log.d("", "Handshake, rtp, and rtcp ports have been bound.");

            Toast.makeText(getApplicationContext(), getString(R.string.allowing_calls_toast_text),
                Toast.LENGTH_SHORT).show();

            waitForHandshakeThread = new Thread(handleIncomingHandshake);
            waitForHandshakeThread.start();
            updateUI(ConnectionStatus.AWAITING_CONNECTION);
        }
    }

    public void updateUI(ConnectionStatus newStatus) {
        connectionStatus = newStatus;
        styleMainActivity();
    }

    public void initiateHandshakeOnClick(View v) {
        waitForHandshakeThread.interrupt();
        initiateHandshakeThread = new Thread(initiateHandshake);
        initiateHandshakeThread.start();
        updateUI(ConnectionStatus.CONNECTING);
    }

    public void openSessionOnClick(View v) {
        if (isMediaManagerReady && !isOpenSessionSent) {

            Toast.makeText(getApplicationContext(),
                getString(R.string.connecting_call_toast_text),
                Toast.LENGTH_SHORT).show();

            String remoteIp = prefs.getString("OTHER_IP_ADDRESS", "localhost");
            int remotePort = prefs.getInt("OTHER_HANDSHAKE_PORT", -1);
            InetAddress addr;
            try {
                addr = InetAddress.getByName(remoteIp);
            } catch (UnknownHostException e) {
                e.printStackTrace();
                return;
            }
            InetSocketAddress rtpAddr = new InetSocketAddress(addr, remotePort);
            Log.d("", "IP: " + remoteIp + "\tPORT: " + remotePort);

            RtcpConfig rtcpConfig = new RtcpConfig.Builder()
                .setCanonicalName("steve")
                .setTransmitPort(remotePort + 1)
                .setIntervalSec(5)
                .setRtcpXrBlockTypes(0)
                .build();

            MediaQualityThreshold mThreshold = new MediaQualityThreshold.Builder()
                .setRtpInactivityTimerMillis(20)
                .setRtcpInactivityTimerMillis(20)
                .setPacketLossPeriodMillis(10000)
                .setPacketLossThreshold(1)
                .setJitterPeriodMillis(300)
                .setJitterThresholdMillis(5000)
                .build();

            EvsParams mEvs = new EvsParams(EvsParams.EVS_MODE_7,
                (byte) 3,
                false,
                false);
            AmrParams mAmr = new AmrParams(AmrParams.AMR_MODE_8,
                true,
                0);

            audioConfig = new AudioConfig.Builder()
                .setMediaDirection(RtpConfig.MEDIA_DIRECTION_TRANSMIT_RECEIVE)
                .setAccessNetwork(AccessNetworkType.EUTRAN)
                .setRemoteRtpAddress(rtpAddr)
                .setRtcpConfig(rtcpConfig)
                .setMaxMtuBytes(1500)
                .setDscp(0)
                .setRxPayloadTypeNumber(96)
                .setTxPayloadTypeNumber(96)
                .setSamplingRateKHz((byte) 16)
                .setPtimeMillis((byte) 20)
                .setMaxPtimeMillis((byte) 240)
                .setTxCodecModeRequest((byte) 15)
                .setDtxEnabled(true)
                .setCodecType(AudioConfig.CODEC_AMR_WB)
                .setEvsBandwidth(AudioConfig.EVS_BAND_NONE)
                .setDtmfPayloadTypeNumber(100)
                .setDtmfsamplingRateKHz(16)
                .setAmrParams(mAmr)
                .setEvsParams(mEvs)
                .build();

            sessionCallback = new RtpAudioSessionCallback();
            imsMediaManager.openSession(rtp, rtcp, ImsMediaSession.SESSION_TYPE_AUDIO,
                audioConfig, executor, sessionCallback);
            Log.d("", "starting open session");
        }
    }

    public void closeSessionOnClick(View v) {
        imsMediaManager.closeSession(audioSession);
        updateUI(ConnectionStatus.CONNECTED);
    }

    public void saveSettingsOnClick(View v) {
        int port = getOtherDevicePortEditText();
        String ip = getOtherDeviceIpEditText();

        editor.putInt("OTHER_HANDSHAKE_PORT", port);
        editor.putString("OTHER_IP_ADDRESS", ip);
        editor.apply();

        Toast.makeText(getApplicationContext(), R.string.save_button_action_toast,
            Toast.LENGTH_SHORT).show();
    }

    private class MediaManagerCallback implements ImsMediaManager.OnConnectedCallback {

        @Override
        public void onConnected() {
            Log.d("", "ImsMediaManager - connected");
            isMediaManagerReady = true;
        }
    }

    private class RtpAudioSessionCallback extends AudioSessionCallback {

        @Override
        public void onModifySessionResponse(AudioConfig config, int result) {
            Log.d("", "onModifySessionResponse");
        }

        @Override
        public void onOpenSessionFailure(int error) {
            Log.d("", "onOpenSessionFailure - error=" + error);
        }

        @Override
        public void onOpenSessionSuccess(ImsMediaSession session) {
            audioSession = (ImsAudioSession) session;
            Log.d("", "onOpenSessionSuccess: id=" + audioSession.getSessionId());
            isOpenSessionSent = true;
            updateUI(ConnectionStatus.ACTIVE_CALL);
        }

        @Override
        public void onSessionChanged(@ImsMediaSession.SessionState int state) {
            Log.d("", "onSessionChanged - state=" + state);
        }

        @Override
        public void onAddConfigResponse(AudioConfig config, int result) {
            Log.d("", "onAddConfigResponse");
        }

        @Override
        public void onConfirmConfigResponse(AudioConfig config, int result) {
            Log.d("", "onConfirmConfigResponse");
        }

        @Override
        public void onFirstMediaPacketReceived(AudioConfig config) {
            Log.d("", "onFirstMediaPacketReceived");
        }

    }

}
