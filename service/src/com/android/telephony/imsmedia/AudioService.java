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

import android.os.Handler;
import android.telephony.imsmedia.AudioConfig;
import android.telephony.imsmedia.MediaQualityThreshold;
import android.telephony.ims.RtpHeaderExtension;
import android.telephony.Rlog;

import com.android.telephony.imsmedia.Utils.OpenSessionParams;

import java.util.List;

/**
 * Audio RTP stack service
 */
public class AudioService {
    private static final String LOG_TAG = "AudioService";
    private Handler mHandler;
    private ImsMediaController.OpenSessionCallback mMediaControllerCallback;

    AudioService(Handler handler) {
        mHandler = handler;

        // TODO: Initialize audio RTP stack
    }

    public void openSession(int sessionId, OpenSessionParams sessionParams) {
        Rlog.d(LOG_TAG, "openSession");
        mMediaControllerCallback = sessionParams.getCallback();
        // TODO
        // create localEndPoint
        // covert telephony AudioConfig internal AudioConfig
        // Invoke internal openSession(sessionId, localEndPoint, config);
        //
        // Once openSession is success notify using ImsMediaController.OpenSessionCallback

        // Stub code
        // mMediaControllerCallback.onOpenSessionSuccess(sessionId, new Object());
    }

    public void closeSession(int sessionId) {
        Rlog.d(LOG_TAG, "closeSession");
    }

    public void modifySession(AudioConfig config) {
        Rlog.d(LOG_TAG, "modifySession");
    }

    public void addConfig(AudioConfig config) {
        Rlog.d(LOG_TAG, "addConfig");
    }

    public void deleteConfig(AudioConfig config) {
        Rlog.d(LOG_TAG, "deleteConfig");
    }

    public void confirmConfig(AudioConfig config) {
        Rlog.d(LOG_TAG, "confirmConfig");
    }

    public void startDtmf(char dtmfDigit, int volume, int duration) {
        Rlog.d(LOG_TAG, "startDtmf");
    }

    public void stopDtmf() {
        Rlog.d(LOG_TAG, "stopDtmf");
    }

    public void sendHeaderExtension(List<RtpHeaderExtension> extensions) {
        Rlog.d(LOG_TAG, "sendHeaderExtension");
    }

    public void setMediaQualityThreshold(MediaQualityThreshold threshold) {
        Rlog.d(LOG_TAG, "setMediaQualityThreshold");
    }
}
