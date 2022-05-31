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

#include <ImsMediaCamera.h>
#include <ImsMediaTrace.h>
#include <unistd.h>
#include <cinttypes>
#include <thread>
#include <camera/NdkCaptureRequest.h>
#include <media/NdkImage.h>

#define UNKNOWN_TAG    "UNKNOWN_TAG"
#define MAKE_PAIR(val) std::make_pair(val, #val)
template <typename T>
const char* GetPairStr(T key, std::vector<std::pair<T, const char*>>& store)
{
    typedef typename std::vector<std::pair<T, const char*>>::iterator iterator;
    for (iterator it = store.begin(); it != store.end(); ++it)
    {
        if (it->first == key)
        {
            return it->second;
        }
    }
    IMLOGW1("[GetPairStr] %#08x : UNKNOWN_TAG", key);
    return UNKNOWN_TAG;
}

/*
 * camera_status_t error translation
 */
using ERROR_PAIR = std::pair<camera_status_t, const char*>;
static std::vector<ERROR_PAIR> errorInfo{
        MAKE_PAIR(ACAMERA_OK),
        MAKE_PAIR(ACAMERA_ERROR_UNKNOWN),
        MAKE_PAIR(ACAMERA_ERROR_INVALID_PARAMETER),
        MAKE_PAIR(ACAMERA_ERROR_CAMERA_DISCONNECTED),
        MAKE_PAIR(ACAMERA_ERROR_NOT_ENOUGH_MEMORY),
        MAKE_PAIR(ACAMERA_ERROR_METADATA_NOT_FOUND),
        MAKE_PAIR(ACAMERA_ERROR_CAMERA_DEVICE),
        MAKE_PAIR(ACAMERA_ERROR_CAMERA_SERVICE),
        MAKE_PAIR(ACAMERA_ERROR_SESSION_CLOSED),
        MAKE_PAIR(ACAMERA_ERROR_INVALID_OPERATION),
        MAKE_PAIR(ACAMERA_ERROR_STREAM_CONFIGURE_FAIL),
        MAKE_PAIR(ACAMERA_ERROR_CAMERA_IN_USE),
        MAKE_PAIR(ACAMERA_ERROR_MAX_CAMERA_IN_USE),
        MAKE_PAIR(ACAMERA_ERROR_CAMERA_DISABLED),
        MAKE_PAIR(ACAMERA_ERROR_PERMISSION_DENIED),
};
const char* GetErrorStr(camera_status_t err)
{
    return GetPairStr<camera_status_t>(err, errorInfo);
}

/**
 * Range of Camera Exposure Time:
 *     Camera's capability range have a very long range which may be disturbing
 *     on camera. For this sample purpose, clamp to a range showing visible
 *     video on preview: 100000ns ~ 250000000ns
 */
static const uint64_t kMinExposureTime = static_cast<uint64_t>(1000000);
static const uint64_t kMaxExposureTime = static_cast<uint64_t>(250000000);

ImsMediaCamera::ImsMediaCamera() :
        mManager(NULL),
        mSessionOutputContainer(NULL),
        mCaptureSession(NULL),
        mCaptureSessionState(CaptureSessionState::kStateMax),
        mExposureTime(0),
        mSensitivity(0),
        mCameraMode(kCameraModePreview),
        mCameraFacing(ACAMERA_LENS_FACING_FRONT),
        mCameraOrientation(0),
        mActiveCameraId(""),
        mCameraZoom(0),
        mFramerate(-1)
{
    mValid = false;
    mCameraIds.clear();
    mManager = ACameraManager_create();
    if (mManager == NULL)
    {
        IMLOGD0("[ImsMediaCamera] manager is not created");
        return;
    }

    EnumerateCamera();
    mCaptureRequest.outputNativeWindows.resize(0);
    mCaptureRequest.sessionOutputs.resize(0);
    mCaptureRequest.targets.resize(0);
}

ImsMediaCamera::~ImsMediaCamera()
{
    mValid = false;
    mCameraIds.clear();
    if (mManager)
    {
        ACameraManager_unregisterAvailabilityCallback(mManager, GetManagerListener());
        ACameraManager_delete(mManager);
        mManager = NULL;
    }
}

void ImsMediaCamera::OpenCamera()
{
    // Create back facing camera device
    camera_status_t status = ACameraManager_openCamera(mManager, mActiveCameraId.c_str(),
            GetDeviceListener(), &mCameraIds[mActiveCameraId].mDevice);

    if (status != ACAMERA_OK)
    {
        IMLOGE1("[OpenCamera] cannot open camera, error[%s]", GetErrorStr(status));
        return;
    }

    status = ACameraManager_registerAvailabilityCallback(mManager, GetManagerListener());
    if (status != ACAMERA_OK)
    {
        IMLOGE1("[OpenCamera] fail to register manager callback, error[%s]", GetErrorStr(status));
    }

    // Initialize camera controls(exposure time and sensitivity), pick
    // up value of 2% * range + min as starting value (just a number, no magic)
    ACameraMetadata* metadataObj;
    ACameraManager_getCameraCharacteristics(mManager, mActiveCameraId.c_str(), &metadataObj);
    ACameraMetadata_const_entry val;
    status = ACameraMetadata_getConstEntry(
            metadataObj, ACAMERA_SENSOR_INFO_EXPOSURE_TIME_RANGE, &val);

    if (status == ACAMERA_OK)
    {
        mExposureRange.min = val.data.i64[0];
        if (mExposureRange.min < kMinExposureTime)
        {
            mExposureRange.min = kMinExposureTime;
        }
        mExposureRange.max = val.data.i64[1];
        if (mExposureRange.max > kMaxExposureTime)
        {
            mExposureRange.max = kMaxExposureTime;
        }

        mExposureTime = mExposureRange.value(2);
    }
    else
    {
        IMLOGW0("[OpenCamera] Unsupported ACAMERA_SENSOR_INFO_EXPOSURE_TIME_RANGE");
        mExposureRange.min = mExposureRange.max = 0l;
        mExposureTime = 0l;
    }
    status =
            ACameraMetadata_getConstEntry(metadataObj, ACAMERA_SENSOR_INFO_SENSITIVITY_RANGE, &val);

    if (status == ACAMERA_OK)
    {
        mSensitivityRange.min = val.data.i32[0];
        mSensitivityRange.max = val.data.i32[1];
        mSensitivity = mSensitivityRange.value(2);
    }
    else
    {
        IMLOGW0("[OpenCamera] failed for ACAMERA_SENSOR_INFO_SENSITIVITY_RANGE");
        mSensitivityRange.min = mSensitivityRange.max = 0;
        mSensitivity = 0;
    }

    mValid = true;
}

void ImsMediaCamera::CloseCamera()
{
    if (mCameraIds[mActiveCameraId].mAvailable == false)
        return;
    IMLOGD1("[CloseCamera] id[%s]", mActiveCameraId.c_str());
    ACameraDevice_close(mCameraIds[mActiveCameraId].mDevice);
    mCameraIds[mActiveCameraId].mDevice = NULL;
    mActiveCameraId = std::string("");
    mValid = false;
}

void ImsMediaCamera::SetCameraConfig(int32_t cameraId, int32_t cameraZoom, int32_t framerate)
{
    cameraId = 1;
    IMLOGD3("[SetCameraConfig] id[%d], zoom[%d], FPS[%d]", cameraId, cameraZoom, framerate);
    uint32_t idx = 0;
    for (std::map<std::string, CameraId>::iterator it = mCameraIds.begin(); it != mCameraIds.end();
            ++it)
    {
        if (idx == cameraId)
        {
            mActiveCameraId = (it->second).mId;
            break;
        }
        ++idx;
    }

    mCameraZoom = cameraZoom;
    mFramerate = framerate;
}

void ImsMediaCamera::CreateSession(ANativeWindow* preview, ANativeWindow* recording)
{
    if (preview == NULL)
    {
        return;
    }

    if (MatchCaptureSizeRequest(preview) == false)
    {
        IMLOGE0("[CreateSession] resolution is not matched");
        return;
    }

    mCaptureRequest.outputNativeWindows.push_back(preview);

    if (recording != NULL)
    {
        mCaptureRequest.outputNativeWindows.push_back(recording);
    }

    mCaptureRequest.sessionOutputs.resize(mCaptureRequest.outputNativeWindows.size());
    mCaptureRequest.targets.resize(mCaptureRequest.outputNativeWindows.size());

    // Create output from this app's ANativeWindow, and add into output container
    recording == NULL ? mCaptureRequest.requestTemplate = TEMPLATE_PREVIEW
                      : mCaptureRequest.requestTemplate = TEMPLATE_RECORD;

    camera_status_t status = ACaptureSessionOutputContainer_create(&mSessionOutputContainer);
    if (status != ACAMERA_OK)
    {
        IMLOGE1("[CreateSession] create output container, error[%s]", GetErrorStr(status));
    }

    for (int idxTarget = 0; idxTarget < mCaptureRequest.outputNativeWindows.size(); idxTarget++)
    {
        if (mCaptureRequest.outputNativeWindows[idxTarget] == NULL)
            continue;

        IMLOGD0("[CreateSession] acquire window");
        ANativeWindow_acquire(mCaptureRequest.outputNativeWindows[idxTarget]);
        status = ACaptureSessionOutput_create(mCaptureRequest.outputNativeWindows[idxTarget],
                &mCaptureRequest.sessionOutputs[idxTarget]);
        if (status != ACAMERA_OK)
        {
            IMLOGE1("[CreateSession] create capture output, error[%s]", GetErrorStr(status));
            continue;
        }

        ACaptureSessionOutputContainer_add(
                mSessionOutputContainer, mCaptureRequest.sessionOutputs[idxTarget]);
        status = ACameraOutputTarget_create(mCaptureRequest.outputNativeWindows[idxTarget],
                &mCaptureRequest.targets[idxTarget]);
        if (status != ACAMERA_OK)
        {
            IMLOGE1("[CreateSession] create output target, error[%s]", GetErrorStr(status));
            continue;
        }
    }

    status = ACameraDevice_createCaptureRequest(mCameraIds[mActiveCameraId].mDevice,
            mCaptureRequest.requestTemplate, &mCaptureRequest.request);
    if (status != ACAMERA_OK)
    {
        IMLOGE1("[CreateSession] create capture request, error[%s]", GetErrorStr(status));
    }

    for (int idxTarget = 0; idxTarget < mCaptureRequest.outputNativeWindows.size(); idxTarget++)
    {
        IMLOGD0("[CreateSession] add target");
        ACaptureRequest_addTarget(mCaptureRequest.request, mCaptureRequest.targets[idxTarget]);
    }

    // Create a capture session for the given preview request
    mCaptureSessionState = CaptureSessionState::kStateReady;
    status = ACameraDevice_createCaptureSession(mCameraIds[mActiveCameraId].mDevice,
            mSessionOutputContainer, GetSessionListener(), &mCaptureSession);
    if (status != ACAMERA_OK)
    {
        IMLOGE1("[CreateSession] create capture session, error[%s]", GetErrorStr(status));
    }

    /*
        uint8_t aeModeOff = ACAMERA_CONTROL_AE_MODE_OFF;
        ACaptureRequest_setEntry_u8(mCaptureRequest.request,
                                    ACAMERA_CONTROL_AE_MODE, 1, &aeModeOff);
        ACaptureRequest_setEntry_i32(mCaptureRequest.request,
                                     ACAMERA_SENSOR_SENSITIVITY, 1, &mSensitivity);
        ACaptureRequest_setEntry_i64(mCaptureRequest.request,
                                     ACAMERA_SENSOR_EXPOSURE_TIME, 1, &mExposureTime);*/

    uint8_t afModeAuto = ACAMERA_CONTROL_AF_MODE_AUTO;
    ACaptureRequest_setEntry_u8(mCaptureRequest.request, ACAMERA_CONTROL_AF_MODE, 1, &afModeAuto);
    const int32_t targetFps[2] = {mFramerate, mFramerate};
    ACaptureRequest_setEntry_i32(
            mCaptureRequest.request, ACAMERA_CONTROL_AE_TARGET_FPS_RANGE, 2, targetFps);
}

void ImsMediaCamera::DeleteSession()
{
    IMLOGD0("[DeleteSession]");
    std::lock_guard<std::mutex> guard(mMutex);

    if (mCaptureSession != NULL)
    {
        IMLOGD0("[DeleteSession] session close");
        ACameraCaptureSession_close(mCaptureSession);
    }

    for (int idxTarget = 0; idxTarget < mCaptureRequest.outputNativeWindows.size(); idxTarget++)
    {
        if (mCaptureRequest.outputNativeWindows[idxTarget] == NULL)
            continue;

        ACaptureRequest_removeTarget(mCaptureRequest.request, mCaptureRequest.targets[idxTarget]);
        ACameraOutputTarget_free(mCaptureRequest.targets[idxTarget]);
        ACaptureSessionOutputContainer_remove(
                mSessionOutputContainer, mCaptureRequest.sessionOutputs[idxTarget]);
        ACaptureSessionOutput_free(mCaptureRequest.sessionOutputs[idxTarget]);
        ANativeWindow_release(mCaptureRequest.outputNativeWindows[idxTarget]);
    }

    IMLOGD0("[DeleteSession] free resources");

    if (mCaptureRequest.request != NULL)
    {
        ACaptureRequest_free(mCaptureRequest.request);
    }

    mCaptureRequest.outputNativeWindows.resize(0);
    mCaptureRequest.sessionOutputs.resize(0);
    mCaptureRequest.targets.resize(0);

    if (mSessionOutputContainer != NULL)
    {
        ACaptureSessionOutputContainer_free(mSessionOutputContainer);
    }
}

bool ImsMediaCamera::StartSession(bool bRecording)
{
    IMLOGD1("[StartSession] recording[%d]", bRecording);

    camera_status_t status;
    bRecording ? mCameraMode = kCameraModeRecord : kCameraModePreview;
    status = ACameraCaptureSession_setRepeatingRequest(
            mCaptureSession, NULL, 1, &mCaptureRequest.request, NULL);

    if (status != ACAMERA_OK)
    {
        IMLOGE1("[StartSession] error[%s]", GetErrorStr(status));
        return false;
    }

    return true;
}

void ImsMediaCamera::StopSession()
{
    IMLOGD1("[StopSession] state[%d]", mCaptureSessionState);
    if (mCaptureSessionState == CaptureSessionState::kStateActive)
    {
        /*
        camera_status_t status = ACameraCaptureSession_abortCaptures(mCaptureSession);
        if (status != ACAMERA_OK)
        {
            IMLOGE1("[StopSession] abortCaptures error[%s]", GetErrorStr(status));
        }*/

        camera_status_t status = ACameraCaptureSession_stopRepeating(mCaptureSession);
        if (status != ACAMERA_OK)
        {
            IMLOGE1("[StopSession] stopRepeating error[%s]", GetErrorStr(status));
        }
    }
}

/*
 * Camera Manager Listener object
 */
void OnCameraAvailable(void* context, const char* id)
{
    reinterpret_cast<ImsMediaCamera*>(context)->OnCameraStatusChanged(id, true);
}
void OnCameraUnavailable(void* context, const char* id)
{
    reinterpret_cast<ImsMediaCamera*>(context)->OnCameraStatusChanged(id, false);
}

void ImsMediaCamera::OnCameraStatusChanged(const char* id, bool available)
{
    if (mValid)
    {
        IMLOGD2("[OnCameraStatusChanged] id[%s], availablep[%d]", id, available);
        mCameraIds[std::string(id)].mAvailable = available ? true : false;
    }
}

ACameraManager_AvailabilityCallbacks* ImsMediaCamera::GetManagerListener()
{
    static ACameraManager_AvailabilityCallbacks cameraMgrListener = {
            .context = this,
            .onCameraAvailable = ::OnCameraAvailable,
            .onCameraUnavailable = ::OnCameraUnavailable,
    };
    return &cameraMgrListener;
}

/*
 * CameraDevice callbacks
 */
void OnDeviceStateChanges(void* context, ACameraDevice* dev)
{
    if (context != NULL)
    {
        reinterpret_cast<ImsMediaCamera*>(context)->OnDeviceState(dev);
    }
}

void OnDeviceErrorChanges(void* context, ACameraDevice* dev, int err)
{
    if (context != NULL)
    {
        reinterpret_cast<ImsMediaCamera*>(context)->OnDeviceError(dev, err);
    }
}

ACameraDevice_stateCallbacks* ImsMediaCamera::GetDeviceListener()
{
    static ACameraDevice_stateCallbacks cameraDeviceListener = {
            .context = this,
            .onDisconnected = ::OnDeviceStateChanges,
            .onError = ::OnDeviceErrorChanges,
    };
    return &cameraDeviceListener;
}

void ImsMediaCamera::OnDeviceState(ACameraDevice* dev)
{
    std::string id(ACameraDevice_getId(dev));
    IMLOGW1("[OnDeviceState] device %s is disconnected", id.c_str());
    mCameraIds[id].mAvailable = false;
    ACameraDevice_close(mCameraIds[id].mDevice);
}

/*
 * CameraDevice error state translation, used in
 *     ACameraDevice_ErrorStateCallback
 */
using DEV_ERROR_PAIR = std::pair<int, const char*>;
static std::vector<DEV_ERROR_PAIR> devErrors{
        MAKE_PAIR(ERROR_CAMERA_IN_USE),
        MAKE_PAIR(ERROR_MAX_CAMERAS_IN_USE),
        MAKE_PAIR(ERROR_CAMERA_DISABLED),
        MAKE_PAIR(ERROR_CAMERA_DEVICE),
        MAKE_PAIR(ERROR_CAMERA_SERVICE),
};

const char* GetCameraDeviceErrorStr(int err)
{
    return GetPairStr<int>(err, devErrors);
}

void PrintCameraDeviceError(int err)
{
    IMLOGD2("[PrintCameraDeviceError] CameraDeviceError(%#x): %s", err,
            GetCameraDeviceErrorStr(err));
}

void ImsMediaCamera::OnDeviceError(ACameraDevice* dev, int err)
{
    std::string id(ACameraDevice_getId(dev));
    IMLOGE2("[OnDeviceError] CameraDevice %s is in error %#x", id.c_str(), err);
    PrintCameraDeviceError(err);

    CameraId& cam = mCameraIds[id];

    switch (err)
    {
        case ERROR_CAMERA_IN_USE:
            cam.mAvailable = false;
            cam.mOwner = false;
            break;
        case ERROR_CAMERA_SERVICE:
        case ERROR_CAMERA_DEVICE:
        case ERROR_CAMERA_DISABLED:
        case ERROR_MAX_CAMERAS_IN_USE:
            cam.mAvailable = false;
            cam.mOwner = false;
            break;
        default:
            IMLOGD1("[OnDeviceError] Unknown Camera Device Error: %#x", err);
    }
}

// CaptureSession state callbacks
void OnSessionClosed(void* context, ACameraCaptureSession* session)
{
    IMLOGW1("[OnSessionClosed] session[%p] closed", session);
    reinterpret_cast<ImsMediaCamera*>(context)->OnSessionState(
            session, CaptureSessionState::kStateClosed);
}

void OnSessionReady(void* context, ACameraCaptureSession* session)
{
    IMLOGW1("[OnSessionReady] session[%p] ready", session);
    reinterpret_cast<ImsMediaCamera*>(context)->OnSessionState(session,
            CaptureSessionState::kStateReady);
}

void OnSessionActive(void* context, ACameraCaptureSession* session)
{
    IMLOGW1("[OnSessionActive] session[%p] active", session);
    reinterpret_cast<ImsMediaCamera*>(context)->OnSessionState(
            session, CaptureSessionState::kStateActive);
}

ACameraCaptureSession_stateCallbacks* ImsMediaCamera::GetSessionListener()
{
    static ACameraCaptureSession_stateCallbacks sessionListener = {
            .context = this,
            .onClosed = ::OnSessionClosed,
            .onReady = ::OnSessionReady,
            .onActive = ::OnSessionActive,
    };
    return &sessionListener;
}

void ImsMediaCamera::OnSessionState(ACameraCaptureSession* session, CaptureSessionState state)
{
    if (!session || session != mCaptureSession)
    {
        IMLOGW1("[OnSessionState] CaptureSession is %s", (session ? "NOT our session" : "NULL"));
        return;
    }

    if (state >= CaptureSessionState::kStateMax)
    {
        IMLOGE1("[OnSessionState] Wrong state[%d]", state);
    }
    else
    {
        mCaptureSessionState = state;
        IMLOGD1("[OnSessionState] state[%d]", state);
    }
}

void ImsMediaCamera::EnumerateCamera()
{
    if (mManager == NULL)
        return;

    ACameraIdList* cameraIds = NULL;
    ACameraManager_getCameraIdList(mManager, &cameraIds);

    for (int i = 0; i < cameraIds->numCameras; ++i)
    {
        const char* id = cameraIds->cameraIds[i];
        ACameraMetadata* metadataObj;
        ACameraManager_getCameraCharacteristics(mManager, id, &metadataObj);

        int32_t count = 0;
        const uint32_t* tags = NULL;
        ACameraMetadata_getAllTags(metadataObj, &count, &tags);
        for (int tagIdx = 0; tagIdx < count; ++tagIdx)
        {
            if (ACAMERA_LENS_FACING == tags[tagIdx])
            {
                ACameraMetadata_const_entry lensInfo;
                ACameraMetadata_getConstEntry(metadataObj, tags[tagIdx], &lensInfo);
                CameraId cam(id);
                cam.mFacing = static_cast<acamera_metadata_enum_android_lens_facing_t>(
                        lensInfo.data.u8[0]);
                cam.mOwner = false;
                cam.mDevice = NULL;
                mCameraIds[cam.mId] = cam;
                IMLOGD2("[EnumerateCamera] cameraId[%s], facing[%d]", cam.mId.c_str(), cam.mFacing);
            }
        }
        ACameraMetadata_free(metadataObj);
    }

    if (mCameraIds.size() == 0)
    {
        IMLOGD0("[EnumerateCamera] No Camera Available on the device");
    }

    ACameraManager_deleteCameraIdList(cameraIds);
}

bool ImsMediaCamera::GetSensorOrientation(const int cameraId, int32_t* facing, int32_t* angle)
{
    if (!mManager)
    {
        return false;
    }
    camera_status_t status;
    ACameraMetadata* metadataObj;
    std::string targetCameraId;
    uint32_t idx = 0;
    for (std::map<std::string, CameraId>::iterator it = mCameraIds.begin(); it != mCameraIds.end();
            ++it)
    {
        if (idx == cameraId)
        {
            status = ACameraManager_getCameraCharacteristics(
                    mManager, mCameraIds[(it->second).mId].mId.c_str(), &metadataObj);
            if (status == ACAMERA_OK)
            {
                ACameraMetadata_const_entry face, orientation;
                ACameraMetadata_getConstEntry(metadataObj, ACAMERA_LENS_FACING, &face);
                mCameraFacing = static_cast<int32_t>(face.data.u8[0]);
                ACameraMetadata_getConstEntry(
                        metadataObj, ACAMERA_SENSOR_ORIENTATION, &orientation);
                ACameraMetadata_free(metadataObj);
                mCameraOrientation = orientation.data.i32[0];

                if (facing)
                {
                    *facing = mCameraFacing;
                }
                if (angle)
                {
                    *angle = mCameraOrientation;
                }

                IMLOGD2("[GetSensorOrientation] facing[%d], sensor[%d]", facing, angle);
                return true;
            }
        }
        ++idx;
    }

    return false;
}

bool ImsMediaCamera::GetExposureRange(int64_t* min, int64_t* max, int64_t* curVal)
{
    if (!mExposureRange.Supported() || !mExposureTime || !min || !max || !curVal)
    {
        return false;
    }
    *min = mExposureRange.min;
    *max = mExposureRange.max;
    *curVal = mExposureTime;

    return true;
}

bool ImsMediaCamera::GetSensitivityRange(int64_t* min, int64_t* max, int64_t* curVal)
{
    if (!mSensitivityRange.Supported() || !mSensitivity || !min || !max || !curVal)
    {
        return false;
    }
    *min = static_cast<int64_t>(mSensitivityRange.min);
    *max = static_cast<int64_t>(mSensitivityRange.max);
    *curVal = mSensitivity;
    return true;
}

/**
 * A helper class to assist image size comparison, by comparing the absolute
 * size
 * regardless of the portrait or landscape mode.
 */
class DisplayDimension
{
public:
    DisplayDimension(int32_t w, int32_t h) :
            w_(w),
            h_(h),
            portrait_(false)
    {
        if (h > w)
        {
            // make it landscape
            w_ = h;
            h_ = w;
            portrait_ = true;
        }
    }
    DisplayDimension(const DisplayDimension& other)
    {
        w_ = other.w_;
        h_ = other.h_;
        portrait_ = other.portrait_;
    }

    DisplayDimension(void)
    {
        w_ = 0;
        h_ = 0;
        portrait_ = false;
    }
    DisplayDimension& operator=(const DisplayDimension& other)
    {
        w_ = other.w_;
        h_ = other.h_;
        portrait_ = other.portrait_;

        return (*this);
    }

    bool IsSameRatio(DisplayDimension& other) { return (w_ * other.h_ == h_ * other.w_); }
    bool operator>(DisplayDimension& other) { return (w_ >= other.w_ & h_ >= other.h_); }
    bool operator==(DisplayDimension& other)
    {
        return (w_ == other.w_ && h_ == other.h_ && portrait_ == other.portrait_);
    }
    DisplayDimension operator-(DisplayDimension& other)
    {
        DisplayDimension delta(w_ - other.w_, h_ - other.h_);
        return delta;
    }
    void Flip(void) { portrait_ = !portrait_; }
    bool IsPortrait(void) { return portrait_; }
    int32_t width(void) { return w_; }
    int32_t height(void) { return h_; }
    int32_t org_width(void) { return (portrait_ ? h_ : w_); }
    int32_t org_height(void) { return (portrait_ ? w_ : h_); }

private:
    int32_t w_, h_;
    bool portrait_;
};

bool ImsMediaCamera::MatchCaptureSizeRequest(ANativeWindow* window)
{
    DisplayDimension disp(ANativeWindow_getWidth(window), ANativeWindow_getHeight(window));
    IMLOGD3("[MatchCaptureSizeRequest] request width[%d], height[%d], camOrientation[%d]",
            disp.width(), disp.height(), mCameraOrientation);
    if (mCameraOrientation == 90 || mCameraOrientation == 270)
    {
        disp.Flip();
    }

    ACameraMetadata* metadata;
    ACameraManager_getCameraCharacteristics(mManager, mActiveCameraId.c_str(), &metadata);
    ACameraMetadata_const_entry entry;
    ACameraMetadata_getConstEntry(metadata, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry);

    for (uint32_t i = 0; i < entry.count; i += 4)
    {
        int32_t input = entry.data.i32[i + 3];
        int32_t format = entry.data.i32[i + 0];
        if (input)
            continue;

        if (format == AIMAGE_FORMAT_YUV_420_888 || format == AIMAGE_FORMAT_JPEG)
        {
            DisplayDimension dimension(entry.data.i32[i + 1], entry.data.i32[i + 2]);
            if (!disp.IsSameRatio(dimension))
                continue;
            if (format == AIMAGE_FORMAT_YUV_420_888 && disp == dimension)
            {
                return true;
            }
        }
    }

    return false;
}
