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

#include <RtcpChunk.h>
#include <rtp_trace.h>
#include <rtp_pf_memory.h>

#include <stdio.h>

eRtp_Bool Rtp_ReleaseSdesItem(IN RtpDt_Void **ppObj)
{
    tRTCP_SDES_ITEM *pstChunkItem = RTP_NULL;
    pstChunkItem = RTP_STATIC_CAST(tRTCP_SDES_ITEM*,*ppObj);
    if(pstChunkItem->pValue != RTP_NULL)
    {
        delete[] pstChunkItem->pValue;
    }
    delete pstChunkItem;
    return eRTP_SUCCESS;
}

eRtp_Bool Rtp_CompareSDesItem(IN RtpDt_Void *pvNode1,
                                     IN RtpDt_Void *pvNode2)
{
    (RtpDt_Void)pvNode1, (RtpDt_Void)pvNode2;
    return eRTP_TRUE;
}

RtcpChunk::RtcpChunk():
                m_uiSsrc(RTP_ZERO)
{
    //initialize the tRTCP_SDES_ITEM list

    m_objSdesItem.InitList(Rtp_ReleaseSdesItem,
                            Rtp_CompareSDesItem);
}

RtcpChunk::~RtcpChunk()
{
    RtpDt_UInt16 usSize = RTP_ZERO;
    RtpDt_UInt16 usError = RTP_ZERO;

    //m_objSdesItem
    m_objSdesItem.GetSize(&usSize, &usError);
    for(RtpDt_UInt16 usCount = RTP_ZERO;usCount < usSize;
        usCount = usCount + RTP_ONE)
    {
        m_objSdesItem.DeleteAtPos(RTP_ZERO, &usError);
    }
}

RtpDt_Void RtcpChunk::setSsrc(IN RtpDt_UInt32 uiSsrc)
{
    m_uiSsrc = uiSsrc;
}

RtpDt_UInt32 RtcpChunk::getSsrc()
{
    return m_uiSsrc;
}

RtpList* RtcpChunk::getSdesItemList()
{
    return &m_objSdesItem;
}

eRTP_STATUS_CODE RtcpChunk::decodeRtcpChunk(IN RtpDt_UChar* pucChunkBuf,
                                         IN RtpDt_UInt16 &usChunkLen,
                                         IN RtcpConfigInfo *pobjRtcpCfgInfo)
{

    //SSRC
    m_uiSsrc = RtpOsUtil::Ntohl(*((RtpDt_UInt32*)pucChunkBuf));
    pucChunkBuf = pucChunkBuf + RTP_WORD_SIZE;
    usChunkLen = usChunkLen + RTP_WORD_SIZE;

    //SDES items
    RtpDt_UInt32 uiSdesItemCnt = pobjRtcpCfgInfo->getSdesItemCount();
    tRTCP_SDES_ITEM *pstSdesItem = RTP_NULL;
    eRtp_Bool bCName = eRTP_FALSE;

    while(uiSdesItemCnt > RTP_ZERO)
    {
        RtpDt_UInt16    usError;
        RtpDt_UChar    *pcSdesBuf = RTP_NULL;

        pstSdesItem = new tRTCP_SDES_ITEM();
        if(pstSdesItem == RTP_NULL)
        {
            RTP_TRACE_WARNING("decodeRtcpChunk, new returned NULL...!",
                RTP_ZERO,RTP_ZERO);
            return RTP_MEMORY_FAIL;
        }
        RtpPf_Memset(pstSdesItem, RTP_ZERO, sizeof(tRTCP_SDES_ITEM));

        //type
        pstSdesItem->ucType = *(RtpDt_UChar*)pucChunkBuf;
        pucChunkBuf = pucChunkBuf + RTP_ONE;
        usChunkLen = usChunkLen + RTP_ONE;

        if(pstSdesItem->ucType == RTP_ONE)
        {
            bCName = eRTP_TRUE;
        }
        //length
        pstSdesItem->ucLength = *(RtpDt_UChar*)pucChunkBuf;
        pucChunkBuf = pucChunkBuf + RTP_ONE;
        usChunkLen = usChunkLen + RTP_ONE;

        RTP_TRACE_NORMAL("decodeRtcpChunk , [Sdes item type =%d], [Sdes item length = %d]",
                pstSdesItem->ucType, pstSdesItem->ucLength);

        //value
        pcSdesBuf = new RtpDt_UChar[pstSdesItem->ucLength];
        if(pcSdesBuf == RTP_NULL)
        {
            RTP_TRACE_WARNING("decodeRtcpChunk, new returned NULL...!",
                RTP_ZERO,RTP_ZERO);
            delete pstSdesItem;
            return RTP_MEMORY_FAIL;
        }
        RtpPf_Memcpy(pcSdesBuf, pucChunkBuf, pstSdesItem->ucLength);

        pucChunkBuf = pucChunkBuf + pstSdesItem->ucLength;
        usChunkLen = usChunkLen + pstSdesItem->ucLength;
        pstSdesItem->pValue = pcSdesBuf;

        m_objSdesItem.Append(pstSdesItem, &usError);

        //decrement uiSdesItemCnt by 1
        uiSdesItemCnt = uiSdesItemCnt - RTP_ONE;
    }//while

    if(bCName == eRTP_FALSE)
    {
        return RTP_DECODE_ERROR;
    }

    return RTP_SUCCESS;
}//decodeRtcpChunk


eRTP_STATUS_CODE RtcpChunk::formRtcpChunk(OUT RtpBuffer* pobjRtcpPktBuf)
{
    RtpDt_UInt32 uiCurPos = pobjRtcpPktBuf->getLength();
    RtpDt_UChar *pucBuffer =  pobjRtcpPktBuf->getBuffer();

    pucBuffer = pucBuffer + uiCurPos;

    //m_uiSsrc
    *(RtpDt_UInt32*)pucBuffer = RtpOsUtil::Ntohl(m_uiSsrc);
    pucBuffer = pucBuffer + RTP_WORD_SIZE;
    uiCurPos = uiCurPos + RTP_WORD_SIZE;

    //m_objSdesItem
    RtpDt_UInt16 usSize = RTP_ZERO;
    RtpDt_UInt16 usError = RTP_ZERO;
    m_objSdesItem.GetSize(&usSize, &usError);

    eRtp_Bool bCName = eRTP_FALSE;

    for(RtpDt_UInt32 uiCount = RTP_ZERO;uiCount < usSize;
        uiCount = uiCount + RTP_ONE)
    {
        RtpDt_Void    *pvElement = RTP_NULL;
        m_objSdesItem.GetElement(uiCount,&pvElement, &usError);
        tRTCP_SDES_ITEM *pobjSdesItem = RTP_STATIC_CAST(tRTCP_SDES_ITEM*,pvElement);

        //ucType
        *(RtpDt_UChar*)pucBuffer = pobjSdesItem->ucType;
        pucBuffer = pucBuffer + RTP_ONE;
        uiCurPos = uiCurPos + RTP_ONE;

        if(pobjSdesItem->ucType == RTP_ONE)
        {
            bCName = eRTP_TRUE;
        }

        //ucLength
        *(RtpDt_UChar*)pucBuffer = pobjSdesItem->ucLength;
        pucBuffer = pucBuffer + RTP_ONE;
        uiCurPos = uiCurPos + RTP_ONE;

        //pValue
        RtpPf_Memcpy(pucBuffer, pobjSdesItem->pValue, pobjSdesItem->ucLength);
        pucBuffer = pucBuffer + pobjSdesItem->ucLength;
        uiCurPos = uiCurPos + pobjSdesItem->ucLength;

       //to add type(0)
        uiCurPos = uiCurPos + RTP_ONE;
        pucBuffer[0]=(RtpDt_UChar)RTP_ZERO;
        pucBuffer = pucBuffer + RTP_ONE;

        //to align the memory
        RtpDt_UInt32 uiPadLen = uiCurPos % RTP_WORD_SIZE;
        if(uiPadLen > RTP_ZERO)
        {
            uiPadLen = RTP_WORD_SIZE - uiPadLen;
            uiCurPos = uiCurPos + uiPadLen;
            RtpPf_Memset(pucBuffer, RTP_ZERO, uiPadLen);
            pucBuffer = pucBuffer + uiPadLen;
        }
    }//for

    pobjRtcpPktBuf->setLength(uiCurPos);

    if(bCName == eRTP_FALSE)
    {
        return RTP_ENCODE_ERROR;
    }

    return RTP_SUCCESS;
}//formRtcpChunk
