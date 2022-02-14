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

#ifndef __RTP_ACTIVE_SESSIONDB_H__
#define __RTP_ACTIVE_SESSIONDB_H__

#include <RtpGlobal.h>
#include <RtpList.h>
#include <list>

/**
* @class    RtpActiveSessionDb
* @brief    It maintains the active session count
*/
class RtpActiveSessionDb
{
    private:
        // RtpActiveSessionDb pointer.
        static RtpActiveSessionDb* m_pInstance;

        // It maintains the list of active rtp sessions
        std::list<RtpDt_Void*> m_list;

        // constructor
        RtpActiveSessionDb();

        // destructor
        ~RtpActiveSessionDb();

    public:
        // it creates RtpActiveSessionDb instance.
        static RtpActiveSessionDb* getInstance();

        // add rtp session
        eRtp_Bool addRtpSession(IN RtpDt_Void* pvData);

        // it validates the rtp session pointer
        eRtp_Bool validateRtpSession(IN RtpDt_Void* pvData, OUT RtpDt_UInt16* pusPosition);

        // it deletes the rtp session pointer from db
        eRtp_Bool deleteRtpSession(IN RtpDt_Void* pvData);
};

#endif /* __RTP_ACTIVE_SESSIONDB_H__*/

/** @}*/
