/** \addtogroup  RTP_Stack
 *  @{
 */

/**
 * @class   RtpStackUtl
 * @brief   This class provides RTP utility functions
 */

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

#ifndef __RTP_STACK_UTIL_H__
#define __RTP_STACK_UTIL_H__

#include <RtpGlobal.h>
#include <RtpBuffer.h>

class RtpStackUtil
{
public:
    // Constructor
    RtpStackUtil();
    // Destructor
    ~RtpStackUtil();

    /**
        Parse and retrieve seq number from a RTP packet
        @param[in] pobjRecvdPkt RTCP packet from network
        */
    static RtpDt_UInt16 getSequenceNumber(IN RtpDt_UChar* pcRtpHdrBuf);
    /**
        Parse and retrieve ssrc from a RTP packet
        @param[in] pobjRecvdPkt RTP packet from network
    */
    static RtpDt_UInt32 getRtpSsrc(IN RtpBuffer* pobjRecvdPkt);

    /**
        Parse and retrieve ssrc from a RTCP packet
        @param[in] pobjRecvdPkt RTCP packet from network
    */
    static RtpDt_UInt32 getRtcpSsrc(IN RtpBuffer* pobjRecvdPkt);

    /**
    Utility to generate new ssrc
    @return new generated ssrc
    */
    static RtpDt_UInt32 generateNewSsrc(IN RtpDt_UInt32 uiTermNum);

    /** It calculates RTP time stamp
     */
    static RtpDt_UInt32 calcRtpTimestamp(IN RtpDt_UInt32 uiPrevRtpTs, IN tRTP_NTP_TIME* stCurRtpTs,
            IN tRTP_NTP_TIME* stPrevRtpTs, IN RtpDt_UInt32 uiSamplingRate);

    /**
    It gets middle four octets from Ntp timestamp
    */
    static RtpDt_UInt32 getMidFourOctets(IN tRTP_NTP_TIME* pstNtpTs);
};

#endif  //__RTP_STACK_UTIL_H__
/** @}*/
