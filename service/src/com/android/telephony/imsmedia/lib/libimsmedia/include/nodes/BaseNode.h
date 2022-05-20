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

#ifndef BASE_NODE_H
#define BASE_NODE_H

#include <stdint.h>
#include <BaseNodeID.h>
#include <ImsMediaDataQueue.h>
#include <BaseSessionCallback.h>
#include <StreamSchedulerCallback.h>

#define MAX_AUDIO_PAYLOAD_SIZE 1500
#define MAX_FRAME_IN_PACKET    (MAX_AUDIO_PAYLOAD_SIZE - 1) / 32

enum kBaseNodeState
{
    /* the state after stop method called normally*/
    kNodeStateStopped,
    /* the state after start without error*/
    kNodeStateRunning,
};

/*!
 *    @class        BaseNode
 */
class BaseNode
{
protected:
    BaseNode();
    virtual ~BaseNode();

public:
    static BaseNode* Load(BaseNodeID eID, BaseSessionCallback* callback = NULL);
    static void UnLoad(BaseNode* pNode);
    void SetSessionCallback(BaseSessionCallback* callback);
    void SetSchedulerCallback(std::shared_ptr<StreamSchedulerCallback> callback);
    void ConnectRearNode(BaseNode* pRearNode);
    void DisconnectFrontNode(BaseNode* pFrontNode);
    void DisconnectRearNode(BaseNode* pRearNode);
    std::list<BaseNode*>* GetFrontNodeList();
    std::list<BaseNode*>* GetRearNodeList();
    void ConnectRearNodeList(std::list<BaseNode*>* pRearNodeList);
    void ClearDataQueue();
    // Child Node should implements methods below.
    virtual BaseNodeID GetNodeID() = 0;
    virtual ImsMediaResult Start() = 0;
    virtual void Stop() = 0;
    virtual bool IsRunTime() = 0;
    virtual bool IsSourceNode() = 0;
    virtual void SetConfig(void* config);
    virtual bool IsSameConfig(void* config);
    virtual ImsMediaResult UpdateConfig(void* config);
    // Scheduler Interface
    virtual void ProcessData();
    const char* GetNodeName();
    void SetMediaType(ImsMediaType eType);
    ImsMediaType GetMediaType();
    // Graph Interface
    kBaseNodeState GetState();
    // Methods for Child Node
    // Child Node can/should use methods below to access the queue.
    virtual uint32_t GetDataCount();
    virtual bool GetData(ImsMediaSubType* psubtype, uint8_t** ppData, uint32_t* pnDataSize,
            uint32_t* pnTimestamp, bool* pbMark, uint32_t* pnSeqNum, ImsMediaSubType* pnDataType);
    virtual void DeleteData();
    void SendDataToRearNode(ImsMediaSubType subtype, uint8_t* pData, uint32_t nDataSize,
            uint32_t nTimestamp, bool bMark, uint32_t nSeqNum,
            ImsMediaSubType nDataType = ImsMediaSubType::MEDIASUBTYPE_UNDEFINED);
    // front node call this method to send data
    virtual void OnDataFromFrontNode(ImsMediaSubType subtype, uint8_t* pData, uint32_t nDataSize,
            uint32_t nTimestamp, bool bMark, uint32_t nSeqNum,
            ImsMediaSubType nDataType = ImsMediaSubType::MEDIASUBTYPE_UNDEFINED);

protected:
    std::shared_ptr<StreamSchedulerCallback> mScheduler;
    BaseSessionCallback* mCallback;
    kBaseNodeState mNodeState;
    ImsMediaDataQueue mDataQueue;
    std::list<BaseNode*> mFrontNodeList;
    std::list<BaseNode*> mRearNodeList;
    ImsMediaType mMediaType;
};

#endif