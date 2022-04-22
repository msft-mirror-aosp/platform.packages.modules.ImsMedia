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

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.support.annotation.GuardedBy;
import android.telephony.Rlog;
import android.telephony.imsmedia.IImsAudioSession;
import android.telephony.imsmedia.IImsAudioSessionCallback;
import android.telephony.imsmedia.IImsMedia;
import android.telephony.imsmedia.IImsVideoSession;
import android.telephony.imsmedia.IImsVideoSessionCallback;
import android.telephony.imsmedia.ImsMediaSession;
import android.telephony.imsmedia.RtpConfig;
import android.util.SparseArray;

import com.android.telephony.imsmedia.Utils.OpenSessionParams;

import java.util.concurrent.atomic.AtomicInteger;

/** Controller that maintains all IMS Media sessions */
public class ImsMediaController extends Service {

    private static final String TAG = "ImsMediaController";
    private AtomicInteger mSessionId = new AtomicInteger();
    private OpenSessionCallback mCallback = new OpenSessionCallback();

    @GuardedBy("mSessions")
    private final SparseArray<IMediaSession> mSessions = new SparseArray();

    private class ImsMediaBinder extends IImsMedia.Stub {
        // TODO add permission checks
        @Override
        public void openSession(
            final ParcelFileDescriptor rtpFd, final ParcelFileDescriptor rtcpFd,
            final int sessionType, final RtpConfig rtpConfig, final IBinder callback) {
            final int sessionId = mSessionId.getAndIncrement();
            IMediaSession session;
            Rlog.d(TAG, "openSession: sessionId = " + sessionId
                    + ", type=" + sessionType + "," + rtpConfig);
            synchronized (mSessions) {
                switch (sessionType) {
                    case ImsMediaSession.SESSION_TYPE_AUDIO:
                        session = new AudioSession(sessionId,
                                IImsAudioSessionCallback.Stub.asInterface(callback));
                        break;
                    case ImsMediaSession.SESSION_TYPE_VIDEO:
                        session = new VideoSession(sessionId,
                                IImsVideoSessionCallback.Stub.asInterface(callback));
                        break;
                    case ImsMediaSession.SESSION_TYPE_RTT:
                    default:
                        session = null;
                }

                if (session != null) {
                    mSessions.append(sessionId, session);
                    session.openSession(new OpenSessionParams(rtpFd, rtcpFd, rtpConfig,
                        mCallback));

                }
            }
        }

        @Override
        public void closeSession(IBinder session) {
            Rlog.d(TAG, "closeSession: " + session);
            synchronized (mSessions) {
                /** TODO handle for all types */
                if (session instanceof AudioSession) {
                    final AudioSession audioSession =
                            (AudioSession) IImsAudioSession.Stub.asInterface(session);
                    audioSession.closeSession();
                    mSessions.remove(audioSession.getSessionId());
                } else if (session instanceof VideoSession) {
                    final VideoSession videoSession =
                            (VideoSession) IImsVideoSession.Stub.asInterface(session);
                    videoSession.closeSession();
                    mSessions.remove(videoSession.getSessionId());
                }
            }
        }
    }

    private IImsMedia.Stub mImsMediaBinder = new ImsMediaBinder();

    @Override
    public IBinder onBind(Intent intent) {
        Rlog.d(TAG, Thread.currentThread().getName() + " onBind");
        return mImsMediaBinder;
    }

    @Override
    public void onCreate() {
        Rlog.d(TAG, "onCreate");
    }

    @Override
    public void onDestroy() {
        Rlog.d(TAG, "onDestroy");
    }

    private IMediaSession getSession(int sessionId) {
        return mSessions.get(sessionId);
    }

    public class OpenSessionCallback {
        public void onOpenSessionSuccess(int sessionId, Object session) {
            getSession(sessionId).onOpenSessionSuccess(session);
        }

        public void onOpenSessionFailure(int sessionId, int error) {
            synchronized (mSessions) {
                getSession(sessionId).onOpenSessionFailure(error);
                mSessions.remove(sessionId);
            }
        }
    }
}
