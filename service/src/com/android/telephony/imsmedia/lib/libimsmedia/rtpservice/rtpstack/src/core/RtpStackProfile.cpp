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

#include <RtpStackProfile.h>

/*********************************************************
 * Function name        : RtpStackProfile
 * Description          : Constructor
 * Return type          : None
 * Argument             : None
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpStackProfile::RtpStackProfile():
                m_uiRtcpSessionBw(RTP_DEF_RTCP_BW_SIZE),
                m_uiMTUSize(RTP_CONF_MTU_SIZE),
                m_uiTermNum(RTP_ZERO)
{
}

/*********************************************************
 * Function name        : ~RtpStackProfile
 * Description          : Destructor
 * Return type          : None
 * Argument             : None
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpStackProfile::~RtpStackProfile()
{
}

/*********************************************************
 * Function name        : setRtcpBw
 * Description          : set method for rtcpBwFraction
 * Return type          : RtpDt_Void
 * Argument             : RtpDt_UInt32 : In
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpDt_Void RtpStackProfile::setRtcpBw(IN RtpDt_UInt32 uiRtcpBwFrac)
{
    m_uiRtcpSessionBw = uiRtcpBwFrac;
}

/*********************************************************
 * Function name        : getRtcpBw
 * Description          : get method for rtcpBwFraction
 * Return type          : RtpDt_UInt32
 * Argument             : None
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpDt_UInt32 RtpStackProfile::getRtcpBw()
{
    return m_uiRtcpSessionBw;
}
/*********************************************************
 * Function name        : setMtuSize
 * Description          : set method for uiMTUSize
 * Return type          : RtpDt_Void
 * Argument             : RtpDt_UInt32 : In
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpDt_Void RtpStackProfile::setMtuSize(IN RtpDt_UInt32 uiMtuSize)
{
    m_uiMTUSize = uiMtuSize;
}

/*********************************************************
 * Function name        : getMtuSize
 * Description          : get method for uiMTUSize
 * Return type          : RtpDt_UInt32
 * Argument             : None
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpDt_UInt32 RtpStackProfile::getMtuSize()
{
    return m_uiMTUSize;
}

/*********************************************************
 * Function name        : setTermNumber
 * Description          : set method for m_uiTermNum
 * Return type          : RtpDt_Void
 * Argument             : RtpDt_UInt32 : In
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpDt_Void RtpStackProfile::setTermNumber(IN RtpDt_UInt32 uiTermNum)
{
    m_uiTermNum = uiTermNum;
}

/*********************************************************
 * Function name        : getTermNumber
 * Description          : get method for m_uiTermNum
 * Return type          : RtpDt_UInt32
 * Argument             : None
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpDt_UInt32 RtpStackProfile::getTermNumber()
{
    return m_uiTermNum;
}
