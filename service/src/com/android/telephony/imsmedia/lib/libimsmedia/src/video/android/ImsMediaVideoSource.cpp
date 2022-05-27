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

#include <ImsMediaVideoSource.h>
#include <ImsMediaTrace.h>
#include <ImsMediaTimer.h>
#include <thread>
#include <list>

ImsMediaVideoSource::ImsMediaVideoSource()
{
    mCamera = NULL;
    mWindow = NULL;
    mCodec = NULL;
    mFormat = NULL;
    mRecordingSurface = NULL;
    mCodecType = -1;
    mVideoMode = -1;
    mCodecProfile = 0;
    mCodecLevel = 0;
    mCameraId = 0;
    mCameraZoom = 0;
    mWidth = 0;
    mHeight = 0;
    mFramerate = 0;
    mBitrate = 0;
    mIntraInterval = 1;
    mImagePath = android::String8("");
    mDeviceOrientation = -1;
    mTimestamp = 0;
    mPrevTimestamp = 0;
}

ImsMediaVideoSource::~ImsMediaVideoSource() {}

void ImsMediaVideoSource::SetListener(IVideoSourceCallback* listener)
{
    std::lock_guard<std::mutex> guard(mMutex);
    mListener = listener;
}

void ImsMediaVideoSource::SetVideoMode(const int32_t mode)
{
    IMLOGD1("[SetVideoMode] mode[%d]", mode);
    mVideoMode = mode;
}

void ImsMediaVideoSource::SetCameraConfig(const uint32_t cameraId, const uint32_t cameraZoom)
{
    IMLOGD2("[SetCameraConfig] id[%d], zoom[%d]", cameraId, cameraZoom);
    mCameraId = cameraId;
    mCameraZoom = cameraZoom;
}

void ImsMediaVideoSource::SetImagePath(const android::String8 path)
{
    IMLOGD1("[SetImagePath] path[%s]", path.string());
    mImagePath = path;
}

void ImsMediaVideoSource::SetCodecConfig(int32_t codecType, const uint32_t profile,
        const uint32_t level, const uint32_t bitrate, const uint32_t framerate,
        const uint32_t interval)
{
    IMLOGD6("[SetCodecConfig] type[%d], profile[%d], level[%d], bitrate[%d], FPS[%d], interval[%d]",
            codecType, profile, level, bitrate, framerate, interval);
    mCodecType = codecType;
    mCodecProfile = profile;
    mCodecLevel = level;
    mBitrate = bitrate;
    mFramerate = framerate;
    mIntraInterval = interval;
}

void ImsMediaVideoSource::SetResolution(const uint32_t width, const uint32_t height)
{
    IMLOGD2("[SetResolution] width[%d], height[%d]", width, height);
    mWidth = width;
    mHeight = height;
}

void ImsMediaVideoSource::SetSurface(ANativeWindow* window)
{
    IMLOGD1("[SetSurface] surface[%p]", window);
    mWindow = window;
}

void ImsMediaVideoSource::SetDeviceOrientation(const uint32_t degree)
{
    if (mDeviceOrientation != degree)
    {
        if (mCamera != NULL && mVideoMode == kVideoModeRecording)
        {
            int32_t facing, angle;
            if (mCamera->GetSensorOrientation(mCameraId, &facing, &angle) && mListener != NULL)
            {
                IMLOGD0("[SetDeviceOrientation] send event");
                mListener->OnEvent(kVideoSourceEventUpdateCamera, facing, angle);
            }
        }
        mDeviceOrientation = degree;
    }
}

bool ImsMediaVideoSource::Start()
{
    IMLOGD0("[Start]");
    mRecordingSurface = NULL;

    if (mVideoMode == kVideoModeRecording)
    {
        mFormat = AMediaFormat_new();
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_WIDTH, mWidth);
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_HEIGHT, mHeight);

        char kMimeType[128] = {'\0'};
        sprintf(kMimeType, "video/avc");
        if (mCodecType == kVideoCodecHevc)
        {
            sprintf(kMimeType, "video/hevc");
        }

        AMediaFormat_setString(mFormat, AMEDIAFORMAT_KEY_MIME, kMimeType);

        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT,
                0x7F000789);  // #0x7F000789 : COLOR_FormatSurface
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_BIT_RATE, mBitrate * 1000);
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_PROFILE, mCodecProfile);
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_LEVEL, mCodecLevel);
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_BITRATE_MODE,
                2);  // #2 : BITRATE_MODE_CBR
        AMediaFormat_setFloat(mFormat, AMEDIAFORMAT_KEY_FRAME_RATE, mFramerate);
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, mIntraInterval);
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_MAX_INPUT_SIZE, mWidth * mHeight * 10);

        mCodec = AMediaCodec_createEncoderByType(kMimeType);
        if (mCodec == NULL)
        {
            IMLOGE0("[Start] Unable to create encoder");
            return false;
        }

        media_status_t err = AMediaCodec_configure(
                mCodec, mFormat, NULL, NULL, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
        if (err != AMEDIA_OK)
        {
            IMLOGE1("[Start] configure error[%d]", err);
            AMediaCodec_delete(mCodec);
            mCodec = NULL;
            AMediaFormat_delete(mFormat);
            mFormat = NULL;
            return false;
        }

        err = AMediaCodec_createInputSurface(mCodec, &mRecordingSurface);
        if (err != AMEDIA_OK)
        {
            IMLOGE1("[Start] create input surface error[%d]", err);
            AMediaCodec_delete(mCodec);
            mCodec = NULL;
            AMediaFormat_delete(mFormat);
            mFormat = NULL;
            return false;
        }

        err = AMediaCodec_start(mCodec);
        if (err != AMEDIA_OK)
        {
            IMLOGE1("[Start] codec start[%d]", err);
            AMediaCodec_delete(mCodec);
            mCodec = NULL;
            AMediaFormat_delete(mFormat);
            mFormat = NULL;
            return false;
        }
    }

    if (mVideoMode == kVideoModePreview || mVideoMode == kVideoModeRecording)
    {
        mCamera = new ImsMediaCamera();
        mCamera->SetCameraConfig(mCameraId, mCameraZoom, mFramerate);
        mCamera->OpenCamera();
        mCamera->CreateSession(mWindow, mRecordingSurface);
        if (mCamera->StartSession(mVideoMode == kVideoModeRecording ? true : false) == false)
        {
            IMLOGE0("[Start] camera start");
            AMediaCodec_delete(mCodec);
            mCodec = NULL;
            AMediaFormat_delete(mFormat);
            mFormat = NULL;
            return false;
        }
    }

    // start encoder output thread
    IMLOGD0("[Start] test thread");
    std::thread t1(&ImsMediaVideoSource::processOutputBuffer, this);
    t1.detach();

    IMLOGD0("[Start] exit");
    return true;
}

void ImsMediaVideoSource::Stop()
{
    IMLOGD0("[Stop]");
    std::lock_guard<std::mutex> guard(mMutex);
    if (mCamera != NULL)
    {
        mCamera->StopSession();
    }

    if (mCodec != NULL)
    {
        AMediaCodec_stop(mCodec);
        AMediaCodec_delete(mCodec);
        mCodec = NULL;
    }

    if (mCamera != NULL)
    {
        mCamera->DeleteSession();
        mCamera->CloseCamera();
        delete mCamera;
        mCamera = NULL;
    }

    if (mRecordingSurface != NULL)
    {
        ANativeWindow_release(mRecordingSurface);
        mRecordingSurface = NULL;
    }

    if (mFormat != NULL)
    {
        AMediaFormat_delete(mFormat);
        mFormat = NULL;
    }
}

void ImsMediaVideoSource::processOutputBuffer()
{
    static int kTimeout = 100000;  // be responsive on signal
    static int kMaxUplinkBuffer = 100;
    uint32_t nNextTime = ImsMediaTimer::GetTimeInMilliSeconds();
    uint32_t nTimeInterval = 66;
    std::list<uint8_t*> uplinkBuffers;

    if (mFramerate != 0)
    {
        nTimeInterval = 1000 / mFramerate;
    }

    for (;;)
    {
        uint32_t nCurrTime;
        mMutex.lock();
        if (mCodec == NULL || mListener == NULL)
        {
            IMLOGD0("[processOutputBuffer] terminated");
            mMutex.unlock();
            break;
        }
        mMutex.unlock();

        AMediaCodecBufferInfo info;
        IMLOGD0("[processOutputBuffer] dequeue");
        auto index = AMediaCodec_dequeueOutputBuffer(mCodec, &info, kTimeout);

        if (index >= 0)
        {
            IMLOGD_PACKET5(IM_PACKET_LOG_VIDEO,
                    "[processOutputBuffer] index[%d], size[%d], offset[%d], time[%ld], flags[%d]",
                    index, info.size, info.offset, info.presentationTimeUs, info.flags);

            if (info.size > 0)
            {
                size_t buffCapacity;
                uint8_t* buf = AMediaCodec_getOutputBuffer(mCodec, index, &buffCapacity);
                uint8_t* data = new uint8_t[info.size];
                memcpy(data, buf, info.size);
                uplinkBuffers.push_back(data);
                mListener->OnUplinkEvent(data, info.size, info.presentationTimeUs, info.flags);
            }

            IMLOGD0("[processOutputBuffer] release");
            AMediaCodec_releaseOutputBuffer(mCodec, index, false);
        }
        else if (index == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED)
        {
            IMLOGD0("[processOutputBuffer] Encoder output buffer changed");
        }
        else if (index == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED)
        {
            if (mFormat != NULL)
            {
                AMediaFormat_delete(mFormat);
            }
            mFormat = AMediaCodec_getOutputFormat(mCodec);
            IMLOGD1("[processOutputBuffer] Encoder format changed, format[%s]",
                    AMediaFormat_toString(mFormat));
        }
        else if (index == AMEDIACODEC_INFO_TRY_AGAIN_LATER)
        {
            IMLOGD0("[processOutputBuffer] no output buffer");
        }
        else
        {
            IMLOGD1("[processOutputBuffer] unexpected index[%d]", index);
        }

        if (uplinkBuffers.size() > kMaxUplinkBuffer)
        {
            uint8_t* data = uplinkBuffers.front();
            delete[] data;
            uplinkBuffers.pop_front();
        }

        nNextTime += nTimeInterval;
        nCurrTime = ImsMediaTimer::GetTimeInMilliSeconds();
        IMLOGD_PACKET1(IM_PACKET_LOG_VIDEO, "[processOutputBuffer] nCurrTime[%u]", nCurrTime);
        if (nNextTime > nCurrTime)
        {
            ImsMediaTimer::Sleep(nNextTime - nCurrTime);
        }
    }

    while (uplinkBuffers.size())
    {
        uint8_t* data = uplinkBuffers.front();
        delete data;
        uplinkBuffers.pop_front();
    }
    uplinkBuffers.clear();
}