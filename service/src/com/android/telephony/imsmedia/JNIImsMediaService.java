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

import android.os.Parcel;
import android.os.ParcelFileDescriptor;
import android.telephony.Rlog;
import android.telephony.CallQuality;
import android.telephony.imsmedia.RtpConfig;
import android.telephony.ims.RtpHeaderExtension;

import java.io.FileDescriptor;
import java.util.Hashtable;

/** JNI interface class to send message to libimsmediajni */
public class JNIImsMediaService {
    private static final String TAG = "JNIImsMediaService";
    public static JNIImsMediaService sService = null;
    private final Object mLock = new Object();

    /** for media service based on type ex. audio, video, rtt */
    private static Hashtable<Long, JNIImsMediaListener> sListeners
        = new Hashtable<Long, JNIImsMediaListener>();

    /**
     * Gets instance object of BaseManager with the corresponding media type
     *
     * @param mediatype Audio/Video/Text type
     * @param attributionSource permission information of the current process
     * @return
     */
    public static native long getInterface(int mediatype, Parcel attributionSource);

    /**
     * Send message to libimsmediajni to operate libimsmedia library to operate with corresponding
     * arguments
     *
     * @param nativeObj An unique object identifier of BaseManager to operate
     * @param sessionid An unique session identifier
     * @param baData A parameter to operate session
     */
    public static native void sendMessage(long nativeObj, int sessionid, byte[] baData);

    /**
     * Gets intance of JNIImsMediaService for jni interface
     *
     * @return instance of JNIImsMediaService
     */
    public static JNIImsMediaService getInstance() {
        if (sService == null) {
            sService = new JNIImsMediaService();
        }
        return sService;
    }

    /**
     * Sets listener to get callback from libimsmediajni
     *
     * @param nativeObj An unique object identifier of BaseManager to use as a key to
     * acquire a paired listener
     * @param listener A listener to set for getting messages
     */
    public static void setListener(final long nativeObj, final JNIImsMediaListener listener) {
        Rlog.d(TAG,"setListener");
        if (nativeObj == 0 || listener == null) {
            return;
        }
        Long key = Long.valueOf(nativeObj);
        synchronized (sListeners) {
            sListeners.put(key, listener);
        }
    }

    /**
     * Gets a listener with the key to match
     *
     * @param nativeObj An unique key identifier to get the paired listener
     * @return A JNIImsMediaListener listener
     */
    public static JNIImsMediaListener getListener(final long nativeObj) {
        Rlog.d(TAG,"getListener");
        if (nativeObj == 0) {
            return null;
        }
        Long key = Long.valueOf(nativeObj);
        JNIImsMediaListener listener = null;
        synchronized (sListeners) {
            listener = sListeners.get(key);
        }

        return listener;
    }

    /**
     * Sends callback parcel message from libimsmediajni to java
     *
     * @param nativeObj An unique key idenfier to find corresponding listener object to send message
     * @param baData byte array form of data to send
     * @return 1 if it is success to send data, -1 when it fails
     */
    public static int sendData2Java(final long nativeObj, final byte[] baData) {
        Rlog.d(TAG,"sendData2Java");
        JNIImsMediaListener listener = getListener(nativeObj);

        if (listener == null) {
            Rlog.e(TAG, "No listener :: nativeObject=" + nativeObj);
            return -1;
        }

        // retrieve parcel object from pool
        Parcel parcel = Parcel.obtain();
        parcel.unmarshall(baData, 0, baData.length);
        parcel.setDataPosition(0);
        listener.onMessage(parcel);
        parcel.recycle();

        return 1;
    }


    /** local shared libimsmediajni library */
    static {
        try {
            Rlog.d(TAG,"libimsmediajni :: loading");
            System.loadLibrary("imsmediajni");
            Rlog.d(TAG,"libimsmediajni :: load completed");
        } catch (UnsatisfiedLinkError e) {
            Rlog.e(TAG,"Loading fail : libimsmediajni.so");
            e.printStackTrace();
        }
    }
}
