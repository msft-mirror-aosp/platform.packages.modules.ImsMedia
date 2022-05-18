/*
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

#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ImsMediaNetworkUtil.h>
#include <gtest/gtest.h>
#include <ImsMediaTrace.h>

int createSocketFD(const char* pIPAddr, unsigned int port, int af)
{
    int soc = 0;
    if ((soc = socket(af, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        IMLOGE1("[createSocketFD] error[%d]", errno);
        return -1;
    }

    if (af == AF_INET)
    {
        sockaddr_in sin;
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);

        if (inet_pton(AF_INET, pIPAddr, &sin.sin_addr) <= 0)
        {
            IMLOGE1("[createSocketFD] inet_pton error[%d]", errno);
            return -1;
        }

        if (bind(soc, (struct sockaddr*)&sin, sizeof(sin)) < 0)
        {
            IMLOGE1("[createSocketFD] bind error[%d]", errno);
            return -1;
        }

        if (connect(soc, (struct sockaddr*)&sin, sizeof(sockaddr_in)) < 0)
        {
            IMLOGE1("[createSocketFD] connect error[%d]", errno);
            return -1;
        }
    }
    else if (af == AF_INET6)
    {
        sockaddr_in6 sin6;
        sin6.sin6_family = AF_INET6;
        sin6.sin6_port = htons(port);

        if (inet_pton(AF_INET6, pIPAddr, &sin6.sin6_addr) <= 0)
        {
            IMLOGE1("[createSocketFD] error[%d]", errno);
            return -1;
        }

        if (bind(soc, (struct sockaddr*)&sin6, sizeof(sin6)) < 0)
        {
            IMLOGE1("[createSocketFD] bind error[%d]", errno);
            return -1;
        }

        if (connect(soc, (struct sockaddr*)&sin6, sizeof(sockaddr_in6)) < 0)
        {
            IMLOGE1("[createSocketFD] error[%d]", errno);
            return -1;
        }
    }

    return soc;
}

void closeSocketFD(int socketFd)
{
    shutdown(socketFd, SHUT_RDWR);
    close(socketFd);
}

TEST(ImsMediaNetworkUtilTest, GetLocalIPPortFromSocketFDUsingLoopBackIPAddress)
{
    const char szTestIP[] = "127.0.0.1";
    unsigned int nTestPort = 12340;
    int nTestSocFD = createSocketFD(szTestIP, nTestPort, AF_INET);
    ASSERT_NE(nTestSocFD, -1);

    char IPAddr[32] = {'\0'};
    unsigned int port;
    bool res = ImsMediaNetworkUtil::GetLocalIPPortFromSocketFD(nTestSocFD, IPAddr, 32, port);

    ASSERT_EQ(res, true);
    ASSERT_EQ(strncmp(IPAddr, szTestIP, strlen(szTestIP)), 0);
    ASSERT_EQ(port, nTestPort);

    closeSocketFD(nTestSocFD);
}

TEST(ImsMediaNetworkUtilTest, GetLocalIPPortFromSocketFDUsingIpv6Address)
{
    const char szTestIP[] = "::1";
    unsigned int nTestPort = 56780;
    int nTestSocFD = createSocketFD(szTestIP, nTestPort, AF_INET6);
    ASSERT_NE(nTestSocFD, -1);

    char IPAddr[32] = {'\0'};
    unsigned int port;
    bool res = ImsMediaNetworkUtil::GetLocalIPPortFromSocketFD(nTestSocFD, IPAddr, 32, port);

    ASSERT_EQ(res, true);
    ASSERT_EQ(strncmp(IPAddr, szTestIP, strlen(szTestIP)), 0);
    ASSERT_EQ(port, nTestPort);

    closeSocketFD(nTestSocFD);
}