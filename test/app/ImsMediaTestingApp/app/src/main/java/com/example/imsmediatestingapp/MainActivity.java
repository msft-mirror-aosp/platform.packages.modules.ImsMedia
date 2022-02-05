package com.example.imsmediatestingapp;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
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
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    enum AudioCodec {
        AMR_NB,
        AMR_WB,
        EVS,
        PCMA,
        PCMU
    }

    enum EvsBandwidth {
        NONE,
        NARROW_BAND,
        WIDE_BAND,
        SUPER_WIDE_BAND,
        FULL_BAND,
    }

    enum VideoCodec {
        H264,
        HEVC
    }

    private final List<AudioCodec> selectedAudioCodecs = new ArrayList<>();
    private final List<VideoCodec> selectedVideoCodecs = new ArrayList<>();
    public static final String PREF_NAME = "preferences";
    private final String HANDSHAKE_PORT_PREF = "HANDSHAKE_PORT_OPEN";
    SharedPreferences prefs;
    SharedPreferences.Editor editor;
    DatagramReceiver receiver;
    DatagramReceiver rtpSocket;
    DatagramReceiver rtcpSocket;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        prefs = getSharedPreferences(PREF_NAME, MODE_PRIVATE);
        editor = prefs.edit();
        editor.putBoolean(HANDSHAKE_PORT_PREF, false);
        editor.apply();

        askForPermissions();
        //setup
        setupInfo();
        receiver = new DatagramReceiver(prefs);
        rtpSocket = new DatagramReceiver(prefs);
        rtcpSocket = new DatagramReceiver(prefs);
    }

    @Override
    protected void onStart() {
        super.onStart();
        askForPermissions();
        setupInfo();
    }

    @Override
    protected void onResume() {
        super.onResume();
        setupInfo();
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
                setupInfo();
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

    @SuppressLint("SetTextI18n")
    public void onClickClientButton(View v) {
        Button btn = findViewById(R.id.clientButton);
        TextView portText = findViewById(R.id.receptionPort);
        TextView rtpPortText = findViewById(R.id.rtpReceptionPort);
        TextView rtcpPortText = findViewById(R.id.rtcpReceptionPort);

        if (prefs.getBoolean(HANDSHAKE_PORT_PREF, false)) {
            //stopping the server
            Log.d("", "Closing ports");
            btn.setText("OPEN PORTS");
            btn.setBackgroundColor(getResources().getColor(R.color.mint_green));

            //stop the server
            rtpSocket.close();
            rtcpSocket.close();
            receiver.close();

            rtpPortText.setText(getString(R.string.port_closed));
            rtcpPortText.setText(getString(R.string.port_closed));
            portText.setText(getString(R.string.port_closed));

            Toast.makeText(getApplicationContext(), "Closing ports", Toast.LENGTH_SHORT).show();

        } else {
            //starting the server
            Log.d("", "Opening ports");
            btn.setText("CLOSE PORTS");
            btn.setBackgroundColor(getResources().getColor(R.color.coral_red));

            //start the server
            new Thread(rtpSocket).start();
            new Thread(rtcpSocket).start();
            new Thread(receiver).start();

            // Wait for the port to open before attempting to get the port number.
            while (!prefs.getBoolean(HANDSHAKE_PORT_PREF, false)) {

            }
            rtpPortText.setText(getString(R.string.rtp_reception_port_placeholder,
                String.valueOf(rtpSocket.getBoundSocket())));
            rtcpPortText.setText(getString(R.string.rtcp_reception_port_placeholder,
                String.valueOf(rtcpSocket.getBoundSocket())));
            portText.setText(getString(R.string.reception_port_placeholder,
                String.valueOf(receiver.getBoundSocket())));

            Toast.makeText(getApplicationContext(), "Opening ports", Toast.LENGTH_SHORT).show();

        }
    }

    public String getOtherDeviceIp() {
        EditText ipAddress = findViewById(R.id.other_device_ip);
        return ipAddress.getText().toString().trim();
    }

    public String getOtherDevicePort() {
        EditText portNum = findViewById(R.id.port_number);
        return portNum.getText().toString().trim();
    }

    public void initiateHandshake(View v) {
        try {
            int port = Integer.parseInt(getOtherDevicePort());
            String ip = getOtherDeviceIp();
            getAudioCodecSelections();
            getVideoCodecSelections();

            int handshakePort = receiver.getBoundSocket();
            InetAddress ipAddress = InetAddress.getByName("192.168.1.75");

            DeviceInfo myDeviceInfo = new DeviceInfo(handshakePort, ipAddress, selectedAudioCodecs,
                selectedVideoCodecs);
            byte[] myDeviceInfoBytes = myDeviceInfo.getBytes();
            int length = myDeviceInfoBytes.length;

            DatagramSender sender = new DatagramSender(ip, port);
            sender.setData(myDeviceInfoBytes, length);
            new Thread(sender).start();
        } catch (Exception e) {
            Log.e("", e.toString());
        }

    }

    public void saveSettings(View v) {
        int port = Integer.parseInt(getOtherDevicePort());
        String ip = getOtherDeviceIp();

        editor.putInt("OTHER_HANDSHAKE_PORT", port);
        editor.putString("OTHER_IP_ADDRESS", ip);
        editor.apply();

        Toast.makeText(getApplicationContext(), "Settings Saved", Toast.LENGTH_SHORT).show();
    }

    public void setupSettingsPage() {
        EditText ipAddress = findViewById(R.id.other_device_ip);
        EditText portNumber = findViewById(R.id.port_number);
        String ip = prefs.getString("OTHER_IP_ADDRESS", "");
        String portNum = String.valueOf(prefs.getInt("OTHER_HANDSHAKE_PORT", 0));

        ipAddress.setText(ip);
        portNumber.setText(portNum);
    }

    private void setupInfo() {
        WifiInfo wifiInfo = retrieveNetworkConfig();
        setupIpLabel(wifiInfo);

        TextView otherIp = findViewById(R.id.otherIpAddress);
        String ip = prefs.getString("OTHER_IP_ADDRESS", "");
        otherIp.setText(String.format("IP: %s", ip));

        TextView otherPort = findViewById(R.id.otherHandshakePort);
        String portNum = String.valueOf(prefs.getInt("OTHER_HANDSHAKE_PORT", 0));
        otherPort.setText(String.format("Port: %s", portNum));

    }

    private WifiInfo retrieveNetworkConfig() {
        WifiManager wifiManager = (WifiManager) getApplication()
            .getSystemService(Context.WIFI_SERVICE);
        return wifiManager.getConnectionInfo();
    }

    private void setupIpLabel(WifiInfo wifiInfo) {
        TextView ipText = findViewById(R.id.ipAddress);

        String ip = Formatter.formatIpAddress(wifiInfo.getIpAddress());
        ipText.setText(getString(R.string.local_ip_placeholder, ip));
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
                            + "\n Precise location permissions are needed to access WiFi information.");
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
        EditText ipBox = findViewById(R.id.other_device_ip);
        return ipBox.getText().toString();
    }

    private String getPortNumber() {
        EditText portNumberBox = findViewById(R.id.port_number);
        return portNumberBox.getText().toString();
    }

}
