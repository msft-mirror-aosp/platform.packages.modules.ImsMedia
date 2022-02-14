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

#include <RtpActiveSessionDb.h>
#include <RtpError.h>

RtpActiveSessionDb* RtpActiveSessionDb::m_pInstance = RTP_NULL;


// constructor
RtpActiveSessionDb::RtpActiveSessionDb()
{

}

// destructor
RtpActiveSessionDb::~RtpActiveSessionDb()
{
}

// it creates RtpActiveSessionDb instance.
RtpActiveSessionDb* RtpActiveSessionDb::getInstance()
{
    if(m_pInstance == RTP_NULL)
    {
        m_pInstance = new RtpActiveSessionDb();
    }
    return m_pInstance;
}

// add rtp session
eRtp_Bool RtpActiveSessionDb::addRtpSession(IN RtpDt_Void* pvData)
{
    m_list.push_back(pvData);
    return eRTP_SUCCESS;
}

// it validates the rtp session pointer
eRtp_Bool RtpActiveSessionDb::validateRtpSession(IN RtpDt_Void* pvData,
                                               OUT RtpDt_UInt16* pusPosition)
{
    *pusPosition = RTP_ZERO;
    RtpDt_UInt16 usPos = RTP_ZERO;
    for (auto&i:m_list)
    {
        if(i == RTP_NULL)
        {
            return eRTP_FALSE;
        }

        if(i == pvData)
        {
            *pusPosition = usPos;
            return eRTP_TRUE;
        }
        usPos++;
    }

    return eRTP_FALSE;
}

// it deletes the rtp session pointer from db
eRtp_Bool RtpActiveSessionDb::deleteRtpSession(IN RtpDt_Void* pvData)
{
    m_list.remove(pvData);
    return eRTP_TRUE;
}
