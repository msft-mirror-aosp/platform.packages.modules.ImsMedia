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

#include <RtpHeader.h>
#include <rtp_trace.h>
#include <rtp_error.h>

#include <stdio.h>

eRtp_Bool Rtp_ReleaseCsrcElm(IN RtpDt_Void **ppObj)
{
    RtpDt_UInt32 *puiCsrcElm = RTP_NULL;
    puiCsrcElm = RTP_STATIC_CAST(RtpDt_UInt32*,*ppObj);
    delete puiCsrcElm;
    return eRTP_SUCCESS;
}

eRtp_Bool Rtp_CompareCsrcElm(IN RtpDt_Void *pvNode1,
                                     IN RtpDt_Void *pvNode2)
{
    if(*(RTP_STATIC_CAST(RtpDt_UInt32*,pvNode1)) != RTP_NULL
        && *(RTP_STATIC_CAST(RtpDt_UInt32*,pvNode2)) != RTP_NULL)
    {
        if(*(RTP_STATIC_CAST(RtpDt_UInt32*,pvNode1)) == *(RTP_STATIC_CAST(RtpDt_UInt32*,pvNode2)))
            return eRTP_TRUE;
        else
            return eRTP_FALSE;
    }
    else
        return eRTP_FALSE;
}

RtpHeader::RtpHeader():
            m_ucVersion(RTP_ZERO),
            m_ucPadding(RTP_ZERO),
            m_ucExtension(RTP_ZERO),
            m_ucCsrcCnt(RTP_ZERO),
            m_ucMarker(RTP_ZERO),
            m_ucPldType(RTP_ZERO),
            m_usSeqNum(RTP_ZERO),
            m_uiTimestamp(RTP_ZERO),
            m_uiSsrc(RTP_ZERO)

{
    m_objCsrcList.InitList(Rtp_ReleaseCsrcElm,
        Rtp_CompareCsrcElm);
}

RtpHeader::~RtpHeader()
{
    RtpDt_UInt16 usSize = RTP_ZERO;
    RtpDt_UInt16 usError = RTP_ZERO;

    m_objCsrcList.GetSize(&usSize, &usError);
    for(RtpDt_UInt16 usCount = RTP_ZERO;usCount < usSize;
        usCount = usCount + RTP_ONE)
    {
        m_objCsrcList.DeleteAtPos(RTP_ZERO, &usError);
    }
}

RtpDt_Void RtpHeader::setVersion(IN RtpDt_UChar ucVersion)
{
    m_ucVersion = ucVersion;
}

RtpDt_UChar RtpHeader::getVersion()
{
    return m_ucVersion;
}

RtpDt_Void RtpHeader::setPadding()
{
    m_ucPadding = RTP_ONE;
}

RtpDt_UChar RtpHeader::getPadding()
{
    return m_ucPadding;
}

RtpDt_Void RtpHeader::setExtension(RtpDt_UChar ext)
{
    m_ucExtension = ext;
}

RtpDt_UChar RtpHeader::getExtension()
{
    return m_ucExtension;
}

RtpDt_Void RtpHeader::setCsrcCount(IN RtpDt_UChar ucCsrcCnt)
{
    m_ucCsrcCnt = ucCsrcCnt;
}

RtpDt_UChar RtpHeader::getCsrcCount()
{
    return m_ucCsrcCnt;
}

RtpList* RtpHeader::getCsrcList()
{
    return &m_objCsrcList;
}

eRtp_Bool RtpHeader::addElmToCsrcList(IN RtpDt_UInt32 uiCsrc)
{
    RtpDt_UInt32 *puiCsrcElm = new RtpDt_UInt32();
    if(puiCsrcElm == RTP_NULL)
    {
        return eRTP_FALSE;
    }
    *puiCsrcElm = uiCsrc;

    RtpDt_UInt16 usError = RTP_ZERO;
    m_objCsrcList.Append(puiCsrcElm, &usError);
    return eRTP_TRUE;
}

RtpDt_Void RtpHeader::setMarker()
{
    m_ucMarker = RTP_ONE;
}

RtpDt_UChar RtpHeader::getMarker()
{
    return m_ucMarker;
}

RtpDt_Void RtpHeader::setPldType(IN RtpDt_UChar ucPldType)
{
    m_ucPldType = ucPldType;
}

RtpDt_UChar RtpHeader::getPldType()
{
    return m_ucPldType;
}

RtpDt_Void RtpHeader::setSeqNum(IN RtpDt_UInt16 usSeqNum)
{
    m_usSeqNum = usSeqNum;
}

RtpDt_UInt16 RtpHeader::getSeqNum()
{
    return m_usSeqNum;
}

RtpDt_Void RtpHeader::setRtpTimestamp(IN RtpDt_UInt32 uiTimestamp)
{
    m_uiTimestamp = uiTimestamp;
}

RtpDt_UInt32 RtpHeader::getRtpTimestamp()
{
    return m_uiTimestamp;
}

RtpDt_Void RtpHeader::setRtpSsrc(IN RtpDt_UInt32 uiSsrc)
{
    m_uiSsrc = uiSsrc;
}

RtpDt_UInt32 RtpHeader::getRtpSsrc()
{
    return m_uiSsrc;
}

eRtp_Bool RtpHeader::formHeader(IN RtpBuffer* pobjRtpPktBuf)
{
    RtpDt_UInt16 usTmpData    = RTP_ZERO;
    RtpDt_UInt16 usUtlData = RTP_ZERO;

    //get RtpBuffer data
    RtpDt_UChar *pcRtpHdrBuf = pobjRtpPktBuf->getBuffer();

    //version 2 bits
    RTP_FORM_HDR_UTL(usUtlData, m_ucVersion, RTP_VER_SHIFT_VAL, usTmpData);

    //padding 1 bit
    RTP_FORM_HDR_UTL(usUtlData, m_ucPadding, RTP_PAD_SHIFT_VAL, usTmpData);

    //extension 1 bit
    RTP_FORM_HDR_UTL(usUtlData, m_ucExtension, RTP_EXT_SHIFT_VAL, usTmpData);

    //CC. CSRC count 4 bits.
    RTP_FORM_HDR_UTL(usUtlData, m_ucCsrcCnt, RTP_CC_SHIFT_VAL, usTmpData);

    //Marker. 1 bit
    RTP_FORM_HDR_UTL(usUtlData, m_ucMarker, RTP_MARK_SHIFT_VAL, usTmpData);

    //payload type. 7 bits
    RTP_FORM_HDR_UTL(usUtlData, m_ucPldType, RTP_PLTYPE_SHIFT_VAL, usTmpData);

    RtpDt_UInt32 uiByte4Data = usTmpData;
    uiByte4Data = uiByte4Data << RTP_SIXTEEN;

    //sequence number. 16 bits
    uiByte4Data = uiByte4Data | m_usSeqNum;

    *(RtpDt_UInt32*)pcRtpHdrBuf = RtpOsUtil::Ntohl(uiByte4Data);
    pcRtpHdrBuf = pcRtpHdrBuf + RTP_WORD_SIZE;

    //time stamp
    *(RtpDt_UInt32*)pcRtpHdrBuf = RtpOsUtil::Ntohl(m_uiTimestamp);
    pcRtpHdrBuf = pcRtpHdrBuf + RTP_WORD_SIZE;

    //ssrc
    *(RtpDt_UInt32*)pcRtpHdrBuf = RtpOsUtil::Ntohl(m_uiSsrc);
    pcRtpHdrBuf = pcRtpHdrBuf + RTP_WORD_SIZE;

    RtpDt_UInt32 uiBufLen = RTP_ZERO;
    uiBufLen = uiBufLen + RTP_FIXED_HDR_LEN;

    //csrc list
    RtpDt_UInt16 usError = RTP_ZERO;
    RtpDt_UInt16 usSize = RTP_ZERO;
    m_objCsrcList.GetSize(&usSize, &usError);

    for(RtpDt_UInt16 usCount = RTP_ZERO; usCount < usSize; usCount++)
    {
        RtpDt_Void    *pvElement = RTP_NULL;
        RtpDt_UInt32 *puiCsrc = RTP_NULL;

        m_objCsrcList.GetElement(usCount,&pvElement, &usError);
        if(usError == ERR_LIST_INV_INPUT)
        {
            RTP_TRACE_WARNING(" Error in fetching the element from list...!",
                RTP_ZERO,RTP_ZERO);
            return eRTP_FALSE;
        }

        //typecast to RtpDt_UInt32*
        puiCsrc = RTP_STATIC_CAST(RtpDt_UInt32*,pvElement);

        *(RtpDt_UInt32*)pcRtpHdrBuf = RtpOsUtil::Ntohl(*puiCsrc);
        pcRtpHdrBuf = pcRtpHdrBuf + RTP_WORD_SIZE;
    }

    uiBufLen = uiBufLen + (RTP_WORD_SIZE * usSize);

    pobjRtpPktBuf->setLength(uiBufLen);

    return eRTP_TRUE;
}

eRtp_Bool RtpHeader::decodeHeader(IN RtpBuffer* pobjRtpPktBuf,
                                      OUT RtpDt_UInt32 &uiBufPos)
{
    //get RtpBuffer data
    RtpDt_UChar *pcRtpHdrBuf = pobjRtpPktBuf->getBuffer();

    RtpDt_UInt32 uiByte4Data = RtpOsUtil::Ntohl(*((RtpDt_UInt32*)pcRtpHdrBuf));
    pcRtpHdrBuf = pcRtpHdrBuf + RTP_FOUR;
    uiBufPos  = uiBufPos + RTP_WORD_SIZE;

    //version 2 bits
    RtpDt_UInt16 usUtl2Data = (RtpDt_UInt16)(uiByte4Data >> RTP_SIXTEEN);
    m_ucVersion = (RtpDt_UChar) (usUtl2Data >> RTP_VER_SHIFT_VAL);

    //padding 1 bit
    m_ucPadding = (RtpDt_UChar) ((usUtl2Data >> RTP_PAD_SHIFT_VAL) & RTP_HEX_1_BIT_MAX);

    //extension 1 bit
    m_ucExtension = (RtpDt_UChar) ((usUtl2Data >> RTP_EXT_SHIFT_VAL) & RTP_HEX_1_BIT_MAX);

    //CC. CSRC count 4 bits.
    m_ucCsrcCnt = (RtpDt_UChar) ((usUtl2Data >> RTP_CC_SHIFT_VAL) & RTP_HEX_4_BIT_MAX);


    //Marker. 1 bit
    usUtl2Data  = usUtl2Data & RTP_HEX_8_BIT_MAX;
    m_ucMarker = (RtpDt_UChar) ((usUtl2Data >> RTP_MARK_SHIFT_VAL) & RTP_HEX_1_BIT_MAX);

    //payload type. 7 bits
    m_ucPldType = (RtpDt_UChar) ((usUtl2Data) & RTP_HEX_7_BIT_MAX);

    //sequence number. 16 bitsusError
    m_usSeqNum = (RtpDt_UInt16) (uiByte4Data & RTP_HEX_16_BIT_MAX);

    //timestamp
    m_uiTimestamp = RtpOsUtil::Ntohl(*((RtpDt_UInt32*)pcRtpHdrBuf));
    pcRtpHdrBuf = pcRtpHdrBuf + RTP_FOUR;
    uiBufPos  = uiBufPos + RTP_WORD_SIZE;

    //Synchronization source
    m_uiSsrc = RtpOsUtil::Ntohl(*((RtpDt_UInt32*)pcRtpHdrBuf));
    pcRtpHdrBuf = pcRtpHdrBuf + RTP_FOUR;
    uiBufPos  = uiBufPos + RTP_WORD_SIZE;

    RtpDt_UInt16 usError = RTP_ZERO;

    //csrc list
    for(RtpDt_UInt32 usCsrcIdx=RTP_ZERO; usCsrcIdx < m_ucCsrcCnt;
                                usCsrcIdx++)
    {
        RtpDt_UInt32 *puiCsrcItem = new RtpDt_UInt32();
        if(puiCsrcItem == RTP_NULL)
        {
            return eRTP_FALSE;
        }
        *(puiCsrcItem) = RtpOsUtil::Ntohl(*((RtpDt_UInt32*)pcRtpHdrBuf));
        pcRtpHdrBuf = pcRtpHdrBuf + RTP_FOUR;
        uiBufPos  = uiBufPos + RTP_WORD_SIZE;

        RTP_TRACE_NORMAL("Got %d CSRC[%d]",usCsrcIdx,*puiCsrcItem);
        //append puiCsrcItem into list.
        m_objCsrcList.Append(puiCsrcItem, &usError);
    }

    return eRTP_SUCCESS;
}
