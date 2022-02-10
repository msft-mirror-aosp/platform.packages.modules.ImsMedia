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

#include <rtp_pf_memory.h>
#include <RtpList.h>
#include <rtp_error.h>
#include <stdio.h>
#include <rtp_trace.h>

RtpList::RtpList()
{
    pstHead = RTP_NULL;
    pstTail = RTP_NULL;
    usCount = RTP_ZERO;
}

RtpList::RtpList
(
    UfRtpListFp_FreeElement    stPfnFxnFreeMem,
    UfRtpListFp_CompareElement    stPfnFxnComp
)
{
    pstHead = RTP_NULL;
    pstTail = RTP_NULL;
    usCount = RTP_ZERO;

    if (stPfnFxnFreeMem)
    {
        stFxnFreeEle = stPfnFxnFreeMem;
    }
    else
    {
        stFxnFreeEle = RTP_NULL;
        /*
        *pusError = eErr_ListFreeFxnNotSet;
        return eRTP_FAILURE;
        */
    }
    stFxnCmpEle = stPfnFxnComp;

}

RtpDt_Void RtpList::InitList
(
    UfRtpListFp_FreeElement    stPfnFxnFreeMem,
    UfRtpListFp_CompareElement    stPfnFxnComp
)
{

    pstHead = RTP_NULL;
    pstTail = RTP_NULL;
    usCount = RTP_ZERO;

    if (stPfnFxnFreeMem)
    {
        stFxnFreeEle = stPfnFxnFreeMem;
    }
    else
    {
        stFxnFreeEle = RTP_NULL;
    }
    stFxnCmpEle = stPfnFxnComp;
}

eRtp_Bool RtpList::Append
(
    RtpDt_Void    *pvElement,
    RtpDt_UInt16    *pusError
)
{
    RtpSt_Node *pstNode = RTP_NULL;

/*    if (pstList == RTP_NULL)
    {
        *pusError = eErr_ListNotExist;
        return eRTP_FAILURE;
    }
*/
    pstNode = new RtpSt_Node;

    //pstNode = (RtpSt_Node*)RtpPf_MallocMemset(sizeof(RtpSt_Node));
    if (pstNode == RTP_NULL)
    {
        *pusError  = ERR_MALLOC_FAILED;
        return eRTP_FAILURE;
    }
    RtpPf_Memset(pstNode,0x00,sizeof(RtpSt_Node));
    pstNode->pvElement = pvElement;

    //this is first node
    if (usCount == RTP_ZERO)
    {
        pstHead =     pstNode;
    }
    else
    {
        pstTail->pstNext = pstNode ;
    }
    pstTail = pstNode;
    usCount++;

    *pusError = eRTP_SUCCESS;
    return eRTP_SUCCESS;
}

eRtp_Bool RtpList::DeleteAtPos
(
    RtpDt_UInt16 usPos,
    RtpDt_UInt16 *pusError
)
{

    if (usPos > usCount || pstHead == RTP_NULL)
    {
        *pusError = ERR_LIST_INV_INPUT;
        return eRTP_FAILURE;
    }

    RtpSt_Node *pstNode = pstHead;
    //if first node
    if (usPos == RTP_ZERO)
    {
        pstHead = pstNode->pstNext;
        stFxnFreeEle(&(pstNode->pvElement));
        pstNode->pvElement = RTP_NULL;
        pstNode->pstNext = RTP_NULL;
        delete pstNode;
        pstNode = RTP_NULL;
    }
    else
    {
        for (RtpDt_UInt16 usSize = RTP_ZERO; usSize < (usPos - RTP_ONE); usSize++)
        {
            pstNode = pstNode->pstNext;
        }

        RtpSt_Node *pstTempNode = pstNode->pstNext;
        pstNode->pstNext = pstTempNode->pstNext;
        //if last node is to be deleted
        if (pstTempNode->pstNext == RTP_NULL)
        {
            //set tail
            pstTail = pstNode;
        }

        stFxnFreeEle(&pstTempNode->pvElement);
        pstTempNode->pvElement = RTP_NULL;
        pstTempNode->pstNext = RTP_NULL;
        delete pstTempNode;
    }

    usCount--;

    //if list empty
    if (usCount == RTP_ZERO)
    {
        pstHead = RTP_NULL;
        pstTail = RTP_NULL;
    }

    return eRTP_SUCCESS;

}

RtpList::~RtpList()
{
    RtpDt_UInt16 usError = RTP_ZERO;

    RtpDt_UInt16 usSize = usCount;
    for (RtpDt_Int16 sIndex = usSize-RTP_ONE;sIndex >=RTP_ZERO;sIndex--)
    {
        DeleteAtPos(sIndex,&usError);
    }
//    RtpPf_Free((RtpDt_Void **)ppstList);
//    *ppstList = RTP_NULL;
//    return eRTP_SUCCESS;

}

eRtp_Bool RtpList::GetElement
(
    RtpDt_UInt16 usPos,
    RtpDt_Void        **ppvElement,
    RtpDt_UInt16    *pusError
)
{

    if (usPos >= usCount || pstHead == RTP_NULL)
    {
        *pusError = ERR_LIST_INV_INPUT;
        return eRTP_FAILURE;
    }

    RtpSt_Node *pstNode = pstHead;

    for (RtpDt_UInt16 iIndex = RTP_ZERO;((iIndex < usPos) && (iIndex < usCount));iIndex++)
    {
        pstNode = pstNode->pstNext;
    }

    *ppvElement = pstNode->pvElement;

    return eRTP_SUCCESS;


}

eRtp_Bool RtpList::GetSize(RtpDt_UInt16 *pusSize,RtpDt_UInt16* /*pusError*/)
{
    *pusSize = usCount;
    return eRTP_SUCCESS;
}
