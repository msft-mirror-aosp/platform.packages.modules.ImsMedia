/** \addtogroup  RTP_Stack
 *  @{
 */

 /**
  * @brief This represents the RTP stack. This class stores one instance of a the stack.
  * RTP sessions should be created as part of an RtpStack instance.
  * Each instance can have any number of unrelated RTP sessions which share only the profile as
  * defined by RtpStackProfile.
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

#ifndef __RTP_STACK_H__
#define __RTP_STACK_H__

#include <RtpGlobal.h>
#include <RtpStackProfile.h>
#include <RtpSession.h>
#include <list>

class RtpSession;

class RtpStack
{
    /**
    list of Rtp_Session currently active in the stack
    */
    std::list<RtpSession *> m_objRtpSessionList;
    /**
    Profile for this stack
    */
    RtpStackProfile* m_pobjStackProfile;

public:
    /**
    Create stack with default profile
    */
    RtpStack();
    /**
    Delete stack
    */
    ~RtpStack();

    /**
     * Create stack with pobjStackProfile.
     * However application can modify this profile at a later stage by using setStackProfile().
     * @param[in] pobjStackProfile Configure the stack as per profile.
     */
    RtpStack(IN RtpStackProfile* pobjStackProfile);


    /**
     * Create a RTP session.
     */
    RtpSession* createRtpSession();

    /**
     * Delete a RTP session.
     * memory of pobjSession will be freed
     */
    eRTP_STATUS_CODE deleteRtpSession(IN RtpSession* pobjSession);
    /**
     * Get method for m_pobjStackProfile
     */
    RtpStackProfile* getStackProfile();
    /**
     * Set method for m_pobjStackProfile
     */
    RtpDt_Void setStackProfile(IN RtpStackProfile* pobjStackProfile);

    eRtp_Bool isRtpSessionPresent(IN RtpSession* pobjSession);
};

#endif    //__RTP_STACK_H__

/** @}*/
