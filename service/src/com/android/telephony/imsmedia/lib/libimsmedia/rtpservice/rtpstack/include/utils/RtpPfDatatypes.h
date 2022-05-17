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

#ifndef _RTP_PF_DATATYPES_H_
#define _RTP_PF_DATATYPES_H_

#define RTP_NULL            0
#define RTP_FALSE           0
#define RTP_TRUE            1
#define RTP_INVALID         -1
#define RTP_EQUALS          0
#define RTP_MATCHES         0
#define RTP_NOT_MATCH       1
#define RTP_YES             1 /* To Know Request/Response Headaer */
#define RTP_NO              0
#define RTP_INDEX_ZERO      0
#define RTP_START_INDEX     0
#define RTP_ENABLE          1
#define RTP_DISABLE         0
#define RTP_NOT_EXISTS      0
#define RTP_EXISTS          1
#define RTP_NULL_CHAR       '\0'
/* Numerals while used as INDEX or SIZE */
#define RTP_ZERO            0
#define RTP_ONE             1
#define RTP_TWO             2
#define RTP_THREE           3
#define RTP_FOUR            4
#define RTP_FIVE            5
#define RTP_SIX             6
#define RTP_SEVEN           7
#define RTP_EIGHT           8
#define RTP_NINE            9
#define RTP_TEN             10
#define RTP_11              11
#define RTP_12              12
#define RTP_13              13
#define RTP_14              14
#define RTP_15              15
#define RTP_16              16
#define RTP_17              17
#define RTP_18              18
#define RTP_19              19
#define RTP_20              20
#define RTP_21              21
#define RTP_22              22
#define RTP_23              23
#define RTP_24              24
#define RTP_25              25
#define RTP_26              26
#define RTP_27              27
#define RTP_28              28
#define RTP_29              29
#define RTP_30              30
#define RTP_31              31
#define RTP_32              32

#define RTP_SEC_TO_MILLISEC 1000
#define RTP_MILLISEC_MICRO  1000
#define RTP_MIN_SEC         60
#define RTP_HOUR_SEC        3600

/* Function Parameter Notation */
#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef IN_OUT
#define IN_OUT
#endif

#ifndef CONST
#define CONST const
#endif

typedef enum _eRtp_Bool
{
    eRTP_FAILURE = 0,
    eRTP_SUCCESS = 1,
    eRTP_FALSE = 0,
    eRTP_TRUE = 1
} eRtp_Bool;

/* Basic Data Types */
typedef void RtpDt_Void;
typedef unsigned char RtpDt_UChar;
typedef char RtpDt_Char;
typedef signed char RtpDt_SChar;
typedef unsigned short RtpDt_UInt16;
typedef short RtpDt_Int16;
typedef unsigned int RtpDt_UInt32;
typedef int RtpDt_Int32;
typedef unsigned long RtpDt_ULong;
typedef long RtpDt_Long;
typedef double RtpDt_Double;
typedef unsigned int RtpDt_Index_Type;

typedef struct
{
    RtpDt_UInt32 m_uiNtpHigh32Bits;
    RtpDt_UInt32 m_uiNtpLow32Bits;
} tRTP_NTP_TIME;

typedef RtpDt_UInt16 RtpSvc_Length;
typedef RtpDt_UInt32 RTP_STACK_HANDLE;
typedef RtpDt_UInt32 RTP_TXN_HANDLE_KEY;
typedef RtpDt_UInt32 RTP_MSG_HANDLE;
typedef RtpDt_UInt32 RTP_TIMER_HANDLE;

#ifdef IMS_RTTI_ENABLED

#define RTP_CONST_CAST(TYPE, VALUE)       (const_cast<TYPE>(VALUE))
#define RTP_DYNAMIC_CAST(TYPE, VALUE)     (dynamic_cast<TYPE>(VALUE))
#define RTP_REINTERPRET_CAST(TYPE, VALUE) (reinterpret_cast<TYPE>(VALUE))
#define RTP_STATIC_CAST(TYPE, VALUE)      (static_cast<TYPE>(VALUE))

#else

// C-style type casting
#define RTP_CONST_CAST(TYPE, VALUE)       (const_cast<TYPE>(VALUE))
#define RTP_DYNAMIC_CAST(TYPE, VALUE)     ((TYPE)(VALUE))
#define RTP_REINTERPRET_CAST(TYPE, VALUE) (reinterpret_cast<TYPE>(VALUE))
#define RTP_STATIC_CAST(TYPE, VALUE)      (static_cast<TYPE>(VALUE))

#endif  // IMS_RTTI_ENABLED
#endif  //_RTP_PF_DATATYPES_H_

/** @}*/
