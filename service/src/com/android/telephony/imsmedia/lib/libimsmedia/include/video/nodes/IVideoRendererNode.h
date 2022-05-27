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

#ifndef IVIDEO_RENDERER_NODE_H_INCLUDED
#define IVIDEO_RENDERER_NODE_H_INCLUDED

#include <BaseNode.h>
#include <JitterBufferControlNode.h>
#include <IImsMediaThread.h>
#include <ImsMediaCondition.h>
#include <ImsMediaVideoRenderer.h>
#include <ImsMediaVideoUtil.h>
#include <android/native_window.h>
#include <mutex>

#define USE_JITTER_BUFFER  // off this definition ONLY for test purpose.
#define DEMON_NTP2MSEC 65.55

/**
 * @brief This class describes an interface between depacketization module and audio device
 */
class IVideoRendererNode : public JitterBufferControlNode
{
public:
    static BaseNode* GetInstance();
    static void ReleaseInstance(BaseNode* pNode);
    virtual BaseNodeID GetNodeID();
    virtual ImsMediaResult Start();
    virtual void Stop();
    virtual bool IsRunTime();
    virtual bool IsSourceNode();
    virtual void SetConfig(void* config);
    virtual bool IsSameConfig(void* config);
    virtual void ProcessData();
    void UpdateSurface(ANativeWindow* window);

protected:
    IVideoRendererNode();
    virtual ~IVideoRendererNode();

private:
    bool IsIntraFrame(uint8_t* pbBuffer, uint32_t nBufferSize);
    bool IsConfigFrame(uint8_t* pbBuffer, uint32_t nBufferSize, uint32_t* nBufferOffset = NULL);
    bool IsSps(uint8_t* pbBuffer, uint32_t nBufferSize, uint32_t* nBufferOffset = NULL);
    void SaveConfigFrame(uint8_t* pbBuffer, uint32_t nBufferSize, uint32_t type);
    /**
     * @brief Remove Access Uint Delimiter Nal Unit.
     *
     * @param pInBuffer
     * @param nInBufferSize
     * @param pOutBuffer
     * @param pOutBufferSize
     * @return true
     * @return false
     */
    bool RemoveAUDNalUnit(uint8_t* pInBuffer, uint32_t nInBufferSize, uint8_t** pOutBuffer,
            uint32_t* pOutBufferSize);
    void CheckResolution(uint32_t nWidth, uint32_t nHeight);
    ImsMediaResult ParseAvcSps(uint8_t* pbBuffer, uint32_t nBufferSize, tCodecConfig* pInfo);
    ImsMediaResult ParseHevcSps(uint8_t* pbBuffer, uint32_t nBufferSize, tCodecConfig* pInfo);
    void QueueConfigFrame(uint32_t timestamp);

    std::unique_ptr<ImsMediaVideoRenderer> mVideoRenderer;
    ANativeWindow* mWindow;
    ImsMediaCondition mCond;
    std::mutex mMutex;
    int32_t mCodecType;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mSamplingRate;
    int32_t mCvoValue;
    uint8_t mConfigBuffer[MAX_CONFIG_INDEX][MAX_CONFIG_LEN];
    uint32_t mConfigLen[MAX_CONFIG_INDEX];
    uint32_t mDeviceOrientation;
    bool mFirstFrame;
    ImsMediaSubType mSubtype;
    uint32_t mFramerate;
    uint32_t mWaitIntraFrame;
};

#endif