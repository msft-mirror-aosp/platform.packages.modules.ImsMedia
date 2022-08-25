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

#include <SocketReaderNode.h>
#include <ImsMediaTrace.h>
#include <thread>

SocketReaderNode::SocketReaderNode(BaseSessionCallback* callback) :
        BaseNode(callback),
        mLocalFd(0)
{
}

SocketReaderNode::~SocketReaderNode()
{
    IMLOGD1("[~SocketReaderNode] queue size[%d]", GetDataCount());
}

kBaseNodeId SocketReaderNode::GetNodeId()
{
    return kNodeIdSocketReader;
}

ImsMediaResult SocketReaderNode::Start()
{
    IMLOGD1("[Start] media[%d]", mMediaType);
    mSocket = ISocket::GetInstance(mLocalAddress.port, mPeerAddress.ipAddress, mPeerAddress.port);

    if (mSocket == NULL)
    {
        IMLOGE0("[Start] can't create socket instance");
        return RESULT_NOT_READY;
    }

    // set socket local/peer address here
    mSocket->SetLocalEndpoint(mLocalAddress.ipAddress, mLocalAddress.port);
    mSocket->SetPeerEndpoint(mPeerAddress.ipAddress, mPeerAddress.port);

    if (mSocket->Open(mLocalFd) != true)
    {
        IMLOGE0("[Start] can't open socket");
        mSocketOpened = false;
        return RESULT_PORT_UNAVAILABLE;
    }

    mSocket->Listen(this);
    mSocketOpened = true;
    mNodeState = kNodeStateRunning;
    return RESULT_SUCCESS;
}

void SocketReaderNode::Stop()
{
    IMLOGD1("[Stop] media[%d]", mMediaType);
    std::lock_guard<std::mutex> guard(mMutex);

    if (mSocket != NULL)
    {
        mSocket->Listen(NULL);

        if (mSocketOpened)
        {
            mSocket->Close();
        }

        ISocket::ReleaseInstance(mSocket);
        mSocket = NULL;
        mSocketOpened = false;
    }

    mNodeState = kNodeStateStopped;
}

void SocketReaderNode::ProcessData()
{
    std::lock_guard<std::mutex> guard(mMutex);
    uint8_t* data = NULL;
    uint32_t dataSize = 0;
    uint32_t timeStamp = 0;
    bool bMark = false;
    uint32_t seqNum = 0;
    ImsMediaSubType subtype;
    ImsMediaSubType dataType;

    while (GetData(&subtype, &data, &dataSize, &timeStamp, &bMark, &seqNum, &dataType))
    {
        IMLOGD_PACKET2(
                IM_PACKET_LOG_SOCKET, "[ProcessData] media[%d], size[%d]", mMediaType, dataSize);

        SendDataToRearNode(MEDIASUBTYPE_UNDEFINED, (uint8_t*)data, dataSize, 0, 0, 0);
        DeleteData();
    }
}

bool SocketReaderNode::IsRunTime()
{
    return false;
}

bool SocketReaderNode::IsSourceNode()
{
    return true;
}

void SocketReaderNode::SetConfig(void* config)
{
    if (config == NULL)
    {
        return;
    }

    RtpConfig* pConfig = reinterpret_cast<RtpConfig*>(config);

    if (mProtocolType == RTP)
    {
        mPeerAddress = RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort());
    }
    else if (mProtocolType == RTCP)
    {
        mPeerAddress =
                RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort() + 1);
    }
}

bool SocketReaderNode::IsSameConfig(void* config)
{
    if (config == NULL)
    {
        return true;
    }

    RtpConfig* pConfig = reinterpret_cast<RtpConfig*>(config);
    RtpAddress peerAddress;

    if (mProtocolType == RTP)
    {
        peerAddress = RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort());
    }
    else if (mProtocolType == RTCP)
    {
        peerAddress = RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort() + 1);
    }

    return (mPeerAddress == peerAddress);
}

void SocketReaderNode::OnReadDataFromSocket()
{
    int nLen = mSocket->ReceiveFrom(mBuffer, DEFAULT_MTU);

    if (nLen > 0)
    {
        IMLOGD_PACKET3(IM_PACKET_LOG_SOCKET,
                "[OnReadDataFromSocket] media[%d], data size[%d], queue size[%d]", mMediaType, nLen,
                GetDataCount());
        std::lock_guard<std::mutex> guard(mMutex);
        OnDataFromFrontNode(MEDIASUBTYPE_UNDEFINED, mBuffer, nLen, 0, 0, 0);
    }
}

void SocketReaderNode::SetLocalFd(int fd)
{
    mLocalFd = fd;
}

void SocketReaderNode::SetLocalAddress(const RtpAddress address)
{
    mLocalAddress = address;
}

void SocketReaderNode::SetPeerAddress(const RtpAddress address)
{
    mPeerAddress = address;
}