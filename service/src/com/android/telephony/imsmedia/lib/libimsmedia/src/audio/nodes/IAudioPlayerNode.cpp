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

#include <IAudioPlayerNode.h>
#include <ImsMediaAudioPlayer.h>
#include <ImsMediaTrace.h>
#include <ImsMediaTimer.h>
#include <AudioConfig.h>
#include <string.h>

IAudioPlayerNode::IAudioPlayerNode()
    : JitterBufferControlNode(IMS_MEDIA_AUDIO){
    std::unique_ptr<ImsMediaAudioPlayer> track(new ImsMediaAudioPlayer());
    mAudioPlayer = std::move(track);
}

IAudioPlayerNode::~IAudioPlayerNode() {

}

BaseNode* IAudioPlayerNode::GetInstance() {
    return new IAudioPlayerNode();
}

void IAudioPlayerNode::ReleaseInstance(BaseNode* pNode) {
    delete (IAudioPlayerNode*)pNode;
}

BaseNodeID IAudioPlayerNode::GetNodeID() {
    return BaseNodeID::NODEID_AUDIOPLAYER;
}

ImsMediaResult IAudioPlayerNode::Start() {
    IMLOGD2("[Start] codec[%d], mode[%d]", mCodecType, mMode);
    if (mJitterBuffer) {
        mJitterBuffer->SetCodecType(mCodecType);
    }

    //reset the jitter
    Reset();
    mMutex.lock();
    if (mAudioPlayer) {
        mAudioPlayer->SetCodec(mCodecType);
        mAudioPlayer->SetCodecMode(mMode);
        mAudioPlayer->Start();
    }
    mFirstFrame = false;
    mMutex.unlock();
    mNodeState = NODESTATE_RUNNING;
    StartThread();
    return RESULT_SUCCESS;
}

void IAudioPlayerNode::Stop() {
    IMLOGD0("[Stop]");
    mMutex.lock();
    if (mAudioPlayer) {
        mAudioPlayer->Stop();
    }
    StopThread();
    mMutex.unlock();
    mCond.wait();
    mNodeState = NODESTATE_STOPPED;
}

bool IAudioPlayerNode::IsRunTime() {
    return true;
}

bool IAudioPlayerNode::IsSourceNode() {
    return false;
}

void IAudioPlayerNode::SetConfig(void* config) {
    AudioConfig *pConfig = reinterpret_cast<AudioConfig*>(config);
    if (pConfig != NULL) {
        SetCodec(pConfig->getCodecType());
        if (mCodecType == AUDIO_AMR || mCodecType == AUDIO_AMR_WB) {
            SetCodecMode(pConfig->getAmrParams().getAmrMode());
        }
        //testing parameter
        SetJitterBufferSize(4, 4, 9);
        SetJitterOptions(80, 1, (double)25 / 10, true, true);
    }
}

bool IAudioPlayerNode::IsSameConfig(void* config) {
    if (config == NULL) return true;
    AudioConfig* pConfig = reinterpret_cast<AudioConfig*>(config);

    if (mCodecType == pConfig->getCodecType()) {
        if (mCodecType == AUDIO_AMR || mCodecType == AUDIO_AMR_WB) {
            if (mMode == pConfig->getAmrParams().getAmrMode()) {
                return true;
            }
        } else if (mCodecType == AUDIO_EVS) {
            if (mMode == pConfig->getEvsParams().getEvsMode()) {
                return true;
            }
        }
    }

    //TBD later jitter configuration compare
    return false;
}

void IAudioPlayerNode::SetCodec(int32_t type) {
    switch (type) {
        case AudioConfig::CODEC_AMR:
            mCodecType = AUDIO_AMR;
            break;
        case AudioConfig::CODEC_AMR_WB:
            mCodecType = AUDIO_AMR_WB;
            break;
        case AudioConfig::CODEC_EVS:
            mCodecType = AUDIO_EVS;
            break;
        case AudioConfig::CODEC_PCMA:
            mCodecType = AUDIO_G711_PCMA;
            break;
        case AudioConfig::CODEC_PCMU:
            mCodecType = AUDIO_G711_PCMU;
            break;
        default:
            break;
    }
}

void IAudioPlayerNode::SetCodecMode(uint32_t mode) {
    mMode = mode;
}

void* IAudioPlayerNode::run() {
    ImsMediaSubType subtype = MEDIASUBTYPE_UNDEFINED;
    ImsMediaSubType datatype = MEDIASUBTYPE_UNDEFINED;
    uint8_t* pData = NULL;
    uint32_t nDataSize = 0;
    uint32_t nTimestamp = 0;
    bool bMark = false;
    uint32_t nSeqNum = 0;
    uint64_t nNextTime = ImsMediaTimer::GetTimeInMicroSeconds();
    uint64_t nCurrTime = 0;
    int64_t nTime = 0;

    while (true) {
        mMutex.lock();
        if (IsThreadStopped()) {
            IMLOGD0("[run] exit");
            mMutex.unlock();
            mCond.signal();
            break;
        }

        if (GetData(&subtype, &pData, &nDataSize, &nTimestamp, &bMark,
            &nSeqNum, &datatype) == true) {
            if (nDataSize != 0) {
                IMLOGD2("[run] write buffer size[%d], timestamp[%u]", nDataSize, nTimestamp);
                if (mAudioPlayer->onDataFrame(pData, nDataSize)) {
                    // send buffering complete message to client
                    if (mFirstFrame == false) {
                        mCallback->SendEvent(EVENT_NOTIFY_FIRST_MEDIA_PACKET_RECEIVED);
                        mFirstFrame = true;
                    }
                }
            }

            DeleteData();
        }

        mMutex.unlock();
        nNextTime += 20000;
        nCurrTime = ImsMediaTimer::GetTimeInMicroSeconds();
        nTime = nNextTime - nCurrTime;

        if (nTime < 0) {
            continue;
        }

        IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO, "[Run] wait[%d]", nTime);
        ImsMediaTimer::USleep(nTime);
    }
    return NULL;
}