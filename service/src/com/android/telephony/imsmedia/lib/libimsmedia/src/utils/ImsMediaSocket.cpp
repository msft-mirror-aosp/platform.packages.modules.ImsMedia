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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <ImsMediaSocket.h>
#include <ImsMediaTrace.h>
#include <ImsMediaNetworkUtil.h>

// static valuable
std::list<ImsMediaSocket*> ImsMediaSocket::slistRxSocket;
std::list<ImsMediaSocket*> ImsMediaSocket::slistSocket;
int32_t ImsMediaSocket::sRxSocketCount = 0;
bool ImsMediaSocket::mSocketListUpdated = false;
bool ImsMediaSocket::mbTerminateMonitor = false;
ImsMediaCondition ImsMediaSocket::mConditionExit;
std::mutex ImsMediaSocket::sMutexRxSocket;
std::mutex ImsMediaSocket::sMutexSocketList;
std::mutex ImsMediaSocket::sMutexSocketMonitorThread;

ImsMediaSocket* ImsMediaSocket::GetInstance(
        uint32_t localPort, const char* peerIpAddress, uint32_t peerPort)
{
    ImsMediaSocket* pImsMediaSocket = NULL;
    std::lock_guard<std::mutex> guard(sMutexSocketList);
    for (auto& i : slistSocket)
    {
        if (strcmp(i->GetPeerIPAddress(), peerIpAddress) == 0 && i->GetLocalPort() == localPort &&
                i->GetPeerPort() == peerPort)
        {
            return i;
        }
    }
    pImsMediaSocket = new ImsMediaSocket();
    return pImsMediaSocket;
}

void ImsMediaSocket::ReleaseInstance(ImsMediaSocket* pSocket)
{
    if (pSocket != NULL && pSocket->mRefCount == 0)
    {
        delete pSocket;
    }
}

ImsMediaSocket::ImsMediaSocket()
{
    mListener = NULL;
    mRefCount = 0;
    // for test
    mLocalIPVersion = IPV4;
    mPeerIPVersion = IPV4;
    mLocalPort = 0;
    mPeerPort = 0;
    // mToS = pPortInfo->nTOS;
    mSocketFd = -1;
    memset(mPeerIPBin, 0, sizeof(mPeerIPBin));
    mbReceivingIPFiltering = true;
    IMLOGD0("[ImsMediaSocket] enter");
}

ImsMediaSocket::~ImsMediaSocket()
{
    IMLOGD_PACKET5(IM_PACKET_LOG_SOCKET, "[~ImsMediaSocket] %x, %s:%d %s:%d", this, mLocalIP,
            mLocalPort, mPeerIP, mPeerPort);
}

void ImsMediaSocket::SetLocalEndpoint(const char* ipAddress, const uint32_t port)
{
    std::strcpy(mLocalIP, ipAddress);
    mLocalPort = port;
}

void ImsMediaSocket::SetPeerEndpoint(const char* ipAddress, const uint32_t port)
{
    std::strcpy(mPeerIP, ipAddress);
    mPeerPort = port;

    if (strstr(mPeerIP, ":") == NULL)
    {
        mPeerIPVersion = IPV4;
    }
    else
    {
        mPeerIPVersion = IPV6;
    }

    ImsMediaNetworkUtil::ConvertIPStrToBin(mPeerIP, mPeerIPBin, mPeerIPVersion);
}

int ImsMediaSocket::GetLocalPort()
{
    return mLocalPort;
}
int ImsMediaSocket::GetPeerPort()
{
    return mPeerPort;
}
char* ImsMediaSocket::GetLocalIPAddress()
{
    return mLocalIP;
}
char* ImsMediaSocket::GetPeerIPAddress()
{
    return mPeerIP;
}

bool ImsMediaSocket::Open(int socketFd)
{
    if (socketFd == -1)
        return false;
    IMLOGD5("[Open] %s:%d, %s:%d, nRefCount[%d]", mLocalIP, mLocalPort, mPeerIP, mPeerPort,
            mRefCount);

    if (mRefCount > 0)
    {
        IMLOGD0("[Open] exit - Socket is opened already");
        mRefCount++;
        return true;
    }

    mSocketFd = socketFd;
    sMutexSocketList.lock();
    slistSocket.push_back(this);
    mRefCount++;
    sMutexSocketList.unlock();
    return true;
}

bool ImsMediaSocket::Listen(ISocketListener* listener)
{
    IMLOGD0("[Listen]");
    mListener = listener;
    std::lock_guard<std::mutex> guard(sMutexSocketMonitorThread);
    if (listener != NULL)
    {
        // add socket list, run thread
        sMutexRxSocket.lock();
        slistRxSocket.push_back(this);
        sMutexRxSocket.unlock();

        if (sRxSocketCount == 0)
        {
            StartSocketMonitor();
        }
        else
        {
            mSocketListUpdated = true;
        }
        sRxSocketCount++;
        IMLOGD1("[Listen] add sRxSocketCount[%d]", sRxSocketCount);
    }
    else
    {
        sMutexRxSocket.lock();
        slistRxSocket.remove(this);
        sMutexRxSocket.unlock();
        sRxSocketCount--;

        if (sRxSocketCount <= 0)
        {
            StopSocketMonitor();
            sRxSocketCount = 0;
        }
        else
        {
            mSocketListUpdated = true;
        }
        IMLOGD1("[Listen] remove RxSocketCount[%d]", sRxSocketCount);
    }
    return true;
}

uint32_t ImsMediaSocket::SendTo(uint8_t* pData, uint32_t nDataSize)
{
    uint32_t nLen;
    IMLOGD_PACKET2(IM_PACKET_LOG_SOCKET, "[SendTo] fd[%d],[%d] bytes", mSocketFd, nDataSize);

    if (nDataSize == 0)
        return 0;

    struct sockaddr_in stAddr4;
    struct sockaddr_in6 stAddr6;
    struct sockaddr* pstSockAddr = NULL;
    socklen_t nSockAddrLen = 0;

    if (mPeerIPVersion == IPV4)
    {
        // IPv4
        nSockAddrLen = sizeof(stAddr4);
        memset(&stAddr4, 0, nSockAddrLen);
        stAddr4.sin_family = AF_INET;
        stAddr4.sin_port = htons(mPeerPort);
        // stAddr4.sin_addr.s_addr = inet_addr(mPeerIP);
        if (inet_pton(AF_INET, mPeerIP, &(stAddr4.sin_addr.s_addr)) != 1)
        {
            IMLOGE1("[ImsMediaSocket:SendTo] IPv4[%s]", mPeerIP);
            return 0;
        }

        pstSockAddr = (struct sockaddr*)&stAddr4;
    }
    else
    {
        // IPv6
        nSockAddrLen = sizeof(stAddr6);
        memset(&stAddr6, 0, nSockAddrLen);
        stAddr6.sin6_family = AF_INET6;
        stAddr6.sin6_port = htons(mPeerPort);

        if (inet_pton(AF_INET6, mPeerIP, &(stAddr6.sin6_addr.s6_addr)) != 1)
        {
            IMLOGE1("[ImsMediaSocket:SendTo] Ipv6[%s]", mPeerIP);
            return 0;
        }

        pstSockAddr = (struct sockaddr*)&stAddr6;
    }

    nLen = sendto(mSocketFd, (const char*)pData, (size_t)nDataSize, 0, pstSockAddr, nSockAddrLen);

    if (nLen < 0)
    {
        IMLOGE4("[ImsMediaSocket:SendTo] FAILED nLen(%d), nDataSize(%d) failed (%d, %s)", nLen,
                nDataSize, errno, strerror(errno));
    }

    return nLen;
}

uint32_t ImsMediaSocket::ReceiveFrom(uint8_t* pData, uint32_t nBufferSize)
{
    uint32_t nLen;
    struct sockaddr* pstSockAddr = NULL;
    socklen_t nSockAddrLen = 0;
    sockaddr_storage ss;
    pstSockAddr = reinterpret_cast<sockaddr*>(&ss);

    nLen = recvfrom(mSocketFd, pData, nBufferSize, 0, pstSockAddr, &nSockAddrLen);

    if (nLen > 0)
    {
        static char pSourceIP[MAX_IP_LEN];
        memset(pSourceIP, 0, sizeof(pSourceIP));
        // TODO : add filtering operation with peer ip address and port
        IMLOGD_PACKET1(IM_PACKET_LOG_SOCKET, "[ReceiveFrom] str_len[%d]", nLen);
    }
    else if (EWOULDBLOCK == errno)
    {
        IMLOGE0("[ReceiveFrom], WBlock");
    }
    else
    {
        IMLOGE0("[ReceiveFrom] Fail");
    }

    return nLen;
}

void ImsMediaSocket::Close()
{
    IMLOGD1("[Close] enter, nRefCount[%d]", mRefCount);
    mRefCount--;
    if (mRefCount > 0)
    {
        IMLOGD0("[Close] exit - Socket is used");
        return;
    }

    // close(mSocketFd);
    std::lock_guard<std::mutex> guard(sMutexSocketList);
    slistSocket.remove(this);
    IMLOGD0("[Close] exit");
}

bool ImsMediaSocket::SetSocketOpt(eSocketOpt nOption, uint32_t nOptionValue)
{
    if (mSocketFd == -1)
    {
        IMLOGD0("[SetSocketOpt] socket handle is null..");
        return false;
    }

    switch (nOption)
    {
        case SOCKET_OPT_IP_QOS:
            mToS = nOptionValue;
            if (mLocalIPVersion == IPV4)
            {
                if (-1 == setsockopt(mSocketFd, IPPROTO_IP, IP_TOS, (void*)&mToS, sizeof(uint32_t)))
                {
                    IMLOGE0("[SetSocketOpt] IP_TOS - IPv4");
                    return false;
                }
            }
            else
            {
                if (-1 ==
                        setsockopt(mSocketFd, IPPROTO_IPV6, IPV6_TCLASS, (void*)&mToS,
                                sizeof(uint32_t)))
                {
                    IMLOGE0("[SetSocketOpt] IP_TOS -IPv6");
                    return false;
                }
            }
            IMLOGD1("[SetSocketOpt] IP_QOS[%d]", mToS);
            break;
        default:
            IMLOGD1("[SetSocketOpt] Unsupported socket option[%d]", nOption);
            return false;
    }

    return true;
}

int32_t ImsMediaSocket::GetSocketFd()
{
    return mSocketFd;
}

ISocketListener* ImsMediaSocket::GetListener()
{
    return mListener;
}

void ImsMediaSocket::StartSocketMonitor()
{
    if (mbTerminateMonitor == true)
    {
        IMLOGD0("[StartSocketMonitor] Send Signal");
        mbTerminateMonitor = false;
        mConditionExit.signal();
        return;
    }

    mbTerminateMonitor = false;
    IMLOGD_PACKET0(IM_PACKET_LOG_SOCKET, "[StartSocketMonitor] start monitor thread");

    pthread_t thr;
    pthread_attr_t attr;

    if (pthread_attr_init(&attr) != 0)
    {
        IMLOGE0("[StartSocketMonitor] pthread_attr_init() FAILED");
        return;
    }

    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
    {
        pthread_attr_destroy(&attr);
        IMLOGE0("[StartSocketMonitor] pthread_attr_setdetachstate() FAILED");
        return;
    }

    if (pthread_create(&thr, &attr, SocketMonitorThread, NULL) != 0)
    {
        pthread_attr_destroy(&attr);
        IMLOGE0("[StartSocketMonitor] pthread_create() FAILED");
        return;
    }

    if (pthread_attr_destroy(&attr) != 0)
    {
        IMLOGE0("[StartSocketMonitor] pthread_attr_destroy() FAILED");
        return;
    }
}

void ImsMediaSocket::StopSocketMonitor()
{
    IMLOGD_PACKET0(IM_PACKET_LOG_SOCKET, "[StopSocketMonitor] stop monitor thread");
    mbTerminateMonitor = true;
    mConditionExit.wait();
}

uint32_t ImsMediaSocket::SetSocketFD(void* pReadFds, void* pWriteFds, void* pExceptFds)
{
    uint32_t nMaxSD = 0;
    std::lock_guard<std::mutex> guard(sMutexRxSocket);
    FD_ZERO((fd_set*)pReadFds);
    FD_ZERO((fd_set*)pWriteFds);
    FD_ZERO((fd_set*)pExceptFds);
    IMLOGD_PACKET0(IM_PACKET_LOG_SOCKET, "[SetSocketFD]");

    for (auto& i : slistRxSocket)
    {
        int32_t socketFD = i->GetSocketFd();
        FD_SET(socketFD, (fd_set*)pReadFds);
        if (socketFD > nMaxSD)
            nMaxSD = socketFD;
    }

    mSocketListUpdated = false;
    return nMaxSD;
}

void ImsMediaSocket::SendNotify(void* pReadfds)
{
    std::lock_guard<std::mutex> guard(sMutexRxSocket);
    IMLOGD_PACKET0(IM_PACKET_LOG_SOCKET, "[SendNotify]");

    for (auto& rxSocket : slistRxSocket)
    {
        if (rxSocket != NULL)
        {
            int32_t socketFD = rxSocket->GetSocketFd();
            if (FD_ISSET(socketFD, (fd_set*)pReadfds))
            {
                IMLOGD_PACKET1(IM_PACKET_LOG_SOCKET, "[SendNotify] send notify to listener %p",
                        rxSocket->GetListener());
                if (rxSocket->GetListener() != NULL)
                    rxSocket->GetListener()->OnReceiveEnabled();
            }
        }
    }
}

void* ImsMediaSocket::SocketMonitorThread(void*)
{
    static fd_set ReadFds;
    static fd_set WriteFds;
    static fd_set ExceptFds;
    static fd_set TmpReadfds;
    static fd_set TmpWritefds;
    static fd_set TmpExcepfds;
    int nMaxSD;
    IMLOGD0("[SocketMonitorThread] enter");
    nMaxSD = SetSocketFD(&ReadFds, &WriteFds, &ExceptFds);

    for (;;)
    {
        uint32_t nRes;
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;  // micro-second
        if (mbTerminateMonitor)
        {
            break;
        }
        if (mSocketListUpdated)
        {
            nMaxSD = SetSocketFD(&ReadFds, &WriteFds, &ExceptFds);
        }

        memcpy(&TmpReadfds, &ReadFds, sizeof(fd_set));
        memcpy(&TmpWritefds, &WriteFds, sizeof(fd_set));
        memcpy(&TmpExcepfds, &ExceptFds, sizeof(fd_set));

        nRes = select(nMaxSD + 1, &TmpReadfds, &TmpWritefds, &TmpExcepfds, &tv);
        if (mbTerminateMonitor)
        {
            break;
        }
        else if (-1 == nRes)
        {
            IMLOGE0("[SocketMonitorThread] select function Error!!");
        }
        else if (0 == nRes)
        {
            // IMLOGD_PACKET0(IM_PACKET_LOG_SOCKET, "[SocketMonitorThread] timeout!!");
        }
        else
        {
            SendNotify(&TmpReadfds);
        }
    }

    IMLOGD0("[SocketMonitorThread] exit");
    mbTerminateMonitor = false;
    mConditionExit.signal();
    return NULL;
}
