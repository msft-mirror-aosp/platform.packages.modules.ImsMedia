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

#ifndef IAUDIO_SOURCE_NODE_H_INCLUDED
#define IAUDIO_SOURCE_NODE_H_INCLUDED

#include <BaseNode.h>
#include <ImsMediaAudioSource.h>
#include <IFrameCallback.h>

/**
 * @brief This class is interface between audio device and ims media packetization node
 */
class IAudioSourceNode : public BaseNode, IFrameCallback
{
private:
    IAudioSourceNode();
    ~IAudioSourceNode();

public:
    /**
     * @brief Creates the instance of IAudioSourceNode
     *
     * @return BaseNode* The node of instance created
     */
    static BaseNode* GetInstance();

    /**
     * @brief Release the instance of iAudioSourceNode
     *
     * @param pNode The node to release
     */
    static void ReleaseInstance(BaseNode* pNode);
    virtual BaseNodeID GetNodeID();
    virtual ImsMediaResult Start();
    virtual void Stop();
    virtual bool IsRunTime();
    virtual bool IsSourceNode();
    virtual void SetConfig(void* config);
    virtual bool IsSameConfig(void* config);

    /**
     * @brief Uplink callback of audio invoked when the audio read the valid audio frames from the
     * device
     *
     * @param buffer The data frame
     * @param size The size of the data frame
     * @param timestamp The timestamp of the data in milliseconds unit.
     * @param flag The flags of the frame
     */
    void onDataFrame(uint8_t* buffer, uint32_t size, int64_t timestamp, uint32_t flag);

public:
    bool mFirstFrame;
    std::unique_ptr<ImsMediaAudioSource> mAudioSource;
    int32_t mCodecType;
    uint32_t mMode;
    uint32_t mPtime;
    kEvsBandwidth mEvsBandwidth;
    int32_t mSamplingRate;
    int32_t mEvsChAwOffset;
};

#endif
