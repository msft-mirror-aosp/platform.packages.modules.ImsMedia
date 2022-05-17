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

#include <string.h>
#include <utils/Log.h>
#include <sys/time.h>
#include <rtp_trace.h>

namespace RtpTrace
{
#define TRACEMAXSTRING 1024
#define RTP_TAG        "RTP"
#define RTP_DEBUG_TAG  "RTP_DEBUG"
#define __RTPLOG__(RTPLOGTYPE, TAG)                        \
    do                                                     \
    {                                                      \
        va_list args;                                      \
        char szBuffer[TRACEMAXSTRING];                     \
        va_start(args, format);                            \
        vsnprintf(szBuffer, TRACEMAXSTRING, format, args); \
        va_end(args);                                      \
        android_printLog(RTPLOGTYPE, TAG, "%s", szBuffer); \
    } while (0)

static bool rtplogd_enabled = true;
static int rtplogd_packet_enabled_types = 0;  // check later

void RTPLOGE_ARG(const char* format, ...)
{
    __RTPLOG__(ANDROID_LOG_ERROR, RTP_TAG);
}

void RTPLOGW_ARG(const char* format, ...)
{
    __RTPLOG__(ANDROID_LOG_WARN, RTP_TAG);
}

void RTPLOGD_ARG(const char* format, ...)
{
    if (rtplogd_enabled)
    {
        __RTPLOG__(ANDROID_LOG_DEBUG, RTP_TAG);
    }
}

void RTPLOGD_PACKET_ARG(int type, const char* format, ...)
{
    if (rtplogd_packet_enabled_types & type)
    {
        __RTPLOG__(ANDROID_LOG_DEBUG, RTP_DEBUG_TAG);
    }
}

void RTPSetDebugLog(int type)
{
    rtplogd_packet_enabled_types = type;
}

int RTPGetDebugLog()
{
    return rtplogd_packet_enabled_types;
}

char* RTP_Strrchr(char* pszSrc, char cChar)
{
    char* pszDest = NULL;
    do
    {
        if (*pszSrc == cChar)
            pszDest = (char*)pszSrc;
        if ((*pszSrc) == 0)
        {
            break;
        }

        pszSrc++;

    } while (1);

    return (pszDest);
}

char* RTP_StripFileName(char* pcFileName)
{
    char* pcTemp = NULL;

    pcTemp = RTP_Strrchr(pcFileName, '/');

    if (pcTemp)
    {
        pcTemp++;
    }
    else
    {
        pcTemp = pcFileName;
    }
    return pcTemp;
}
}  // namespace RtpTrace