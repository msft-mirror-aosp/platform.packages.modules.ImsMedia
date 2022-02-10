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

#include <IVoiceRendererNode.h>
#include <ImsMediaVoiceRenderer.h>
#include <ImsMediaTrace.h>
#include <ImsMediaTimer.h>
#include <string.h>

IVoiceRendererNode::IVoiceRendererNode()
    : JitterBufferControlNode(IMS_MEDIA_AUDIO){
    std::unique_ptr<ImsMediaVoiceRenderer> track(new ImsMediaVoiceRenderer());
    mVoiceRenderer = std::move(track);
}

IVoiceRendererNode::~IVoiceRendererNode() {

}

BaseNode* IVoiceRendererNode::GetInstance() {
    return new IVoiceRendererNode();
}

void IVoiceRendererNode::ReleaseInstance(BaseNode* pNode) {
    delete (IVoiceRendererNode*)pNode;
}

BaseNodeID IVoiceRendererNode::GetNodeID() {
    return BaseNodeID::NODEID_VOICERENDERER;
}

ImsMediaResult IVoiceRendererNode::Start() {
    IMLOGD2("[Start] codec[%d], mode[%d]", mCodecType, mMode);
    if (mJitterBuffer) {
        mJitterBuffer->SetCodecType(mCodecType);
    }

    //reset the jitter
    Reset();

    if (mVoiceRenderer) {
        mVoiceRenderer->SetCodec(mCodecType);
        mVoiceRenderer->SetCodecMode(mMode);
        mVoiceRenderer->Start();
    }
    mFirstFrame = false;
    mNodeState = NODESTATE_RUNNING;
    StartThread();
    return IMS_MEDIA_OK;
}

void IVoiceRendererNode::Stop() {
    IMLOGD0("[Stop]");
    if (mVoiceRenderer) {
        mVoiceRenderer->Stop();
    }
    mMutex.lock();
    StopThread();
    mMutex.unlock();
    mCond.wait();
    mNodeState = NODESTATE_STOPPED;
}

bool IVoiceRendererNode::IsRunTime() {
    return true;
}

bool IVoiceRendererNode::IsSourceNode() {
    return false;
}

void IVoiceRendererNode::SetRtpSessionParams(ImsMediaHal::RtpSessionParams* params) {
    mSessionParams = std::make_shared<ImsMediaHal::RtpSessionParams>();
    memcpy(mSessionParams.get(), params, sizeof(ImsMediaHal::RtpSessionParams));
}

void IVoiceRendererNode::SetCodec(eAudioCodecType eCodecType) {
    mCodecType = eCodecType;
}

void IVoiceRendererNode::SetCodecMode(uint32_t mode) {
    mMode = mode;
}

void* IVoiceRendererNode::run() {
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
                if (mVoiceRenderer->onDataFrame(pData, nDataSize)) {
                    // send buffering complete message to client
                    if (mFirstFrame == false) {
                        //ImsMediaEventHandler::SendEvent("_ResponseHandler", RESPONSEEVENT_NOTIFY);
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