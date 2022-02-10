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

#include <RtpString.h>
#include <rtp_error.h>
#include <rtp_trace.h>
#include <string.h>

RtpDt_UInt32 Rtp_Strlen(const RtpDt_Char *pszStr)
{

    if (pszStr == RTP_NULL)
    {
        RTP_TRACE_WARNING("Rtp_Strlen::pszStr == RTP_NULL",
                RTP_ZERO,RTP_ZERO);
        return 0;
    }

    RtpDt_UInt32 nCount = 0;

    while (*pszStr)
    {
        ++pszStr;
        ++nCount;
    }

    return nCount;
}


RtpDt_Char* Rtp_Strrchr(RtpDt_Char *pszSrc, RtpDt_Char cChar)
{
    RtpDt_Char *pszDest = RTP_NULL;

    do
    {
        if (*pszSrc == cChar)
            pszDest = (RtpDt_Char*) pszSrc;

        if((*pszSrc) ==  RTP_NULL)
        {
            break;
        }
        pszSrc++;

    }while (1);

    return (pszDest);
}

RtpDt_Char* Rtp_StripFileName(RtpDt_Char *pcFileName)
{

    RtpDt_Char *pcTemp = Rtp_Strrchr(pcFileName,'/');
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
