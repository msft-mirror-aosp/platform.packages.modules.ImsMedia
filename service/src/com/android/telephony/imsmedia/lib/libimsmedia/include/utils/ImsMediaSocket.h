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

#ifndef IMS_MEDIA_SOCKET_H
#define IMS_MEDIA_SOCKET_H

#include <ImsMediaDefine.h>
#include <ImsMediaCondition.h>
#include <ISocket.h>
#include <stdint.h>
#include <list>
#include <mutex>

class ImsMediaSocket : public ISocket
{
public:
    static ImsMediaSocket* GetInstance(uint32_t localPort,
        const char* peerIpAddress, uint32_t peerPort);
    static void ReleaseInstance(ImsMediaSocket* node);

private:
    ImsMediaSocket();
    virtual ~ImsMediaSocket();
    static void StartSocketMonitor();
    static void StopSocketMonitor();
    static void* SocketMonitorThread(void*);
    static uint32_t SetSocketFD(void* pReadFds, void* pWriteFds, void* pExceptFds);
    static void SendNotify(void* pReadfds);

public:
    virtual void SetLocalEndpoint(const char* ipAddress, const uint32_t port);
    virtual void SetPeerEndpoint(const char* ipAddress, const uint32_t port);
    virtual int GetLocalPort();
    virtual int GetPeerPort();
    virtual char* GetLocalIPAddress();
    virtual char* GetPeerIPAddress();
    virtual bool Open(int socketFd = -1);
    virtual bool Listen(ISocketListener* listener);
    virtual uint32_t SendTo(uint8_t* pData, uint32_t nDataSize);
    virtual uint32_t ReceiveFrom(uint8_t* pData, uint32_t nBufferSize);
    virtual void Close(eSocketMode mode);
    virtual bool SetSocketOpt(eSocketOpt nOption, uint32_t nOptionValue);
    int32_t GetSocketFd();
    ISocketListener* GetListener();

private:
    static std::list<ImsMediaSocket*> slistSocket;
    static std::list<ImsMediaSocket*> slistRxSocket;
    static int32_t sRxSocketCount;
    static bool mSocketListUpdated;
    static bool mbTerminateMonitor;
    static std::mutex sMutexRxSocket;
    static std::mutex sMutexSocketList;
    static std::mutex sMutexSocketMonitorThread;
    static ImsMediaCondition mCondExit;
    //local end point
    char mPeerIPBin[MAX_IP_LEN];
    int32_t mSocketFd;
    int32_t mRefCount;
    bool mbReceivingIPFiltering;
    int32_t mToS;
    ISocketListener* mListener;
    //eSocketClass mSocketClass;
    eIPVersion mLocalIPVersion;
    eIPVersion mPeerIPVersion;
    char mLocalIP[MAX_IP_LEN];
    char mPeerIP[MAX_IP_LEN];
    uint32_t mLocalPort;
    uint32_t mPeerPort;
};

#endif