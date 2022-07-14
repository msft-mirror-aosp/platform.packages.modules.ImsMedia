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

#include <SocketWriterNode.h>
#include <ImsMediaTrace.h>

SocketWriterNode::SocketWriterNode(BaseSessionCallback* callback) :
        BaseNode(callback)
{
    mSocket = NULL;
    mbSocketOpened = false;
    mDisableSocket = false;
}

SocketWriterNode::~SocketWriterNode()
{
    if (mSocket != NULL)
    {
        Stop();
    }
}

kBaseNodeId SocketWriterNode::GetNodeId()
{
    return kNodeIdSocketWriter;
}

ImsMediaResult SocketWriterNode::Start()
{
    IMLOGD0("[Start]");
    mSocket = ISocket::GetInstance(mLocalAddress.port, mPeerAddress.ipAddress, mPeerAddress.port);

    if (mSocket == NULL)
    {
        IMLOGE0("[Start] can't create socket instance");
        mbSocketOpened = false;
        return RESULT_NOT_READY;
    }

    // set local/peer address here
    mSocket->SetLocalEndpoint(mLocalAddress.ipAddress, mLocalAddress.port);
    mSocket->SetPeerEndpoint(mPeerAddress.ipAddress, mPeerAddress.port);
    mSocket->SetSocketOpt(SOCKET_OPT_IP_QOS, mDscp);

    if (mSocket->Open(mLocalFd) == false)
    {
        IMLOGE0("[Start] can't open socket");
        mbSocketOpened = false;
        return RESULT_PORT_UNAVAILABLE;
    }

    mbSocketOpened = true;
    mNodeState = kNodeStateRunning;
    return RESULT_SUCCESS;
}

void SocketWriterNode::Stop()
{
    IMLOGD0("[Stop]");
    if (mSocket != NULL)
    {
        mSocket->Close();
        ISocket::ReleaseInstance(mSocket);
        mSocket = NULL;
        mbSocketOpened = false;
    }
    mNodeState = kNodeStateStopped;
}

bool SocketWriterNode::IsRunTime()
{
    return true;
}

bool SocketWriterNode::IsSourceNode()
{
    return true;
}

void SocketWriterNode::SetConfig(void* config)
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

    mDscp = pConfig->getDscp();
}

bool SocketWriterNode::IsSameConfig(void* config)
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

    return (mPeerAddress == peerAddress && mDscp == pConfig->getDscp());
}

void SocketWriterNode::OnDataFromFrontNode(ImsMediaSubType subtype, uint8_t* pData,
        uint32_t nDataSize, uint32_t nTimestamp, bool bMark, uint32_t nSeqNum,
        ImsMediaSubType nDataType)
{
    (void)nDataType;
    (void)bMark;

    if (mDisableSocket == true && subtype != MEDIASUBTYPE_RTCPPACKET_BYE)
    {
        IMLOGW3("[OnDataFromFrontNode] media[%d] subtype[%d] socket is disabled, bytes[%d]",
                mMediaType, subtype, nDataSize);
    }

    IMLOGD_PACKET3(IM_PACKET_LOG_SOCKET, "[OnDataFromFrontNode] TS[%d], SeqNum[%u], size[%u]",
            nTimestamp, nSeqNum, nDataSize);

    if (mSocket == NULL)
    {
        return;
    }

    mSocket->SendTo(pData, nDataSize);
}

void SocketWriterNode::SetLocalFd(int fd)
{
    mLocalFd = fd;
}

void SocketWriterNode::SetLocalAddress(const RtpAddress address)
{
    mLocalAddress = address;
}

void SocketWriterNode::SetPeerAddress(const RtpAddress address)
{
    mPeerAddress = address;
}