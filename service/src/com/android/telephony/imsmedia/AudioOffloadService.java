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

import android.os.IBinder;
import android.os.RemoteException;
import android.telephony.imsmedia.RtpConfig;
import android.telephony.Rlog;
import android.hardware.radio.ims.media.IImsMedia;
import android.hardware.radio.ims.media.IImsMediaSession;
import android.hardware.radio.ims.media.IImsMediaListener;
import android.hardware.radio.ims.media.SpeechCodec;

import com.android.telephony.imsmedia.Utils.OpenSessionParams;

/**
 * This connects to IImsMedia HAL and invokes all the HAL APIs
 */
final class AudioOffloadService {

    private static final String LOG_TAG = "AudioOffloadService";

    private IImsMedia mImsMedia;
    private ImsMediaController.OpenSessionCallback mMediaControllerCallback;
    private static AudioOffloadService sInstance;

    private AudioOffloadService() {
        // TODO: Register for IImsMedia HAL service notification
    }

    public static AudioOffloadService getInstance() {
        if (sInstance == null) {
            sInstance = new AudioOffloadService();
        }

        return sInstance;
    }

    // Initializes the HAL
    private synchronized void initMediaHal() {
        Rlog.d(LOG_TAG, "initMediaHal");

        try {
          // TODO
          // Connect to the HAL
          // Set response functions
          // Set Death receipeint
        } catch (Exception e) {
            Rlog.e(LOG_TAG, "initMediaHal: Exception: " + e);
        }
    }

    public IImsMedia getIImsMedia() {

        if (mImsMedia != null) {
            return mImsMedia;
        }

        // Reconnect to ImsMedia HAL
        initMediaHal();

        return mImsMedia;
    }

    public void openSession(int sessionId, OpenSessionParams sessionParams) {
        mMediaControllerCallback = sessionParams.getCallback();
        // TODO
        // create localEndPoint
        // covert telephony AudioConfig to AIDL RtpConfig
        // getIImsMedia().openSession(sessionId, localEndPoint, config);
    }

    public void closeSession(int sessionId) {
        try {
            getIImsMedia().closeSession(sessionId);
        } catch (RemoteException e) {
            Rlog.e(LOG_TAG, "closeSession: " + e);
        }
    }

    private class ImsMediaListener extends IImsMediaListener.Stub {
        @Override
        public void onOpenSessionSuccess(int sessionId, IImsMediaSession session) {
            mMediaControllerCallback.onOpenSessionSuccess(sessionId, session);
        }

        @Override
        public void onOpenSessionFailure(int sessionId, int error) {
            mMediaControllerCallback.onOpenSessionFailure(sessionId, error);
        }

        @Override
        public void onMediaStackStateChanged(int state) {
           // TODO: Remove this API in HAL
        }

        @Override
        public void onBringupResponse(int state, SpeechCodec[] codecs) {
           // TODO: Remove this API in HAL
        }
    }
}
