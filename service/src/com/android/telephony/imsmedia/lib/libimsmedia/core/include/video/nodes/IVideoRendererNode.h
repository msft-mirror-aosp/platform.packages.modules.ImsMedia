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
    IVideoRendererNode(BaseSessionCallback* callback = nullptr);
    virtual ~IVideoRendererNode();
    virtual kBaseNodeId GetNodeId();
    virtual ImsMediaResult Start();
    virtual void Stop();
    virtual bool IsRunTime();
    virtual bool IsSourceNode();
    virtual void SetConfig(void* config);
    virtual bool IsSameConfig(void* config);
    virtual void ProcessData();

    /**
     * @brief Updates display surface
     *
     * @param window surface buffer to update
     */
    void UpdateSurface(ANativeWindow* window);

    /**
     * @brief Update network round trip time delay to the VideoJitterBuffer
     *
     * @param delay time delay in ntp timestamp unit
     */
    void UpdateRoundTripTimeDelay(int32_t delay);

    /**
     * @brief Set the packet loss monitoring duration and packet loss rate threshold
     *
     * @param time The time duration of milliseconds unit to monitor the packet loss
     * @param rate The packet loss rate threshold in the monitoring duration range
     */
    void SetPacketLossParam(uint32_t time, uint32_t rate);

private:
    bool IsIntraFrame(uint8_t* pbBuffer, uint32_t nBufferSize);
    bool IsConfigFrame(uint8_t* pbBuffer, uint32_t nBufferSize, uint32_t* nBufferOffset = nullptr);
    bool IsSps(uint8_t* pbBuffer, uint32_t nBufferSize, uint32_t* nBufferOffset = nullptr);
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
    void NotifyPeerDimensionChanged();

    std::unique_ptr<ImsMediaVideoRenderer> mVideoRenderer;
    ANativeWindow* mWindow;
    ImsMediaCondition mCondition;
    std::mutex mMutex;
    int32_t mCodecType;
    uint32_t mWidth;
    uint32_t mHeight;
    int8_t mSamplingRate;
    int32_t mCvoValue;
    uint8_t mConfigBuffer[MAX_CONFIG_INDEX][MAX_CONFIG_LEN];
    uint32_t mConfigLen[MAX_CONFIG_INDEX];
    uint8_t mBuffer[MAX_RTP_PAYLOAD_BUFFER_SIZE];
    uint32_t mDeviceOrientation;
    bool mFirstFrame;
    ImsMediaSubType mSubtype;
    uint32_t mFramerate;
    uint32_t mWaitIntraFrame;
    uint32_t mLossDuration;
    uint32_t mLossRateThreshold;
};

#endif