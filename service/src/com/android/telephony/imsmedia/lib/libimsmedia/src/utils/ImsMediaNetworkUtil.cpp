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

#include <errno.h>
#include <arpa/inet.h>
#include <ImsMediaNetworkUtil.h>
#include <ImsMediaTrace.h>

bool ImsMediaNetworkUtil::ConvertIPStrToBin(char* pszSourceIP, char* pszDestBin,
    eIPVersion eIPver) {
    if (pszSourceIP == NULL || pszDestBin == NULL) return false;

    if (eIPver == IPV4) {
        inet_pton(AF_INET, pszSourceIP, pszDestBin);
        IMLOGD_PACKET1(IM_PACKET_LOG_SOCKET,
            "[ConvertIPStrToBin] inet_ntop(INET6) %s", pszDestBin);
    } else {    //ipv6
        inet_pton(AF_INET6, pszSourceIP, pszDestBin);
        IMLOGD_PACKET1(IM_PACKET_LOG_SOCKET,
            "[ConvertIPStrToBin] inet_ntop(INET6) %s", pszDestBin);
    }
    return true;
}

bool ImsMediaNetworkUtil::GetLocalIPPortFromSocketFD(
    int nSocketFD, char *pIPAddress, int len, unsigned int &port) {

    if (pIPAddress == NULL) {
        return false;
    }

    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(addr);
    errno = 0;
    int res = getsockname(nSocketFD, (struct sockaddr *)&addr, &addr_size);
    if (res == 0) {
        strncpy(pIPAddress, inet_ntoa(addr.sin_addr), len);
        port = ntohs(addr.sin_port);
        IMLOGD3("GetLocalIPPortFromSocketFD FD[%d]->%s:%u", nSocketFD, pIPAddress, port);
        return true;
    }

    IMLOGE1("GetLocalIPPortFromSocketFD->getpeername failed. Error[%d]", errno);
    return false;
}

bool ImsMediaNetworkUtil::GetRemoteIPPortFromSocketFD(
    int nSocketFD, char *pIPAddress, int len, unsigned int &port) {

    if (pIPAddress == NULL) {
        return false;
    }

    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(addr);
    errno = 0;
    int res = getpeername(nSocketFD, (struct sockaddr *)&addr, &addr_size);
    if (res == 0) {
        strncpy(pIPAddress, inet_ntoa(addr.sin_addr), len);
        port = ntohs(addr.sin_port);
        IMLOGD3("GetRemoteIPPortFromSocketFD FD[%d]->%s:%u", nSocketFD, pIPAddress, port);
        return true;
    }

    IMLOGE1("GetRemoteIPPortFromSocketFD->getpeername failed. Error[%d]", errno);
    return false;
}