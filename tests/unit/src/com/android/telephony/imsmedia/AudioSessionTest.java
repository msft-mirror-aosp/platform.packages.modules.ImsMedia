/**
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.telephony.imsmedia;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.telephony.imsmedia.AudioConfig;
import android.telephony.imsmedia.IImsAudioSessionCallback;
import android.telephony.imsmedia.ImsMediaSession;
import android.telephony.imsmedia.MediaQualityThreshold;
import android.telephony.ims.RtpHeaderExtension;
import android.testing.TestableLooper;

import com.android.telephony.imsmedia.AudioSession;
import com.android.telephony.imsmedia.AudioService;
import com.android.telephony.imsmedia.Utils;
import com.android.telephony.imsmedia.Utils.OpenSessionParams;

import java.util.ArrayList;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(JUnit4.class)
public class AudioSessionTest {
    private static final int SESSION_ID = 1;
    private static final int DTMF_VOL = 50;
    private static final int DTMF_DURATION = 120;
    private static final int UNUSED = -1;
    private static final int FAILURE = ImsMediaSession.RESULT_FAILURE;
    private static final int SUCCESS = ImsMediaSession.RESULT_SUCCESS;
    private static final int RTP = ImsMediaSession.PACKET_TYPE_RTP;
    private static final int RTCP = ImsMediaSession.PACKET_TYPE_RTCP;
    private static final int INACTIVITY_TIMEOUT = 20;
    private static final int PACKET_LOSS = 15;
    private static final int JITTER = 200;
    private static final char DTMF_DIGIT = '7';
    private AudioSession audioSession;
    private AudioSession.AudioSessionHandler handler;
    @Mock
    private AudioService audioService;
    @Mock
    private IImsAudioSessionCallback callback;
    private TestableLooper looper;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        audioSession = new AudioSession(SESSION_ID, callback, audioService);
        handler = audioSession.getAudioSessionHandler();
        try {
            looper = new TestableLooper(handler.getLooper());
        } catch (Exception e) {
            fail("Unable to create TestableLooper");
        }
    }

    @After
    public void tearDown() throws Exception {
        if (looper != null) {
            looper.destroy();
            looper = null;
        }
    }

    @Test
    public void testOpenSession() {
        OpenSessionParams params = new OpenSessionParams();
        audioSession.openSession(params);
        processAllMessages();
        verify(audioService, times(1)).openSession(eq(SESSION_ID), eq(params));
    }

    @Test
    public void testCloseSession() {
        audioSession.closeSession();
        processAllMessages();
        verify(audioService, times(1)).closeSession(eq(SESSION_ID));
    }

    @Test
    public void testGetSessionState() {
        assertThat(audioSession.getSessionId()).isEqualTo(SESSION_ID);
        assertThat(audioSession.getSessionState()).isEqualTo(ImsMediaSession.SESSION_STATE_CLOSED);
        Utils.sendMessage(handler, AudioSession.EVENT_SESSION_CHANGED_IND,
                ImsMediaSession.SESSION_STATE_OPEN);
        processAllMessages();
        assertThat(audioSession.getSessionState()).isEqualTo(ImsMediaSession.SESSION_STATE_OPEN);
    }

    @Test
    public void testModifySession() {
        // Modify Session Request
        AudioConfig config = AudioConfigTest.createAudioConfig();
        audioSession.modifySession(config);
        processAllMessages();
        verify(audioService, times(1)).modifySession(eq(config));

        // Modify Session Response - SUCCESS
        Utils.sendMessage(handler, AudioSession.EVENT_MODIFY_SESSION_RESPONSE,
                SUCCESS, UNUSED, config);
        processAllMessages();
        try {
            verify(callback, times(1)).onModifySessionResponse(eq(config), eq(SUCCESS));
        }  catch(RemoteException e) {
            fail("Failed to notify modifySessionResponse: " + e);
        }

        // Modify Session Response - FAILURE
        Utils.sendMessage(handler, AudioSession.EVENT_MODIFY_SESSION_RESPONSE,
                FAILURE, UNUSED, config);
        processAllMessages();
        try {
            verify(callback, times(1)).onModifySessionResponse(eq(config), eq(FAILURE));
        }  catch(RemoteException e) {
            fail("Failed to notify modifySessionResponse: " + e);
        }
    }

    @Test
    public void testAddConfig() {
        // Add Config Request
        AudioConfig config = AudioConfigTest.createAudioConfig();
        audioSession.addConfig(config);
        processAllMessages();
        verify(audioService, times(1)).addConfig(eq(config));

        // Add Config Response - SUCCESS
        Utils.sendMessage(handler, AudioSession.EVENT_ADD_CONFIG_RESPONSE, SUCCESS, UNUSED, config);
        processAllMessages();
        try {
            verify(callback, times(1)).onAddConfigResponse(eq(config), eq(SUCCESS));
        }  catch(RemoteException e) {
            fail("Failed to notify addConfigResponse: " + e);
        }

        // Add Config Response - FAILURE
        Utils.sendMessage(handler, AudioSession.EVENT_ADD_CONFIG_RESPONSE, FAILURE, UNUSED, config);
        processAllMessages();
        try {
            verify(callback, times(1)).onAddConfigResponse(eq(config), eq(FAILURE));
        }  catch(RemoteException e) {
            fail("Failed to notify addConfigResponse: " + e);
        }
    }

    @Test
    public void testDeleteConfig() {
        // Delete Config Request
        AudioConfig config = AudioConfigTest.createAudioConfig();
        audioSession.deleteConfig(config);
        processAllMessages();
        verify(audioService, times(1)).deleteConfig(eq(config));
    }

    @Test
    public void testConfirmConfig() {
        // Confirm Config Request
        AudioConfig config = AudioConfigTest.createAudioConfig();
        audioSession.confirmConfig(config);
        processAllMessages();
        verify(audioService, times(1)).confirmConfig(eq(config));

        // Confirm Config Response - SUCCESS
        Utils.sendMessage(handler, AudioSession.EVENT_CONFIRM_CONFIG_RESPONSE,
                SUCCESS, UNUSED, config);
        processAllMessages();
        try {
            verify(callback, times(1)).onConfirmConfigResponse(eq(config), eq(SUCCESS));
        }  catch(RemoteException e) {
            fail("Failed to notify confirmConfigResponse: " + e);
        }

        // Confirm Config Response - FAILURE
        Utils.sendMessage(handler, AudioSession.EVENT_CONFIRM_CONFIG_RESPONSE,
                FAILURE, UNUSED, config);
        processAllMessages();
        try {
            verify(callback, times(1)).onConfirmConfigResponse(eq(config), eq(FAILURE));
        }  catch(RemoteException e) {
            fail("Failed to notify confirmConfigResponse: " + e);
        }
    }

    @Test
    public void testStartDtmf() {
        audioSession.startDtmf(DTMF_DIGIT, DTMF_VOL, DTMF_DURATION);
        processAllMessages();
        verify(audioService, times(1)).startDtmf(eq(DTMF_DIGIT), eq(DTMF_VOL), eq(DTMF_DURATION));
    }

    @Test
    public void testStopDtmf() {
        audioSession.stopDtmf();
        processAllMessages();
        verify(audioService, times(1)).stopDtmf();
    }

    @Test
    public void testSetMediaQualityThreshold() {
        // Set Media Quality Threshold
        MediaQualityThreshold threshold = MediaQualityThresholdTest.createMediaQualityThreshold();
        audioSession.setMediaQualityThreshold(threshold);
        processAllMessages();
        verify(audioService, times(1)).setMediaQualityThreshold(eq(threshold));
    }

    @Test
    public void testMediaInactivityInd() {
        // Receive Inactivity - RTP
        Utils.sendMessage(handler, AudioSession.EVENT_MEDIA_INACTIVITY_IND,
                RTP, INACTIVITY_TIMEOUT);
        processAllMessages();
        try {
            verify(callback, times(1)).notifyMediaInactivity(eq(RTP), eq(INACTIVITY_TIMEOUT));
        }  catch(RemoteException e) {
            fail("Failed to notify notifyMediaInactivity: " + e);
        }

        // Receive Inactivity - RTCP
        Utils.sendMessage(handler, AudioSession.EVENT_MEDIA_INACTIVITY_IND,
                RTCP, INACTIVITY_TIMEOUT);
        processAllMessages();
        try {
            verify(callback, times(1)).notifyMediaInactivity(eq(RTCP), eq(INACTIVITY_TIMEOUT));
        }  catch(RemoteException e) {
            fail("Failed to notify notifyMediaInactivity: " + e);
        }
    }

    @Test
    public void testPacketLossInd() {
        // Receive Packet Loss
        Utils.sendMessage(handler, AudioSession.EVENT_PACKET_LOSS_IND, PACKET_LOSS);
        processAllMessages();
        try {
            verify(callback, times(1)).notifyPacketLoss(eq(PACKET_LOSS));
        }  catch(RemoteException e) {
            fail("Failed to notify notifyPacketLoss: " + e);
        }
    }

    @Test
    public void testJitterInd() {
        // Receive Jitter Indication
        Utils.sendMessage(handler, AudioSession.EVENT_JITTER_IND, JITTER);
        processAllMessages();
        try {
            verify(callback, times(1)).notifyJitter(eq(JITTER));
        }  catch(RemoteException e) {
            fail("Failed to notify notifyJitter: " + e);
        }
    }

    @Test
    public void testFirstMediaPacketReceivedInd() {
        // Receive First MediaPacket Received Indication
        AudioConfig config = AudioConfigTest.createAudioConfig();
        Utils.sendMessage(handler, AudioSession.EVENT_FIRST_MEDIA_PACKET_IND, config);
        processAllMessages();
        try {
            verify(callback, times(1)).onFirstMediaPacketReceived(eq(config));
        }  catch(RemoteException e) {
            fail("Failed to notify onFirstMediaPacketReceived: " + e);
        }
    }

    @Test
    public void testHeaderExtension() {
        // Send RtpHeaderExtension
        ArrayList extensions = new ArrayList<RtpHeaderExtension>();
        audioSession.sendHeaderExtension(extensions);
        processAllMessages();
        verify(audioService, times(1)).sendHeaderExtension(eq(extensions));

        // Receive RtpHeaderExtension
        Utils.sendMessage(handler, AudioSession.EVENT_RTP_HEADER_EXTENSION_IND, extensions);
        processAllMessages();
        try {
            verify(callback, times(1)).onHeaderExtensionReceived(eq(extensions));
        }  catch(RemoteException e) {
            fail("Failed to notify onHeaderExtensionReceived: " + e);
        }
    }

    private void processAllMessages() {
        while (!looper.getLooper().getQueue().isIdle()) {
            looper.processAllMessages();
        }
    }
}
