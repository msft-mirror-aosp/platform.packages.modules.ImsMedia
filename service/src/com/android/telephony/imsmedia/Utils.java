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
import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.support.annotation.VisibleForTesting;
import android.telephony.imsmedia.RtpConfig;

import com.android.telephony.imsmedia.ImsMediaController.OpenSessionCallback;

/**
 * Class consists of utility methods and sub classes
 *
 * @hide
 */
final class Utils {
    /** Class to encapsulate open session parameters */
    static final class OpenSessionParams {
        private ParcelFileDescriptor rtpFd;
        private ParcelFileDescriptor rtcpFd;
        private RtpConfig rtpConfig;
        private OpenSessionCallback callback;

        @VisibleForTesting
        OpenSessionParams() {
        }

        OpenSessionParams(ParcelFileDescriptor rtpFd,
                ParcelFileDescriptor rtcpFd,
                RtpConfig rtpConfig,
                OpenSessionCallback callback) {
            this.rtpFd = rtpFd;
            this.rtcpFd = rtcpFd;
            this.rtpConfig = rtpConfig;
            this.callback = callback;
        }

        ParcelFileDescriptor getRtpFd() {
            return rtpFd;
        }

        ParcelFileDescriptor getRtcpFd() {
            return rtcpFd;
        }

        RtpConfig getRtpConfig() {
            return rtpConfig;
        }

        OpenSessionCallback getCallback() {
            return callback;
        }
    }

    static void sendMessage(Handler handler, int command) {
        Message msg = handler.obtainMessage(command);
        msg.sendToTarget();
    }

    static void sendMessage(Handler handler, int command, Object argument) {
        Message msg = handler.obtainMessage(command, argument);
        msg.sendToTarget();
    }

    static void sendMessage(Handler handler, int command, int arg1, int arg2) {
        Message msg = handler.obtainMessage(command, arg1, arg2);
        msg.sendToTarget();
    }

    static void sendMessage(Handler handler, int command, int arg1, int arg2, Object object) {
        Message msg = handler.obtainMessage(command, arg1, arg2, object);
        msg.sendToTarget();
    }
}
