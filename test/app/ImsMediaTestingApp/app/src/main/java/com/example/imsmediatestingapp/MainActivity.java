package com.example.imsmediatestingapp;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.hardware.radio.ims.media.AmrMode;
import android.hardware.radio.ims.media.CodecType;
import android.hardware.radio.ims.media.EvsBandwidth;
import android.hardware.radio.ims.media.EvsMode;
import android.media.AudioManager;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.IBinder;
import android.telephony.AccessNetworkConstants.AccessNetworkType;
import android.telephony.imsmedia.AmrParams;
import android.telephony.imsmedia.AudioConfig;
import android.telephony.imsmedia.AudioSessionCallback;
import android.telephony.imsmedia.EvsParams;
import android.telephony.imsmedia.ImsAudioSession;
import android.telephony.imsmedia.ImsMediaManager;
import android.telephony.imsmedia.ImsMediaSession;
import android.telephony.imsmedia.RtcpConfig;
import android.telephony.imsmedia.RtpConfig;
import android.text.format.Formatter;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.PopupMenu;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.SwitchCompat;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

/**
 * The MainActivity is the main and default layout for the application.
 */
public class MainActivity extends AppCompatActivity {
    private SharedPreferences prefs;
    private SharedPreferences.Editor editor;
    private SharedPrefsHandler prefsHandler;
    public static final String PREF_NAME = "preferences";
    private static final String HANDSHAKE_PORT_PREF = "HANDSHAKE_PORT_OPEN";
    private static final String CONFIRMATION_MESSAGE = "CONNECTED";
    private static final String TAG = MainActivity.class.getName();

    private static final int MAX_MTU_BYTES = 1500;
    private static final int DSCP = 0;
    private static final int RX_PAYLOAD_TYPE_NUMBER = 96;
    private static final int TX_PAYLOAD_TYPE_NUMBER = 96;
    private static final int SAMPLING_RATE_KHZ = 16;
    private static final int P_TIME_MILLIS = 20;
    private static final int MAX_P_TIME_MILLIS = 240;
    private static final int TX_CODEC_MODE_REQUEST = 15;
    private static final int DTMF_PAYLOAD_TYPE_NUMBER = 100;
    private static final int DTMF_SAMPLING_RATE_KHZ = 16;

    private static final float DISABLED_ALPHA = 0.3f;
    private static final float ENABLED_ALPHA = 1.0f;

    private Set<Integer> selectedCodecTypes = new HashSet<>();
    private Set<Integer> selectedAmrModes = new HashSet<>();
    private Set<Integer> selectedEvsBandwidths = new HashSet<>();
    private Set<Integer> selectedEvsModes = new HashSet<>();
    private Set<Integer> selectedVideoCodecs = new HashSet<>();

    // The order of these values determines the priority in which they would be selected if there
    // is a common match between the two devices' selections during the handshake process.
    private static final int[] CODEC_ORDER = new int[]{CodecType.AMR, CodecType.AMR_WB,
        CodecType.EVS, CodecType.PCMA, CodecType.PCMU};
    private static final int[] EVS_BANDWIDTH_ORDER = new int[]{EvsBandwidth.NONE,
        EvsBandwidth.NARROW_BAND, EvsBandwidth.WIDE_BAND, EvsBandwidth.SUPER_WIDE_BAND,
        EvsBandwidth.FULL_BAND};
    private static final int[] AMR_MODE_ORDER = new int[]{AmrMode.AMR_MODE_0, AmrMode.AMR_MODE_1,
        AmrMode.AMR_MODE_2, AmrMode.AMR_MODE_3, AmrMode.AMR_MODE_4, AmrMode.AMR_MODE_5,
        AmrMode.AMR_MODE_6, AmrMode.AMR_MODE_7, AmrMode.AMR_MODE_8};
    private static final int[] EVS_MODE_ORDER = new int[]{EvsMode.EVS_MODE_0, EvsMode.EVS_MODE_1,
        EvsMode.EVS_MODE_2, EvsMode.EVS_MODE_3, EvsMode.EVS_MODE_4, EvsMode.EVS_MODE_5,
        EvsMode.EVS_MODE_6, EvsMode.EVS_MODE_7, EvsMode.EVS_MODE_8, EvsMode.EVS_MODE_9,
        EvsMode.EVS_MODE_10, EvsMode.EVS_MODE_11, EvsMode.EVS_MODE_12, EvsMode.EVS_MODE_13,
        EvsMode.EVS_MODE_14, EvsMode.EVS_MODE_15, EvsMode.EVS_MODE_16, EvsMode.EVS_MODE_17,
        EvsMode.EVS_MODE_18, EvsMode.EVS_MODE_19, EvsMode.EVS_MODE_20};

    private boolean loopbackModeEnabled = false;
    private boolean isMediaManagerReady = false;
    private boolean isOpenSessionSent = false;
    private final StringBuilder dtmfInput = new StringBuilder();

    private ConnectionStatus connectionStatus;
    private ImsAudioSession audioSession;
    private AudioConfig audioConfig;
    private ImsMediaManager imsMediaManager;
    private Executor executor;
    private Thread waitForHandshakeThread;
    private HandshakeReceiver handshakeReceptionSocket;
    private DatagramSocket rtp;
    private DatagramSocket rtcp;
    private DeviceInfo remoteDeviceInfo;
    private DeviceInfo localDeviceInfo;
    private BottomSheetDialer bottomSheetDialog;
    private BottomSheetAudioCodecSettings bottomSheetAudioCodecSettings;
    private TextView localHandshakePortLabel;
    private TextView localRtpPortLabel;
    private TextView localRtcpPortLabel;
    private TextView remoteIpLabel;
    private TextView remoteHandshakePortLabel;
    private TextView remoteRtpPortLabel;
    private TextView remoteRtcpPortLabel;
    private Button allowCallsButton;
    private Button connectButton;
    private Button openSessionButton;
    private SwitchCompat loopbackSwitch;
    private LinearLayout activeCallToolBar;

    /**
     * Enum of the CodecType from android.hardware.radio.ims.media.CodecType with the matching
     * Integer value.
     */
    public enum CodecTypeEnum {
        AMR(CodecType.AMR),
        AMR_WB(CodecType.AMR_WB),
        EVS(CodecType.EVS),
        PCMA(CodecType.PCMA),
        PCMU(CodecType.PCMU);

        private final int value;

        CodecTypeEnum(int value) {
            this.value = value;
        }

        public int getValue() {
            return value;
        }
    }

    /**
     * Enum of the AmrMode from android.hardware.radio.ims.media.AmrMode with the matching
     * Integer value.
     */
    public enum AmrModeEnum {
        AMR_MODE_0(AmrMode.AMR_MODE_0),
        AMR_MODE_1(AmrMode.AMR_MODE_1),
        AMR_MODE_2(AmrMode.AMR_MODE_2),
        AMR_MODE_3(AmrMode.AMR_MODE_3),
        AMR_MODE_4(AmrMode.AMR_MODE_4),
        AMR_MODE_5(AmrMode.AMR_MODE_5),
        AMR_MODE_6(AmrMode.AMR_MODE_6),
        AMR_MODE_7(AmrMode.AMR_MODE_7),
        AMR_MODE_8(AmrMode.AMR_MODE_8);

        private final int value;

        AmrModeEnum(int value) {
            this.value = value;
        }

        public int getValue() {
            return value;
        }

    }

    /**
     * Enum of the EvsBandwidth from android.hardware.radio.ims.media.EvsBandwidth with the
     * matching Integer value.
     */
    public enum EvsBandwidthEnum {
        NONE(EvsBandwidth.NONE),
        NARROW_BAND(EvsBandwidth.NARROW_BAND),
        WIDE_BAND(EvsBandwidth.WIDE_BAND),
        SUPER_WIDE_BAND(EvsBandwidth.SUPER_WIDE_BAND),
        FULL_BAND(EvsBandwidth.FULL_BAND);

        private final int value;

        EvsBandwidthEnum(int value) {
            this.value = value;
        }

        public int getValue() {
            return value;
        }
    }

    /**
     * Enum of the EvsMode from android.hardware.radio.ims.media.EvsMode with the matching
     * Integer value.
     */
    public enum EvsModeEnum {
        EVS_MODE_0(EvsMode.EVS_MODE_0),
        EVS_MODE_1(EvsMode.EVS_MODE_1),
        EVS_MODE_2(EvsMode.EVS_MODE_2),
        EVS_MODE_3(EvsMode.EVS_MODE_3),
        EVS_MODE_4(EvsMode.EVS_MODE_4),
        EVS_MODE_5(EvsMode.EVS_MODE_5),
        EVS_MODE_6(EvsMode.EVS_MODE_6),
        EVS_MODE_7(EvsMode.EVS_MODE_7),
        EVS_MODE_8(EvsMode.EVS_MODE_8),
        EVS_MODE_9(EvsMode.EVS_MODE_9),
        EVS_MODE_10(EvsMode.EVS_MODE_10),
        EVS_MODE_11(EvsMode.EVS_MODE_11),
        EVS_MODE_12(EvsMode.EVS_MODE_12),
        EVS_MODE_13(EvsMode.EVS_MODE_13),
        EVS_MODE_14(EvsMode.EVS_MODE_14),
        EVS_MODE_15(EvsMode.EVS_MODE_15),
        EVS_MODE_16(EvsMode.EVS_MODE_16),
        EVS_MODE_17(EvsMode.EVS_MODE_17),
        EVS_MODE_18(EvsMode.EVS_MODE_18),
        EVS_MODE_19(EvsMode.EVS_MODE_19),
        EVS_MODE_20(EvsMode.EVS_MODE_20);

        private final int value;

        EvsModeEnum(int value) {
            this.value = value;
        }

        public int getValue() {
            return value;
        }

    }

    public enum VideoCodecEnum {
        H264(0),
        HEVC(1);

        private final int value;

        VideoCodecEnum(int value) {
            this.value = value;
        }

        public int getValue() {
            return value;
        }
    }

    /**
     * Enum of the different states the application can be in. Mainly used to decide how
     * different features of the app UI will be styled.
     */
    public enum ConnectionStatus {
        OFFLINE(0),
        DISCONNECTED(1),
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
        prefsHandler = new SharedPrefsHandler(prefs);
        editor = prefs.edit();
        editor.putBoolean(HANDSHAKE_PORT_PREF, false);
        editor.apply();

        Context context = getApplicationContext();
        MediaManagerCallback callback = new MediaManagerCallback();
        executor = Executors.newSingleThreadExecutor();
        imsMediaManager = new ImsMediaManager(context, executor, callback);

        bottomSheetDialog = new BottomSheetDialer(this);
        bottomSheetDialog.setContentView(R.layout.dialer);

        bottomSheetAudioCodecSettings = new BottomSheetAudioCodecSettings(this);
        bottomSheetAudioCodecSettings.setContentView(R.layout.audio_codec_change);

        updateCodecSelectionFromPrefs();

        updateUI(ConnectionStatus.OFFLINE);
    }

    @Override
    protected void onStart() {
        super.onStart();
        styleMainActivity();
    }

    @Override
    protected void onResume() {
        super.onResume();
        styleMainActivity();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (rtp != null) {
            rtp.close();
        }
        if (rtcp != null) {
            rtcp.close();
        }
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
            case R.id.homeMenuButton:
                setContentView(R.layout.activity_main);
                styleMainActivity();
                break;

            case R.id.settingsMenuButton:
                setContentView(R.layout.settings);
                setupSettingsPage();
                break;

            default:
                throw new IllegalStateException("Unexpected value: " + item.getItemId());
        }
        return super.onOptionsItemSelected(item);
    }

    private class MediaManagerCallback implements ImsMediaManager.OnConnectedCallback {

        @Override
        public void onConnected() {
            Log.d(TAG, "ImsMediaManager - connected");
            isMediaManagerReady = true;
        }

        @Override
        public void onDisconnected() {
            Log.d(TAG, "ImsMediaManager - disconnected");
            isMediaManagerReady = false;
            updateUI(ConnectionStatus.CONNECTED);
        }
    }

    private class RtpAudioSessionCallback extends AudioSessionCallback {

        @Override
        public void onModifySessionResponse(AudioConfig config, int result) {
            Log.d(TAG, "onModifySessionResponse");
        }

        @Override
        public void onOpenSessionFailure(int error) {
            Log.e(TAG, "onOpenSessionFailure - error=" + error);
        }

        @Override
        public void onOpenSessionSuccess(ImsMediaSession session) {
            audioSession = (ImsAudioSession) session;
            Log.d(TAG, "onOpenSessionSuccess: id=" + audioSession.getSessionId());
            isOpenSessionSent = true;
            updateUI(ConnectionStatus.ACTIVE_CALL);
            AudioManager audioManager = getSystemService(AudioManager.class);
            audioManager.setMode(AudioManager.MODE_IN_CALL);
            audioManager.setSpeakerphoneOn(true);
        }

        @Override
        public void onSessionChanged(@ImsMediaSession.SessionState int state) {
            Log.d(TAG, "onSessionChanged - state=" + state);
        }

        @Override
        public void onAddConfigResponse(AudioConfig config, int result) {
            Log.d(TAG, "onAddConfigResponse");
        }

        @Override
        public void onConfirmConfigResponse(AudioConfig config, int result) {
            Log.d(TAG, "onConfirmConfigResponse");
        }

        @Override
        public void onFirstMediaPacketReceived(AudioConfig config) {
            Log.d(TAG, "onFirstMediaPacketReceived");
        }

        @Override
        public IBinder getBinder() {
            return super.getBinder();
        }

        @Override
        public void setExecutor(Executor executor) {
            super.setExecutor(executor);
        }

        @Override
        public void notifyMediaInactivity(int packetType, int duration) {
            super.notifyMediaInactivity(packetType, duration);
        }

        @Override
        public void notifyPacketLoss(int packetLossPercentage) {
            super.notifyPacketLoss(packetLossPercentage);
        }

        @Override
        public void notifyJitter(int jitter) {
            super.notifyJitter(jitter);
        }

    }

    private WifiInfo retrieveNetworkConfig() {
        WifiManager wifiManager = (WifiManager) getApplication()
            .getSystemService(Context.WIFI_SERVICE);
        return wifiManager.getConnectionInfo();
    }

    private String getLocalIpAddress() {
        return Formatter.formatIpAddress(retrieveNetworkConfig().getIpAddress());
    }

    private String getOtherDeviceIp() {
        return prefs.getString("OTHER_IP_ADDRESS", "localhost");
    }

    private int getOtherDevicePort() {
        return prefs.getInt("OTHER_HANDSHAKE_PORT", -1);
    }

    private int getRemoteDevicePortEditText() {
        EditText portBox = findViewById(R.id.remotePortNumberEditText);
        return Integer.parseInt(portBox.getText().toString());
    }

    private String getRemoteDeviceIpEditText() {
        EditText ipBox = findViewById(R.id.remoteDeviceIpEditText);
        return ipBox.getText().toString();
    }

    /**
     * Opens two datagram sockets for audio rtp and rtcp, and a third for the handshake between
     * devices if true is passed in the parameter.
     * @param openHandshakePort boolean value to open a port for the handshake.
     */
    private void openRtpPorts(boolean openHandshakePort) {
        Executor socketBindingExecutor = Executors.newSingleThreadExecutor();

        Runnable bindSockets = () -> {
            try {
                rtp = new DatagramSocket();
                rtp.setReuseAddress(true);

                rtcp = new DatagramSocket(rtp.getLocalPort() + 1);
                rtcp.setReuseAddress(true);

                if (openHandshakePort) {
                    handshakeReceptionSocket = new HandshakeReceiver(prefs);
                    handshakeReceptionSocket.run();
                }
            } catch (SocketException e) {
                Log.e(TAG, e.toString());
            }
        };

        socketBindingExecutor.execute(bindSockets);
    }

    /**
     * Closes the handshake, rtp, and rtcp ports if they have been opened or instantiated.
     */
    private void closePorts() {
        if (handshakeReceptionSocket != null) {
            handshakeReceptionSocket.close();
        }

        if (rtp != null) {
            rtp.close();
        }

        if (rtcp != null) {
            rtcp.close();
        }
    }

    /**
     * After the ports are open this runnable is called to wait for in incoming handshake to pair
     * with the remote device.
     */
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
                    remoteDeviceInfo.getInetAddress(), remoteDeviceInfo.getHandshakePort());
                localDeviceInfo = createMyDeviceInfo();
                handshakeSender.setData(localDeviceInfo);
                handshakeSender.run();

                while (!handshakeReceptionSocket.isConfirmationReceived()) {
                    if (Thread.currentThread().isInterrupted()) {
                        throw new InterruptedException();
                    }
                }

                handshakeSender = new HandshakeSender(remoteDeviceInfo.getInetAddress(),
                    remoteDeviceInfo.getHandshakePort());
                handshakeSender.setData(CONFIRMATION_MESSAGE);
                handshakeSender.run();
                Log.d(TAG, "Handshake has been completed. Devices are connected.");
                editor.putString("OTHER_IP_ADDRESS",
                    remoteDeviceInfo.getInetAddress().getHostName());
                editor.putInt("OTHER_HANDSHAKE_PORT", remoteDeviceInfo.getRtpPort());
                editor.apply();
                updateUI(ConnectionStatus.CONNECTED);
            } catch (InterruptedException e) {
                Log.e(TAG, e.toString());
            }

        }
    };

    /**
     * This runnable controls the handshake process from the user that is attempting to connect
     * to the remote device. First it will create and send a DeviceInfo object that contains the
     * local devices info, and wait until it receives the remote DeviceInfo. After it receives
     * the remote DeviceInfo it will save it into memory and send a conformation String back, then
     * wait until it receives a conformation String.
     */
    Runnable initiateHandshake = new Runnable() {
        @Override
        public void run() {
            try {
                HandshakeSender sender = new HandshakeSender(
                    InetAddress.getByName(getOtherDeviceIp()), getOtherDevicePort());
                localDeviceInfo = createMyDeviceInfo();
                sender.setData(localDeviceInfo);
                sender.run();

                while (!handshakeReceptionSocket.isHandshakeReceived()) {

                }
                remoteDeviceInfo = handshakeReceptionSocket.getReceivedDeviceInfo();

                sender.setData(CONFIRMATION_MESSAGE);
                sender.run();

                while (!handshakeReceptionSocket.isConfirmationReceived()) {

                }

                Log.d(TAG, "Handshake successful, devices connected.");
                updateUI(ConnectionStatus.CONNECTED);
            } catch (Exception e) {
                Log.e("initiateHandshake(): ", e.toString());
            }
        }
    };

    /**
     * Creates and returns a DeviceInfo object with the local port, ip, and audio codec settings
     * @return DeviceInfo object containing the local device's information
     */
    public DeviceInfo createMyDeviceInfo() {
        try {
            return new DeviceInfo.Builder()
                .setInetAddress(InetAddress.getByName(getLocalIpAddress()))
                .setHandshakePort(handshakeReceptionSocket.getBoundSocket())
                .setRtpPort(rtp.getLocalPort())
                .setRtcpPort(rtcp.getLocalPort())
                .setAudioCodecs(selectedCodecTypes)
                .setAmrModes(selectedAmrModes)
                .setEvsBandwidths(selectedEvsBandwidths)
                .setEvsModes(selectedEvsModes)
                .build();

        } catch (UnknownHostException e) {
            e.printStackTrace();
        }
        return null;
    }

    /**
     * Updates the connectionStatus and restyles the UI
     * @param newStatus The new ConnectionStatus used to style the UI
     */
    public void updateUI(ConnectionStatus newStatus) {
        connectionStatus = newStatus;
        styleMainActivity();
    }

    /**
     * Creates and returns an InetSocketAddress from the remote device that is connected to the
     * local device.
     * @return the InetSocketAddress of the remote device
     */
    private InetSocketAddress getRemoteSocketAddress() {
        int remotePort = remoteDeviceInfo.getRtpPort();
        InetAddress remoteInetAddress = remoteDeviceInfo.getInetAddress();
        return new InetSocketAddress(remoteInetAddress, remotePort);
    }

    /**
     * Builds and returns an RtcpConfig for the remote device that is connected to the local device.
     * @return the RtcpConfig for the remote device
     */
    private RtcpConfig getRemoteRtcpConfig() {
        return new RtcpConfig.Builder()
            .setCanonicalName("rtp config")
            .setTransmitPort(remoteDeviceInfo.getRtpPort() + 1)
            .setIntervalSec(5)
            .setRtcpXrBlockTypes(0)
            .build();
    }

    /**
     * Creates and returns a new AudioConfig
     * @param remoteRtpAddress - InetSocketAddress of the remote device
     * @param rtcpConfig - RtcpConfig of the remove device
     * @param audioCodec - the type of AudioCodec
     * @param amrParams - the settings if the AudioCodec is an AMR variant
     * @param evsParams - the settings if the AudioCodec is EVS
     * @return an AudioConfig with the given params
     */
    private AudioConfig createAudioConfig(InetSocketAddress remoteRtpAddress,
        RtcpConfig rtcpConfig, int audioCodec, AmrParams amrParams, EvsParams evsParams) {
        AudioConfig config;

        if(audioCodec == AudioConfig.CODEC_AMR || audioCodec == AudioConfig.CODEC_AMR_WB) {
            config = new AudioConfig.Builder()
                .setMediaDirection(RtpConfig.MEDIA_DIRECTION_TRANSMIT_RECEIVE)
                .setAccessNetwork(AccessNetworkType.EUTRAN)
                .setRemoteRtpAddress(remoteRtpAddress)
                .setRtcpConfig(rtcpConfig)
                .setMaxMtuBytes(MAX_MTU_BYTES)
                .setDscp((byte) DSCP)
                .setRxPayloadTypeNumber((byte) RX_PAYLOAD_TYPE_NUMBER)
                .setTxPayloadTypeNumber((byte) TX_PAYLOAD_TYPE_NUMBER)
                .setSamplingRateKHz((byte) SAMPLING_RATE_KHZ)
                .setPtimeMillis((byte) P_TIME_MILLIS)
                .setMaxPtimeMillis((byte) MAX_P_TIME_MILLIS)
                .setTxCodecModeRequest((byte) TX_CODEC_MODE_REQUEST)
                .setDtxEnabled(true)
                .setDtmfPayloadTypeNumber((byte) DTMF_PAYLOAD_TYPE_NUMBER)
                .setDtmfSamplingRateKHz((byte) DTMF_SAMPLING_RATE_KHZ)
                .setCodecType(audioCodec)
                .setAmrParams(amrParams)
                // TODO - audio is currently only working when evs params are set as well
                .setEvsParams(evsParams)
                .build();

        } else if(audioCodec == AudioConfig.CODEC_EVS) {
            config = new AudioConfig.Builder()
                .setMediaDirection(RtpConfig.MEDIA_DIRECTION_TRANSMIT_RECEIVE)
                .setAccessNetwork(AccessNetworkType.EUTRAN)
                .setRemoteRtpAddress(remoteRtpAddress)
                .setRtcpConfig(rtcpConfig)
                .setMaxMtuBytes(MAX_MTU_BYTES)
                .setDscp((byte) DSCP)
                .setRxPayloadTypeNumber((byte) RX_PAYLOAD_TYPE_NUMBER)
                .setTxPayloadTypeNumber((byte) TX_PAYLOAD_TYPE_NUMBER)
                .setSamplingRateKHz((byte) SAMPLING_RATE_KHZ)
                .setPtimeMillis((byte) P_TIME_MILLIS)
                .setMaxPtimeMillis((byte) MAX_P_TIME_MILLIS)
                .setTxCodecModeRequest((byte) TX_CODEC_MODE_REQUEST)
                .setDtxEnabled(true)
                .setDtmfPayloadTypeNumber((byte) DTMF_PAYLOAD_TYPE_NUMBER)
                .setDtmfSamplingRateKHz((byte) DTMF_SAMPLING_RATE_KHZ)
                .setCodecType(audioCodec)
                .setEvsParams(evsParams)
                .build();

        } else {
            config = new AudioConfig.Builder()
                .setMediaDirection(RtpConfig.MEDIA_DIRECTION_TRANSMIT_RECEIVE)
                .setAccessNetwork(AccessNetworkType.EUTRAN)
                .setRemoteRtpAddress(remoteRtpAddress)
                .setRtcpConfig(rtcpConfig)
                .setMaxMtuBytes(MAX_MTU_BYTES)
                .setDscp((byte) DSCP)
                .setRxPayloadTypeNumber((byte) RX_PAYLOAD_TYPE_NUMBER)
                .setTxPayloadTypeNumber((byte) TX_PAYLOAD_TYPE_NUMBER)
                .setSamplingRateKHz((byte) SAMPLING_RATE_KHZ)
                .setPtimeMillis((byte) P_TIME_MILLIS)
                .setMaxPtimeMillis((byte) MAX_P_TIME_MILLIS)
                .setTxCodecModeRequest((byte) TX_CODEC_MODE_REQUEST)
                .setDtxEnabled(true)
                .setDtmfPayloadTypeNumber((byte) DTMF_PAYLOAD_TYPE_NUMBER)
                .setDtmfSamplingRateKHz((byte) DTMF_SAMPLING_RATE_KHZ)
                .setCodecType(audioCodec)
                .build();
        }
        return config;
    }

    /**
     * @param amrMode Integer value of the AmrMode
     * @return AmrParams object with the passed AmrMode value
     */
    private AmrParams createAmrParams(int amrMode) {
        return new AmrParams.Builder()
            .setAmrMode(amrMode)
            .setOctetAligned(true)
            .setMaxRedundancyMillis(0)
            .build();
    }

    /**
     * @param evsBand Integer value of the EvsBandwidth
     * @param evsMode Integer value of the EvsMode
     * @return EvsParams object with the passed EvsBandwidth and EvsMode
     */
    private EvsParams createEvsParams(int evsBand, int evsMode) {
        return new EvsParams.Builder()
            .setEvsbandwidth(evsBand)
            .setEvsMode(evsMode)
            .setChannelAwareMode((byte) 3)
            .setHeaderFullOnlyOnTx(true)
            .setHeaderFullOnlyOnRx(true)
            .build();
    }

    /**
     * Determines the audio codec to use to configure the AudioConfig object. The function uses
     * the order arrays of Integers to determine the priority of a given codec, mode, and
     * bandwidth. Then creates and returns a AudioConfig object containing it.
     * @param localDevice DeviceInfo object containing the local device's information
     * @param remoteDevice DeviceInfo object containing the remote device's information
     * @return AudioConfig containing the selected audio codec, determined by the algorithm
     */
    private AudioConfig determineAudioConfig(DeviceInfo localDevice, DeviceInfo remoteDevice) {
        AmrParams amrParams = null;
        EvsParams evsParams = null;

        int selectedCodec = determineCommonCodecSettings(localDevice.getAudioCodecs(),
            remoteDevice.getAudioCodecs(), CODEC_ORDER);

        switch (selectedCodec) {
            case CodecType.AMR: case CodecType.AMR_WB:
                int amrMode = determineCommonCodecSettings(localDevice.getAmrModes(),
                    remoteDevice.getAmrModes(), AMR_MODE_ORDER);
                amrParams = createAmrParams(amrMode);
                break;

            case CodecType.EVS:
                int evsMode = determineCommonCodecSettings(localDevice.getEvsModes(),
                    remoteDevice.getEvsModes(), EVS_MODE_ORDER);
                int evsBand = determineCommonCodecSettings(localDevice.getEvsBandwidths(),
                    remoteDevice.getEvsBandwidths(), EVS_BANDWIDTH_ORDER);
                evsParams = createEvsParams(evsBand, evsMode);
                break;

            case -1:
                return createAudioConfig(CodecType.AMR_WB, createAmrParams(AmrMode.AMR_MODE_4),
                    null);
        }

        return createAudioConfig(selectedCodec, amrParams, evsParams);
    }

    /**
     * Helper function used to determine the highest ranking codec, mode, or bandwidth between
     * two devices.
     * @param localSet the set containing the local device's selection of codecs, modes, or
     * bandwidths
     * @param remoteSet the set containing the remote device's selection of codecs, modes, or
     * bandwidths
     * @param codecSetting the Integer array containing the ranking order of the different values
     * @return highest ranking mode, codec, bandwidth, or -1 if no match is found
     */
    private int determineCommonCodecSettings(Set<Integer> localSet, Set<Integer> remoteSet,
        int[] codecSetting) {
        for (int setting : codecSetting) {
            if(localSet.contains(setting) && remoteSet.contains(setting)) {
                return setting;
            }
        }
        return -1;
    }

    /**
     * Creates an AudioConfig object depending on the passed parameters and returns it.
     * @param audioCodec Integer value of the CodecType
     * @param amrParams AmrParams object to be set in the AudioConfig
     * @param evsParams EvsParams object to be set in the AudioConfig
     * @return an AudioConfig with the passed parameters and default values.
     */
    private AudioConfig createAudioConfig(int audioCodec, AmrParams amrParams,
        EvsParams evsParams) {
        AudioConfig audioConfig = null;
        // TODO - evs params must be present to hear audio currently, regardless of codec
        EvsParams mEvs = new EvsParams.Builder()
            .setEvsbandwidth(EvsParams.EVS_BAND_NONE)
            .setEvsMode(EvsParams.EVS_MODE_0)
            .setChannelAwareMode((byte) 3)
            .setHeaderFullOnlyOnTx(true)
            .setHeaderFullOnlyOnRx(true)
            .build();

        switch (audioCodec) {
            case CodecType.AMR: case CodecType.AMR_WB:
                audioConfig = createAudioConfig(getRemoteSocketAddress(), getRemoteRtcpConfig(),
                    audioCodec, amrParams, mEvs);
                break;

            case CodecType.EVS:
                audioConfig = createAudioConfig(getRemoteSocketAddress(), getRemoteRtcpConfig(),
                    audioCodec, null, evsParams);
                break;

            case CodecType.PCMA: case CodecType.PCMU:
                audioConfig = createAudioConfig(getRemoteSocketAddress(), getRemoteRtcpConfig(),
                    audioCodec, null, null);
                break;

        }

        return audioConfig;
    }

    /**
     * Displays the dialer BottomSheetDialog when the button is clicked
     * @param view the view form the button click
     */
    public void openDialer(View view) {
        if (!bottomSheetDialog.isOpen()) {
            bottomSheetDialog.show();
        }
    }

    /**
     * Sends a DTMF input to the current AudioSession and updates the TextView to display the input.
     * @param view the view from the button click
     */
    public void sendDtmfOnClick(View view) {
        char digit = ((Button) view).getText().toString().charAt(0);
        dtmfInput.append(digit);

        TextView dtmfInputBox = bottomSheetDialog.getDtmfInput();
        dtmfInputBox.setText(dtmfInput.toString());

        audioSession.startDtmf(digit, 50, 3);
        audioSession.stopDtmf();
    }

    /**
     * Resets the TextView containing the DTMF input
     * @param view the view from the button click
     */
    public void clearDtmfInputOnClick(View view) {
        dtmfInput.setLength(0);
        TextView dtmfInputBox = bottomSheetDialog.getDtmfInput();
        dtmfInputBox.setText(getString(R.string.dtmfInputPlaceholder));
    }

    /**
     * Calls closeSession() on ImsMediaManager and resets the flag on isOpenSessionSent
     * @param view the view from the button click
     */
    public void closeSessionOnClick(View view) {
        imsMediaManager.closeSession(audioSession);
        isOpenSessionSent = false;
    }

    /**
     * When the button is clicked a menu is opened containing the different media directions and
     * the onMenuItemClickListener is set to handle the user's selection.
     * @param view The view passed in from the button that is clicked
     */
    @SuppressLint("NonConstantResourceId")
    public void mediaDirectionOnClick(View view) {
        PopupMenu mediaDirectionMenu = new PopupMenu(this, findViewById(R.id.mediaDirectionButton));
        mediaDirectionMenu.getMenuInflater()
            .inflate(R.menu.media_direction_menu, mediaDirectionMenu.getMenu());
        mediaDirectionMenu.setOnMenuItemClickListener(item -> {
            switch (item.getItemId()) {

                case R.id.noFlowDirectionMenuItem:
                    audioConfig.setMediaDirection(AudioConfig.MEDIA_DIRECTION_NO_FLOW);
                    break;

                case R.id.transmitReceiveDirectionMenuItem:
                    audioConfig.setMediaDirection(AudioConfig.MEDIA_DIRECTION_TRANSMIT_RECEIVE);
                    break;

                case R.id.receiveOnlyDirectionMenuItem:
                    audioConfig.setMediaDirection(AudioConfig.MEDIA_DIRECTION_RECEIVE_ONLY);
                    break;

                case R.id.transmitOnlyDirectionMenuItem:
                    audioConfig.setMediaDirection(AudioConfig.MEDIA_DIRECTION_TRANSMIT_ONLY);
                    break;

                default:
                    return false;
            }

            audioSession.modifySession(audioConfig);
            return true;
        });
        mediaDirectionMenu.show();
    }

    /**
     * Displays the audio codec change BottomSheetDialog when the button is clicked
     * @param view the view form the button click
     */
    public void openChangeAudioCodecSheet(View view) {
        if (!bottomSheetAudioCodecSettings.isOpen()) {
            bottomSheetAudioCodecSettings.show();
        }
    }

    /**
     * Calls openSession() on the ImsMediaManager
     * @param view the view from the button click
     */
    public void openSessionOnClick(View view) {
        if (isMediaManagerReady && !isOpenSessionSent) {

            Toast.makeText(getApplicationContext(), getString(R.string.connecting_call_toast_text),
                Toast.LENGTH_SHORT).show();

            audioConfig = determineAudioConfig(localDeviceInfo, remoteDeviceInfo);
            Log.d(TAG, audioConfig.toString());

            RtpAudioSessionCallback sessionCallback = new RtpAudioSessionCallback();
            imsMediaManager.openSession(rtp, rtcp, ImsMediaSession.SESSION_TYPE_AUDIO,
                audioConfig, executor, sessionCallback);
            Log.d(TAG, "openSession(): " + remoteDeviceInfo.getInetAddress() + ":" +
                remoteDeviceInfo.getRtpPort());
            Log.d(TAG, "AudioConfig: " + audioConfig.toString());
        }
    }

    /**
     * Saves the inputted ip address and port number to SharedPreferences.
     * @param view the view from the button click
     */
    public void saveSettingsOnClick(View view) {
        int port = getRemoteDevicePortEditText();
        String ip = getRemoteDeviceIpEditText();

        editor.putInt("OTHER_HANDSHAKE_PORT", port);
        editor.putString("OTHER_IP_ADDRESS", ip);
        editor.apply();

        Toast.makeText(getApplicationContext(), R.string.save_button_action_toast,
            Toast.LENGTH_SHORT).show();
    }

    /**
     * Calls modifySession to change the audio codec on the current AudioSession. Also contains
     * the logic to create the new AudioConfig.
     * @param view the view form the button click
     */
    public void changeAudioCodecOnClick(View view) {
        AudioConfig config = null;
        AmrParams amrParams;
        EvsParams evsParams;
        int audioCodec = bottomSheetAudioCodecSettings.getAudioCodec();

        switch (audioCodec) {
            case CodecType.AMR: case CodecType.AMR_WB:
                amrParams = createAmrParams(bottomSheetAudioCodecSettings.getAmrMode());
                config = createAudioConfig(getRemoteSocketAddress(), getRemoteRtcpConfig(),
                    audioCodec, amrParams, null);
                Log.d(TAG, String.format("AudioConfig switched to Codec: %s\t Params: %s",
                    bottomSheetAudioCodecSettings.getAudioCodec(),
                    config.getAmrParams().toString()));
                break;

            case CodecType.EVS:
                evsParams = createEvsParams(bottomSheetAudioCodecSettings.getEvsBand(),
                    bottomSheetAudioCodecSettings.getEvsMode());
                config = createAudioConfig(getRemoteSocketAddress(), getRemoteRtcpConfig(),
                    audioCodec, null, evsParams);
                Log.d(TAG, String.format("AudioConfig switched to Codec: %s\t Params: %s",
                    bottomSheetAudioCodecSettings.getAudioCodec(),
                    config.getEvsParams().toString()));
                break;

            case CodecType.PCMA: case CodecType.PCMU:
                config = createAudioConfig(getRemoteSocketAddress(), getRemoteRtcpConfig(),
                    audioCodec, null, null);
                Log.d(TAG, String.format("AudioConfig switched to Codec: %s",
                    bottomSheetAudioCodecSettings.getAudioCodec()));
                break;
        }

        audioSession.modifySession(config);
        bottomSheetAudioCodecSettings.dismiss();
    }

    /**
     * Changes the flag of loopback mode, changes the ConnectionStatus sate, and restyles the UI
     * @param view the view from, the button click
     */
    public void loopbackOnClick(View view) {
        SwitchCompat loopbackSwitch = findViewById(R.id.loopbackModeSwitch);
        if (loopbackSwitch.isChecked()) {
            openRtpPorts(true);
            editor.putString("OTHER_IP_ADDRESS", getLocalIpAddress()).apply();
            remoteDeviceInfo = createMyDeviceInfo();
            localDeviceInfo = createMyDeviceInfo();
            loopbackModeEnabled = true;
            updateUI(ConnectionStatus.CONNECTED);
        } else {
            closePorts();
            loopbackModeEnabled = false;
            updateUI(ConnectionStatus.OFFLINE);
        }
    }

    /**
     * Opens or closes ports and starts the waiting handshake runnable depending on the current
     * state of the button.
     * @param view view from the button click
     */
    public void allowCallsOnClick(View view) {
        if (prefs.getBoolean(HANDSHAKE_PORT_PREF, false)) {
            closePorts();
            Log.d(TAG, "Closed handshake, rtp, and rtcp ports.");

            Toast.makeText(getApplicationContext(),
                "Closing ports",
                Toast.LENGTH_SHORT).show();
            updateUI(ConnectionStatus.OFFLINE);
        } else {
            openRtpPorts(true);
            while (!prefs.getBoolean(HANDSHAKE_PORT_PREF, false)) {
            }
            Log.d(TAG, "Handshake, rtp, and rtcp ports have been bound.");

            Toast.makeText(getApplicationContext(), getString(R.string.allowing_calls_toast_text),
                Toast.LENGTH_SHORT).show();

            waitForHandshakeThread = new Thread(handleIncomingHandshake);
            waitForHandshakeThread.start();
            updateUI(ConnectionStatus.DISCONNECTED);
        }
    }

    /**
     * Starts the handshake process runnable that attempts to connect to two device together.
     * @param view view from the button click
     */
    public void initiateHandshakeOnClick(View view) {
        waitForHandshakeThread.interrupt();
        Thread initiateHandshakeThread = new Thread(initiateHandshake);
        initiateHandshakeThread.start();
        updateUI(ConnectionStatus.CONNECTING);
    }

    /**
     * Handles the styling of the settings layout.
     */
    public void setupSettingsPage() {
        EditText ipAddress = findViewById(R.id.remoteDeviceIpEditText);
        EditText portNumber = findViewById(R.id.remotePortNumberEditText);
        ipAddress.setText(prefs.getString("OTHER_IP_ADDRESS", ""));
        portNumber.setText(String.valueOf(prefs.getInt("OTHER_HANDSHAKE_PORT", 0)));

        setupAudioCodecSelectionLists();
        setupCodecSelectionOnClickListeners();
    }

    /**
     * Gets the saved user selections for the audio codec settings and updates the UI's lists to
     * match.
     */
    private void setupAudioCodecSelectionLists() {
        updateCodecSelectionFromPrefs();

        ArrayAdapter<CodecTypeEnum> codecTypeAdapter = new ArrayAdapter<>(
            this, android.R.layout.simple_list_item_multiple_choice, CodecTypeEnum.values());
        ListView codecTypeList = findViewById(R.id.audioCodecList);
        codecTypeList.setAdapter(codecTypeAdapter);
        for(int i = 0; i < codecTypeAdapter.getCount(); i++) {
            CodecTypeEnum mode = (CodecTypeEnum) codecTypeList.getItemAtPosition(i);
            codecTypeList.setItemChecked(i, selectedCodecTypes.contains(mode.getValue()));
        }

        ArrayAdapter<EvsBandwidthEnum> evsBandAdaptor = new ArrayAdapter<>(
            this, android.R.layout.simple_list_item_multiple_choice, EvsBandwidthEnum.values());
        ListView evsBandwidthList = findViewById(R.id.evsBandwidthsList);
        evsBandwidthList.setAdapter(evsBandAdaptor);
        for(int i = 0; i < evsBandAdaptor.getCount(); i++) {
            EvsBandwidthEnum mode = (EvsBandwidthEnum) evsBandwidthList.getItemAtPosition(i);
            evsBandwidthList.setItemChecked(i, selectedEvsBandwidths.contains(mode.getValue()));
        }

        ArrayAdapter<AmrModeEnum> amrModeAdapter = new ArrayAdapter<>(
            this, android.R.layout.simple_list_item_multiple_choice, AmrModeEnum.values());
        ListView amrModesList = findViewById(R.id.amrModesList);
        amrModesList.setAdapter(amrModeAdapter);
        for(int i = 0; i < amrModeAdapter.getCount(); i++) {
            AmrModeEnum mode = (AmrModeEnum) amrModesList.getItemAtPosition(i);
            amrModesList.setItemChecked(i, selectedAmrModes.contains(mode.getValue()));
        }

        ArrayAdapter<EvsModeEnum> evsModeAdaptor = new ArrayAdapter<>(
            this, android.R.layout.simple_list_item_multiple_choice, EvsModeEnum.values());
        ListView evsModeList = findViewById(R.id.evsModesList);
        evsModeList.setAdapter(evsModeAdaptor);
        for(int i = 0; i < evsModeAdaptor.getCount(); i++) {
            EvsModeEnum mode = (EvsModeEnum) evsModeList.getItemAtPosition(i);
            evsModeList.setItemChecked(i, selectedEvsModes.contains(mode.getValue()));
        }
    }

    /**
     * Updates all of the lists containing the user's codecs selections.
     */
    private void updateCodecSelectionFromPrefs() {
        selectedCodecTypes = prefsHandler.getIntegerSetFromPrefs(SharedPrefsHandler.CODECS_PREF);
        selectedEvsBandwidths =
            prefsHandler.getIntegerSetFromPrefs(SharedPrefsHandler.EVS_BANDS_PREF);
        selectedAmrModes = prefsHandler.getIntegerSetFromPrefs(SharedPrefsHandler.AMR_MODES_PREF);
        selectedEvsModes = prefsHandler.getIntegerSetFromPrefs(SharedPrefsHandler.EVS_MODES_PREF);
    }

    /**
     * Adds onClickListeners to the 4 check box lists on the settings page, to handle the user input
     * of the codec, bandwidth, and mode selections.
     */
    public void setupCodecSelectionOnClickListeners() {
        ListView audioCodecList, evsBandList, amrModeList, evsModeList;

        audioCodecList = findViewById(R.id.audioCodecList);
        evsBandList = findViewById(R.id.evsBandwidthsList);
        amrModeList = findViewById(R.id.amrModesList);
        evsModeList = findViewById(R.id.evsModesList);

        audioCodecList.setOnItemClickListener((adapterView, view, position, id) -> {
            CodecTypeEnum item = (CodecTypeEnum) audioCodecList.getItemAtPosition(position);

            if(audioCodecList.isItemChecked(position)) {
                selectedCodecTypes.add(item.getValue());
                if(item == CodecTypeEnum.AMR || item == CodecTypeEnum.AMR_WB) {
                    amrModeList.setAlpha(ENABLED_ALPHA);
                    amrModeList.setEnabled(true);
                } else if(item == CodecTypeEnum.EVS) {
                    evsBandList.setAlpha(ENABLED_ALPHA);
                    amrModeList.setEnabled(true);
                    evsModeList.setAlpha(ENABLED_ALPHA);
                    evsModeList.setEnabled(true);
                }
            } else {
                selectedCodecTypes.remove(item.getValue());
                if(item == CodecTypeEnum.AMR || item == CodecTypeEnum.AMR_WB) {
                    amrModeList.setAlpha(0.3f);
                    amrModeList.setEnabled(false);
                } else if(item == CodecTypeEnum.EVS) {
                    evsBandList.setAlpha(0.3f);
                    evsBandList.setEnabled(false);
                    evsModeList.setAlpha(0.3f);
                    evsModeList.setEnabled(false);
                }
            }

            prefsHandler.saveIntegerSetToPrefs(SharedPrefsHandler.CODECS_PREF, selectedCodecTypes);

        });
        evsBandList.setOnItemClickListener((adapterView, view, position, id) -> {
            EvsBandwidthEnum item = (EvsBandwidthEnum) evsBandList.getItemAtPosition(position);

            if(evsBandList.isItemChecked(position)) {
                selectedEvsBandwidths.add(item.getValue());
            } else {
                selectedEvsBandwidths.remove(item.getValue());
            }

            prefsHandler.saveIntegerSetToPrefs(SharedPrefsHandler.EVS_BANDS_PREF,
                selectedEvsBandwidths);
        });
        evsModeList.setOnItemClickListener((adapterView, view, position, id) -> {
            EvsModeEnum item = (EvsModeEnum) evsModeList.getItemAtPosition(position);

            if(evsModeList.isItemChecked(position)) {
                selectedEvsModes.add(item.getValue());
            } else {
                selectedEvsModes.remove(item.getValue());
            }

            prefsHandler.saveIntegerSetToPrefs(SharedPrefsHandler.EVS_MODES_PREF, selectedEvsModes);
        });
        amrModeList.setOnItemClickListener((adapterView, view, position, id) -> {
            AmrModeEnum item = (AmrModeEnum) amrModeList.getItemAtPosition(position);

            if(amrModeList.isItemChecked(position)) {
                selectedAmrModes.add(item.getValue());
            } else {
                selectedAmrModes.remove(item.getValue());
            }

            prefsHandler.saveIntegerSetToPrefs(SharedPrefsHandler.AMR_MODES_PREF, selectedAmrModes);
        });
    }

    /**
     * Styles the main activity UI based on the current ConnectionStatus enum state.
     */
    private void styleMainActivity() {
        runOnUiThread(() -> {
            updateUiViews();
            styleDevicesInfo();
            switch (connectionStatus) {
                case OFFLINE:
                    styleOffline();
                    break;

                case DISCONNECTED:
                    styleDisconnected();
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
        });
    }

    private void styleDevicesInfo() {
        TextView localIpLabel = findViewById(R.id.localIpLabel);
        localIpLabel.setText(getString(R.string.local_ip_label, getLocalIpAddress()));

        remoteIpLabel = findViewById(R.id.remoteIpLabel);
        remoteIpLabel.setText(getString(R.string.other_device_ip_label,
            prefs.getString("OTHER_IP_ADDRESS", "null")));

        remoteHandshakePortLabel = findViewById(R.id.remoteHandshakePortLabel);
        remoteHandshakePortLabel.setText(getString(R.string.handshake_port_label,
            String.valueOf(getOtherDevicePort())));
    }

    private void styleOffline() {
        localHandshakePortLabel.setText(getString(R.string.port_closed_label));
        localRtpPortLabel.setText(getString(R.string.port_closed_label));
        localRtcpPortLabel.setText(getString(R.string.port_closed_label));
        remoteRtpPortLabel.setText(getString(R.string.port_closed_label));
        remoteRtcpPortLabel.setText(getString(R.string.port_closed_label));

        allowCallsButton.setText(R.string.allow_calls_button_text);
        allowCallsButton.setBackgroundColor(getColor(R.color.mint_green));
        styleButtonEnabled(allowCallsButton);

        connectButton.setText(R.string.connect_button_text);
        connectButton.setBackgroundColor(getColor(R.color.mint_green));
        styleButtonDisabled(connectButton);

        styleLoopbackSwitchEnabled();
        styleButtonDisabled(openSessionButton);
        styleLayoutChildrenDisabled(activeCallToolBar);
    }

    private void styleDisconnected() {
        localHandshakePortLabel.setText(getString(R.string.handshake_port_label,
            String.valueOf(handshakeReceptionSocket.getBoundSocket())));
        localRtpPortLabel.setText(getString(R.string.rtp_reception_port_label,
            String.valueOf(rtp.getLocalPort())));
        localRtcpPortLabel.setText(getString(R.string.rtcp_reception_port_label,
            String.valueOf(rtcp.getLocalPort())));
        remoteRtpPortLabel.setText(getString(R.string.port_closed_label));
        remoteRtcpPortLabel.setText(getString(R.string.port_closed_label));

        allowCallsButton.setText(R.string.disable_calls_button_text);
        allowCallsButton.setBackgroundColor(getColor(R.color.coral_red));
        styleButtonEnabled(allowCallsButton);

        connectButton.setText(R.string.connect_button_text);
        connectButton.setBackgroundColor(getColor(R.color.mint_green));
        styleButtonEnabled(connectButton);

        styleLoopbackSwitchDisabled();
        styleButtonDisabled(openSessionButton);
        styleLayoutChildrenDisabled(activeCallToolBar);
    }

    private void styleConnected() {
        if (loopbackModeEnabled) {
            allowCallsButton.setText(R.string.allow_calls_button_text);
            allowCallsButton.setBackgroundColor(getColor(R.color.mint_green));
            styleButtonDisabled(allowCallsButton);

            connectButton.setText(R.string.connect_button_text);
            connectButton.setBackgroundColor(getColor(R.color.mint_green));
            styleButtonDisabled(connectButton);
            styleLoopbackSwitchEnabled();

        } else {
            allowCallsButton.setText(R.string.disable_calls_button_text);
            allowCallsButton.setBackgroundColor(getColor(R.color.coral_red));
            styleButtonEnabled(allowCallsButton);

            connectButton.setText(R.string.disconnect_button_text);
            connectButton.setBackgroundColor(getColor(R.color.coral_red));
            styleButtonEnabled(connectButton);
            styleLoopbackSwitchDisabled();
        }

        localHandshakePortLabel.setText(getString(R.string.reception_port_label,
            String.valueOf(handshakeReceptionSocket.getBoundSocket())));
        localRtpPortLabel.setText(getString(R.string.rtp_reception_port_label,
            String.valueOf(rtp.getLocalPort())));
        localRtcpPortLabel.setText(getString(R.string.rtcp_reception_port_label,
            String.valueOf(rtcp.getLocalPort())));
        remoteRtpPortLabel.setText(getString(R.string.rtp_reception_port_label,
            String.valueOf(remoteDeviceInfo.getRtpPort())));
        remoteRtcpPortLabel.setText(getString(R.string.rtcp_reception_port_label,
            String.valueOf(remoteDeviceInfo.getRtcpPort())));
        styleButtonEnabled(openSessionButton);
        styleLayoutChildrenDisabled(activeCallToolBar);
    }

    private void styleActiveCall() {
        if(loopbackModeEnabled) {
            styleLoopbackSwitchEnabled();

            allowCallsButton.setText(R.string.allow_calls_button_text);
            allowCallsButton.setBackgroundColor(getColor(R.color.mint_green));
            styleButtonDisabled(allowCallsButton);

            connectButton.setText(R.string.connect_button_text);
            connectButton.setBackgroundColor(getColor(R.color.mint_green));
            styleButtonDisabled(connectButton);
            styleLoopbackSwitchEnabled();

        } else  {
            styleLoopbackSwitchDisabled();

            allowCallsButton.setText(R.string.disable_calls_button_text);
            allowCallsButton.setBackgroundColor(getColor(R.color.coral_red));
            styleButtonDisabled(allowCallsButton);

            connectButton.setText(R.string.disconnect_button_text);
            connectButton.setBackgroundColor(getColor(R.color.coral_red));
            styleButtonDisabled(connectButton);
            styleLoopbackSwitchDisabled();
        }

        localHandshakePortLabel
            .setText(getString(R.string.reception_port_label, getString(R.string.connected)));
        localRtpPortLabel.setText(getString(R.string.rtp_reception_port_label,
            String.valueOf(rtp.getLocalPort())));
        localRtcpPortLabel.setText(getString(R.string.rtcp_reception_port_label,
            String.valueOf(rtcp.getLocalPort())));
        remoteRtpPortLabel.setText(getString(R.string.rtp_reception_port_label,
            String.valueOf(remoteDeviceInfo.getRtpPort())));
        remoteRtcpPortLabel.setText(getString(R.string.rtcp_reception_port_label,
            String.valueOf(remoteDeviceInfo.getRtcpPort())));

        styleButtonDisabled(openSessionButton);
        styleLayoutChildrenEnabled(activeCallToolBar);
    }

    private void styleButtonDisabled(Button button) {
        button.setEnabled(false);
        button.setClickable(false);
        button.setAlpha(DISABLED_ALPHA);
    }

    private void styleButtonEnabled(Button button) {
        button.setEnabled(true);
        button.setClickable(true);
        button.setAlpha(ENABLED_ALPHA);
    }

    private void styleLayoutChildrenDisabled(LinearLayout linearLayout) {
        linearLayout.setAlpha(DISABLED_ALPHA);
        for (int x = 0; x < linearLayout.getChildCount(); x++) {
            linearLayout.getChildAt(x).setAlpha(DISABLED_ALPHA);
            linearLayout.getChildAt(x).setEnabled(false);
            linearLayout.getChildAt(x).setClickable(false);
        }
    }

    private void styleLayoutChildrenEnabled(LinearLayout linearLayout) {
        linearLayout.setAlpha(ENABLED_ALPHA);
        for (int x = 0; x < linearLayout.getChildCount(); x++) {
            linearLayout.getChildAt(x).setAlpha(ENABLED_ALPHA);
            linearLayout.getChildAt(x).setEnabled(true);
            linearLayout.getChildAt(x).setClickable(true);
        }
    }

    private void updateUiViews() {
        localHandshakePortLabel = findViewById(R.id.localHandshakePortLabel);
        localRtpPortLabel = findViewById(R.id.localRtpPortLabel);
        localRtcpPortLabel = findViewById(R.id.localRtcpPortLabel);
        remoteHandshakePortLabel = findViewById(R.id.remoteHandshakePortLabel);
        remoteRtpPortLabel = findViewById(R.id.remoteRtpPortLabel);
        remoteRtcpPortLabel = findViewById(R.id.remoteRtcpPortLabel);
        allowCallsButton = findViewById(R.id.allowCallsButton);
        connectButton = findViewById(R.id.connectButton);
        openSessionButton = findViewById(R.id.openSessionButton);
        activeCallToolBar = findViewById(R.id.activeCallActionsLayout);
        loopbackSwitch = findViewById(R.id.loopbackModeSwitch);
    }

    private void styleLoopbackSwitchEnabled() {
        loopbackSwitch.setChecked(loopbackModeEnabled);
        loopbackSwitch.setEnabled(true);
        loopbackSwitch.setClickable(true);
        loopbackSwitch.setAlpha(ENABLED_ALPHA);
    }

    private void styleLoopbackSwitchDisabled() {
        loopbackSwitch.setChecked(loopbackModeEnabled);
        loopbackSwitch.setEnabled(false);
        loopbackSwitch.setClickable(false);
        loopbackSwitch.setAlpha(DISABLED_ALPHA);
    }
}
