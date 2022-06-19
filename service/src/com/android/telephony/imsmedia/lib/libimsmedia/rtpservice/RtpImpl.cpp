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

#include <RtpImpl.h>
#include <RtpService.h>
#include <RtpTrace.h>
#include <RtpTimer.h>
#include <string.h>

RtpImpl::RtpImpl() {}

RtpImpl::~RtpImpl() {}

eRtp_Bool RtpImpl::rtpSsrcCollisionInd(IN RtpDt_Int32 uiOldSsrc, IN RtpDt_Int32 uiNewSsrc)
{
    (RtpDt_Void) uiOldSsrc, (RtpDt_Void)uiNewSsrc;
    return eRTP_FALSE;
}

RtpDt_Void RtpImpl::setAppdata(IN RtpDt_Void* pvAppdata)
{
    m_pvAppdata = pvAppdata;
}

RtpDt_Void* RtpImpl::getAppdata()
{
    return m_pvAppdata;
}

eRtp_Bool RtpImpl::rtpNewMemberJoinInd(IN RtpDt_Int32 uiSsrc)
{
    (RtpDt_Void) uiSsrc;
    return eRTP_FALSE;
}

eRtp_Bool RtpImpl::rtpMemberLeaveInd(IN eRTP_LEAVE_REASON eLeaveReason, IN RtpDt_Int32 uiSsrc)
{
    (RtpDt_Void) eLeaveReason, (RtpDt_Void)uiSsrc;
    return eRTP_FALSE;
}

eRtp_Bool RtpImpl::rtcpPacketSendInd(IN RtpBuffer* pobjRtcpBuf, IN RtpSession* pobjRtpSession)
{
    RtpServiceListener* pobjIRtpSession = (RtpServiceListener*)getAppdata();
    if (pobjIRtpSession == RTP_NULL || pobjRtcpBuf == RTP_NULL || pobjRtpSession == RTP_NULL)
        return eRTP_FALSE;

    // dispatch to peer
    if (pobjIRtpSession->OnRtcpPacket(pobjRtcpBuf->getBuffer(), pobjRtcpBuf->getLength()) == -1)
    {
        pobjRtcpBuf->setBufferInfo(RTP_ZERO, RTP_NULL);
        return eRTP_FALSE;
    }

    return eRTP_TRUE;
}

eRtp_Bool RtpImpl::rtcpAppPayloadReqInd(
        OUT RtpDt_UInt16& pusSubType, OUT RtpDt_UInt32& uiName, OUT RtpBuffer* pobjPayload)
{
    if (pobjPayload == RTP_NULL)
    {
        return eRTP_FALSE;
    }

    /* App packet is not used by IMS application and below is only a test data */
    pusSubType = 1;
    uiName = 1111;
    // allocated memory will be released by the RTP stack
    RtpDt_UChar* pcAppData = new RtpDt_UChar[25];
    memset(pcAppData, 0, 25);
    memcpy(pcAppData, "application specific data", 25);
    pobjPayload->setBufferInfo(25, pcAppData);

    return eRTP_TRUE;
}

eRtp_Bool RtpImpl::getRtpHdrExtInfo(OUT RtpBuffer* pobjExtHdrInfo)
{
    if (pobjExtHdrInfo == RTP_NULL)
    {
        return eRTP_FALSE;
    }
    // allocated memory will be released by the RTP stack
    RtpDt_UChar* pcExtHdrInfo = new RtpDt_UChar[21];
    memset(pcExtHdrInfo, 0, 21);
    memcpy(pcExtHdrInfo, "extension header info", 21);
    pobjExtHdrInfo->setBufferInfo(21, pcExtHdrInfo);
    return eRTP_TRUE;
}

eRtp_Bool RtpImpl::deleteRcvrInfo(
        RtpDt_UInt32 uiRemoteSsrc, RtpBuffer* pobjDestAddr, RtpDt_UInt16 usRemotePort)
{
    (RtpDt_Void) uiRemoteSsrc, (RtpDt_Void)pobjDestAddr, (RtpDt_Void)usRemotePort;
    return eRTP_TRUE;
}

eRtp_Bool RtpImpl::rtcpTimerHdlErrorInd(IN eRTP_STATUS_CODE eStatus)
{
    (RtpDt_Void) eStatus;
    return eRTP_TRUE;
}

RtpDt_Void* RtpImpl::RtpStartTimer(IN RtpDt_UInt32 uiDuration, IN eRtp_Bool bRepeat,
        IN RTPCB_TIMERHANDLER pfnTimerCb, IN RtpDt_Void* pvData)
{
    RtpDt_Void* pvTimerId = (RtpDt_Void*)RtpTimer::TimerStart(
            (RtpDt_UInt32)uiDuration, (bool)bRepeat, (fn_TimerCb)pfnTimerCb, pvData);

    RTP_TRACE_MESSAGE("RtpStartTimer pvTimerId[%x], Duration= [%d]", pvTimerId, uiDuration);
    return pvTimerId;
    (void)uiDuration;
    (void)bRepeat;
    (void)pfnTimerCb;
    (void)pvData;
    return RTP_NULL;
}

eRtp_Bool RtpImpl::RtpStopTimer(IN RtpDt_Void* pTimerId, OUT RtpDt_Void** ppUserData)
{
    RTP_TRACE_MESSAGE("RtpStopTimer pvTimerId[%x]", pTimerId, 0);
    RtpTimer::TimerStop((hTimerHandler)pTimerId, ppUserData);
    (void)ppUserData;
    return eRTP_TRUE;
}
