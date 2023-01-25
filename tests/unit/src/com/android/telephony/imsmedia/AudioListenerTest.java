/*
 * Copyright (C) 2023 The Android Open Source Project
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

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.os.Parcel;
import android.os.RemoteException;
import android.telephony.CallQuality;
import android.telephony.imsmedia.AudioConfig;
import android.telephony.imsmedia.IImsAudioSessionCallback;
import android.telephony.imsmedia.ImsMediaSession;
import android.testing.TestableLooper;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(JUnit4.class)
public class AudioListenerTest {
    private static final int SESSION_ID = 1;
    private static final char DTMF_DIGIT = '7';
    private AudioListener mAudioListener;
    @Mock
    private AudioService mAudioService;
    @Mock
    private AudioLocalSession mMockAudioLocalSession;
    @Mock
    private ImsMediaController.OpenSessionCallback mMockCallback;
    @Mock
    private IImsAudioSessionCallback mMockIImsAudioSessionCallback;
    private AudioConfig mAudioConfig;
    private TestableLooper mLooper;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        AudioSession audioSession = new AudioSession(SESSION_ID, mMockIImsAudioSessionCallback,
                mAudioService, mMockAudioLocalSession, null);
        AudioSession.AudioSessionHandler handler = audioSession.getAudioSessionHandler();
        mAudioListener = new AudioListener(handler);
        mAudioListener.setMediaCallback(mMockCallback);
        mAudioConfig = AudioConfigTest.createAudioConfig();
        try {
            mLooper = new TestableLooper(handler.getLooper());
        } catch (Exception e) {
            throw new AssertionError("Unable to create TestableLooper", e);
        }
    }

    @After
    public void tearDown() throws Exception {
        if (mLooper != null) {
            mLooper.destroy();
            mLooper = null;
        }
    }

    private Parcel createParcel(int event, int result, AudioConfig config) {
        Parcel parcel = Parcel.obtain();
        parcel.writeInt(event);
        parcel.writeInt(result);
        if (config != null) {
            config.writeToParcel(parcel, 0);
        }
        parcel.setDataPosition(0);
        return parcel;
    }

    @Test
    public void testOpenSessionSuccess() {
        mAudioListener.onMessage(createParcel(AudioSession.EVENT_OPEN_SESSION_SUCCESS, SESSION_ID,
                mAudioConfig));
        doNothing().when(mMockCallback).onOpenSessionSuccess(eq(SESSION_ID),
                eq(mMockAudioLocalSession));
        verify(mMockCallback,
                times(1)).onOpenSessionSuccess(eq(SESSION_ID), any(AudioLocalSession.class));
    }

    @Test
    public void testOpenSessionFailure() {
        Parcel parcel = Parcel.obtain();
        parcel.writeInt(AudioSession.EVENT_OPEN_SESSION_FAILURE);
        parcel.writeInt(SESSION_ID);
        parcel.writeInt(ImsMediaSession.RESULT_INVALID_PARAM);
        mAudioConfig.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);
        mAudioListener.onMessage(parcel);
        doNothing().when(mMockCallback).onOpenSessionFailure(eq(SESSION_ID),
                eq(ImsMediaSession.RESULT_INVALID_PARAM));
        verify(mMockCallback, times(1)).onOpenSessionFailure(eq(SESSION_ID),
                eq(ImsMediaSession.RESULT_INVALID_PARAM));
    }

    @Test
    public void testEventModifySessionResponse() throws RemoteException {
        mAudioListener.onMessage(createParcel(AudioSession.EVENT_MODIFY_SESSION_RESPONSE,
                ImsMediaSession.RESULT_NO_RESOURCES, mAudioConfig));
        processAllMessages();
        verify(mMockIImsAudioSessionCallback,
                times(1)).onModifySessionResponse(eq(mAudioConfig),
                eq(ImsMediaSession.RESULT_NO_RESOURCES));
    }

    @Test
    public void testEventAddConfigResponse() throws RemoteException {
        mAudioListener.onMessage(createParcel(AudioSession.EVENT_ADD_CONFIG_RESPONSE,
                ImsMediaSession.RESULT_NOT_READY, mAudioConfig));
        processAllMessages();
        verify(mMockIImsAudioSessionCallback,
                times(1)).onAddConfigResponse(eq(mAudioConfig),
                eq(ImsMediaSession.RESULT_NOT_READY));
    }

    @Test
    public void testEventConfirmConfigResponse() throws RemoteException {
        mAudioListener.onMessage(createParcel(AudioSession.EVENT_CONFIRM_CONFIG_RESPONSE,
                ImsMediaSession.RESULT_INVALID_PARAM, mAudioConfig));
        processAllMessages();
        verify(mMockIImsAudioSessionCallback,
                times(1)).onConfirmConfigResponse(eq(mAudioConfig),
                eq(ImsMediaSession.RESULT_INVALID_PARAM));
    }

    @Test
    public void testEventFirstMediaPacketInd() throws RemoteException {
        Parcel parcel = Parcel.obtain();
        parcel.writeInt(AudioSession.EVENT_FIRST_MEDIA_PACKET_IND);
        mAudioConfig.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);
        mAudioListener.onMessage(parcel);
        processAllMessages();
        verify(mMockIImsAudioSessionCallback,
                times(1)).onFirstMediaPacketReceived(eq(mAudioConfig));
    }

    @Test
    public void testEventRtpHeaderExtensionInd() throws RemoteException {
        mAudioListener.onMessage(createParcel(AudioSession.EVENT_RTP_HEADER_EXTENSION_IND,
                SESSION_ID, mAudioConfig));
        processAllMessages();
        verify(mMockIImsAudioSessionCallback,
                times(1)).onHeaderExtensionReceived(any());
    }

    @Test
    public void testEventMediaInactivityInd() throws RemoteException {
        mAudioListener.onMessage(createParcel(AudioSession.EVENT_MEDIA_INACTIVITY_IND,
                ImsMediaSession.RESULT_INVALID_PARAM, mAudioConfig));
        processAllMessages();
        verify(mMockIImsAudioSessionCallback,
                times(1)).notifyMediaInactivity(eq(ImsMediaSession.RESULT_INVALID_PARAM));
    }

    @Test
    public void testEventPacketLossInd() throws RemoteException {
        mAudioListener.onMessage(createParcel(AudioSession.EVENT_PACKET_LOSS_IND,
                ImsMediaSession.RESULT_NO_MEMORY, mAudioConfig));
        processAllMessages();
        verify(mMockIImsAudioSessionCallback,
                times(1)).notifyPacketLoss(eq(ImsMediaSession.RESULT_NO_MEMORY));
    }

    @Test
    public void testEventJitterInd() throws RemoteException {
        mAudioListener.onMessage(createParcel(AudioSession.EVENT_JITTER_IND,
                ImsMediaSession.RESULT_INVALID_PARAM, mAudioConfig));
        processAllMessages();
        verify(mMockIImsAudioSessionCallback,
                times(1)).notifyJitter(eq(ImsMediaSession.RESULT_INVALID_PARAM));
    }

    @Test
    public void testEventTriggerAnbrQueryInd() throws RemoteException {
        Parcel parcel = Parcel.obtain();
        parcel.writeInt(AudioSession.EVENT_TRIGGER_ANBR_QUERY_IND);
        mAudioConfig.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);
        mAudioListener.onMessage(parcel);
        processAllMessages();
        verify(mMockIImsAudioSessionCallback,
                times(1)).triggerAnbrQuery(eq(mAudioConfig));
    }

    @Test
    public void testEventDtmfReceivedInd() throws RemoteException {
        mAudioListener.onMessage(createParcel(AudioSession.EVENT_DTMF_RECEIVED_IND, DTMF_DIGIT,
                mAudioConfig));
        processAllMessages();
        verify(mMockIImsAudioSessionCallback,
                times(1)).onDtmfReceived(eq(DTMF_DIGIT));
    }

    @Test
    public void testEventCallQualityChangeInd() throws RemoteException {
        Parcel parcel = Parcel.obtain();
        parcel.writeInt(AudioSession.EVENT_CALL_QUALITY_CHANGE_IND);
        CallQuality callQuality = CallQualityTest.createCallQuality();
        callQuality.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);
        mAudioListener.onMessage(parcel);
        processAllMessages();
        verify(mMockIImsAudioSessionCallback,
                times(1)).onCallQualityChanged(eq(callQuality));
    }

    @Test
    public void testEventSessionClosed() throws RemoteException {
        mAudioListener.onMessage(createParcel(AudioSession.EVENT_SESSION_CLOSED, SESSION_ID,
                mAudioConfig));
        doNothing().when(mMockCallback).onSessionClosed(eq(SESSION_ID));
        verify(mMockCallback, times(1)).onSessionClosed(eq(SESSION_ID));
    }

    private void processAllMessages() {
        while (!mLooper.getLooper().getQueue().isIdle()) {
            mLooper.processAllMessages();
        }
    }
}
