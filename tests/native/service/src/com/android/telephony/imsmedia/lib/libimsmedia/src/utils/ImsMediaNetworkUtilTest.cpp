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

int createSocketFD(const char *pIPAddr, unsigned int port) {
    int soc =0;
    struct sockaddr_in addr;
    if ((soc = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, pIPAddr, &addr.sin_addr) <= 0) {
        return -1;
    }

    if (connect(soc, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        return -1;
    }

    return soc;
}

TEST(ImsMediaNetworkUtilTest, GetLocalIPPortFromSocketFDUsingLoopBackIPAddress) {
    const char szTestIP[] = "127.0.0.1";
    unsigned int nTestPort = 12340;
    int nTestSocFD = createSocketFD(szTestIP, nTestPort);

    char IPAddr[32] = {'\0'};
    unsigned int port;
    bool res = ImsMediaNetworkUtil::GetLocalIPPortFromSocketFD(nTestSocFD, IPAddr, 32, port);

    ASSERT_EQ(res, true);
    ASSERT_EQ(strncmp(IPAddr, szTestIP, strlen(szTestIP)), 0);
    ASSERT_EQ(port, nTestPort);
}

TEST(ImsMediaNetworkUtilTest, GetLocalIPPortFromSocketFDUsingAnyIPAddress) {
    const char szTestIP[] = "192.168.0.2";
    unsigned int nTestPort = 56780;
    int nTestSocFD = createSocketFD(szTestIP, nTestPort);

    char IPAddr[32] = {'\0'};
    unsigned int port;
    bool res = ImsMediaNetworkUtil::GetLocalIPPortFromSocketFD(nTestSocFD, IPAddr, 32, port);

    ASSERT_EQ(res, true);
    ASSERT_EQ(strncmp(IPAddr, szTestIP, strlen(szTestIP)), 0);
    ASSERT_EQ(port, nTestPort);
}