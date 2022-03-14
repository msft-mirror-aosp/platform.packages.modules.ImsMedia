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

/** \addtogroup  RTP_Stack
 *  @{
 */

#ifndef __RTP_STRING_H__
#define __RTP_STRING_H__

#include <RtpPfDatatypes.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <utils/RefBase.h>

#define RTP_TIME_MAX_SIZE            100
#define RTP_NUM_SEC_HOURS             100000000
#define RTP_NUM_SEC_MIN             60
#define RTP_NUM_1000                1000

#define RTP_TOLOWER(c)                ((((c)>= 'A') && ((c)<= 'Z')) ? ((c)- 'A' + 'a') : (c))
#define RTP_TOUPPER(c)                ((((c)>= 'a') && ((c)<= 'z')) ? ((c)- 'a' + 'A') : (c))


/* Structure of SystemTimer */
typedef struct _RtpSt_Timestamp
{
    RtpDt_UInt16 wYear;
    RtpDt_UInt16 wMonth;
    RtpDt_UInt16 wDayOfWeek;
    RtpDt_UInt16 wDay;
    RtpDt_UInt16 wHour;
    RtpDt_UInt16 wMinute;
    RtpDt_UInt16 wSecond;
    RtpDt_UInt16 wMilliseconds;
}RtpSt_Timestamp;

/**
 * This function gets the length of a null terminated string
 *
 * @param pcSource
 * @return Length of Source string
 */
RtpDt_UInt32 Rtp_Strlen(const RtpDt_Char *pszSource);

/**
 * This function locates last occurrence of a character in a string
 *
 * @param pszSource  String to parse
 * @param cChar      Character to check.
 * @return           Pointer to last occurrence of character in string
 */
RtpDt_Char *Rtp_Strrchr(RtpDt_Char *pszSource,RtpDt_Char cChar);

/**
 * This function strips long file paths
 *
 * @param pcFileName  Filename
 * @return Stripped file name
 */
RtpDt_Char* Rtp_StripFileName(RtpDt_Char *pcFileName);


#endif // __RTP_STRING_H__

/** @}*/
