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

#ifndef __RTCP_HEADER_H__
#define __RTCP_HEADER_H__

#include <rtp_global.h>
#include <RtpBuffer.h>

 /**
 * @class   RtcpHeader
 * @brief   This class provides RTCP packet encode and decode functionality.
 *          RTCP header fields can be accessed via get/set APIs.
 */
class RtcpHeader
{
    private:
        // m_ucVersion identifies the version of RTCP
        RtpDt_UChar m_ucVersion;

        /**
         * If the padding bit is set, RTCP packet contains
         * some additional padding octets at the end which are not part of
         * the control information but are included in the length field.
         */
        RtpDt_UChar m_ucIsPadding;

        /**
         * The number of reception report blocks contained in this packet.
         * A value of zero is valid.
         */
        RtpDt_UChar m_ucRecepRepCnt;

        // It Identifies the packet type. Ex:- SR=200, RR=201...
        RtpDt_UChar m_ucPktType;

        /**
         * The length of the RTCP packet in 32-bit words minus one,
         * including the header and any padding
         */
        RtpDt_UInt16 m_usLength;

        // Synchronization source.
        RtpDt_UInt32    m_uiSsrc;

    public:
        // Constructor
        RtcpHeader();

        // Destructor
        ~RtcpHeader();

        /**
         *  set method for m_ucVersion
         */
        RtpDt_Void setVersion(IN RtpDt_UChar ucVersion);

        /**
         * get method for m_ucVersion
         */
        RtpDt_UChar getVersion();

        /**
         * set method for m_ucPadding
         */
        RtpDt_Void setPadding();

        /**
         * get method for m_ucPadding
         */
        RtpDt_UChar getPadding();

        /**
         * set method for m_ucRecepRepCnt
         */
        RtpDt_Void setRecepRepCnt(IN RtpDt_UChar ucRecReport);

        /**
         * get method for m_ucRecepRepCnt
         */
        RtpDt_UChar getRecepRepCnt();

        /**
         * set method for m_ucPktType
         */
        RtpDt_Void setPacketType(IN RtpDt_UChar ucPktType);

        /**
         * get method for m_ucPktType
         */
        RtpDt_UChar getPacketType();

        /**
         * set method for m_usLength
         */
        RtpDt_Void setLength(IN RtpDt_UInt16 usLength);

        /**
         * get method for m_usLength
         */
        RtpDt_UInt16 getLength();

        /**
         * set method for m_uiSsrc
         */
        RtpDt_Void setSsrc(IN RtpDt_UInt32 uiSsrc);

        /**
         * get method for m_uiSsrc
         */
        RtpDt_UInt32 getSsrc();

        /**
         * Decodes and stores the information of the RTCP Header
         * This function does not allocate memory required for decoding.
         * @param[in] pobjRtcpPktBuf Memory for the buffer is pre-allocated by caller
         * @return eRTP_SUCCESS on successful decoding
        */
        eRtp_Bool decodeRtcpHeader(IN RtpDt_UChar *pucRtcpHdr);

        /**
         * Performs the encoding of the RTCP Header.
         * This function does not allocate memory required for encoding.
         * @param[out] pobjRtcpPktBuf Memory for the buffer is pre-allocated by caller
         * @return eRTP_SUCCESS on successful encoding
        */
        eRtp_Bool formRtcpHeader(OUT RtpBuffer* pobjRtcpPktBuf);

        /**
         * Performs the encoding of the first 4 octets of the RTCP Header.
         * This function does not allocate memory required for encoding.
         * @param[out] pobjRtcpPktBuf Memory for the buffer is pre-allocated by caller
         * @return eRTP_SUCCESS on successful encoding
        */
        eRtp_Bool formPartialRtcpHeader(OUT RtpBuffer* pobjRtcpPktBuf);

        /**
         * It populates RTCP header information
         */
        RtpDt_Void populateRtcpHeader(IN RtpDt_UChar ucRecepRepCnt,
                                     IN RtpDt_UChar ucPktType,
                                     IN RtpDt_UInt32 uiSsrc);

}; // end of RtcpHeader


#endif    //__RTCP_HEADER_H__

/** @}*/
