/** \addtogroup  RTP_Stack
 *  @{
 */

//RTP_TRACE_ENABLE to be set in settings to enable tracing
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

#ifndef __RTP_TRACE_H__
#define __RTP_TRACE_H__

#include <RtpPfDatatypes.h>
#include <RtpString.h>
#define RTP_PACKET_LOG_RTPSTACK 0x00000100
#define RTP_TRACE_WARNING(a,b,c)\
RtpTrace::RTPLOGW_ARG("[%s:%d]" a, RtpTrace::RTP_StripFileName((char*)__FILE__), __LINE__, b, c)
#define RTP_TRACE_ERROR(a,b,c)\
RtpTrace::RTPLOGE_ARG("[%s:%d]" a, RtpTrace::RTP_StripFileName((char*)__FILE__), __LINE__, b, c)
#define RTP_TRACE_MESSAGE(a,b,c) RtpTrace::RTPLOGD_PACKET_ARG\
(RTP_PACKET_LOG_RTPSTACK, "[%s:%d]" a, Rtp_StripFileName((char*)__FILE__), __LINE__, b, c)
#define RTP_TRACE_FXNENTRY RtpTrace::RTPLOGD_PACKET_ARG\
(RTP_PACKET_LOG_RTPSTACK, "[%s:%d] FIN %s", Rtp_StripFileName((char*)__FILE__),\
__LINE__, __FUNCTION__)
#define RTP_TRACE_FXNEXIT RtpTrace::RTPLOGD_PACKET_ARG\
(RTP_PACKET_LOG_RTPSTACK, "[%s:%d] FOUT %s", Rtp_StripFileName((char*)__FILE__),\
__LINE__, __FUNCTION__)
#define RTP_TRACE_NORMAL(a,b,c)\
RtpTrace::RTPLOGD_PACKET_ARG(RTP_PACKET_LOG_RTPSTACK, "[%s:%d]" a,\
Rtp_StripFileName((char*)__FILE__), __LINE__, b, c)
#define RTP_TRACE_BINARY(a,b) //RTPLOGD2("[%s] %s %d %d", #a,b);

namespace RtpTrace
{
void RTPLOGD_PACKET_ARG(int type, const char* format,...);
void RTPLOGD_ARG(const char* format,...);
void RTPLOGE_ARG(const char* format,...);
void RTPLOGW_ARG(const char* format,...);
void RTPSetDebugLog(int type);
int RTPGetDebugLog();
char *RTPTrace_Bin2String(void *s, int length);
void RTPLOGD_BINARY(const char* msg, const void *s, int length);
char* RTP_StripFileName(char *pcFileName);
}

#endif // __RTP_TRACE_H__

/** @}*/
