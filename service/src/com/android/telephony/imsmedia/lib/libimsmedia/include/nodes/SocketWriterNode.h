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

#ifndef SOCKET_WRITER_NODE_H
#define SOCKET_WRITER_NODE_H

#include <BaseNode.h>
#include <ISocket.h>

class SocketWriterNode : public BaseNode
{
public:
    SocketWriterNode(BaseSessionCallback* callback = NULL);
    virtual ~SocketWriterNode();
    virtual kBaseNodeId GetNodeId();
    virtual ImsMediaResult Start();
    virtual void Stop();
    virtual bool IsRunTime();
    virtual bool IsSourceNode();
    virtual void SetConfig(void* config);
    virtual bool IsSameConfig(void* config);
    virtual void OnDataFromFrontNode(ImsMediaSubType subtype, uint8_t* pData, uint32_t nDataSize,
            uint32_t nTimestamp, bool bMark, uint32_t nSeqNum,
            ImsMediaSubType nDataType = ImsMediaSubType::MEDIASUBTYPE_UNDEFINED);
    void SetLocalFd(int fd);
    void SetLocalAddress(const RtpAddress address);
    void SetPeerAddress(const RtpAddress address);
    void SetProtocolType(ImsMediaProtocolType type) { mProtocolType = type; }

private:
    int mLocalFd;
    ISocket* mSocket;
    ImsMediaProtocolType mProtocolType;
    RtpAddress mLocalAddress;
    RtpAddress mPeerAddress;
    int32_t mDscp;
    bool mbSocketOpened;
    bool mDisableSocket;
};

#endif