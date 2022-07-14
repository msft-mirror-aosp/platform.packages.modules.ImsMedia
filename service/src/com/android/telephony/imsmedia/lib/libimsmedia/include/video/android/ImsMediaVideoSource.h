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

#ifndef IMSMEDIA_VIDEO_SOURCE_H_INCLUDED
#define IMSMEDIA_VIDEO_SOURCE_H_INCLUDED

#include <ImsMediaVideoUtil.h>
#include <ImsMediaDefine.h>
#include <android/native_window.h>
#include <ImsMediaCamera.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>
#include <media/NdkImageReader.h>
#include <ImsMediaCondition.h>

class IVideoSourceCallback
{
public:
    virtual ~IVideoSourceCallback() {}
    virtual void OnUplinkEvent(
            uint8_t* pBitstream, uint32_t nSize, int64_t pstUsec, uint32_t flag) = 0;
    virtual void OnEvent(int32_t type, int32_t param1, int32_t param2) = 0;
};

enum kImsMediaVideoMode
{
    kVideoModePreview = 0,
    kVideoModeRecording,
    kVideoModePauseImage,
};

enum kVideoSourceEvent
{
    kVideoSourceEventUpdateCamera = 0,
    kVideoSourceEventCameraError,
};

/**
 * @brief
 *
 */
class ImsMediaVideoSource
{
public:
    ImsMediaVideoSource();
    ~ImsMediaVideoSource();
    void SetListener(IVideoSourceCallback* listener);
    void SetVideoMode(const int32_t mode);
    void SetCameraConfig(const uint32_t cameraId, const uint32_t cameraZoom);
    void SetImagePath(const android::String8 path);
    void SetCodecConfig(const int32_t codecType, const uint32_t profile, const uint32_t level,
            const uint32_t bitrate, const uint32_t framerate, const uint32_t interval);
    void SetResolution(const uint32_t width, const uint32_t height);
    void SetSurface(ANativeWindow* window);
    void SetDeviceOrientation(const uint32_t degree);
    bool Start();
    void Stop();
    void processFormatChanged(AMediaFormat* format);
    void processOutputBuffer();

private:
    ImsMediaCamera* mCamera;
    ANativeWindow* mWindow;
    AMediaCodec* mCodec;
    AMediaFormat* mFormat;
    ANativeWindow* mRecordingSurface;
    ANativeWindow* mImageReaderSurface;
    AImageReader* mImageReader;
    std::mutex mMutex;
    std::mutex mImageReaderMutex;
    ImsMediaCondition mConditionExit;
    IVideoSourceCallback* mListener;
    int32_t mCodecType;
    int32_t mVideoMode;
    uint32_t mCodecProfile;
    uint32_t mCodecLevel;
    uint32_t mCameraId;
    uint32_t mCameraZoom;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mFramerate;
    uint32_t mBitrate;
    uint32_t mIntraInterval;
    android::String8 mImagePath;
    int32_t mDeviceOrientation;
    uint64_t mTimestamp;
    uint64_t mPrevTimestamp;
    bool mStopped;

    ANativeWindow* CreateImageReader(int width, int height);
    void onCameraFrame(AImage* pImage);
    static void ImageCallback(void* context, AImageReader* reader);
};
#endif