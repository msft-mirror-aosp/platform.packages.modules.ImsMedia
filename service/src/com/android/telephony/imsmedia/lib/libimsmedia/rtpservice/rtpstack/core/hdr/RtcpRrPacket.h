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

#ifndef __RTCP_RR_PACKET_H__
#define __RTCP_RR_PACKET_H__

#include <rtp_global.h>
#include <RtcpHeader.h>
#include <RtpBuffer.h>
#include <RtpList.h>
#include <RtcpReportBlock.h>


/**
* @class         rtcp_rr_packet
* @brief         It holds RR packet information
*/
class RtcpRrPacket
{
    private:
        // RTCP header information
        RtcpHeader m_objRtcpHdr;

        // List of RtcpReportBlock objects
        RtpList m_objReportBlk;

        /**
         * Extension header buffer. This is encoded and given by app.
         * After decoding, ExtractExtHeaders, will update this with the extension
         * header buffer
         */
        RtpBuffer* m_pobjExt;

        /**
         * It adds RtcpReportBlock object to m_objReportBlk
         */
        RtpDt_Void addReportBlkElm(IN RtcpReportBlock* pobjReptBlk);

    public:
        // Constructor
        RtcpRrPacket();
        // Destructor
        ~RtcpRrPacket();

        /**
         * get method for m_objRtcpHdr
         */
        RtcpHeader* getRtcpHdrInfo();

        /**
         * get method for m_objReportBlk
         */
        RtpList* getReportBlkList();

        /**
         * get method for m_pobjExt
         */
        RtpBuffer* getExtHdrInfo();

        /**
         * set method for m_pobjExt
         */
        RtpDt_Void setExtHdrInfo(IN RtpBuffer* pobjExtHdr);

        /**
         * Decodes and stores the information of the RTCP RR packet
         * This function does not allocate memory required for decoding.
         *
         * @param pucRrBuf      Rr packet buffer
         * @param usRrLen       Rr packet length
         * @param usExtHdrLen   RTCP extension header length
         * @param bIsRrPkt      It tells the packet belongs to RR packet or to Report block
         *
         * @return RTP_SUCCESS on successful decoding
         */
        eRTP_STATUS_CODE decodeRrPacket(IN RtpDt_UChar* pucRrBuf,
                           IN RtpDt_UInt16 &usRrLen,
                           IN RtpDt_UInt16 usProfExtLen,
                           IN eRtp_Bool bIsRrPkt);

        /**
         * Performs the encoding of the RTCP RR packet.
         * This function does not allocate memory required for encoding.
         *
         * @param pobjRtcpPktBuf    Memory for the buffer is pre-allocated by caller
         * @param bHdrInfo          tells RTCP header shall be encoded or not.
         *
         * @return RTP_SUCCESS on successful encoding
         */
        eRTP_STATUS_CODE formRrPacket(OUT RtpBuffer* pobjRtcpPktBuf,
                                 IN eRtp_Bool bHdrInfo);

}; // end of RtcpRrPacket

#endif    //__RTCP_RR_PACKET_H__

/** @}*/
