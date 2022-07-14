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

SocketReaderNode::SocketReaderNode(BaseSessionCallback* callback) :
        BaseNode(callback),
        mLocalFd(0)
{
}

SocketReaderNode::~SocketReaderNode() {}

void SocketReaderNode::OnReceiveEnabled()
{
    int nLen = mSocket->ReceiveFrom(mBuffer, 1500);
    if (nLen > 0)
    {
        IMLOGD_PACKET1(IM_PACKET_LOG_SOCKET, "[OnReceiveEnabled] read %d bytes", nLen);
        SendDataToRearNode(MEDIASUBTYPE_UNDEFINED, (uint8_t*)mBuffer, nLen, 0, 0, 0);
    }
}

kBaseNodeId SocketReaderNode::GetNodeId()
{
    return kNodeIdSocketReader;
}

ImsMediaResult SocketReaderNode::Start()
{
    IMLOGD0("[Start]");
    mSocket = ISocket::GetInstance(mLocalAddress.port, mPeerAddress.ipAddress, mPeerAddress.port);

    if (mSocket == NULL)
    {
        IMLOGE0("[Start] can't create socket instance");
        mbSocketOpened = false;
        return RESULT_NOT_READY;
    }

    // set socket local/peer address here
    mSocket->SetLocalEndpoint(mLocalAddress.ipAddress, mLocalAddress.port);
    mSocket->SetPeerEndpoint(mPeerAddress.ipAddress, mPeerAddress.port);

    if (mSocket->Open(mLocalFd) != true)
    {
        IMLOGE0("[Start] can't open socket");
        mbSocketOpened = false;
        return RESULT_PORT_UNAVAILABLE;
    }

    if (mSocket->Listen(this) != true)
    {
        IMLOGE0("[Start] can't listen socket");
        mbSocketOpened = false;
        return RESULT_NOT_READY;
    }

    memset(mBuffer, 0, sizeof(mBuffer));
    mbSocketOpened = true;
    mNodeState = kNodeStateRunning;
    return RESULT_SUCCESS;
}

void SocketReaderNode::Stop()
{
    IMLOGD0("[Stop]");
    if (mSocket != NULL)
    {
        mSocket->Listen(NULL);
        mSocket->Close();
        ISocket::ReleaseInstance(mSocket);
        mSocket = NULL;
        mbSocketOpened = false;
    }
    mNodeState = kNodeStateStopped;
}

bool SocketReaderNode::IsRunTime()
{
    return true;
}

bool SocketReaderNode::IsSourceNode()
{
    return true;
}

void SocketReaderNode::SetConfig(void* config)
{
    if (config == NULL)
        return;
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
        return true;
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

void SocketReaderNode::OnDataFromFrontNode(ImsMediaSubType subtype, uint8_t* pData,
        uint32_t nDataSize, uint32_t nTimestamp, bool bMark, uint32_t nSeqNum,
        ImsMediaSubType nDataType)
{
    (void)nDataType;

    IMLOGD_PACKET3(IM_PACKET_LOG_SOCKET,
            "[OnDataFromFrontNode] type[%d], subtype[%d] before sendto %d bytes", mMediaType,
            subtype, nDataSize);

    if (bMark == true)
    {
        IMLOGD_PACKET2(IM_PACKET_LOG_SOCKET, "[OnDataFromFrontNode] TS[%d], SeqNum[%d]", nTimestamp,
                nSeqNum);
    }

    if (mSocket == NULL)
    {
        return;
    }

    mSocket->SendTo(pData, nDataSize);
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