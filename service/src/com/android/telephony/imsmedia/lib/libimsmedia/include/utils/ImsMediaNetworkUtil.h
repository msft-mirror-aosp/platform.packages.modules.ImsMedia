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

#ifndef IMS_MEDIA_NW_UTIL_H
#define IMS_MEDIA_NW_UTIL_H

#include <ImsMediaDefine.h>

class ImsMediaNetworkUtil {
public:
    static bool ConvertIPStrToBin(char* pszSourceIP, char* pszDestBin, eIPVersion eIPver);
    static bool GetLocalIPPortFromSocketFD(
        int nSocketFD, char *pIPAddress, int len,unsigned int &port);
    static bool GetRemoteIPPortFromSocketFD(
        int nSocketFD, char *pIPAddress, int len,unsigned int &port);
};

#endif //IMS_MEDIA_NW_UTIL_H