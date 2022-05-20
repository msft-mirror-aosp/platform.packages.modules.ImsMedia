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

#ifndef IMS_MEDIA_TRACE_H_INCLUDED
#define IMS_MEDIA_TRACE_H_INCLUDED
#include <stdint.h>

enum IM_PACKET_LOG_TYPE
{
    IM_PACKET_LOG_SOCKET = 0x00000001,
    IM_PACKET_LOG_AUDIO = 0x00000002,
    IM_PACKET_LOG_VIDEO = 0x00000004,
    IM_PACKET_LOG_RTP = 0x00000008,
    IM_PACKET_LOG_SCHEDULER = 0x00000010,
    IM_PACKET_LOG_PH = 0x00000020,
    IM_PACKET_LOG_JITTER = 0x00000040,
    IM_PACKET_LOG_RTCP = 0x00000080,
    IM_PACKET_LOG_RTPSTACK = 0x00000100,
    IM_PACKET_LOG_MEMORY = 0x00000200,
    IM_PACKET_LOG_MAX
};

#define IMLOGD_PACKET0(type, format)  \
    ImsMediaTrace::IMLOGD_PACKET_ARG( \
            type, "[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), __LINE__)
#define IMLOGD_PACKET1(type, format, a)                       \
    ImsMediaTrace::IMLOGD_PACKET_ARG(type, "[%s:%d] " format, \
            ImsMediaTrace::IM_StripFileName((char*)__FILE__), __LINE__, a)
#define IMLOGD_PACKET2(type, format, a, b)                    \
    ImsMediaTrace::IMLOGD_PACKET_ARG(type, "[%s:%d] " format, \
            ImsMediaTrace::IM_StripFileName((char*)__FILE__), __LINE__, a, b)
#define IMLOGD_PACKET3(type, format, a, b, c)                 \
    ImsMediaTrace::IMLOGD_PACKET_ARG(type, "[%s:%d] " format, \
            ImsMediaTrace::IM_StripFileName((char*)__FILE__), __LINE__, a, b, c)
#define IMLOGD_PACKET4(type, format, a, b, c, d)              \
    ImsMediaTrace::IMLOGD_PACKET_ARG(type, "[%s:%d] " format, \
            ImsMediaTrace::IM_StripFileName((char*)__FILE__), __LINE__, a, b, c, d)
#define IMLOGD_PACKET5(type, format, a, b, c, d, e)           \
    ImsMediaTrace::IMLOGD_PACKET_ARG(type, "[%s:%d] " format, \
            ImsMediaTrace::IM_StripFileName((char*)__FILE__), __LINE__, a, b, c, d, e)
#define IMLOGD_PACKET6(type, format, a, b, c, d, e, f)        \
    ImsMediaTrace::IMLOGD_PACKET_ARG(type, "[%s:%d] " format, \
            ImsMediaTrace::IM_StripFileName((char*)__FILE__), __LINE__, a, b, c, d, e, f)
#define IMLOGD_PACKET7(type, format, a, b, c, d, e, f, g)     \
    ImsMediaTrace::IMLOGD_PACKET_ARG(type, "[%s:%d] " format, \
            ImsMediaTrace::IM_StripFileName((char*)__FILE__), __LINE__, a, b, c, d, e, f, g)
#define IMLOGD_PACKET8(type, format, a, b, c, d, e, f, g, h)  \
    ImsMediaTrace::IMLOGD_PACKET_ARG(type, "[%s:%d] " format, \
            ImsMediaTrace::IM_StripFileName((char*)__FILE__), __LINE__, a, b, c, d, e, f, g, h)

#define IMLOGD0(format)        \
    ImsMediaTrace::IMLOGD_ARG( \
            "[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), __LINE__)
#define IMLOGD1(format, a)     \
    ImsMediaTrace::IMLOGD_ARG( \
            "[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), __LINE__, a)
#define IMLOGD2(format, a, b)  \
    ImsMediaTrace::IMLOGD_ARG( \
            "[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), __LINE__, a, b)
#define IMLOGD3(format, a, b, c)                                                                   \
    ImsMediaTrace::IMLOGD_ARG("[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), \
            __LINE__, a, b, c)
#define IMLOGD4(format, a, b, c, d)                                                                \
    ImsMediaTrace::IMLOGD_ARG("[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), \
            __LINE__, a, b, c, d)
#define IMLOGD5(format, a, b, c, d, e)                                                             \
    ImsMediaTrace::IMLOGD_ARG("[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), \
            __LINE__, a, b, c, d, e)
#define IMLOGD6(format, a, b, c, d, e, f)                                                          \
    ImsMediaTrace::IMLOGD_ARG("[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), \
            __LINE__, a, b, c, d, e, f)
#define IMLOGD7(format, a, b, c, d, e, f, g)                                                       \
    ImsMediaTrace::IMLOGD_ARG("[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), \
            __LINE__, a, b, c, d, e, f, g)
#define IMLOGD8(format, a, b, c, d, e, f, g, h)                                                    \
    ImsMediaTrace::IMLOGD_ARG("[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), \
            __LINE__, a, b, c, d, e, f, g, h)

#define IMLOGE0(format)        \
    ImsMediaTrace::IMLOGE_ARG( \
            "[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), __LINE__)
#define IMLOGE1(format, a)     \
    ImsMediaTrace::IMLOGE_ARG( \
            "[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), __LINE__, a)
#define IMLOGE2(format, a, b)  \
    ImsMediaTrace::IMLOGE_ARG( \
            "[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), __LINE__, a, b)
#define IMLOGE3(format, a, b, c)                                                                   \
    ImsMediaTrace::IMLOGE_ARG("[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), \
            __LINE__, a, b, c)
#define IMLOGE4(format, a, b, c, d)                                                                \
    ImsMediaTrace::IMLOGE_ARG("[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), \
            __LINE__, a, b, c, d)
#define IMLOGE5(format, a, b, c, d, e)                                                             \
    ImsMediaTrace::IMLOGE_ARG("[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), \
            __LINE__, a, b, c, d, e)
#define IMLOGE6(format, a, b, c, d, e, f)                                                          \
    ImsMediaTrace::IMLOGE_ARG("[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), \
            __LINE__, a, b, c, d, e, f)
#define IMLOGE7(format, a, b, c, d, e, f, g)                                                       \
    ImsMediaTrace::IMLOGE_ARG("[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), \
            __LINE__, a, b, c, d, e, f, g)
#define IMLOGE8(format, a, b, c, d, e, f, g, h)                                                    \
    ImsMediaTrace::IMLOGE_ARG("[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), \
            __LINE__, a, b, c, d, e, f, g, h)

#define IMLOGW0(format)        \
    ImsMediaTrace::IMLOGW_ARG( \
            "[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), __LINE__)
#define IMLOGW1(format, a)     \
    ImsMediaTrace::IMLOGW_ARG( \
            "[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), __LINE__, a)
#define IMLOGW2(format, a, b)  \
    ImsMediaTrace::IMLOGW_ARG( \
            "[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), __LINE__, a, b)
#define IMLOGW3(format, a, b, c)                                                                   \
    ImsMediaTrace::IMLOGW_ARG("[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), \
            __LINE__, a, b, c)
#define IMLOGW4(format, a, b, c, d)                                                                \
    ImsMediaTrace::IMLOGW_ARG("[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), \
            __LINE__, a, b, c, d)
#define IMLOGW5(format, a, b, c, d, e)                                                             \
    ImsMediaTrace::IMLOGW_ARG("[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), \
            __LINE__, a, b, c, d, e)
#define IMLOGW6(format, a, b, c, d, e, f)                                                          \
    ImsMediaTrace::IMLOGW_ARG("[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), \
            __LINE__, a, b, c, d, e, f)
#define IMLOGW7(format, a, b, c, d, e, f, g)                                                       \
    ImsMediaTrace::IMLOGW_ARG("[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), \
            __LINE__, a, b, c, d, e, f, g)
#define IMLOGW8(format, a, b, c, d, e, f, g, h)                                                    \
    ImsMediaTrace::IMLOGW_ARG("[%s:%d] " format, ImsMediaTrace::IM_StripFileName((char*)__FILE__), \
            __LINE__, a, b, c, d, e, f, g, h)

#define IMLOGB(a, b, c) ImsMediaTrace::IMLOGD_BINARY(a, b, c)

class ImsMediaTrace
{
public:
    static void IMLOGD_PACKET_ARG(IM_PACKET_LOG_TYPE type, const char* format, ...);
    static void IMSetDebugLog(uint32_t type);
    static uint32_t IMGetDebugLog();
    static void IMLOGD_ARG(const char* format, ...);
    static void IMLOGE_ARG(const char* format, ...);
    static void IMLOGW_ARG(const char* format, ...);
    static char* IMTrace_Bin2String(void* s, int length);
    static void IMLOGD_BINARY(const char* msg, const void* s, int length);
    static char* IM_StripFileName(char* pcFileName);
};
#define SET_IM_DEBUG_LOG(value) IMSetDebugLog(value);
#define GET_IM_DEBUG_LOG()      IMGetDebugLog()

#endif