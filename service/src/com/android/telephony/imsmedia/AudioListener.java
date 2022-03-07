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
import android.os.Parcel;
import android.telephony.Rlog;
import android.telephony.imsmedia.AudioConfig;
import android.telephony.imsmedia.MediaQualityThreshold;
import android.telephony.ims.RtpHeaderExtension;

/**
 * Audio listener to process JNI messages from local AP based RTP stack
 */
public class AudioListener implements JNIImsMediaListener {
    private static final String LOG_TAG = "AudioListener";
    private static final int UNUSED = -1;
    final private Handler mHandler;
    private ImsMediaController.OpenSessionCallback mCallback;
    private long mNativeObject;

    AudioListener(final Handler handler) {
        mHandler = handler;
    }

    /**
     * Sets callback to call ImsMediaController to handle responses when
     * openSession method called in @AudioService
     *
     * @param callback A Callback of @ImsMediaController#OpenSessionCallback
     */
    public void setMediaCallback(final ImsMediaController.OpenSessionCallback callback) {
        mCallback = callback;
    }

    /**
     * Sets native object to identify the instance of @BaseManager
     *
     * @param object
     */
    public void setNativeObject(final long object) {
        mNativeObject = object;
    }

    /**
     * Processes parcel messages from native code and posts messages to {@link AudioSession}
     *
     * @param parcel A parcel from native @VoiceManager in libimsmedia library
     */
    @Override
    public void onMessage(final Parcel parcel) {
        final int event = parcel.readInt();
        Rlog.d(LOG_TAG, "onMessage=" + event);
        switch (event) {
            case AudioSession.EVENT_OPEN_SESSION_SUCCESS:
                final int sessionId = parcel.readInt();
                mCallback.onOpenSessionSuccess(sessionId,
                    new AudioLocalSession(sessionId, mNativeObject));
                break;
            case AudioSession.EVENT_OPEN_SESSION_FAILURE:
                mCallback.onOpenSessionFailure(parcel.readInt(),
                    parcel.readInt());
                break;
            case AudioSession.EVENT_MODIFY_SESSION_RESPONSE:
            case AudioSession.EVENT_ADD_CONFIG_RESPONSE:
            case AudioSession.EVENT_CONFIRM_CONFIG_RESPONSE:
                final int result = parcel.readInt();
                final AudioConfig config = AudioConfig.CREATOR.createFromParcel(parcel);
                Utils.sendMessage(mHandler, event, result, UNUSED, config);
                break;
            case AudioSession.EVENT_SESSION_CHANGED_IND:
            case AudioSession.EVENT_FIRST_MEDIA_PACKET_IND:
                //TODO : add implementation
                break;
            case AudioSession.EVENT_RTP_HEADER_EXTENSION_IND:
                Utils.sendMessage(mHandler, event);
                break;
            case AudioSession.EVENT_MEDIA_INACTIVITY_IND:
                Utils.sendMessage(mHandler, event, parcel.readInt(), parcel.readInt());
                break;
            case AudioSession.EVENT_PACKET_LOSS_IND:
            case AudioSession.EVENT_JITTER_IND:
                //TODO : add implementation
                break;
            default:
                break;
        }
    }
}
