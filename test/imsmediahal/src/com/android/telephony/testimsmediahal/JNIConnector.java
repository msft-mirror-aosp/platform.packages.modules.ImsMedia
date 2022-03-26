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


package com.android.telephony.testimsmediahal;

import android.content.AttributionSource;
import android.content.AttributionSource.ScopedParcelState;

import android.telephony.imsmedia.ImsMediaSession;
import com.android.telephony.imsmedia.JNIImsMediaService;

import java.util.HashSet;

import android.os.Binder;
import android.os.Parcel;

import android.telephony.Rlog;

/**
 * Connect with {@link libimsmedia} and sending request for opensession
 * or close session, on success it will get respose back based on success
 * or failure case.
 */

public class JNIConnector{

    private static String TAG = "ImsMediaConnector";

    private static long mNativeObject;
    private static AudioListenerProxy mNativeListener = null;
    private static JNIConnector mInstance;
    private static JNIImsMediaService mJNIServiceInstance;

    private JNIConnector() {
        mJNIServiceInstance = JNIImsMediaService.getInstance();
    }

    public static JNIConnector getInstance() {
        if(mInstance == null)
        {
            mInstance = new JNIConnector();
        }

        return mInstance;
    }

    /**
     * Sends RTP session request to {@link libimsmedia}.
     *
     * @param sessionId used to identify unique session.
     * @param Parcel contains event identifier and session related info.
     */

    public void sendRequest(int sessionid, Parcel parcel) {
        if (mNativeObject != 0) {
            byte[] data = parcel.marshall();
            parcel.recycle();
            parcel = null;
            //send to native
            mJNIServiceInstance.sendMessage(mNativeObject, sessionid, data);
        }
    }

    /**
     *  Doing connection with {@link libimsmedia} via {@link libimsmediahaljni}
     */

    public void ConnectJNI() {
        mNativeListener = AudioListenerProxy.getInstance();

        AttributionSource attributionSource = AttributionSource.myAttributionSource();

        if (attributionSource.getPackageName() == null) {
            // Command line utility
            attributionSource = attributionSource.withPackageName("uid:" + Binder.getCallingUid());
        }

        Rlog.d(TAG, "package=" + attributionSource.getPackageName());

        HashSet<String> setPermission = new HashSet<String>();

        setPermission.add("android.permission.RECORD_AUDIO");

        AttributionSource attributionSource2 = new AttributionSource(attributionSource.getUid(),
            attributionSource.getPackageName(), attributionSource.getAttributionTag(),
            setPermission, attributionSource.getNext());

        try (ScopedParcelState attributionSourceState = attributionSource2.asScopedParcelState()) {
            mNativeObject = mJNIServiceInstance.getInterface(
            ImsMediaSession.SESSION_TYPE_AUDIO, attributionSourceState.getParcel());
        } catch (Exception e) {
            Rlog.e(TAG, "exception=" + e);
        }

        mJNIServiceInstance.setListener(mNativeObject, mNativeListener);
    }

}