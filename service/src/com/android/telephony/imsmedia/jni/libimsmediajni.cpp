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

#define LOG_TAG "libimsmediajni"

#include <assert.h>
#include <utils/Log.h>
#include <binder/Parcel.h>
#include <android_os_Parcel.h>
#include <nativehelper/JNIHelp.h>
#include <MediaManagerFactory.h>
#include <VoiceManager.h>

#define IMS_MEDIA_JNI_VERSION JNI_VERSION_1_4

static const char *gClassPath = "com/android/telephony/imsmedia/JNIImsMediaService";

static JavaVM *gJVM = NULL;
static jclass gClass_JNIImsMediaService = NULL;
static jmethodID gMethod_sendData2Java = NULL;

jint ImsMediaServiceJni_OnLoad(JNIEnv* env);

JavaVM* GetJavaVM() {
    return gJVM;
}

JNIEnv* GetJNIEnv() {
    if (gJVM == NULL) {
        return NULL;
    }

    JNIEnv* env = NULL;

    if (gJVM->GetEnv((void**) &env, IMS_MEDIA_JNI_VERSION) != JNI_OK) {
        return NULL;
    }
    return env;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    (void) reserved;
    ALOGD("JNI_OnLoad::JNI_OnLoad");
    gJVM = vm;
    JNIEnv* env = NULL;

    if (vm->GetEnv((void**) &env, IMS_MEDIA_JNI_VERSION) != JNI_OK) {
        ALOGE("JNI_OnLoad::GetEnv failed");
        return (-1);
    }

    assert(env != NULL);

    if (ImsMediaServiceJni_OnLoad(env) < 0) {
        ALOGE("JNI_OnLoad::ImsMediaServiceJni_OnLoad failed");
        return (-1);
    }

    return IMS_MEDIA_JNI_VERSION;
}

static int SendData2Java(long nNativeObject, const android::Parcel &objParcel) {
    JNIEnv* env;
    jlong jNativeObject = nNativeObject;

    if ((gClass_JNIImsMediaService == NULL) || (gMethod_sendData2Java == NULL)) {
        ALOGE(0, "SendData2Java: Method is null", 0, 0, 0);
        return 0;
    }

    JavaVM *jvm = GetJavaVM();

    if (jvm->AttachCurrentThread(&env, NULL) != JNI_OK) {
        ALOGE(0, "SendData2Java: AttachCurrentThread fail", 0, 0, 0);
        return 0;
    }

    jbyteArray baData = env->NewByteArray(objParcel.dataSize());
    jbyte *pBuffer = env->GetByteArrayElements(baData, NULL);

    if (pBuffer != NULL) {
        memcpy(pBuffer, objParcel.data(), objParcel.dataSize());
        env->ReleaseByteArrayElements(baData, pBuffer, 0);
        env->CallStaticIntMethod(gClass_JNIImsMediaService,
            gMethod_sendData2Java, jNativeObject, baData);
    }

    env->DeleteLocalRef(baData);

    return 1;
}

static jlong JNIImsMediaService_getInterface(JNIEnv* env, jobject /* object */,
    jint mediatype, jobject jAttributionSource) {
    ALOGD("JNIImsMediaService_getInterface: type[%d]", mediatype);
    BaseManager* manager = NULL;
    switch (mediatype) {
        case MEDIA_TYPE_AUDIO:
            manager = MediaManagerFactory::getInterface(MEDIA_TYPE_AUDIO);
            if (manager != NULL) {
                manager->setCallback(SendData2Java);
                // create an uninitialized AudioRecord object
                android::Parcel* parcel = android::parcelForJavaObject(env, jAttributionSource);
                android::content::AttributionSourceState attributionSource;
                attributionSource.readFromParcel(parcel);
                ALOGD("JNIImsMediaService_getInterface: client[%s]",
                    attributionSource.toString().c_str());
                VoiceManager::setAttributeSource(attributionSource);
            }
            break;
        default:
            break;
    }

    return static_cast<jlong>(reinterpret_cast<long>(manager));
}

static jint JNIImsMediaService_sendMessage(JNIEnv* env, jobject,
    jlong nativeObj, jint sessionId, jbyteArray baData) {

    BaseManager *manager = reinterpret_cast<BaseManager*>(nativeObj);
    android::Parcel parcel;
    jbyte* pBuff = env->GetByteArrayElements(baData, NULL);
    int nBuffSize = env->GetArrayLength(baData);
    parcel.setData((const uint8_t*)pBuff, nBuffSize);
    parcel.setDataPosition(0);

    if (manager) {
        manager->sendMessage(sessionId, parcel);
    }
    env->ReleaseByteArrayElements(baData, pBuff, 0);
    return 0;
}

static JNINativeMethod gMethods[] = {
    { "getInterface", "(ILandroid/os/Parcel;)J", (void*)JNIImsMediaService_getInterface },
    { "sendMessage", "(JI[B)V", (void*)JNIImsMediaService_sendMessage },
};

jint ImsMediaServiceJni_OnLoad(JNIEnv* env) {
    jclass _jclassImsMediaService = env->FindClass(gClassPath);

    if (_jclassImsMediaService == NULL) {
        ALOGE("ImsMediaServiceJni_OnLoad :: FindClass failed");
        return -1;
    }

    gClass_JNIImsMediaService = reinterpret_cast<jclass>(env->NewGlobalRef(_jclassImsMediaService));

    if (gClass_JNIImsMediaService == NULL) {
        ALOGE("ImsMediaServiceJni_OnLoad :: FindClass failed2");
        return -1;
    }

    if (jniRegisterNativeMethods(env, gClassPath, gMethods, NELEM(gMethods)) < 0) {
        ALOGE("ImsMediaServiceJni_OnLoad: RegisterNatives failed");
        return -1;
    }

    gMethod_sendData2Java = env->GetStaticMethodID(gClass_JNIImsMediaService,
        "sendData2Java", "(J[B)I");

    if (gMethod_sendData2Java == NULL) {
        ALOGE("ImsMediaServiceJni_OnLoad: GetStaticMethodID failed");
        return -1;
    }

     return IMS_MEDIA_JNI_VERSION;
}
