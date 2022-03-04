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

package android.telephony.imsmedia;

import android.annotation.IntDef;
import android.hardware.radio.ims.media.MediaProtocolType;
import android.hardware.radio.ims.media.RtpSessionState;
import android.os.IBinder;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Implemented by classes that encapsulates RTP session eg. Audio, Video or RTT
 *
 * Use the instanceof keyword to determine the underlying type.
 *
 * @hide
 */
public interface ImsMediaSession {
    public static final int SESSION_TYPE_AUDIO = 0;
    public static final int SESSION_TYPE_VIDEO = 1;
    public static final int SESSION_TYPE_RTT = 2;
    /** @hide */
    @IntDef(
        value = {
           SESSION_TYPE_AUDIO,
           SESSION_TYPE_VIDEO,
           SESSION_TYPE_RTT,
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface SessionType {}

    /** The RTP session is opened but media flow has not started */
    public static final int SESSION_STATE_OPEN = RtpSessionState.OPEN;
    /** The RTP session has active media flow */
    public static final int SESSION_STATE_ACTIVE = RtpSessionState.ACTIVE;
    /** The RTP session is suspended */
    public static final int SESSION_STATE_SUSPENDED = RtpSessionState.SUSPENDED;
    /** The RTP session is closed */
    public static final int SESSION_STATE_CLOSED = RtpSessionState.CLOSED;
    /** @hide */
    @IntDef(
        value = {
           SESSION_STATE_OPEN,
           SESSION_STATE_ACTIVE,
           SESSION_STATE_SUSPENDED,
           SESSION_STATE_CLOSED,
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface SessionState {}

    /** Real Time Protocol, see RFC 3550 */
    public static final int PACKET_TYPE_RTP = MediaProtocolType.RTP;
    /** Real Time Control Protocol, see RFC 3550 */
    public static final int PACKET_TYPE_RTCP = MediaProtocolType.RTCP;
    /** @hide */
    @IntDef(
        value = {
           PACKET_TYPE_RTP,
           PACKET_TYPE_RTCP,
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface PacketType {}

    /** Constant represents the result of a session operation is failure */
    public static final int RESULT_FAILURE = 0;
    /** Constant represents the result of a session operation is failure */
    public static final int RESULT_SUCCESS = 1;
    /** @hide */
    @IntDef(
        value = {
           RESULT_FAILURE,
           RESULT_SUCCESS,
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface SessionOperationResult {}

    /** @hide */
    public IBinder getBinder();

    /** Returns the unique session identifier */
    public int getSessionId();

    /** Returns the session state */
    public @SessionState int getSessionState();

    /**
     * Modifies the configuration of the RTP session after the session is opened.
     * It can be used modify the direction, access network, codec parameters
     * RTCP configuration, remote address and remote port number. The service will
     * apply if anything changed in this invocation compared to previous and respond
     * the updated the config in ImsMediaSession#onModifySessionResponse() API
     *
     * @param config provides remote end point info and codec details
     */
    void modifySession(final RtpConfig config);

    /**
     * Sets the media quality threshold parameters of the session to get
     * media quality notifications.
     *
     * @param threshold media quality thresholds for various quality
     *        parameters
     */
    void setMediaQualityThreshold(final MediaQualityThreshold threshold);
}

