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

#include <IVoiceSourceNode.h>
#include <ImsMediaVoiceSource.h>
#include <ImsMediaTrace.h>
#include <string.h>

IVoiceSourceNode::IVoiceSourceNode() {
    std::unique_ptr<ImsMediaVoiceSource> recoder(new ImsMediaVoiceSource());
    mVoiceSource = std::move(recoder);
    m_bFirstFrame = false;
}

IVoiceSourceNode::~IVoiceSourceNode() {
}

BaseNode* IVoiceSourceNode::GetInstance() {
    return new IVoiceSourceNode();
}

void IVoiceSourceNode::ReleaseInstance(BaseNode* pNode) {
    delete (IVoiceSourceNode*)pNode;
}

BaseNodeID IVoiceSourceNode::GetNodeID() {
    return BaseNodeID::NODEID_VOICESOURCE;
}

ImsMediaResult IVoiceSourceNode::Start() {
    IMLOGD2("[Start] codec[%d], mode[%d]", mCodecType, mMode);
    if (mVoiceSource) {
        mVoiceSource->SetAttributionSource(mSource);
        mVoiceSource->SetUplinkCallback(this, IVoiceSourceNode::CB_AudioUplink);
        mVoiceSource->SetCodec(mCodecType);
        mVoiceSource->SetCodecMode(mMode);
        if (mVoiceSource->Start() == true) {
            m_bFirstFrame = false;
        }
    }

    mNodeState = NODESTATE_RUNNING;
    return IMS_MEDIA_OK;
}

void IVoiceSourceNode::Stop() {
    IMLOGD0("[Stop]");
    if (mVoiceSource) {
        mVoiceSource->Stop();
    }
    mNodeState = NODESTATE_STOPPED;
}

bool IVoiceSourceNode::IsRunTime() {
    return true;
}

bool IVoiceSourceNode::IsSourceNode() {
    return true;
}

void IVoiceSourceNode::SetRtpSessionParams(ImsMediaHal::RtpSessionParams* params) {
    mSessionParams = std::make_shared<ImsMediaHal::RtpSessionParams>();
    memcpy(mSessionParams.get(), params, sizeof(ImsMediaHal::RtpSessionParams));
}

void IVoiceSourceNode::SetAttributeSource(android::content::AttributionSourceState& source) {
    mSource = source;
}

void IVoiceSourceNode::SetCodec(eAudioCodecType eCodecType) {
    mCodecType = eCodecType;
}

void IVoiceSourceNode::SetCodecMode(uint32_t mode) {
    mMode = mode;
}

void IVoiceSourceNode::CB_AudioUplink(void* pClient, uint8_t* pBitstream, uint32_t pnSize,
    int64_t pstUsec, uint32_t flag) {
    (void)flag;
    IVoiceSourceNode* client = reinterpret_cast<IVoiceSourceNode*>(pClient);
    if (client != NULL) {
        IMLOGD_PACKET2(IM_PACKET_LOG_AUDIO,
            "[CB_AudioUplink] size[%zu], pts=%" PRId64, pnSize, pstUsec);
        client->SendDataToRearNode(MEDIASUBTYPE_UNDEFINED, pBitstream, pnSize, pstUsec,
            !client->m_bFirstFrame, MEDIASUBTYPE_UNDEFINED);
        if (!client->m_bFirstFrame) {
            client->m_bFirstFrame = true;
        }
    }

}