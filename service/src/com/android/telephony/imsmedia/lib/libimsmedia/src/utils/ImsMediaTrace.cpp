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

#include <ImsMediaTrace.h>
#include <string.h>
#include <utils/Log.h>
#include <sys/time.h>

#ifdef IM_FILE_LOG
#include <stdio.h>
#include <stdlib.h>
#define IM_LOG_FILE "/data/IM.txt"
static uint IM_remove_log = 1;
#endif

#define TRACEMAXSTRING 1024

#ifdef IM_FILE_LOG
#define __IMLOG__(IMLOGTYPE)                               \
    do                                                     \
    {                                                      \
        va_list args;                                      \
        char szBuffer[TRACEMAXSTRING];                     \
        FILE* fp_IM_log = NULL;                            \
        va_start(args, format);                            \
        vsnprintf(szBuffer, TRACEMAXSTRING, format, args); \
        va_end(args);                                      \
        android_printLog(IMLOGTYPE, "IM", "%s", szBuffer); \
        if (IM_remove_log)                                 \
        {                                                  \
            remove(IM_LOG_FILE);                           \
            IM_remove_log = 0;                             \
        }                                                  \
        fp_IM_log = fopen(IM_LOG_FILE, "a+");              \
        if (fp_IM_log != NULL)                             \
        {                                                  \
            fprintf(fp_IM_log, "%s", szBuffer);            \
            fclose(fp_IM_log);                             \
        }                                                  \
    } while (0)

#else
#define IM_TAG       "libimsmedia"
#define IM_DEBUG_TAG "libimsmedia_d"
#define __IMLOG__(IMLOGTYPE, TAG)                          \
    do                                                     \
    {                                                      \
        va_list args;                                      \
        char szBuffer[TRACEMAXSTRING];                     \
        va_start(args, format);                            \
        vsnprintf(szBuffer, TRACEMAXSTRING, format, args); \
        va_end(args);                                      \
        android_printLog(IMLOGTYPE, TAG, "%s", szBuffer);  \
    } while (0)
#endif

static bool IMlogd_enabled = true;
static uint IMlogd_packet_enabled_types = 0;

void ImsMediaTrace::IMLOGD_PACKET_ARG(IM_PACKET_LOG_TYPE type, const char* format, ...)
{
    if (IMlogd_packet_enabled_types & type)
    {
        __IMLOG__(ANDROID_LOG_DEBUG, IM_DEBUG_TAG);
    }
}

void ImsMediaTrace::IMSetDebugLog(uint type)
{
    IMlogd_packet_enabled_types = type;
}

uint ImsMediaTrace::IMGetDebugLog()
{
    return IMlogd_packet_enabled_types;
}

void ImsMediaTrace::IMLOGE_ARG(const char* format, ...)
{
    __IMLOG__(ANDROID_LOG_ERROR, IM_TAG);
}

void ImsMediaTrace::IMLOGW_ARG(const char* format, ...)
{
    __IMLOG__(ANDROID_LOG_WARN, IM_TAG);
}

void ImsMediaTrace::IMLOGD_ARG(const char* format, ...)
{
    if (IMlogd_enabled)
    {
        __IMLOG__(ANDROID_LOG_DEBUG, IM_TAG);
    }
}

#define MAX_PRINT_STRING_LEN 2048
static char buffer[MAX_PRINT_STRING_LEN];

static char hex_char(char nibble)
{
    static char buf[16] = {
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    return buf[nibble & 0xF];
}

char* ImsMediaTrace::IMTrace_Bin2String(void* s, int length)
{
    char* input = (char*)s;
    char* output = buffer;
    int i;

    if (length < 0)
        return 0;
    if (length * 4 > (MAX_PRINT_STRING_LEN - 5))
        length = (MAX_PRINT_STRING_LEN / 4) - 5;

    for (i = 0; i < length; i++)
    {
        *output++ = hex_char(*input >> 4);
        *output++ = hex_char(*input++ & 0xF);
        *output++ = ' ';
        if ((i & 0x03) == 0x03)
            *output++ = ' ';
    }

    *output = 0;
    return buffer;
}

void ImsMediaTrace::IMLOGD_BINARY(const char* msg, const void* s, int length)
{
#define IMLOG_BIN_LINE_WIDTH 32
    char* curr = (char*)s;
    if (msg)
        IMLOGD1("%s", msg);
    while (length > 0)
    {
        int curr_len = length < IMLOG_BIN_LINE_WIDTH ? length : IMLOG_BIN_LINE_WIDTH;
        IMLOGD1("\t%s", IMTrace_Bin2String(curr, curr_len));
        length -= curr_len;
        curr += curr_len;
    }
}

char* IM_Strrchr(char* pszSrc, char cChar)
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

char* ImsMediaTrace::IM_StripFileName(char* pcFileName)
{
    char* pcTemp = NULL;
    pcTemp = IM_Strrchr(pcFileName, '/');

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
