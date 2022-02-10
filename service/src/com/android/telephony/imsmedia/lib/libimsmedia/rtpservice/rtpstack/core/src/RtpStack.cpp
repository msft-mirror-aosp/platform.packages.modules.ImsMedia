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
#include <rtp_error.h>
#include <rtp_trace.h>

/*********************************************************
 * Function name        : Rtp_FreeRtpSessionDb
 * Description          : It deletes RtpSession object
 * Return type          : eRtp_Bool
 *                          eRTP_SUCCESS on success
 * Argument             : RtpDt_Void**: In
 *                          void pointer to pointer which refers RtpSession obj.
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
eRtp_Bool Rtp_FreeRtpSessionDb(IN RtpDt_Void **ppObj)
{
    RtpSession *pobjRtpSession = RTP_NULL;
    eRTP_STATUS_CODE eDeleSesSta = RTP_SUCCESS;


    pobjRtpSession = RTP_STATIC_CAST(RtpSession*,*ppObj);
    eDeleSesSta = pobjRtpSession->deleteRtpSession();
    if(eDeleSesSta != RTP_SUCCESS)
    {
        return eRTP_FAILURE;
    }

    return eRTP_SUCCESS;
}


/*********************************************************
 * Function name        : Rtp_CompareRtpSessionDb
 * Description          : It compares pvNode1 with pvNode2
 * Return type          : eRtp_Bool
 *                          eRTP_SUCCESS on success
 * Argument             : RtpDt_Void*: In
 *                          void pointer which refers RtpSession*
 * Argument                : RtpDt_Void*: In
 *                          void pointer which refers RtpSession*
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
eRtp_Bool Rtp_CompareRtpSessionDb(IN RtpDt_Void *pvNode1,
                                     IN RtpDt_Void *pvNode2)
{
    if(pvNode1 == pvNode2)
        return eRTP_TRUE;
    else
        return eRTP_FALSE;
}

/*********************************************************
 * Function name        : RtpStack
 * Description          : Constructor
 * Return type          : None
 * Argument             : None
 * Preconditions        : None
 * Side Effects            : None
 ********************************************************/
RtpStack::RtpStack():
                m_pobjStackProfile(RTP_NULL)
{

    m_objSessions.InitList(Rtp_FreeRtpSessionDb,
        Rtp_CompareRtpSessionDb);

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
    RtpDt_UInt16 usSize = RTP_ZERO;
    RtpDt_UInt16 usError = RTP_ZERO;


    //clear stack profile
    if(m_pobjStackProfile != RTP_NULL)
    {
        delete m_pobjStackProfile;
    }

    //delete all RTP session objects.
    m_objSessions.GetSize(&usSize, &usError);
    for(RtpDt_UInt16 usCount = RTP_ZERO;usCount < usSize;
        usCount = usCount + RTP_ONE)
    {
        m_objSessions.DeleteAtPos(RTP_ZERO, &usError);
    }
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

    m_objSessions.InitList(Rtp_FreeRtpSessionDb,
        Rtp_CompareRtpSessionDb);

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
    RtpDt_UInt16 usError = RTP_ZERO;

    RtpDt_UInt32 uiTermNum = m_pobjStackProfile->getTermNumber();

    RtpSession *pobjRtpSession = new RtpSession(this);
    if(pobjRtpSession == RTP_NULL)
    {
        RTP_TRACE_WARNING("Memory allocation error ..!",
                RTP_ZERO,RTP_ZERO);
        return RTP_NULL;
    }

    //add session into m_objSessions
    m_objSessions.Append(pobjRtpSession,&usError);
    if(eRTP_FAILURE == usError)
    {
        pobjRtpSession->deleteRtpSession();

        RTP_TRACE_WARNING("createRtpSession: Error in adding pobjRtpSession to list...!",
            RTP_ZERO,RTP_ZERO);
        return RTP_NULL;
    }

    //generate SSRC
    RtpDt_UInt32 uiSsrc = RtpStackUtil::generateNewSsrc(uiTermNum);
    pobjRtpSession->setSsrc(uiSsrc);

    return pobjRtpSession;
}

/*********************************************************
 * Function name        : isRtpSessionPresent
 * Description          : It checks Rtp session is present in RtpSession list.
 * Return type          : eRtp_Bool
 *                          eRTP_SUCCESS if RTP session present in the m_objSessions.
 * Argument             : RtpSession*: In
 *                          Rtp Session pointer
 * Argument                : RtpDt_UInt16* : Out
 *                          Rtp Session object position in m_objSessions
 * Preconditions        : Rtp Stack shall be initialized.
 * Side Effects            : None
 ********************************************************/
eRtp_Bool RtpStack::isRtpSessionPresent(IN RtpSession* pobjSession,
                                            OUT RtpDt_UInt16 *pusPosition)
{
    RtpDt_UInt16    usGetError = RTP_ZERO;
    RtpDt_UInt16    usSize = RTP_ZERO;
    RtpDt_UInt16 usPos = RTP_ZERO;


    m_objSessions.GetSize(&usSize, &usGetError);
    for(; usPos < usSize; usPos++)
    {
        RtpDt_Void    *pvElement = RTP_NULL;
        RtpSession *pobjRtpSesItem = RTP_NULL;

        //get Rtp Session from list
        m_objSessions.GetElement(usPos, &pvElement, &usGetError);
        if(usGetError == ERR_LIST_INV_INPUT)
        {
            RTP_TRACE_WARNING("Error in fetching the element from list...!",
                RTP_ZERO,RTP_ZERO);
            return eRTP_FAILURE;
        }

        //typecast to RtpSession
        pobjRtpSesItem = RTP_STATIC_CAST(RtpSession*,pvElement);

        if(pobjRtpSesItem->compareRtpSessions(pobjSession) == eRTP_SUCCESS)
        {
            *pusPosition = usPos;
            return eRTP_SUCCESS;
        }
    }

    return eRTP_FAILURE;
}


/*********************************************************
 * Function name        : deleteRtpSession
 * Description          : It deletes Rtp session from m_objSessions
 * Return type          : eRTP_STATUS_CODE
 *                          RTP_SUCCESS, if RTP session deletes from m_objSessions
 * Argument             : RtpSession*: In
 *                          Rtp Session pointer
 * Preconditions        : Rtp Stack shall be initialized.
 * Side Effects            : None
 ********************************************************/
eRTP_STATUS_CODE RtpStack::deleteRtpSession(IN RtpSession* pobjSession)
{
    if(pobjSession == RTP_NULL)
    {
        RTP_TRACE_WARNING("deleteRtpSession, pobjSession is NULL ...!",
                RTP_ZERO,RTP_ZERO);
        return RTP_INVALID_PARAMS;
    }

    eRtp_Bool bisRtpSes = eRTP_SUCCESS;
    RtpDt_UInt16 usPosition = RTP_ZERO;
    bisRtpSes = isRtpSessionPresent(pobjSession, &usPosition);

    if(bisRtpSes == eRTP_SUCCESS)
    {
        RtpDt_UInt16 usError = RTP_ZERO;
        m_objSessions.DeleteAtPos(usPosition, &usError);
        if(usError == ERR_LIST_INV_INPUT)
        {
            return RTP_FAILURE;
        }

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
