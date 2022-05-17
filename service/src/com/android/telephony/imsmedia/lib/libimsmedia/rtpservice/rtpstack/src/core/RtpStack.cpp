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

#include <RtpStack.h>
#include <RtpStackUtil.h>
#include <RtpError.h>
#include <rtp_trace.h>

/*********************************************************
 * Function name        : RtpStack
 * Description          : Constructor
 * Return type          : None
 * Argument             : None
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpStack::RtpStack() :
        m_objRtpSessionList(std::list<RtpSession*>()),
        m_pobjStackProfile(RTP_NULL)
{
}

/*********************************************************
 * Function name        : ~RtpStack
 * Description          : Destructor
 * Return type          : None
 * Argument             : None
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpStack::~RtpStack()
{
    // clear stack profile
    if (m_pobjStackProfile != RTP_NULL)
    {
        delete m_pobjStackProfile;
    }

    // delete all RTP session objects.
    for (auto& pobjRtpSession : m_objRtpSessionList)
    {
        pobjRtpSession->deleteRtpSession();
    }
    m_objRtpSessionList.clear();
}

/*********************************************************
 * Function name        : RtpStack
 * Description          : single argument Constructor
 * Return type          : None
 * Argument             : RtpStackProfile*: In
 *                          pointer to RtpStackProfile.
 *                          It contains RTP profile info.
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpStack::RtpStack(IN RtpStackProfile* pobjStackProfile)
{
    m_pobjStackProfile = pobjStackProfile;
}

/*********************************************************
 * Function name        : createRtpSession
 * Description          : It creates RtpSession object and assigns SSRC to it.
 * Return type          : RtpSession*
 *                          Generated RtpSession object pointer.
 * Argument             : None
 * Preconditions        : Rtp Stack shall be initialized.
 * Side Effects            : None
 ********************************************************/
RtpSession* RtpStack::createRtpSession()
{
    RtpDt_UInt32 uiTermNum = m_pobjStackProfile->getTermNumber();

    RtpSession* pobjRtpSession = new RtpSession(this);
    if (pobjRtpSession == RTP_NULL)
    {
        RTP_TRACE_WARNING("Memory allocation error ..!", RTP_ZERO, RTP_ZERO);
        return RTP_NULL;
    }

    // add session into m_objRtpSessionList
    m_objRtpSessionList.push_back(pobjRtpSession);

    // generate SSRC
    RtpDt_UInt32 uiSsrc = RtpStackUtil::generateNewSsrc(uiTermNum);
    pobjRtpSession->setSsrc(uiSsrc);

    return pobjRtpSession;
}

/*********************************************************
 * Function name        : isRtpSessionPresent
 * Description          : It checks Rtp session is present in RtpSession list.
 * Return type          : eRtp_Bool
 *                          eRTP_SUCCESS if RTP session present in the m_objRtpSessionList.
 * Argument             : RtpSession*: In
 *                          Rtp Session pointer
 * Argument                : RtpDt_UInt16* : Out
 *                          Rtp Session object position in m_objRtpSessionList
 * Preconditions        : Rtp Stack shall be initialized.
 * Side Effects            : None
 ********************************************************/
eRtp_Bool RtpStack::isRtpSessionPresent(IN RtpSession* pobjSession)
{
    for (auto& pobjRtpSesItem : m_objRtpSessionList)
    {
        // get Rtp Session from list
        if (pobjRtpSesItem->compareRtpSessions(pobjSession) == eRTP_SUCCESS)
        {
            return eRTP_SUCCESS;
        }
    }

    return eRTP_FAILURE;
}

/*********************************************************
 * Function name        : deleteRtpSession
 * Description          : It deletes Rtp session from m_objRtpSessionList
 * Return type          : eRTP_STATUS_CODE
 *                          RTP_SUCCESS, if RTP session deletes from m_objRtpSessionList
 * Argument             : RtpSession*: In
 *                          Rtp Session pointer
 * Preconditions        : Rtp Stack shall be initialized.
 * Side Effects            : None
 ********************************************************/
eRTP_STATUS_CODE RtpStack::deleteRtpSession(IN RtpSession* pobjRtpSession)
{
    if (pobjRtpSession == RTP_NULL)
    {
        RTP_TRACE_WARNING("deleteRtpSession, pobjRtpSession is NULL ...!", RTP_ZERO, RTP_ZERO);
        return RTP_INVALID_PARAMS;
    }

    eRtp_Bool bisRtpSes = eRTP_SUCCESS;
    bisRtpSes = isRtpSessionPresent(pobjRtpSession);

    if (bisRtpSes == eRTP_SUCCESS)
    {
        pobjRtpSession->deleteRtpSession();
        m_objRtpSessionList.remove(pobjRtpSession);

        return RTP_SUCCESS;
    }

    return RTP_FAILURE;
}

/*********************************************************
 * Function name        : getStackProfile
 * Description          : get method for m_pobjStackProfile
 * Return type          : RtpStackProfile*
 *                          it returns m_pobjStackProfile
 * Argument             : None
 * Preconditions        : Rtp Stack shall be initialized.
 * Side Effects            : None
 ********************************************************/
RtpStackProfile* RtpStack::getStackProfile()
{
    return m_pobjStackProfile;
}

/*********************************************************
 * Function name        : setStackProfile
 * Description          : set method for m_pobjStackProfile
 * Return type          : RtpDt_Void
 * Argument             : RtpStackProfile* : In
 *                          pointer to Rtp Stack profile object.
 * Preconditions        : Rtp Stack shall be initialized.
 * Side Effects            : None
 ********************************************************/
RtpDt_Void RtpStack::setStackProfile(IN RtpStackProfile* pobjStackProfile)
{
    m_pobjStackProfile = pobjStackProfile;
}
