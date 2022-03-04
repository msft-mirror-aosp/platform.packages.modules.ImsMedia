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

import android.telephony.imsmedia.RtpConfig;

/**
 * RTT (Real Time Text) service
 */
final class RttService {
    private static final String LOG_TAG = "RttService";
    private ImsMediaController.OpenSessionCallback mMediaControllerCallback;

    RttService(ImsMediaController.OpenSessionCallback callback) {
        mMediaControllerCallback = callback;

        // TODO: Initialize audio RTP stack
    }

    public void openSession(int sessionId, RtpConfig config) {
        // TODO
        // create localEndPoint
        // covert telephony RtpConfig internal RtpConfig
        // Invoke internal openSession(sessionId, localEndPoint, config);
        //
        // Once openSession is success notify using ImsMediaController.OpenSessionCallback
    }

    public void closeSession(int sessionId) {
        // TODO: Invoke internal closeSession(sessionId);
    }
}
