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

import android.hardware.radio.ims.media.IImsMediaSession;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.support.annotation.VisibleForTesting;
import android.telephony.CallQuality;
import android.telephony.Rlog;
import android.telephony.ims.RtpHeaderExtension;
import android.telephony.imsmedia.AudioConfig;
import android.telephony.imsmedia.IImsAudioSession;
import android.telephony.imsmedia.IImsAudioSessionCallback;
import android.telephony.imsmedia.ImsMediaSession;
import android.telephony.imsmedia.MediaQualityThreshold;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.telephony.imsmedia.Utils.OpenSessionParams;

import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

/**
 * Audio session binder implementation which handles all audio session APIs
 * from the VOIP applications.
 */
public final class AudioSession extends IImsAudioSession.Stub implements IMediaSession {
    private static final String TAG = "AudioSession";

    public static final int CMD_OPEN_SESSION = 101;
    public static final int CMD_CLOSE_SESSION = 102;
    public static final int CMD_MODIFY_SESSION = 103;
    public static final int CMD_ADD_CONFIG = 104;
    public static final int CMD_DELETE_CONFIG = 105;
    public static final int CMD_CONFIRM_CONFIG = 106;
    public static final int CMD_SEND_DTMF = 107;
    public static final int CMD_SEND_RTP_HDR_EXTN = 108;
    public static final int CMD_SET_MEDIA_QUALITY_THRESHOLD = 109;
    public static final int CMD_START_DTMF = 110;
    public static final int CMD_STOP_DTMF = 111;

    public static final int EVENT_OPEN_SESSION_SUCCESS = 201;
    public static final int EVENT_OPEN_SESSION_FAILURE = 202;
    public static final int EVENT_MODIFY_SESSION_RESPONSE = 203;
    public static final int EVENT_ADD_CONFIG_RESPONSE = 204;
    public static final int EVENT_CONFIRM_CONFIG_RESPONSE = 205;
    public static final int EVENT_SESSION_CHANGED_IND = 206;
    public static final int EVENT_FIRST_MEDIA_PACKET_IND = 207;
    public static final int EVENT_RTP_HEADER_EXTENSION_IND = 208;
    public static final int EVENT_MEDIA_INACTIVITY_IND = 209;
    public static final int EVENT_PACKET_LOSS_IND = 210;
    public static final int EVENT_JITTER_IND = 211;
    public static final int EVENT_TRIGGER_ANBR_QUERY_IND = 212;
    public static final int EVENT_DTMF_RECEIVED_IND = 213;
    public static final int EVENT_CALL_QUALITY_CHANGE_IND = 214;

    private static final int DTMF_DEFAULT_DURATION = 140;

    private int mSessionId;
    private int mSessionState;
    private AudioOffloadService mOffloadService;
    private AudioOffloadListener mOffloadListener;
    private IImsAudioSessionCallback mCallback;
    private IImsMediaSession mHalSession;
    private AudioSessionHandler mHandler;
    private boolean mIsAudioOffload;
    private AudioService mAudioService;
    private AudioListener mAudioListener;
    private AudioLocalSession mLocalSession;

    AudioSession(final int sessionId, final IImsAudioSessionCallback callback) {
        mSessionId = sessionId;
        mSessionState = ImsMediaSession.SESSION_STATE_CLOSED;
        mCallback = callback;
        mHandler = new AudioSessionHandler(Looper.getMainLooper());
        if (isAudioOffload()) {
            Rlog.d(TAG, "Initialize offload service");
            mOffloadService = AudioOffloadService.getInstance();
            mOffloadListener = new AudioOffloadListener(mHandler);
        } else {
            Rlog.d(TAG, "Initialize local audio service");
            mAudioService = new AudioService();
            mAudioListener = new AudioListener(mHandler);
            mAudioService.setListener(mAudioListener);
            mAudioListener.setNativeObject(mAudioService.getNativeObject());
        }
    }

    @VisibleForTesting
    AudioSession(final int sessionId,
            final @NonNull IImsAudioSessionCallback callback,
            final @Nullable AudioService audioService,
            final @Nullable AudioLocalSession localSession,
            final @Nullable AudioOffloadService offloadService) {
        mSessionId = sessionId;
        mSessionState = ImsMediaSession.SESSION_STATE_CLOSED;
        mCallback = callback;
        mHandler = new AudioSessionHandler(Looper.getMainLooper());
        mAudioService = audioService;
        mLocalSession = localSession;
        mAudioListener = new AudioListener(mHandler);
        mOffloadService = offloadService;
        mOffloadListener = new AudioOffloadListener(mHandler);
    }

    @VisibleForTesting
    void setAudioOffload(boolean isOffload) {
        mIsAudioOffload = isOffload;
    }

    @VisibleForTesting
    AudioSessionHandler getAudioSessionHandler() {
        return mHandler;
    }

    @VisibleForTesting
    AudioListener getAudioListener() {
        return mAudioListener;
    }

    AudioOffloadListener getOffloadListener() {
        return mOffloadListener;
    }

    @Override
    public void openSession(OpenSessionParams sessionParams) {
        Utils.sendMessage(mHandler, CMD_OPEN_SESSION, sessionParams);
    }

    @Override
    public void closeSession() {
        Utils.sendMessage(mHandler, CMD_CLOSE_SESSION);
    }

    @Override
    public int getSessionId() {
        return mSessionId;
    }

    @Override
    public int getSessionState() {
        return mSessionState;
    }

    @Override
    public void modifySession(AudioConfig config) {
        Rlog.d(TAG, "modifySession: " + config);
        Utils.sendMessage(mHandler, CMD_MODIFY_SESSION, config);
    }

    @Override
    public void addConfig(AudioConfig config) {
        Rlog.d(TAG, "addConfig: " + config);
        Utils.sendMessage(mHandler, CMD_ADD_CONFIG, config);
    }

    @Override
    public void deleteConfig(AudioConfig config) {
        Rlog.d(TAG, "deleteConfig: " + config);
        Utils.sendMessage(mHandler, CMD_DELETE_CONFIG, config);
    }

    @Override
    public void confirmConfig(AudioConfig config) {
        Rlog.d(TAG, "confirmConfig: " + config);
        Utils.sendMessage(mHandler, CMD_CONFIRM_CONFIG, config);
    }

    @Override
    public void sendDtmf(char digit, int duration) {
        Rlog.d(TAG, "sendDtmf: digit=" + digit + ",duration=" + duration);
        Utils.sendMessage(mHandler, CMD_SEND_DTMF, duration, Utils.UNUSED, digit);
    }

    @Override
    public void startDtmf(char digit) {
        Rlog.d(TAG, "startDtmf: digit=" + digit);
        Utils.sendMessage(mHandler, CMD_START_DTMF, digit);
    }

    @Override
    public void stopDtmf() {
        Rlog.d(TAG, "stopDtmf");
        Utils.sendMessage(mHandler, CMD_STOP_DTMF);
    }
    @Override
    public void sendHeaderExtension(List<RtpHeaderExtension> extensions) {
        Rlog.d(TAG, "sendHeaderExtension");
        Utils.sendMessage(mHandler, CMD_SEND_RTP_HDR_EXTN, extensions);
    }

    @Override
    public void setMediaQualityThreshold(MediaQualityThreshold threshold) {
        Rlog.d(TAG, "setMediaQualityThreshold: " + threshold);
        Utils.sendMessage(mHandler, CMD_SET_MEDIA_QUALITY_THRESHOLD, threshold);
    }

    @Override
    public void onOpenSessionSuccess(Object session) {
        Rlog.d(TAG, "onOpenSessionSuccess");
        Utils.sendMessage(mHandler, EVENT_OPEN_SESSION_SUCCESS, session);
    }

    @Override
    public void onOpenSessionFailure(int error) {
        Rlog.d(TAG, "onOpenSessionFailure: error=" + error);
        Utils.sendMessage(mHandler, EVENT_OPEN_SESSION_FAILURE, error);
    }

    private boolean isAudioOffload() {
        return mIsAudioOffload;
    }

    /**
     * Audio session message mHandler
     */
    class AudioSessionHandler extends Handler {
        AudioSessionHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage (Message msg) {
            Rlog.d(TAG, "handleMessage: " + msg.what);
            switch(msg.what) {
                case CMD_OPEN_SESSION:
                    handleOpenSession((OpenSessionParams)msg.obj);
                    break;
                case CMD_CLOSE_SESSION:
                    handleCloseSession();
                    break;
                case CMD_MODIFY_SESSION:
                    handleModifySession((AudioConfig)msg.obj);
                    break;
                case CMD_ADD_CONFIG:
                    handleAddConfig((AudioConfig)msg.obj);
                    break;
                case CMD_DELETE_CONFIG:
                    handleDeleteConfig((AudioConfig)msg.obj);
                    break;
                case CMD_CONFIRM_CONFIG:
                    handleConfirmConfig((AudioConfig)msg.obj);
                    break;
                case CMD_SEND_DTMF:
                    handleSendDtmf((char) msg.obj, msg.arg1);
                    break;
                case CMD_START_DTMF:
                    handleStartDtmf((char) msg.obj);
                    break;
                case CMD_STOP_DTMF:
                    handleStopDtmf();
                    break;
                case CMD_SEND_RTP_HDR_EXTN:
                    handleSendRtpHeaderExtension((List<RtpHeaderExtension>)msg.obj);
                    break;
                case CMD_SET_MEDIA_QUALITY_THRESHOLD:
                    handleSetMediaQualityThreshold((MediaQualityThreshold)msg.obj);
                    break;
                case EVENT_OPEN_SESSION_SUCCESS:
                    handleOpenSuccess(msg.obj);
                    break;
                case EVENT_OPEN_SESSION_FAILURE:
                    handleOpenFailure((int)msg.obj);
                    break;
                case EVENT_MODIFY_SESSION_RESPONSE:
                    handleModifySessionRespose((AudioConfig)msg.obj, msg.arg1);
                    break;
                case EVENT_ADD_CONFIG_RESPONSE:
                    handleAddConfigResponse((AudioConfig)msg.obj, msg.arg1);
                    break;
                case EVENT_CONFIRM_CONFIG_RESPONSE:
                    handleConfirmConfigResponse((AudioConfig)msg.obj, msg.arg1);
                    break;
                case EVENT_SESSION_CHANGED_IND:
                    handleSessionChangedInd((int)msg.obj);
                    break;
                case EVENT_FIRST_MEDIA_PACKET_IND:
                    handleFirstMediaPacketInd((AudioConfig)msg.obj);
                    break;
                case EVENT_RTP_HEADER_EXTENSION_IND:
                    handleRtpHeaderExtensionInd((List<RtpHeaderExtension>)msg.obj);
                    break;
                case EVENT_MEDIA_INACTIVITY_IND:
                    notifyMediaInactivityInd(msg.arg1);
                    break;
                case EVENT_PACKET_LOSS_IND:
                    notifyPacketLossInd((int)msg.obj);
                    break;
                case EVENT_JITTER_IND:
                    notifyJitterInd((int)msg.obj);
                    break;
                case EVENT_TRIGGER_ANBR_QUERY_IND:
                    handleTriggerAnbrQuery((AudioConfig) msg.obj);
                    break;
                case EVENT_DTMF_RECEIVED_IND:
                    handleDtmfReceived((char) msg.obj);
                    break;
                case EVENT_CALL_QUALITY_CHANGE_IND:
                    handleCallQualityChangeInd((CallQuality) msg.obj);
                    break;
                default:
            }
        }
    }

    private void handleOpenSession(OpenSessionParams sessionParams) {
        if (isAudioOffload()) {
            mOffloadService.openSession(mSessionId, sessionParams);
        } else {
            mAudioListener.setMediaCallback(sessionParams.getCallback());
            mAudioService.openSession(mSessionId, sessionParams);
        }
    }

    private void handleCloseSession() {
        Rlog.d(TAG, "handleCloseSession");
        if (isAudioOffload()) {
            mOffloadService.closeSession(mSessionId);
        } else {
            Rlog.d(TAG, "handleCloseSession else");
            mAudioService.closeSession(mSessionId);
        }
    }

    private void handleModifySession(AudioConfig config) {
        if (isAudioOffload()) {
            try {
                mHalSession.modifySession(Utils.convertToRtpConfig(config));
            } catch (RemoteException e) {
                Rlog.e(TAG, "modifySession : " + e);
            }
        } else {
            mLocalSession.modifySession(config);
        }
    }

    private void handleAddConfig(AudioConfig config) {
        if (isAudioOffload()) {
            try {
                mHalSession.addConfig(Utils.convertToRtpConfig(config));
            } catch (RemoteException e) {
                Rlog.e(TAG, "addConfig : " + e);
            }
        } else {
            mLocalSession.addConfig(config);
        }
    }

    private void handleDeleteConfig(AudioConfig config) {
        if (isAudioOffload()) {
            try {
                mHalSession.deleteConfig(Utils.convertToRtpConfig(config));
            } catch (RemoteException e) {
                Rlog.e(TAG, "deleteConfig : " + e);
            }
        } else {
            mLocalSession.deleteConfig(config);
        }
    }

    private void handleConfirmConfig(AudioConfig config) {
        if (isAudioOffload()) {
            try {
                mHalSession.confirmConfig(Utils.convertToRtpConfig(config));
            } catch (RemoteException e) {
                Rlog.e(TAG, "confirmConfig : " + e);
            }
        } else {
            mLocalSession.confirmConfig(config);
        }
    }

    private void handleSendDtmf(char digit, int duration) {
        if (isAudioOffload()) {
            try {
                mHalSession.sendDtmf(digit, duration);
            } catch (RemoteException e) {
                Rlog.e(TAG, "sendDtmf : " + e);
            }
        } else {
            mLocalSession.sendDtmf(digit, duration);
        }
    }

    private void handleStartDtmf(char digit) {
        if (isAudioOffload()) {
            try {
                mHalSession.startDtmf(digit);
            } catch (RemoteException e) {
                Rlog.e(TAG, "startDtmf : " + e);
            }
        } else {
            mLocalSession.sendDtmf(digit, DTMF_DEFAULT_DURATION);
        }
    }

    private void handleStopDtmf() {
        if (isAudioOffload()) {
            try {
                mHalSession.stopDtmf();
            } catch (RemoteException e) {
                Rlog.e(TAG, "stopDtmf : " + e);
            }
        }
    }

    private void handleSendRtpHeaderExtension(List<RtpHeaderExtension> extensions) {
        if (isAudioOffload()) {
            try {
                List<android.hardware.radio.ims.media.RtpHeaderExtension>
                        halExtensions = new ArrayList<>();
                halExtensions = extensions.stream().map(Utils::convertRtpHeaderExtension)
                        .collect(Collectors.toList());
                mHalSession.sendHeaderExtension(halExtensions);
            } catch (RemoteException e) {
                Rlog.e(TAG, "sendHeaderExtension : " + e);
            }
        } else {
            mLocalSession.sendHeaderExtension(extensions);
        }
    }

    private void handleSetMediaQualityThreshold(MediaQualityThreshold threshold) {
        if (isAudioOffload()) {
            try {
                mHalSession.setMediaQualityThreshold(Utils.convertMediaQualityThreshold(threshold));
            } catch (RemoteException e) {
                Rlog.e(TAG, "setMediaQualityThreshold: " + e);
            }
        } else {
            mLocalSession.setMediaQualityThreshold(threshold);
        }
    }

    private void handleOpenSuccess(Object session) {
       if (session instanceof IImsMediaSession) {
            try {
                ((IImsMediaSession)session).setListener(mOffloadListener);
            } catch (RemoteException e) {
                Rlog.e(TAG, "Failed to notify openSuccess: " + e);
            }
            mHalSession = (IImsMediaSession) session;
        } else {
            mLocalSession = (AudioLocalSession)session;
        }

        try {
            mCallback.onOpenSessionSuccess(this);
        } catch (RemoteException e) {
            Rlog.e(TAG, "Failed to notify openSuccess: " + e);
        }
    }

    private void handleOpenFailure(int error) {
        try {
            mCallback.onOpenSessionFailure(error);
        }  catch (RemoteException e) {
            Rlog.e(TAG, "Failed to notify openFailure: " + e);
        }
    }

    private void handleModifySessionRespose(AudioConfig config, int error) {
        try {
            mCallback.onModifySessionResponse(config, error);
        }  catch (RemoteException e) {
            Rlog.e(TAG, "Failed to notify modifySessionResponse: " + e);
        }
    }

    private void handleAddConfigResponse(AudioConfig config, int error) {
        try {
            mCallback.onAddConfigResponse(config, error);
        }  catch (RemoteException e) {
            Rlog.e(TAG, "Failed to notify onAddConfigResponse: " + e);
        }
    }

    private void handleConfirmConfigResponse(AudioConfig config, int error) {
        try {
            mCallback.onConfirmConfigResponse(config, error);
        }  catch (RemoteException e) {
            Rlog.e(TAG, "Failed to notify onConfirmConfigResponse: " + e);
        }
    }

    private void handleSessionChangedInd(int state) {
        try {
            mSessionState = state;
            mCallback.onSessionChanged(state);
        }  catch (RemoteException e) {
            Rlog.e(TAG, "Failed to notify onSessionChanged: " + e);
        }
    }

    private void handleFirstMediaPacketInd(AudioConfig config) {
        try {
            mCallback.onFirstMediaPacketReceived(config);
        }  catch (RemoteException e) {
            Rlog.e(TAG, "Failed to notify first media packet received indication: " + e);
        }
    }

    private void handleRtpHeaderExtensionInd(List<RtpHeaderExtension> extensions) {
        try {
            mCallback.onHeaderExtensionReceived(extensions);
        }  catch (RemoteException e) {
            Rlog.e(TAG, "Failed to notify RTP header extension: " + e);
        }
    }

    private void notifyMediaInactivityInd(int packetType) {
        try {
            mCallback.notifyMediaInactivity(packetType);
        }  catch (RemoteException e) {
            Rlog.e(TAG, "Failed to notify media timeout: " + e);
        }

    }

    private void notifyPacketLossInd(int percentage) {
        try {
            mCallback.notifyPacketLoss(percentage);
        }  catch (RemoteException e) {
            Rlog.e(TAG, "Failed to notify packet loss: " + e);
        }
    }

    private void notifyJitterInd(int jitter) {
        try {
            mCallback.notifyJitter(jitter);
        }  catch (RemoteException e) {
            Rlog.e(TAG, "Failed to notify jitter: " + e);
        }
    }

    private void handleTriggerAnbrQuery(AudioConfig config) {
        try {
            mCallback.triggerAnbrQuery(config);
        }  catch (RemoteException e) {
            Rlog.e(TAG, "Failed to trigger ANBR query: " + e);
        }
    }

    private void handleDtmfReceived(char dtmfDigit) {
        try {
            mCallback.onDtmfReceived(dtmfDigit);
        }  catch (RemoteException e) {
            Rlog.e(TAG, "Failed to Dtmf received: " + e);
        }
    }

    private void handleCallQualityChangeInd(CallQuality callQuality) {
        try {
            mCallback.onCallQualityChanged(callQuality);
        }  catch (RemoteException e) {
            Rlog.e(TAG, "Failed to notify call quality changed indication: " + e);
        }
    }
}
