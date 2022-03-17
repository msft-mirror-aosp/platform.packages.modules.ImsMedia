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

#include <BaseNode.h>
#include <BaseNodeID.h>
#include <ImsMediaNodeList.h>
#include <ImsMediaTrace.h>
#include <stdlib.h>

tNodeListEntry gNodeList[] = {
    { NODEID_SOCKETWRITER, "NODEID_SOCKETWRITER", SocketWriterNode::GetInstance,
        SocketWriterNode::ReleaseInstance },
    { NODEID_SOCKETREADER, "NODEID_SOCKETREADER", SocketReaderNode::GetInstance,
        SocketReaderNode::ReleaseInstance },
    { NODEID_RTPENCODER, "NODEID_RTPENCODER", RtpEncoderNode::GetInstance,
        RtpEncoderNode::ReleaseInstance },
    { NODEID_RTPDECODER, "NODEID_RTPDECODER", RtpDecoderNode::GetInstance,
        RtpDecoderNode::ReleaseInstance },
    { NODEID_RTCPENCODER, "NODEID_RTCPENCODER", RtcpEncoderNode::GetInstance,
        RtcpEncoderNode::ReleaseInstance },
    { NODEID_RTCPDECODER, "NODEID_RTCPDECODER", RtcpDecoderNode::GetInstance,
        RtcpDecoderNode::ReleaseInstance },
    { NODEID_VOICESOURCE, "NODEID_VOICESOURCE", IVoiceSourceNode::GetInstance,
        IVoiceSourceNode::ReleaseInstance },
    { NODEID_VOICERENDERER, "NODEID_VOICERENDERER", IVoiceRendererNode::GetInstance,
        IVoiceRendererNode::ReleaseInstance },
    { NODEID_DTMFENCODER, "NODEID_DTMFENCODER", DtmfEncoderNode::GetInstance,
        DtmfEncoderNode::ReleaseInstance },
    { NODEID_DTMFSENDER, "NODEID_DTMFSENDER", DtmfSenderNode::GetInstance,
        DtmfSenderNode::ReleaseInstance },
    { NODEID_RTPPAYLOAD_ENCODER_AUDIO, "NODEID_RTPPAYLOAD_ENCODER_AUDIO",
        AudioRtpPayloadEncoderNode::GetInstance, AudioRtpPayloadEncoderNode::ReleaseInstance },
    { NODEID_RTPPAYLOAD_DECODER_AUDIO, "NODEID_RTPPAYLOAD_DECODER_AUDIO",
        AudioRtpPayloadDecoderNode::GetInstance, AudioRtpPayloadDecoderNode::ReleaseInstance },
    { NODEID_MAX, "", NULL, NULL },
};

static const char* gNullNodeName = "NODEID_NULL";
static uint32_t g_NumOfNodeList = 0;
uint32_t GetNumOfNodeList() {
    uint32_t i;
    if (g_NumOfNodeList > 0) return g_NumOfNodeList;
    for (i = 0; i < NODEID_MAX; i ++) {
        if (gNodeList[i].NodeID == NODEID_MAX) break;
    }
    g_NumOfNodeList = i;
    return g_NumOfNodeList;
}

BaseNode::BaseNode() {
    mNodeState = NODESTATE_STOPPED;
}

BaseNode::~BaseNode() {
    mNodeState = NODESTATE_STOPPED;
}

BaseNode* BaseNode::Load(BaseNodeID eID, BaseSessionCallback* callback) {
    BaseNode* pNode = NULL;
    uint32_t nNumOfTotalNode = GetNumOfNodeList();
    uint32_t i;
    IMLOGD1("eID[%d]", eID);
    for (i = 0; i < nNumOfTotalNode; i ++) {
        if (gNodeList[i].NodeID == eID) {
            pNode = gNodeList[i].GetInstance();
            pNode->SetSessionCallback(callback);
            break;
        }
    }
    if (pNode == NULL) {
        IMLOGE1("Can't Load Node [%d]", eID);
    } else {
        IMLOGD1("success [%s]", pNode->GetNodeName());
    }
    return pNode;
}

void BaseNode::UnLoad(BaseNode* pNode) {
    BaseNodeID eID = pNode->GetNodeID();
    uint32_t nNumOfTotalNode = GetNumOfNodeList();
    uint32_t i;
    for (i = 0 ; i < nNumOfTotalNode ; i ++) {
        if (gNodeList[i].NodeID == eID) {
            // disconnect front/rear nodes
            for (auto&i:*pNode->GetFrontNodeList()) {
                pNode->DisconnectFrontNode(i);
            }
            for (auto&i:*pNode->GetRearNodeList()) {
                pNode->DisconnectRearNode(i);
            }
            // delete object
            gNodeList[i].DeleteInstance(pNode);
        }
    }
}

void BaseNode::SetSessionCallback(BaseSessionCallback* callback) {
    mCallback = callback;
}

void BaseNode::SetSchedulerCallback(std::shared_ptr<StreamSchedulerCallback> callback) {
    mScheduler = callback;
}

void BaseNode::ConnectRearNode(BaseNode *pRearNode) {
    if (pRearNode == NULL) {
        IMLOGE0("Error - pRearNode is NULL");
        return;
    }

    IMLOGD3("type[%d] connect nodes [%s] -> [%s]",
        this->GetMediaType(), this->GetNodeName(), pRearNode->GetNodeName());
    mRearNodeList.push_back(pRearNode);
    pRearNode->mFrontNodeList.push_back(this);
}

void BaseNode::DisconnectRearNode(BaseNode *pRearNode) {
    if (pRearNode == NULL) {
        IMLOGE0("DisConnnectRearNode - pRearNode is NULL");
        return;
    }

    IMLOGD3("DisConnnectRearNode] type[%s] disconnect nodes[%s] from[%s]",
        this->GetMediaType(), this->GetNodeName(), pRearNode->GetNodeName());

    mRearNodeList.pop_front();
    pRearNode->mFrontNodeList.pop_front();
}

void BaseNode::DisconnectFrontNode(BaseNode *pFrontNode) {
    if (pFrontNode == NULL) {
        IMLOGE0("DisconnectFrontNode - pFrontNode is NULL");
        return;
    }
    IMLOGD3("DisconnectFrontNode - type[%s] disconnect nodes[%s] from[%s]",
        this->GetMediaType(), pFrontNode->GetNodeName(), this->GetNodeName());
    mFrontNodeList.pop_front();
    pFrontNode->mRearNodeList.pop_front();
}

std::list<BaseNode*>* BaseNode::GetFrontNodeList() {
    return &mFrontNodeList;
}

std::list<BaseNode*>* BaseNode::GetRearNodeList() {
    return &mRearNodeList;
}

void BaseNode::ConnectRearNodeList(std::list<BaseNode*>* pRearNodeList) {
    if (pRearNodeList == NULL) {
        IMLOGE0("ConnectRearNodeList] Error - pRearNodeList is NULL");
        return;
    }
    for (auto&i:*pRearNodeList) {
        ConnectRearNode(i);
    }
}

void BaseNode::ClearDataQueue() {
    mDataQueue.Clear();
}

void BaseNode::SetConfig(void* config) {
    (void)config;
    IMLOGE0("[SetConfig] Error - base method");
}

bool BaseNode::IsSameConfig(void* config) {
    (void)config;
    IMLOGE0("[IsSameConfig] Error - base method");
    return true;
}

ImsMediaResult BaseNode::UpdateConfig(void* config) {
    //check config items updates
    bool isUpdateNode = false;
    if (IsSameConfig(config)) {
        IMLOGD0("[UpdateConfig] no update");
        return IMS_MEDIA_OK;
    } else {
        isUpdateNode = true;
    }

    BaseNodeState prevState = mNodeState;
    if (isUpdateNode && mNodeState == NODESTATE_RUNNING) {
        Stop();
    }

    //reset the parameters
    SetConfig(config);

    if (isUpdateNode && prevState == NODESTATE_RUNNING) {
        return Start();
    }

    return IMS_MEDIA_OK;
}

void BaseNode::ProcessData() {
    IMLOGE0("ProcessData] Error - base method");
}

const char* BaseNode::GetNodeName() {
    const char* ret = NULL;
    BaseNodeID eID = GetNodeID();
    uint32_t nNumOfTotalNode = GetNumOfNodeList();
    uint32_t i;

    for (i = 0 ; i < nNumOfTotalNode ; i ++) {
        if (gNodeList[i].NodeID == eID) {
            ret = gNodeList[i].NodeName;
            break;
        }
    }

    if (ret == NULL) ret = gNullNodeName;

    return ret;
}

void BaseNode::SetMediaType(ImsMediaType eType) {
    mMediaType = eType;
}

ImsMediaType BaseNode::GetMediaType() {
    return mMediaType;
}

// Graph Interface
BaseNodeState BaseNode::GetState() {
    return mNodeState;
}

uint32_t BaseNode::GetDataCount() {
    return mDataQueue.GetCount();
}

bool BaseNode::GetData(ImsMediaSubType* psubtype, uint8_t** ppData,
    uint32_t* pnDataSize, uint32_t* pnTimestamp, bool *pbMark, uint32_t* pnSeqNum,
    ImsMediaSubType* peDataType) {
    DataEntry* pEntry;
    if (mDataQueue.Get(&pEntry)) {
        if (psubtype) *psubtype = pEntry->subtype;
        if (ppData) *ppData = pEntry->pbBuffer;
        if (pnDataSize) *pnDataSize = pEntry->nBufferSize;
        if (pnTimestamp) *pnTimestamp = pEntry->nTimestamp;
        if (pbMark) *pbMark = pEntry->bMark;
        if (pnSeqNum) *pnSeqNum = pEntry->nSeqNum;
        if (peDataType) *peDataType = pEntry->eDataType;
        return true;
    } else {
        if (psubtype) *psubtype = MEDIASUBTYPE_UNDEFINED;
        if (ppData) *ppData = NULL;
        if (pnDataSize) *pnDataSize = 0;
        if (pnTimestamp) *pnTimestamp = 0;
        if (pbMark) *pbMark = false;
        if (pnSeqNum) *pnSeqNum = 0;
        if (peDataType) *peDataType = MEDIASUBTYPE_UNDEFINED;
        return false;
    }
}

void BaseNode::DeleteData() {
    mDataQueue.Delete();
}

void BaseNode::SendDataToRearNode(ImsMediaSubType subtype, uint8_t* pData,
    uint32_t nDataSize, uint32_t nTimestamp, bool bMark, uint32_t nSeqNum,
    ImsMediaSubType nDataType) {
    bool nNeedRunCount = false;
    for(auto&i:mRearNodeList) {
        if (i->mNodeState == NODESTATE_RUNNING) {
            i->OnDataFromFrontNode(subtype, pData, nDataSize,
                nTimestamp, bMark, nSeqNum, nDataType);
            if (i->IsRunTime() == false) nNeedRunCount = true;
        }
    }
    if (nNeedRunCount == true && mScheduler != NULL) mScheduler->onAwakeScheduler();
}

void BaseNode::OnDataFromFrontNode(ImsMediaSubType subtype,
    uint8_t* pData, uint32_t nDataSize, uint32_t nTimestamp, bool bMark,
    uint32_t nSeqNum, ImsMediaSubType nDataType) {
    DataEntry entry;
    memset(&entry, 0, sizeof(DataEntry));
    entry.pbBuffer = pData;
    entry.nBufferSize = nDataSize;
    entry.nTimestamp = nTimestamp;
    entry.bMark = bMark;
    entry.nSeqNum = nSeqNum;
    entry.eDataType = nDataType;
    entry.subtype = subtype;
    mDataQueue.Add(&entry);
}
