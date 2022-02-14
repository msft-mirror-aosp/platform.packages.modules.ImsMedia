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

#ifndef __RTP_LIST_H__
#define __RTP_LIST_H__

#include <RtpPfDatatypes.h>

typedef eRtp_Bool (*UfRtpListFp_FreeElement)(RtpDt_Void **ppvMemPtr);
typedef eRtp_Bool (*UfRtpListFp_CompareElement)(RtpDt_Void *pvNode1, RtpDt_Void *pvNode2);

typedef struct _RtpSt_Node
{
    /* void type data */
    RtpDt_Void    *pvElement;
    /*Pointer to the next node*/
    struct _RtpSt_Node    *pstNext;

}RtpSt_Node;

/**
 * @class RtpList
 * @brief Implements linked list
 */
class RtpList{
public:
    RtpList();

    /**
     * This function creates a list. The free function should be set for initializing.
     *
     * @param ppstList  List created
     *
     * @param stPfnFxnFreeMem   (Mandatory) Pointer to free function to free node data.
     *
     * @param stPfnFxnComp (Optional) Pointer to compare function to compare nodes
     *
     * @param pusError Error if any
     */
    RtpList(UfRtpListFp_FreeElement stPfnFxnFreeMem, UfRtpListFp_CompareElement stPfnFxnComp);

    RtpDt_Void InitList(UfRtpListFp_FreeElement stPfnFxnFreeMem,
                        UfRtpListFp_CompareElement stPfnFxnComp);

    virtual ~RtpList();

    /**
     * This function adds a node to the end of the list.
     *
     * @param pstList   List to be updated.
     * @param pvElement Data to insert in node.
     * @param pusError  Error if any
     */
    eRtp_Bool Append
    (
        RtpDt_Void    *pvElement,
        RtpDt_UInt16    *pusError
   );

    /**
     * This function deletes list node  from the position and also free the node element.
     *
     * @param pstList   List to be modified.
     * @param usPos     Position of node to be deleted.
     * @param pusError  Error if any.
     */
    eRtp_Bool DeleteAtPos
    (
        RtpDt_UInt16 usPos,
        RtpDt_UInt16    *pusError
   );

    /**
     * This function fetches element from a specified node position.
     * For First Elemenent, use usPos = 0.
     *
     * @param pstList   list from which data should be fetched
     * @param usPos     Position of element to be fetched
     * @param ppvElement Element fetched
     * @param pusError   Error if any
     */
    eRtp_Bool GetElement
    (
        RtpDt_UInt16 usPos,
        RtpDt_Void        **ppvElement,
        RtpDt_UInt16    *pusError
   );

    /**
     * This function returns the number of nodes in the list.
     *
     * @param pstList   List to be checked
     * @param pusSize   Size of list
     * @param pusError  Error if any
     */
    eRtp_Bool GetSize
    (
        RtpDt_UInt16 *pusSize,
        RtpDt_UInt16    *pusError
   );

private:
    RtpSt_Node    *pstHead;   /*First Node*/
    RtpSt_Node    *pstTail;    /*Last Node*/

    RtpDt_UInt16  usCount;    /*Node Count*/
    UfRtpListFp_FreeElement       stFxnFreeEle;/*free function pointer*/
    UfRtpListFp_CompareElement    stFxnCmpEle;/*compare function pointer*/
};

#endif // __RTP_LIST_H__

/** @}*/
