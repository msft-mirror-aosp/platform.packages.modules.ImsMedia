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

#include <sys/time.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <RtpOsUtil.h>

RtpOsUtil::RtpOsUtil() {}

RtpOsUtil::~RtpOsUtil() {}

RtpDt_Void RtpOsUtil::GetNtpTime(tRTP_NTP_TIME& pstNtpTime)
{
    struct timeval stAndrodTp;

    if (gettimeofday(&stAndrodTp, RTP_NULL) != -1)
    {
        // To convert a UNIX timestamp (seconds since 1970) to NTP time, add 2,208,988,800 seconds
        pstNtpTime.m_uiNtpHigh32Bits = stAndrodTp.tv_sec + 2208988800UL;
        pstNtpTime.m_uiNtpLow32Bits = (RtpDt_UInt32)(stAndrodTp.tv_usec * 4294UL);
    }
}

RtpDt_Void RtpOsUtil::Srand()
{
    struct timeval stSysTime;
    gettimeofday(&stSysTime, RTP_NULL);
    RtpDt_UInt32 uiSeed = stSysTime.tv_usec * 1000;
    srand(uiSeed);
}

RtpDt_UInt32 RtpOsUtil::Rand()
{
    RtpOsUtil::Srand();
    return rand();
}

RtpDt_UInt32 RtpOsUtil::Ntohl(RtpDt_UInt32 uiNetlong)
{
    return ntohl(uiNetlong);
}

RtpDt_Double RtpOsUtil::RRand()
{
    tRTP_NTP_TIME stNtpTs;
    RtpOsUtil::Srand();
    RtpDt_Double dRandNum = rand() / RAND_MAX;
    RtpOsUtil::GetNtpTime(stNtpTs);
    RtpDt_Double dTemp = ((dRandNum * stNtpTs.m_uiNtpHigh32Bits) +
            (stNtpTs.m_uiNtpLow32Bits / RTP_MILLISEC_MICRO));
    if (dTemp > RTP_ZERO)
    {
        return 1.0 / dTemp;
    }
    else
    {
        return 1.0;
    }
}
