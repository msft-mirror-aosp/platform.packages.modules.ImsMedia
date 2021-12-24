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

import android.hardware.radio.ims.media.IImsMediaSessionListener;
import android.hardware.radio.ims.media.RtpHeaderExtension;
import android.hardware.radio.ims.media.RtpConfig;
import android.hardware.radio.ims.media.RtpSession;
import android.os.Handler;

/**
 * This implements the handlers for all indication messages from media HAL
 */
final class AudioOffloadListener extends IImsMediaSessionListener.Stub {
    private static final int UNUSED = -1;
    private Handler handler;

    @Override
    public void onSessionChanged(RtpSession session) {
        Utils.sendMessage(handler, AudioSession.EVENT_SESSION_CHANGED_IND, session.sessionId);
    }

    @Override
    public void onModifySessionResponse(RtpConfig config, int result) {
        // TODO : convert from HAL
        Utils.sendMessage(handler,
                AudioSession.EVENT_MODIFY_SESSION_RESPONSE, result, UNUSED, null);
    }

    @Override
    public void onAddConfigResponse(RtpConfig config, int result) {
        // TODO : convert from HAL
        Utils.sendMessage(handler, AudioSession.EVENT_ADD_CONFIG_RESPONSE, result, UNUSED, null);
    }

    @Override
    public void onConfirmConfigResponse(RtpConfig config, int result) {
        // TODO : convert from HAL
        Utils.sendMessage(handler,
                AudioSession.EVENT_CONFIRM_CONFIG_RESPONSE, result, UNUSED, null);
    }

    @Override
    public void onFirstMediaPacketReceived(RtpConfig config) {
        // TODO : convert from HAL
        Utils.sendMessage(handler, AudioSession.EVENT_FIRST_MEDIA_PACKET_IND, null);
    }

    @Override
    public void onHeaderExtensionReceived(RtpHeaderExtension[] extensions) {
        // TODO : convert from HAL
        Utils.sendMessage(handler, AudioSession.EVENT_RTP_HEADER_EXTENSION_IND, null);
    }

    @Override
    public void notifyMediaInactivity(int packetType, int timeout) {
        Utils.sendMessage(handler, AudioSession.EVENT_MEDIA_INACTIVITY_IND, packetType, timeout);
    }

    @Override
    public void notifyPacketLoss(int packetLossPercentage) {
        Utils.sendMessage(handler, AudioSession.EVENT_PACKET_LOSS_IND, packetLossPercentage);
    }

    @Override
    public void notifyJitter(int jitter) {
        Utils.sendMessage(handler, AudioSession.EVENT_JITTER_IND, jitter);
    }

    AudioOffloadListener(Handler handler) {
        this.handler = handler;
    }
}
