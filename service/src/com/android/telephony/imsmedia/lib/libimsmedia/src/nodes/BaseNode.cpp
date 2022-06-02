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
#include <ImsMediaAudioNodeList.h>
#include <ImsMediaNodeList.h>
#include <ImsMediaTrace.h>
#include <ImsMediaVideoNodeList.h>
#include <stdlib.h>

tNodeListEntry gNodeList[] = {
        {NODEID_SOCKETWRITER, "NODEID_SOCKETWRITER", SocketWriterNode::GetInstance,
         SocketWriterNode::ReleaseInstance},
        {NODEID_SOCKETREADER, "NODEID_SOCKETREADER", SocketReaderNode::GetInstance,
         SocketReaderNode::ReleaseInstance},
        {NODEID_RTPENCODER, "NODEID_RTPENCODER", RtpEncoderNode::GetInstance,
         RtpEncoderNode::ReleaseInstance},
        {NODEID_RTPDECODER, "NODEID_RTPDECODER", RtpDecoderNode::GetInstance,
         RtpDecoderNode::ReleaseInstance},
        {NODEID_RTCPENCODER, "NODEID_RTCPENCODER", RtcpEncoderNode::GetInstance,
         RtcpEncoderNode::ReleaseInstance},
        {NODEID_RTCPDECODER, "NODEID_RTCPDECODER", RtcpDecoderNode::GetInstance,
         RtcpDecoderNode::ReleaseInstance},
        {NODEID_AUDIOSOURCE, "NODEID_AUDIOSOURCE", IAudioSourceNode::GetInstance,
         IAudioSourceNode::ReleaseInstance},
        {NODEID_AUDIOPLAYER, "NODEID_AUDIOPLAYER", IAudioPlayerNode::GetInstance,
         IAudioPlayerNode::ReleaseInstance},
        {NODEID_DTMFENCODER, "NODEID_DTMFENCODER", DtmfEncoderNode::GetInstance,
         DtmfEncoderNode::ReleaseInstance},
        {NODEID_DTMFSENDER, "NODEID_DTMFSENDER", DtmfSenderNode::GetInstance,
         DtmfSenderNode::ReleaseInstance},
        {NODEID_RTPPAYLOAD_ENCODER_AUDIO, "NODEID_RTPPAYLOAD_ENCODER_AUDIO",
         AudioRtpPayloadEncoderNode::GetInstance,
         AudioRtpPayloadEncoderNode::ReleaseInstance},
        {NODEID_RTPPAYLOAD_DECODER_AUDIO, "NODEID_RTPPAYLOAD_DECODER_AUDIO",
         AudioRtpPayloadDecoderNode::GetInstance,
         AudioRtpPayloadDecoderNode::ReleaseInstance},
        {NODEID_VIDEOSOURCE, "NODEID_VIDEOSOURCE", IVideoSourceNode::GetInstance,
         IVideoSourceNode::ReleaseInstance},
        {NODEID_VIDEORENDERER, "NODEID_VIDEORENDERER", IVideoRendererNode::GetInstance,
         IVideoRendererNode::ReleaseInstance},
        {NODEID_RTPPAYLOAD_ENCODER_VIDEO, "NODEID_RTPPAYLOAD_ENCODER_VIDEO",
         VideoRtpPayloadEncoderNode::GetInstance,
         VideoRtpPayloadEncoderNode::ReleaseInstance},
        {NODEID_RTPPAYLOAD_DECODER_VIDEO, "NODEID_RTPPAYLOAD_DECODER_VIDEO",
         VideoRtpPayloadDecoderNode::GetInstance,
         VideoRtpPayloadDecoderNode::ReleaseInstance},
        {NODEID_MAX, "",   NULL, NULL},
};

static const char* gNullNodeName = "NODEID_NULL";
static uint32_t g_NumOfNodeList = 0;
uint32_t GetNumOfNodeList()
{
    uint32_t i;
    if (g_NumOfNodeList > 0)
        return g_NumOfNodeList;
    for (i = 0; i < NODEID_MAX; i++)
    {
        if (gNodeList[i].NodeID == NODEID_MAX)
            break;
    }
    g_NumOfNodeList = i;
    return g_NumOfNodeList;
}

BaseNode::BaseNode()
{
    mScheduler = NULL;
    mCallback = NULL;
    mNodeState = kNodeStateStopped;
    mFrontNode = NULL;
    mRearNode = NULL;
    mMediaType = IMS_MEDIA_AUDIO;
}

BaseNode::~BaseNode()
{
    mNodeState = kNodeStateStopped;
}

BaseNode* BaseNode::Load(BaseNodeID eID, BaseSessionCallback* callback)
{
    BaseNode* pNode = NULL;
    uint32_t nNumOfTotalNode = GetNumOfNodeList();
    uint32_t i;
    IMLOGD1("eID[%d]", eID);
    for (i = 0; i < nNumOfTotalNode; i++)
    {
        if (gNodeList[i].NodeID == eID)
        {
            pNode = gNodeList[i].GetInstance();
            pNode->SetSessionCallback(callback);
            break;
        }
    }
    if (pNode == NULL)
    {
        IMLOGE1("Can't Load Node [%d]", eID);
    }
    else
    {
        IMLOGD1("success [%s]", pNode->GetNodeName());
    }
    return pNode;
}

void BaseNode::UnLoad(BaseNode* pNode)
{
    BaseNodeID eID = pNode->GetNodeID();
    uint32_t nNumOfTotalNode = GetNumOfNodeList();
    uint32_t i;
    for (i = 0; i < nNumOfTotalNode; i++)
    {
        if (gNodeList[i].NodeID == eID)
        {
            // disconnect front/rear nodes
            pNode->DisconnectFrontNode(pNode->GetFrontNode());
            pNode->DisconnectRearNode(pNode->GetRearNode());
            // delete object
            gNodeList[i].DeleteInstance(pNode);
        }
    }
}

void BaseNode::SetSessionCallback(BaseSessionCallback* callback)
{
    mCallback = callback;
}

void BaseNode::SetSchedulerCallback(std::shared_ptr<StreamSchedulerCallback> callback)
{
    mScheduler = callback;
}

void BaseNode::ConnectRearNode(BaseNode* pRearNode)
{
    if (pRearNode == NULL)
    {
        IMLOGE0("Error - pRearNode is NULL");
        return;
    }

    IMLOGD3("type[%d] connect nodes [%s] -> [%s]", mMediaType, GetNodeName(),
            pRearNode->GetNodeName());
    mRearNode = pRearNode;
    pRearNode->mFrontNode = this;
}

void BaseNode::DisconnectRearNode(BaseNode* pRearNode)
{
    if (pRearNode == NULL)
    {
        mRearNode = NULL;
        return;
    }

    IMLOGD3("DisConnnectRearNode] type[%d] disconnect nodes[%s] from[%s]", mMediaType,
            GetNodeName(), pRearNode->GetNodeName());

    mRearNode = NULL;
    pRearNode->mFrontNode = NULL;
}

void BaseNode::DisconnectFrontNode(BaseNode* pFrontNode)
{
    if (pFrontNode == NULL)
    {
        mFrontNode = NULL;
        return;
    }
    IMLOGD3("DisconnectFrontNode] type[%d] disconnect nodes[%s] from[%s]", mMediaType,
            pFrontNode->GetNodeName(), GetNodeName());
    mFrontNode = NULL;
    pFrontNode->mRearNode = NULL;
}

BaseNode* BaseNode::GetFrontNode()
{
    return mFrontNode;
}

BaseNode* BaseNode::GetRearNode()
{
    return mRearNode;
}

void BaseNode::ClearDataQueue()
{
    mDataQueue.Clear();
}

BaseNodeID BaseNode::GetNodeID()
{
    IMLOGW0("[GetNodeID] Error - base method");
    return NODEID_MAX;
}

void BaseNode::SetConfig(void* config)
{
    (void)config;
    IMLOGW0("[SetConfig] Error - base method");
}

bool BaseNode::IsSameConfig(void* config)
{
    (void)config;
    IMLOGW0("[IsSameConfig] Error - base method");
    return true;
}

ImsMediaResult BaseNode::UpdateConfig(void* config)
{
    // check config items updates
    bool isUpdateNode = false;
    if (IsSameConfig(config))
    {
        IMLOGD0("[UpdateConfig] no update");
        return RESULT_SUCCESS;
    }
    else
    {
        isUpdateNode = true;
    }

    kBaseNodeState prevState = mNodeState;
    if (isUpdateNode && mNodeState == kNodeStateRunning)
    {
        Stop();
    }

    // reset the parameters
    SetConfig(config);

    if (isUpdateNode && prevState == kNodeStateRunning)
    {
        return Start();
    }

    return RESULT_SUCCESS;
}

void BaseNode::ProcessData()
{
    IMLOGE0("ProcessData] Error - base method");
}

const char* BaseNode::GetNodeName()
{
    const char* ret = NULL;
    BaseNodeID eID = GetNodeID();
    uint32_t nNumOfTotalNode = GetNumOfNodeList();
    uint32_t i;

    for (i = 0; i < nNumOfTotalNode; i++)
    {
        if (gNodeList[i].NodeID == eID)
        {
            ret = gNodeList[i].NodeName;
            break;
        }
    }

    if (ret == NULL)
        ret = gNullNodeName;

    return ret;
}

void BaseNode::SetMediaType(ImsMediaType eType)
{
    mMediaType = eType;
}

ImsMediaType BaseNode::GetMediaType()
{
    return mMediaType;
}

// Graph Interface
kBaseNodeState BaseNode::GetState()
{
    return mNodeState;
}

uint32_t BaseNode::GetDataCount()
{
    return mDataQueue.GetCount();
}

bool BaseNode::GetData(ImsMediaSubType* psubtype, uint8_t** ppData, uint32_t* pnDataSize,
        uint32_t* pnTimestamp, bool* pbMark, uint32_t* pnSeqNum, ImsMediaSubType* peDataType)
{
    DataEntry* pEntry;
    if (mDataQueue.Get(&pEntry))
    {
        if (psubtype)
            *psubtype = pEntry->subtype;
        if (ppData)
            *ppData = pEntry->pbBuffer;
        if (pnDataSize)
            *pnDataSize = pEntry->nBufferSize;
        if (pnTimestamp)
            *pnTimestamp = pEntry->nTimestamp;
        if (pbMark)
            *pbMark = pEntry->bMark;
        if (pnSeqNum)
            *pnSeqNum = pEntry->nSeqNum;
        if (peDataType)
            *peDataType = pEntry->eDataType;
        return true;
    }
    else
    {
        if (psubtype)
            *psubtype = MEDIASUBTYPE_UNDEFINED;
        if (ppData)
            *ppData = NULL;
        if (pnDataSize)
            *pnDataSize = 0;
        if (pnTimestamp)
            *pnTimestamp = 0;
        if (pbMark)
            *pbMark = false;
        if (pnSeqNum)
            *pnSeqNum = 0;
        if (peDataType)
            *peDataType = MEDIASUBTYPE_UNDEFINED;
        return false;
    }
}

void BaseNode::DeleteData()
{
    mDataQueue.Delete();
}

void BaseNode::SendDataToRearNode(ImsMediaSubType subtype, uint8_t* pData, uint32_t nDataSize,
        uint32_t nTimestamp, bool bMark, uint32_t nSeqNum, ImsMediaSubType nDataType)
{
    bool nNeedRunCount = false;
    if (mRearNode)
    {
        if (mRearNode->mNodeState == kNodeStateRunning)
        {
            mRearNode->OnDataFromFrontNode(
                    subtype, pData, nDataSize, nTimestamp, bMark, nSeqNum, nDataType);

            if (mRearNode->IsRunTime() == false)
            {
                nNeedRunCount = true;
            }
        }
    }

    if (nNeedRunCount == true && mScheduler != NULL)
    {
        mScheduler->onAwakeScheduler();
    }
}

void BaseNode::OnDataFromFrontNode(ImsMediaSubType subtype, uint8_t* pData, uint32_t nDataSize,
        uint32_t nTimestamp, bool bMark, uint32_t nSeqNum, ImsMediaSubType nDataType)
{
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
