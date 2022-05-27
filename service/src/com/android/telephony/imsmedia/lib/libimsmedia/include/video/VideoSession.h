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

#ifndef VIDEO_SESSION_H
#define VIDEO_SESSION_H

#include <ImsMediaDefine.h>
#include <BaseSession.h>
#include <VideoStreamGraphRtpTx.h>
#include <VideoStreamGraphRtpRx.h>
#include <VideoStreamGraphRtcp.h>
#include <RtpConfig.h>
#include <android/native_window.h>

class VideoSession : public BaseSession
{
public:
    VideoSession();
    virtual ~VideoSession();
    virtual SessionState getState();
    virtual ImsMediaResult startGraph(void* config);
    // BaseSessionCallback
    virtual void onEvent(int32_t type, uint64_t param1, uint64_t param2);
    ImsMediaResult setPreviewSurface(ANativeWindow* surface);
    ImsMediaResult setDisplaySurface(ANativeWindow* surface);

private:
    VideoStreamGraphRtpTx* mGraphRtpTx;
    VideoStreamGraphRtpRx* mGraphRtpRx;
    VideoStreamGraphRtcp* mGraphRtcp;
};

#endif