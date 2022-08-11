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

#include <ImsMediaAudioUtil.h>
#include <ImsMediaDefine.h>
#include <AudioConfig.h>
#include <ImsMediaTrace.h>
#include <string.h>

static const uint32_t gaAMRWBLen[32] = {
        17,  // 6.6
        23,  // 8.85
        32,  // 12.65
        36,  // 14.25
        40,  // 15.85
        46,  // 18.25
        50,  // 19.85
        58,  // 23.05
        60,  // 23.85
        5,   // SID
        0,
};

static const uint32_t gaAMRWBbitLen[32] = {
        132,  // 6.6
        177,  // 8.85
        253,  // 12.65
        285,  // 14.25
        317,  // 15.85
        365,  // 18.25
        397,  // 19.85
        461,  // 23.05
        477,  // 23.85
        40,   // SID
        0,
};

static const uint32_t gaEVSPrimaryByteLen[32] = {
        7,    // 2.8 special case
        18,   // 7.2
        20,   // 8.0
        24,   // 9.6
        33,   // 13.2
        41,   // 16.4
        61,   // 24.4
        80,   // 32.0
        120,  // 48.0
        160,  // 64.0
        240,  // 96.0
        320,  // 128.0
        6,    // SID
        0,
};

static const uint32_t gaEVSPrimaryBitLen[32] = {
        56,    // 2.8 Special case
        144,   // 7.2
        160,   // 8.0
        192,   // 9.6
        264,   // 13.2
        328,   // 16.4
        488,   // 24.4
        640,   // 32.0
        960,   // 48.0
        1280,  // 64.0
        1920,  // 96.0
        2560,  // 128.0
        48,    // SID
        0,
};

static const uint32_t gaEVSAMRWBIOLen[32] = {
        17,  // 6.6
        23,  // 8.85
        32,  // 12.65
        36,  // 14.25
        40,  // 15.85
        46,  // 18.25
        50,  // 19.85
        58,  // 23.05
        60,  // 23.85
        5,   // SID
        0,
};

static const uint32_t gaEVSAmrWbIoBitLen[32] = {
        136,  // 6.6 AMR-WB IO
        184,  // 8.85 AMR-WB IO
        256,  // 12.65 AMR-WB IO
        288,  // 14.25 AMR-WB IO
        320,  // 15.85 AMR-WB IO
        368,  // 18.25 AMR-WB IO
        400,  // 19.85 AMR-WB IO
        464,  // 23.05 AMR-WB IO
        480,  // 23.85 AMR-WB IO
        40,   // SID for AMR-WB IO
        0,    /* Note that no Compact frame format EVS AMR-WB IO SID frames is defined.
           For such frames the Header-Full format with CMR byte shall be used*/
};

const uint32_t gaAmrWbBitOrder660a[IMSAMRWB_CLASS_A_BITS_660] = {0, 5, 6, 7, 61, 84, 107, 130, 62,
        85, 8, 4, 37, 38, 39, 40, 58, 81, 104, 127, 60, 83, 106, 129, 108, 131, 128, 41, 42, 80,
        126, 1, 3, 57, 103, 82, 105, 59, 2, 63, 109, 110, 86, 19, 22, 23, 64, 87, 18, 20, 21, 17,
        13, 88};

const uint32_t gaAmrWbBitOrder660b[IMSAMRWB_CLASS_B_BITS_660] = {43, 89, 65, 111, 14, 24, 25, 26,
        27, 28, 15, 16, 44, 90, 66, 112, 9, 11, 10, 12, 67, 113, 29, 30, 31, 32, 34, 33, 35, 36, 45,
        51, 68, 74, 91, 97, 114, 120, 46, 69, 92, 115, 52, 75, 98, 121, 47, 70, 93, 116, 53, 76, 99,
        122, 48, 71, 94, 117, 54, 77, 100, 123, 49, 72, 95, 118, 55, 78, 101, 124, 50, 73, 96, 119,
        56, 79, 102, 125};

const uint32_t gaAmrWbBitOrder885a[IMSAMRWB_CLASS_A_BITS_885] = {0, 4, 6, 7, 5, 3, 47, 48, 49, 112,
        113, 114, 75, 106, 140, 171, 80, 111, 145, 176, 77, 108, 142, 173, 78, 109, 143, 174, 79,
        110, 144, 175, 76, 107, 141, 172, 50, 115, 51, 2, 1, 81, 116, 146, 19, 21, 12, 17, 18, 20,
        16, 25, 13, 10, 14, 24, 23, 22, 26, 8, 15, 52, 117, 31};

const uint32_t gaAmrWbBitOrder885b[IMSAMRWB_CLASS_B_BITS_885] = {82, 147, 9, 33, 11, 83, 148, 53,
        118, 28, 27, 84, 149, 34, 35, 29, 46, 32, 30, 54, 119, 37, 36, 39, 38, 40, 85, 150, 41, 42,
        43, 44, 45, 55, 60, 65, 70, 86, 91, 96, 101, 120, 125, 130, 135, 151, 156, 161, 166, 56, 87,
        121, 152, 61, 92, 126, 157, 66, 97, 131, 162, 71, 102, 136, 167, 57, 88, 122, 153, 62, 93,
        127, 158, 67, 98, 132, 163, 72, 103, 137, 168, 58, 89, 123, 154, 63, 94, 128, 159, 68, 99,
        133, 164, 73, 104, 138, 169, 59, 90, 124, 155, 64, 95, 129, 160, 69, 100, 134, 165, 74, 105,
        139, 170};

const uint32_t gaAmrWbBitOrder1265a[IMSAMRWB_CLASS_A_BITS_1265] = {0, 4, 6, 93, 143, 196, 246, 7, 5,
        3, 47, 48, 49, 50, 51, 150, 151, 152, 153, 154, 94, 144, 197, 247, 99, 149, 202, 252, 96,
        146, 199, 249, 97, 147, 200, 250, 100, 203, 98, 148, 201, 251, 95, 145, 198, 248, 52, 2, 1,
        101, 204, 155, 19, 21, 12, 17, 18, 20, 16, 25, 13, 10, 14, 24, 23, 22, 26, 8, 15, 53, 156,
        31};

const uint32_t gaAmrWbBitOrder1265b[IMSAMRWB_CLASS_B_BITS_1265] = {102, 205, 9, 33, 11, 103, 206,
        54, 157, 28, 27, 104, 207, 34, 35, 29, 46, 32, 30, 55, 158, 37, 36, 39, 38, 40, 105, 208,
        41, 42, 43, 44, 45, 56, 106, 159, 209, 57, 66, 75, 84, 107, 116, 125, 134, 160, 169, 178,
        187, 210, 219, 228, 237, 58, 108, 161, 211, 62, 112, 165, 215, 67, 117, 170, 220, 71, 121,
        174, 224, 76, 126, 179, 229, 80, 130, 183, 233, 85, 135, 188, 238, 89, 139, 192, 242, 59,
        109, 162, 212, 63, 113, 166, 216, 68, 118, 171, 221, 72, 122, 175, 225, 77, 127, 180, 230,
        81, 131, 184, 234, 86, 136, 189, 239, 90, 140, 193, 243, 60, 110, 163, 213, 64, 114, 167,
        217, 69, 119, 172, 222, 73, 123, 176, 226, 78, 128, 181, 231, 82, 132, 185, 235, 87, 137,
        190, 240, 91, 141, 194, 244, 61, 111, 164, 214, 65, 115, 168, 218, 70, 120, 173, 223, 74,
        124, 177, 227, 79, 129, 182, 232, 83, 133, 186, 236, 88, 138, 191, 241, 92, 142, 195, 245};

const uint32_t gaAmrWbBitOrder1425a[IMSAMRWB_CLASS_A_BITS_1425] = {0, 4, 6, 101, 159, 220, 278, 7,
        5, 3, 47, 48, 49, 50, 51, 166, 167, 168, 169, 170, 102, 160, 221, 279, 107, 165, 226, 284,
        104, 162, 223, 281, 105, 163, 224, 282, 108, 227, 106, 164, 225, 283, 103, 161, 222, 280,
        52, 2, 1, 109, 228, 171, 19, 21, 12, 17, 18, 20, 16, 25, 13, 10, 14, 24, 23, 22, 26, 8, 15,
        53, 172, 31};

const uint32_t gaAmrWbBitOrder1425b[IMSAMRWB_CLASS_B_BITS_1425] = {110, 229, 9, 33, 11, 111, 230,
        54, 173, 28, 27, 112, 231, 34, 35, 29, 46, 32, 30, 55, 174, 37, 36, 39, 38, 40, 113, 232,
        41, 42, 43, 44, 45, 56, 114, 175, 233, 62, 120, 181, 239, 75, 133, 194, 252, 57, 115, 176,
        234, 63, 121, 182, 240, 70, 128, 189, 247, 76, 134, 195, 253, 83, 141, 202, 260, 92, 150,
        211, 269, 84, 142, 203, 261, 93, 151, 212, 270, 85, 143, 204, 262, 94, 152, 213, 271, 86,
        144, 205, 263, 95, 153, 214, 272, 64, 122, 183, 241, 77, 135, 196, 254, 65, 123, 184, 242,
        78, 136, 197, 255, 87, 145, 206, 264, 96, 154, 215, 273, 58, 116, 177, 235, 66, 124, 185,
        243, 71, 129, 190, 248, 79, 137, 198, 256, 88, 146, 207, 265, 97, 155, 216, 274, 59, 117,
        178, 236, 67, 125, 186, 244, 72, 130, 191, 249, 80, 138, 199, 257, 89, 147, 208, 266, 98,
        156, 217, 275, 60, 118, 179, 237, 68, 126, 187, 245, 73, 131, 192, 250, 81, 139, 200, 258,
        90, 148, 209, 267, 99, 157, 218, 276, 61, 119, 180, 238, 69, 127, 188, 246, 74, 132, 193,
        251, 82, 140, 201, 259, 91, 149, 210, 268, 100, 158, 219, 277};

const uint32_t gaAmrWbBitOrder1585a[IMSAMRWB_CLASS_A_BITS_1585] = {0, 4, 6, 109, 175, 244, 310, 7,
        5, 3, 47, 48, 49, 50, 51, 182, 183, 184, 185, 186, 110, 176, 245, 311, 115, 181, 250, 316,
        112, 178, 247, 313, 113, 179, 248, 314, 116, 251, 114, 180, 249, 315, 111, 177, 246, 312,
        52, 2, 1, 117, 252, 187, 19, 21, 12, 17, 18, 20, 16, 25, 13, 10, 14, 24, 23, 22, 26, 8, 15,
        53, 188, 31};

const uint32_t gaAmrWbBitOrder1585b[IMSAMRWB_CLASS_B_BITS_1585] = {118, 253, 9, 33, 11, 119, 254,
        54, 189, 28, 27, 120, 255, 34, 35, 29, 46, 32, 30, 55, 190, 37, 36, 39, 38, 40, 121, 256,
        41, 42, 43, 44, 45, 56, 122, 191, 257, 63, 129, 198, 264, 76, 142, 211, 277, 89, 155, 224,
        290, 102, 168, 237, 303, 57, 123, 192, 258, 70, 136, 205, 271, 83, 149, 218, 284, 96, 162,
        231, 297, 62, 128, 197, 263, 75, 141, 210, 276, 88, 154, 223, 289, 101, 167, 236, 302, 58,
        124, 193, 259, 71, 137, 206, 272, 84, 150, 219, 285, 97, 163, 232, 298, 59, 125, 194, 260,
        64, 130, 199, 265, 67, 133, 202, 268, 72, 138, 207, 273, 77, 143, 212, 278, 80, 146, 215,
        281, 85, 151, 220, 286, 90, 156, 225, 291, 93, 159, 228, 294, 98, 164, 233, 299, 103, 169,
        238, 304, 106, 172, 241, 307, 60, 126, 195, 261, 65, 131, 200, 266, 68, 134, 203, 269, 73,
        139, 208, 274, 78, 144, 213, 279, 81, 147, 216, 282, 86, 152, 221, 287, 91, 157, 226, 292,
        94, 160, 229, 295, 99, 165, 234, 300, 104, 170, 239, 305, 107, 173, 242, 308, 61, 127, 196,
        262, 66, 132, 201, 267, 69, 135, 204, 270, 74, 140, 209, 275, 79, 145, 214, 280, 82, 148,
        217, 283, 87, 153, 222, 288, 92, 158, 227, 293, 95, 161, 230, 296, 100, 166, 235, 301, 105,
        171, 240, 306, 108, 174, 243, 309};

const uint32_t gaAmrWbBitOrder1825a[IMSAMRWB_CLASS_A_BITS_1825] = {0, 4, 6, 121, 199, 280, 358, 7,
        5, 3, 47, 48, 49, 50, 51, 206, 207, 208, 209, 210, 122, 200, 281, 359, 127, 205, 286, 364,
        124, 202, 283, 361, 125, 203, 284, 362, 128, 287, 126, 204, 285, 363, 123, 201, 282, 360,
        52, 2, 1, 129, 288, 211, 19, 21, 12, 17, 18, 20, 16, 25, 13, 10, 14, 24, 23, 22, 26, 8, 15,
        53, 212, 31};

const uint32_t gaAmrWbBitOrder1825b[IMSAMRWB_CLASS_B_BITS_1825] = {130, 289, 9, 33, 11, 131, 290,
        54, 213, 28, 27, 132, 291, 34, 35, 29, 46, 32, 30, 55, 214, 37, 36, 39, 38, 40, 133, 292,
        41, 42, 43, 44, 45, 56, 134, 215, 293, 198, 299, 136, 120, 138, 60, 279, 58, 62, 357, 139,
        140, 295, 156, 57, 219, 297, 63, 217, 137, 170, 300, 222, 64, 106, 61, 78, 294, 92, 142,
        141, 135, 221, 296, 301, 343, 59, 298, 184, 329, 315, 220, 216, 265, 251, 218, 237, 352,
        223, 157, 86, 171, 87, 164, 351, 111, 302, 65, 178, 115, 323, 72, 192, 101, 179, 93, 73,
        193, 151, 337, 309, 143, 274, 69, 324, 165, 150, 97, 338, 110, 310, 330, 273, 68, 107, 175,
        245, 114, 79, 113, 189, 246, 259, 174, 71, 185, 96, 344, 100, 322, 83, 334, 316, 333, 252,
        161, 348, 147, 82, 269, 232, 260, 308, 353, 347, 163, 231, 306, 320, 188, 270, 146, 177,
        266, 350, 256, 85, 149, 116, 191, 160, 238, 258, 336, 305, 255, 88, 224, 99, 339, 230, 228,
        227, 272, 242, 241, 319, 233, 311, 102, 74, 180, 275, 66, 194, 152, 325, 172, 247, 244, 261,
        117, 158, 166, 354, 75, 144, 108, 312, 94, 186, 303, 80, 234, 89, 195, 112, 340, 181, 345,
        317, 326, 276, 239, 167, 118, 313, 70, 355, 327, 253, 190, 176, 271, 104, 98, 153, 103, 90,
        76, 267, 277, 248, 225, 262, 182, 84, 154, 235, 335, 168, 331, 196, 341, 249, 162, 307, 148,
        349, 263, 321, 257, 243, 229, 356, 159, 119, 67, 187, 173, 145, 240, 77, 304, 332, 314, 342,
        109, 254, 81, 278, 105, 91, 346, 318, 183, 250, 197, 328, 95, 155, 169, 268, 226, 236, 264};

const uint32_t gaAmrWbBitOrder1985a[IMSAMRWB_CLASS_A_BITS_1985] = {0, 4, 6, 129, 215, 304, 390, 7,
        5, 3, 47, 48, 49, 50, 51, 222, 223, 224, 225, 226, 130, 216, 305, 391, 135, 221, 310, 396,
        132, 218, 307, 393, 133, 219, 308, 394, 136, 311, 134, 220, 309, 395, 131, 217, 306, 392,
        52, 2, 1, 137, 312, 227, 19, 21, 12, 17, 18, 20, 16, 25, 13, 10, 14, 24, 23, 22, 26, 8, 15,
        53, 228, 31};

const uint32_t gaAmrWbBitOrder1985b[IMSAMRWB_CLASS_B_BITS_1985] = {138, 313, 9, 33, 11, 139, 314,
        54, 229, 28, 27, 140, 315, 34, 35, 29, 46, 32, 30, 55, 230, 37, 36, 39, 38, 40, 141, 316,
        41, 42, 43, 44, 45, 56, 142, 231, 317, 63, 73, 92, 340, 82, 324, 149, 353, 159, 334, 165,
        338, 178, 163, 254, 77, 168, 257, 153, 343, 57, 248, 238, 79, 252, 166, 67, 80, 201, 101,
        267, 143, 164, 341, 255, 339, 187, 376, 318, 78, 328, 362, 115, 232, 242, 253, 290, 276, 62,
        58, 158, 68, 93, 179, 319, 148, 169, 154, 72, 385, 329, 333, 344, 102, 83, 144, 233, 323,
        124, 243, 192, 354, 237, 64, 247, 202, 209, 150, 116, 335, 268, 239, 299, 188, 196, 298, 94,
        195, 258, 123, 363, 384, 109, 325, 371, 170, 370, 84, 110, 295, 180, 74, 210, 191, 106, 291,
        205, 367, 381, 377, 206, 355, 122, 119, 120, 383, 160, 105, 108, 277, 380, 294, 284, 285,
        345, 208, 269, 249, 366, 386, 300, 297, 259, 125, 369, 197, 97, 194, 286, 211, 281, 280,
        183, 372, 87, 155, 283, 59, 348, 327, 184, 76, 111, 330, 203, 349, 69, 98, 152, 145, 189,
        66, 320, 337, 173, 358, 251, 198, 174, 263, 262, 126, 241, 193, 88, 388, 117, 95, 387, 112,
        359, 287, 244, 103, 272, 301, 171, 162, 234, 273, 127, 373, 181, 292, 85, 378, 302, 121,
        107, 364, 346, 356, 212, 278, 213, 65, 382, 288, 207, 113, 175, 99, 296, 374, 368, 199, 260,
        185, 336, 331, 161, 270, 264, 250, 240, 75, 350, 151, 60, 89, 321, 156, 274, 360, 326, 70,
        282, 167, 146, 352, 81, 91, 389, 266, 245, 177, 235, 190, 256, 204, 342, 128, 118, 303, 104,
        379, 182, 114, 375, 200, 96, 293, 172, 214, 365, 279, 86, 289, 351, 347, 357, 261, 186, 176,
        271, 90, 100, 147, 322, 275, 361, 71, 332, 61, 265, 157, 246, 236};

const uint32_t gaAmrWbBitOrder2305a[IMSAMRWB_CLASS_A_BITS_2305] = {0, 4, 6, 145, 247, 352, 454, 7,
        5, 3, 47, 48, 49, 50, 51, 254, 255, 256, 257, 258, 146, 248, 353, 455, 151, 253, 358, 460,
        148, 250, 355, 457, 149, 251, 356, 458, 152, 359, 150, 252, 357, 459, 147, 249, 354, 456,
        52, 2, 1, 153, 360, 259, 19, 21, 12, 17, 18, 20, 16, 25, 13, 10, 14, 24, 23, 22, 26, 8, 15,
        53, 260, 31};

const uint32_t gaAmrWbBitOrder2305b[IMSAMRWB_CLASS_B_BITS_2305] = {154, 361, 9, 33, 11, 155, 362,
        54, 261, 28, 27, 156, 363, 34, 35, 29, 46, 32, 30, 55, 262, 37, 36, 39, 38, 40, 157, 364,
        41, 42, 43, 44, 45, 56, 158, 263, 365, 181, 192, 170, 79, 57, 399, 90, 159, 297, 377, 366,
        275, 68, 183, 388, 286, 194, 299, 92, 70, 182, 401, 172, 59, 91, 58, 400, 368, 161, 81, 160,
        264, 171, 80, 389, 390, 378, 379, 193, 298, 69, 266, 265, 367, 277, 288, 276, 287, 184, 60,
        195, 82, 93, 71, 369, 402, 173, 162, 444, 300, 391, 98, 76, 278, 61, 267, 374, 135, 411,
        167, 102, 380, 200, 87, 178, 65, 94, 204, 124, 72, 342, 189, 305, 381, 396, 433, 301, 226,
        407, 289, 237, 113, 215, 185, 128, 309, 403, 116, 320, 196, 331, 370, 422, 174, 64, 392, 83,
        425, 219, 134, 188, 432, 112, 427, 139, 279, 163, 436, 208, 447, 218, 236, 229, 97, 294,
        385, 230, 166, 268, 177, 443, 225, 426, 101, 272, 138, 127, 290, 117, 347, 199, 414, 95,
        140, 240, 410, 395, 209, 129, 283, 346, 105, 241, 437, 86, 308, 448, 203, 345, 186, 107,
        220, 415, 334, 319, 106, 313, 118, 123, 73, 207, 421, 214, 384, 373, 438, 62, 371, 341, 75,
        449, 168, 323, 164, 242, 416, 324, 304, 197, 335, 404, 271, 63, 191, 325, 96, 169, 231, 280,
        312, 187, 406, 84, 201, 100, 67, 382, 175, 336, 202, 330, 269, 393, 376, 383, 293, 307, 409,
        179, 285, 314, 302, 372, 398, 190, 180, 89, 99, 103, 232, 78, 88, 77, 136, 387, 165, 198,
        394, 125, 176, 428, 74, 375, 238, 227, 66, 273, 282, 141, 306, 412, 114, 85, 130, 348, 119,
        291, 296, 386, 233, 397, 303, 405, 284, 445, 423, 221, 210, 205, 450, 108, 274, 434, 216,
        343, 337, 142, 243, 321, 408, 451, 310, 292, 120, 109, 281, 439, 270, 429, 332, 295, 418,
        211, 315, 222, 326, 131, 430, 244, 327, 349, 417, 316, 143, 338, 440, 234, 110, 212, 452,
        245, 121, 419, 350, 223, 132, 441, 328, 413, 317, 339, 126, 104, 137, 446, 344, 239, 435,
        115, 333, 206, 322, 217, 228, 424, 453, 311, 351, 111, 442, 224, 213, 122, 431, 340, 235,
        246, 133, 144, 420, 329, 318};

const uint32_t gaAmrWbBitOrder2385a[IMSAMRWB_CLASS_A_BITS_2385] = {0, 4, 6, 145, 251, 360, 466, 7,
        5, 3, 47, 48, 49, 50, 51, 262, 263, 264, 265, 266, 146, 252, 361, 467, 151, 257, 366, 472,
        148, 254, 363, 469, 149, 255, 364, 470, 156, 371, 150, 256, 365, 471, 147, 253, 362, 468,
        52, 2, 1, 157, 372, 267, 19, 21, 12, 17, 18, 20, 16, 25, 13, 10, 14, 24, 23, 22, 26, 8, 15,
        53, 268, 31};

const uint32_t gaAmrWbBitOrder2385b[IMSAMRWB_CLASS_B_BITS_2385] = {152, 153, 154, 155, 258, 259,
        260, 261, 367, 368, 369, 370, 473, 474, 475, 476, 158, 373, 9, 33, 11, 159, 374, 54, 269,
        28, 27, 160, 375, 34, 35, 29, 46, 32, 30, 55, 270, 37, 36, 39, 38, 40, 161, 376, 41, 42, 43,
        44, 45, 56, 162, 271, 377, 185, 196, 174, 79, 57, 411, 90, 163, 305, 389, 378, 283, 68, 187,
        400, 294, 198, 307, 92, 70, 186, 413, 176, 59, 91, 58, 412, 380, 165, 81, 164, 272, 175, 80,
        401, 402, 390, 391, 197, 306, 69, 274, 273, 379, 285, 296, 284, 295, 188, 60, 199, 82, 93,
        71, 381, 414, 177, 166, 456, 308, 403, 98, 76, 286, 61, 275, 386, 135, 423, 171, 102, 392,
        204, 87, 182, 65, 94, 208, 124, 72, 350, 193, 313, 393, 408, 445, 309, 230, 419, 297, 241,
        113, 219, 189, 128, 317, 415, 116, 328, 200, 339, 382, 434, 178, 64, 404, 83, 437, 223, 134,
        192, 444, 112, 439, 139, 287, 167, 448, 212, 459, 222, 240, 233, 97, 302, 397, 234, 170,
        276, 181, 455, 229, 438, 101, 280, 138, 127, 298, 117, 355, 203, 426, 95, 140, 244, 422,
        407, 213, 129, 291, 354, 105, 245, 449, 86, 316, 460, 207, 353, 190, 107, 224, 427, 342,
        327, 106, 321, 118, 123, 73, 211, 433, 218, 396, 385, 450, 62, 383, 349, 75, 461, 172, 331,
        168, 246, 428, 332, 312, 201, 343, 416, 279, 63, 195, 333, 96, 173, 235, 288, 320, 191, 418,
        84, 205, 100, 67, 394, 179, 344, 206, 338, 277, 405, 388, 395, 301, 315, 421, 183, 293, 322,
        310, 384, 410, 194, 184, 89, 99, 103, 236, 78, 88, 77, 136, 399, 169, 202, 406, 125, 180,
        440, 74, 387, 242, 231, 66, 281, 290, 141, 314, 424, 114, 85, 130, 356, 119, 299, 304, 398,
        237, 409, 311, 417, 292, 457, 435, 225, 214, 209, 462, 108, 282, 446, 220, 351, 345, 142,
        247, 329, 420, 463, 318, 300, 120, 109, 289, 451, 278, 441, 340, 303, 430, 215, 323, 226,
        334, 131, 442, 248, 335, 357, 429, 324, 143, 346, 452, 238, 110, 216, 464, 249, 121, 431,
        358, 227, 132, 453, 336, 425, 325, 347, 126, 104, 137, 458, 352, 243, 447, 115, 341, 210,
        330, 221, 232, 436, 465, 319, 359, 111, 454, 228, 217, 122, 443, 348, 239, 250, 133, 144,
        432, 337, 326};
/* Structure containing bit orders */
typedef struct
{
    int32_t nLenA;
    uint8_t* pClassA;
    int32_t nLenB;
    uint8_t* pClassB;
    int32_t nLenC;
    uint8_t* pClassC;
} AmrFrameOrder;

typedef struct
{
    int32_t nLenA;
    uint32_t* pClassA;
    int32_t nLenB;
    uint32_t* pClassB;
    int32_t nLenC;
    uint32_t* pClassC;
} AmrWbFrameOrder;

const AmrWbFrameOrder gaAmrWb660Table = {IMSAMRWB_CLASS_A_BITS_660, (uint32_t*)gaAmrWbBitOrder660a,
        IMSAMRWB_CLASS_B_BITS_660, (uint32_t*)gaAmrWbBitOrder660b, IMSAMRWB_CLASS_C_BITS_660, NULL};

const AmrWbFrameOrder gaAmrWb885Table = {IMSAMRWB_CLASS_A_BITS_885, (uint32_t*)gaAmrWbBitOrder885a,
        IMSAMRWB_CLASS_B_BITS_885, (uint32_t*)gaAmrWbBitOrder885b, IMSAMRWB_CLASS_C_BITS_885, NULL};

const AmrWbFrameOrder gaAmrWb1265Table = {IMSAMRWB_CLASS_A_BITS_1265,
        (uint32_t*)gaAmrWbBitOrder1265a, IMSAMRWB_CLASS_B_BITS_1265,
        (uint32_t*)gaAmrWbBitOrder1265b, IMSAMRWB_CLASS_C_BITS_1265, NULL};

const AmrWbFrameOrder gaAmrWb1425Table = {IMSAMRWB_CLASS_A_BITS_1425,
        (uint32_t*)gaAmrWbBitOrder1425a, IMSAMRWB_CLASS_B_BITS_1425,
        (uint32_t*)gaAmrWbBitOrder1425b, IMSAMRWB_CLASS_C_BITS_1425, NULL};

const AmrWbFrameOrder gaAmrWb1585Table = {IMSAMRWB_CLASS_A_BITS_1585,
        (uint32_t*)gaAmrWbBitOrder1585a, IMSAMRWB_CLASS_B_BITS_1585,
        (uint32_t*)gaAmrWbBitOrder1585b, IMSAMRWB_CLASS_C_BITS_1585, NULL};

const AmrWbFrameOrder gaAmrWb1825Table = {IMSAMRWB_CLASS_A_BITS_1825,
        (uint32_t*)gaAmrWbBitOrder1825a, IMSAMRWB_CLASS_B_BITS_1825,
        (uint32_t*)gaAmrWbBitOrder1825b, IMSAMRWB_CLASS_C_BITS_1825, NULL};

const AmrWbFrameOrder gaAmrWb1985Table = {IMSAMRWB_CLASS_A_BITS_1985,
        (uint32_t*)gaAmrWbBitOrder1985a, IMSAMRWB_CLASS_B_BITS_1985,
        (uint32_t*)gaAmrWbBitOrder1985b, IMSAMRWB_CLASS_C_BITS_1985, NULL};

const AmrWbFrameOrder gaAmrWb2305Table = {IMSAMRWB_CLASS_A_BITS_2305,
        (uint32_t*)gaAmrWbBitOrder2305a, IMSAMRWB_CLASS_B_BITS_2305,
        (uint32_t*)gaAmrWbBitOrder2305b, IMSAMRWB_CLASS_C_BITS_2305, NULL};

const AmrWbFrameOrder gaAmrWb2385Table = {IMSAMRWB_CLASS_A_BITS_2385,
        (uint32_t*)gaAmrWbBitOrder2385a, IMSAMRWB_CLASS_B_BITS_2385,
        (uint32_t*)gaAmrWbBitOrder2385b, IMSAMRWB_CLASS_C_BITS_2385, NULL};

/* Bit ordering tables indexed by imsamrwb_mode_type */
const AmrWbFrameOrder* gaAmrWbFmtTable[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        &gaAmrWb660Table, &gaAmrWb885Table, &gaAmrWb1265Table, &gaAmrWb1425Table, &gaAmrWb1585Table,
        &gaAmrWb1825Table, &gaAmrWb1985Table, &gaAmrWb2305Table, &gaAmrWb2385Table};

static const uint32_t gaAMRLen[16] = {
        12,  // 4.75
        13,  // 5.15
        15,  // 5.90
        17,  // 6.70
        19,  // 7.40
        20,  // 7.95
        26,  // 10.20
        31,  // 12.20
        5,   // SID
        0,
};

static const uint32_t gaAMRBitLen[16] = {
        95,   // 4.75
        103,  // 5.15
        118,  // 5.90
        134,  // 6.70
        148,  // 7.40
        159,  // 7.95
        204,  // 10.20
        244,  // 12.20
        39,   // SID
        0,
};

const uint8_t gaAmrBitOrder475a[IMSAMR_CLASS_A_BITS_475] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
        12, 13, 14, 15, 23, 24, 25, 26, 27, 28, 48, 49, 61, 62, 82, 83, 47, 46, 45, 44, 81, 80, 79,
        78, 17, 18, 20, 22, 77, 76};

const uint8_t gaAmrBitOrder475b[IMSAMR_CLASS_B_BITS_475] = {
        /*------*/ 75, 74, 29, 30, 43, 42, 41, 40, 38, 39, 16, 19, 21, 50, 51, 59, 60, 63, 64, 72,
        73, 84, 85, 93, 94, 32, 33, 35, 36, 53, 54, 56, 57, 66, 67, 69, 70, 87, 88, 90, 91, 34, 55,
        68, 89, 37, 58, 71, 92, 31, 52, 65, 86};

const AmrFrameOrder gaAmr475Table = {IMSAMR_CLASS_A_BITS_475, (uint8_t*)gaAmrBitOrder475a,
        IMSAMR_CLASS_B_BITS_475, (uint8_t*)gaAmrBitOrder475b, IMSAMR_CLASS_C_BITS_475, NULL};

/* ======================== 5.15 kbps mode ========================== */
const uint8_t gaAmrBitOrder515a[IMSAMR_CLASS_A_BITS_515] = {
        7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8, 23, 24, 25, 26, 27, 46, 65, 84, 45,
        44, 43, 64, 63, 62, 83, 82, 81, 102, 101, 100, 42, 61, 80, 99, 28, 47, 66, 85, 18, 41, 60,
        79, 98 /**/
};

const uint8_t gaAmrBitOrder515b[IMSAMR_CLASS_B_BITS_515] = {
        /*----------------------------------------*/ 29, 48, 67, 17, 20, 22, 40, 59, 78, 97, 21, 30,
        49, 68, 86, 19, 16, 87, 39, 38, 58, 57, 77, 35, 54, 73, 92, 76, 96, 95, 36, 55, 74, 93, 32,
        51, 33, 52, 70, 71, 89, 90, 31, 50, 69, 88, 37, 56, 75, 94, 34, 53, 72, 91};

const AmrFrameOrder gaAmr515Table = {IMSAMR_CLASS_A_BITS_515, (uint8_t*)gaAmrBitOrder515a,
        IMSAMR_CLASS_B_BITS_515, (uint8_t*)gaAmrBitOrder515b, IMSAMR_CLASS_C_BITS_515, NULL};

/* ======================== 5.90 kbps mode ========================== */
const uint8_t gaAmrBitOrder590a[IMSAMR_CLASS_A_BITS_590] = {0, 1, 4, 5, 3, 6, 7, 2, 13, 15, 8, 9,
        11, 12, 14, 10, 16, 28, 74, 29, 75, 27, 73, 26, 72, 30, 76, 51, 97, 50, 71, 96, 117, 31, 77,
        52, 98, 49, 70, 95, 116, 53, 99, 32, 78, 33, 79, 48, 69, 94, 115, 47, 68, 93, 114};

const uint8_t gaAmrBitOrder590b[IMSAMR_CLASS_B_BITS_590] = {
        /*--------------------*/ 46, 67, 92, 113, 19, 21, 23, 22, 18, 17, 20, 24, 111, 43, 89, 110,
        64, 65, 44, 90, 25, 45, 66, 91, 112, 54, 100, 40, 61, 86, 107, 39, 60, 85, 106, 36, 57, 82,
        103, 35, 56, 81, 102, 34, 55, 80, 101, 42, 63, 88, 109, 41, 62, 87, 108, 38, 59, 84, 105,
        37, 58, 83, 104};

const AmrFrameOrder gaAmr590Table = {IMSAMR_CLASS_A_BITS_590, (uint8_t*)gaAmrBitOrder590a,
        IMSAMR_CLASS_B_BITS_590, (uint8_t*)gaAmrBitOrder590b, IMSAMR_CLASS_C_BITS_590, NULL};

/* ======================== 6.70 kbps mode ========================== */
const uint8_t gaAmrBitOrder670a[IMSAMR_CLASS_A_BITS_670] = {0, 1, 4, 3, 5, 6, 13, 7, 2, 8, 9, 11,
        15, 12, 14, 10, 28, 82, 29, 83, 27, 81, 26, 80, 30, 84, 16, 55, 109, 56, 110, 31, 85, 57,
        111, 48, 73, 102, 127, 32, 86, 51, 76, 105, 130, 52, 77, 106, 131, 58, 112, 33, 87, 19, 23,
        53, 78, 107};

const uint8_t gaAmrBitOrder670b[IMSAMR_CLASS_B_BITS_670] = {
        /*-----------------------------------*/ 132, 21, 22, 18, 17, 20, 24, 25, 50, 75, 104, 129,
        47, 72, 101, 126, 54, 79, 108, 133, 46, 71, 100, 125, 128, 103, 74, 49, 45, 70, 99, 124, 42,
        67, 96, 121, 39, 64, 93, 118, 38, 63, 92, 117, 35, 60, 89, 114, 34, 59, 88, 113, 44, 69, 98,
        123, 43, 68, 97, 122, 41, 66, 95, 120, 40, 65, 94, 119, 37, 62, 91, 116, 36, 61, 90, 115};

const AmrFrameOrder gaAmr670Table = {IMSAMR_CLASS_A_BITS_670, (uint8_t*)gaAmrBitOrder670a,
        IMSAMR_CLASS_B_BITS_670, (uint8_t*)gaAmrBitOrder670b, IMSAMR_CLASS_C_BITS_670, NULL};

/* ======================== 7.40 kbps mode ========================== */
const uint8_t gaAmrBitOrder740a[IMSAMR_CLASS_A_BITS_740] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
        12, 13, 14, 15, 16, 26, 87, 27, 88, 28, 89, 29, 90, 30, 91, 51, 80, 112, 141, 52, 81, 113,
        142, 54, 83, 115, 144, 55, 84, 116, 145, 58, 119, 59, 120, 21, 22, 23, 17, 18, 19, 31, 60,
        92, 121, 56, 85, 117, 146};

const uint8_t gaAmrBitOrder740b[IMSAMR_CLASS_B_BITS_740] = {
        /*-*/ 20, 24, 25, 50, 79, 111, 140, 57, 86, 118, 147, 49, 78, 110, 139, 48, 77, 53, 82, 114,
        143, 109, 138, 47, 76, 108, 137, 32, 33, 61, 62, 93, 94, 122, 123, 41, 42, 43, 44, 45, 46,
        70, 71, 72, 73, 74, 75, 102, 103, 104, 105, 106, 107, 131, 132, 133, 134, 135, 136, 34, 63,
        95, 124, 35, 64, 96, 125, 36, 65, 97, 126, 37, 66, 98, 127, 38, 67, 99, 128, 39, 68, 100,
        129, 40, 69, 101, 130};

const AmrFrameOrder gaAmr740Table = {IMSAMR_CLASS_A_BITS_740, (uint8_t*)gaAmrBitOrder740a,
        IMSAMR_CLASS_B_BITS_740, (uint8_t*)gaAmrBitOrder740b, IMSAMR_CLASS_C_BITS_740, NULL};

/* ======================== 7.95 kbps mode ========================== */
const uint8_t gaAmrBitOrder795a[IMSAMR_CLASS_A_BITS_795] = {8, 7, 6, 5, 4, 3, 2, 14, 16, 9, 10, 12,
        13, 15, 11, 17, 20, 22, 24, 23, 19, 18, 21, 56, 88, 122, 154, 57, 89, 123, 155, 58, 90, 124,
        156, 52, 84, 118, 150, 53, 85, 119, 151, 27, 93, 28, 94, 29, 95, 30, 96, 31, 97, 61, 127,
        62, 128, 63, 129, 59, 91, 125, 157, 32, 98, 64, 130, 1, 0, 25, 26, 33, 99, 34, 100};

const uint8_t gaAmrBitOrder795b[IMSAMR_CLASS_B_BITS_795] = {
        /*--------------------*/ 65, 131, 66, 132, 54, 86, 120, 152, 60, 92, 126, 158, 55, 87, 121,
        153, 117, 116, 115, 46, 78, 112, 144, 43, 75, 109, 141, 40, 72, 106, 138, 36, 68, 102, 134,
        114, 149, 148, 147, 146, 83, 82, 81, 80, 51, 50, 49, 48, 47, 45, 44, 42, 39, 35, 79, 77, 76,
        74, 71, 67, 113, 111, 110, 108, 105, 101, 145, 143, 142, 140, 137, 133, 41, 73, 107, 139,
        37, 69, 103, 135, 38, 70, 104, 136};

const AmrFrameOrder gaAmr795Table = {IMSAMR_CLASS_A_BITS_795, (uint8_t*)gaAmrBitOrder795a,
        IMSAMR_CLASS_B_BITS_795, (uint8_t*)gaAmrBitOrder795b, IMSAMR_CLASS_C_BITS_795, NULL};

/* ======================== 10.2 kbps mode ========================== */
const uint8_t gaAmrBitOrder102a[IMSAMR_CLASS_A_BITS_102] = {7, 6, 5, 4, 3, 2, 1, 0, 16, 15, 14, 13,
        12, 11, 10, 9, 8, 26, 27, 28, 29, 30, 31, 115, 116, 117, 118, 119, 120, 72, 73, 161, 162,
        65, 68, 69, 108, 111, 112, 154, 157, 158, 197, 200, 201, 32, 33, 121, 122, 74, 75, 163, 164,
        66, 109, 155, 198, 19, 23, 21, 22, 18, 17, 20, 24};

const uint8_t gaAmrBitOrder102b[IMSAMR_CLASS_B_BITS_102] = {
        /*--------------------*/ 25, 37, 36, 35, 34, 80, 79, 78, 77, 126, 125, 124, 123, 169, 168,
        167, 166, 70, 67, 71, 113, 110, 114, 159, 156, 160, 202, 199, 203, 76, 165, 81, 82, 92, 91,
        93, 83, 95, 85, 84, 94, 101, 102, 96, 104, 86, 103, 87, 97, 127, 128, 138, 137, 139, 129,
        141, 131, 130, 140, 147, 148, 142, 150, 132, 149, 133, 143, 170, 171, 181, 180, 182, 172,
        184, 174, 173, 183, 190, 191, 185, 193, 175, 192, 176, 186, 38, 39, 49, 48, 50, 40, 52, 42,
        41, 51, 58, 59, 53, 61};

const uint8_t gaAmrBitOrder102c[IMSAMR_CLASS_C_BITS_102] = {
        /*---------------*/ 43, 60, 44, 54, 194, 179, 189, 196, 177, 195, 178, 187, 188, 151, 136,
        146, 153, 134, 152, 135, 144, 145, 105, 90, 100, 107, 88, 106, 89, 98, 99, 62, 47, 57, 64,
        45, 63, 46, 55, 56};

const AmrFrameOrder gaAmr102Table = {IMSAMR_CLASS_A_BITS_102, (uint8_t*)gaAmrBitOrder102a,
        IMSAMR_CLASS_B_BITS_102, (uint8_t*)gaAmrBitOrder102b, IMSAMR_CLASS_C_BITS_102,
        (uint8_t*)gaAmrBitOrder102c};

/* ======================== 12.2 kbps mode ========================== */
const uint8_t gaAmrBitOrder122a[IMSAMR_CLASS_A_BITS_122] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
        12, 13, 14, 23, 15, 16, 17, 18, 19, 20, 21, 22, 24, 25, 26, 27, 28, 38, 141, 39, 142, 40,
        143, 41, 144, 42, 145, 43, 146, 44, 147, 45, 148, 46, 149, 47, 97, 150, 200, 48, 98, 151,
        201, 49, 99, 152, 202, 86, 136, 189, 239, 87, 137, 190, 240, 88, 138, 191, 241, 91, 194, 92,
        195, 93, 196, 94, 197, 95, 198};

const uint8_t gaAmrBitOrder122b[IMSAMR_CLASS_B_BITS_122] = {
        /**/ 29, 30, 31, 32, 33, 34, 35, 50, 100, 153, 203, 89, 139, 192, 242, 51, 101, 154, 204,
        55, 105, 158, 208, 90, 140, 193, 243, 59, 109, 162, 212, 63, 113, 166, 216, 67, 117, 170,
        220, 36, 37, 54, 53, 52, 58, 57, 56, 62, 61, 60, 66, 65, 64, 70, 69, 68, 104, 103, 102, 108,
        107, 106, 112, 111, 110, 116, 115, 114, 120, 119, 118, 157, 156, 155, 161, 160, 159, 165,
        164, 163, 169, 168, 167, 173, 172, 171, 207, 206, 205, 211, 210, 209, 215, 214, 213, 219,
        218, 217, 223, 222, 221, 73, 72};

const uint8_t gaAmrBitOrder122c[IMSAMR_CLASS_C_BITS_122] = {
        /* ------------- */ 71, 76, 75, 74, 79, 78, 77, 82, 81, 80, 85, 84, 83, 123, 122, 121, 126,
        125, 124, 129, 128, 127, 132, 131, 130, 135, 134, 133, 176, 175, 174, 179, 178, 177, 182,
        181, 180, 185, 184, 183, 188, 187, 186, 226, 225, 224, 229, 228, 227, 232, 231, 230, 235,
        234, 233, 238, 237, 236, 96, 199};

const AmrFrameOrder gaAmr122Table = {IMSAMR_CLASS_A_BITS_122, (uint8_t*)gaAmrBitOrder122a,
        IMSAMR_CLASS_B_BITS_122, (uint8_t*)gaAmrBitOrder122b, IMSAMR_CLASS_C_BITS_122,
        (uint8_t*)gaAmrBitOrder122c};

/* ______________________________________________________________________________________ */
/* Bit ordering tables indexed by imsamr_mode_type */
const AmrFrameOrder* gaAmrFmtTable[] = {&gaAmr475Table, &gaAmr515Table, &gaAmr590Table,
        &gaAmr670Table, &gaAmr740Table, &gaAmr795Table, &gaAmr102Table, &gaAmr122Table};
/* ______________________________________________________________________________________ */

void amrFmt_Framing(uint8_t* pEcdData, int32_t* pnEcdDstBitIndex, uint8_t* pRawData,
        int32_t nNumOfBits, uint8_t* paOdrTable);
void amrFmt_Deframing(uint8_t* pDst, uint8_t* pRawData, int32_t* pnSrcBitIndex, int32_t nNumOfBits,
        uint8_t* paOdrTable);

void amrFmt_Framing(uint8_t* pEcdData, int32_t* pnEcdDstBitIndex, uint8_t* pRawData,
        int32_t nNumOfBits, uint8_t* paOdrTable)
{
    uint32_t nDstMask = 0x00000080 >> ((*pnEcdDstBitIndex) & 0x7);
    uint8_t* pDst = &pEcdData[(*pnEcdDstBitIndex) >> 3];
    uint32_t nSrcBit, nSrcMask;
    /* Prepare to process all bits */
    *pnEcdDstBitIndex += nNumOfBits;
    nNumOfBits++;
    while (--nNumOfBits)
    {
        /* Get the location of the bit in the input buffer */
        nSrcBit = (uint32_t)*paOdrTable++;
        nSrcMask = 0x00000080 >> (nSrcBit & 0x7);
        /* Set the value of the output bit equal to the input bit */
        if (pRawData[nSrcBit >> 3] & nSrcMask)
        {
            *pDst |= (uint8_t)nDstMask;
        }
        /* Set the destination bit mask and increment pointer if necessary */
        nDstMask >>= 1;
        if (nDstMask == 0)
        {
            nDstMask = 0x00000080;
            pDst++;
        }
    }
}

void amrFmt_Deframing(uint8_t* pDst, uint8_t* pRawData, int32_t* pnSrcBitIndex, int32_t nNumOfBits,
        uint8_t* paOdrTable)
{
    uint32_t nSrcMask = 0x00000080 >> ((*pnSrcBitIndex) & 0x7);
    uint8_t* pSrc = &pRawData[(*pnSrcBitIndex) >> 3];
    uint32_t nSrcVal = (uint32_t)*pSrc++;
    uint32_t nDstBit, nDstMask;

    /* Prepare to process all bits
     */
    *pnSrcBitIndex += nNumOfBits;
    nNumOfBits++;

    while (--nNumOfBits)
    {
        /* Get the location of the next bit in the output buffer */
        nDstBit = (uint32_t)*paOdrTable++;
        nDstMask = 0x00000080 >> (nDstBit & 0x7);

        /* Set the value of the output bit equal to the input bit */
        if (nSrcVal & nSrcMask)
        {
            pDst[nDstBit >> 3] |= (uint8_t)nDstMask;
        }

        /* Set the source bit mask and increment pointer if necessary */
        nSrcMask >>= 1;
        if (nSrcMask == 0)
        {
            nSrcMask = 0x00000080;
            nSrcVal = *pSrc++;
        }
    }
}

void amrWbFmt_Framing(uint8_t* pEcdData, int32_t* pnEcdDstBitIndex, uint8_t* pRawData,
        int32_t nNumOfBits, uint32_t* paOdrTable)
{
    uint32_t nDstMask = 0x00000080 >> ((*pnEcdDstBitIndex) & 0x7);
    uint8_t* pDst = &pEcdData[(*pnEcdDstBitIndex) >> 3];
    uint32_t nSrcBit, nSrcMask;

    /* Prepare to process all bits
     */
    *pnEcdDstBitIndex += nNumOfBits;
    nNumOfBits++;

    while (--nNumOfBits)
    {
        /* Get the location of the bit in the input buffer */
        nSrcBit = (uint32_t)*paOdrTable++;
        nSrcMask = 0x00000080 >> (nSrcBit & 0x7);
        /* Set the value of the output bit equal to the input bit */
        if (pRawData[nSrcBit >> 3] & nSrcMask)
        {
            *pDst |= (uint8_t)nDstMask;
        }
        /* Set the destination bit mask and increment pointer if necessary */
        nDstMask >>= 1;
        if (nDstMask == 0)
        {
            nDstMask = 0x00000080;
            pDst++;
        }
    }
}

void amrWbFmt_Deframing(uint8_t* pDst, uint8_t* pRawData, int32_t* pnSrcBitIndex,
        int32_t nNumOfBits, uint32_t* paOdrTable)
{
    uint32_t nSrcMask = 0x00000080 >> ((*pnSrcBitIndex) & 0x7);
    uint8_t* pSrc = &pRawData[(*pnSrcBitIndex) >> 3];
    uint32_t nSrcVal = (uint32_t)*pSrc++;
    uint32_t nDstBit, nDstMask;

    /* Prepare to process all bits
     */
    *pnSrcBitIndex += nNumOfBits;
    nNumOfBits++;

    while (--nNumOfBits)
    {
        /* Get the location of the next bit in the output buffer */
        nDstBit = (uint32_t)*paOdrTable++;
        nDstMask = 0x00000080 >> (nDstBit & 0x7);

        /* Set the value of the output bit equal to the input bit */
        if (nSrcVal & nSrcMask)
        {
            pDst[nDstBit >> 3] |= (uint8_t)nDstMask;
        }

        /* Set the source bit mask and increment pointer if necessary */
        nSrcMask >>= 1;
        if (nSrcMask == 0)
        {
            nSrcMask = 0x00000080;
            nSrcVal = *pSrc++;
        }
    }
}

int32_t ImsMediaAudioUtil::ConvertCodecType(int32_t type)
{
    switch (type)
    {
        default:
        case AudioConfig::CODEC_AMR:
            return kAudioCodecAmr;
        case AudioConfig::CODEC_AMR_WB:
            return kAudioCodecAmrWb;
        case AudioConfig::CODEC_EVS:
            return kAudioCodecEvs;
        case AudioConfig::CODEC_PCMA:
            return kAudioCodecPcma;
        case AudioConfig::CODEC_PCMU:
            return kAudioCodecPcmu;
    }
}

void ImsMediaAudioUtil::ConvertEvsBandwidthToStr(kEvsBandwidth bandwidth, char* nBandwidth)
{
    switch (bandwidth)
    {
        case kEvsBandwidthNone:
            strcpy(nBandwidth, "NONE");
            break;
        case kEvsBandwidthNB:
            strcpy(nBandwidth, "NB");
            break;
        case kEvsBandwidthWB:
            strcpy(nBandwidth, "WB");
            break;
        case kEvsBandwidthSWB:
            strcpy(nBandwidth, "SWB");
            break;
        case kEvsBandwidthFB:
            strcpy(nBandwidth, "FB");
            break;
        default:
            strcpy(nBandwidth, "SWB");
            break;
    }
}

int32_t ImsMediaAudioUtil::ConvertEvsCodecMode(int32_t evsMode)
{
    if (evsMode > 8 && evsMode <= 20)
    {
        return kEvsCodecModePrimary;
    }
    else if (evsMode >= 0 && evsMode <= 8)
    {
        return kEvsCodecModeAmrIo;
    }
    else
    {
        return kEvsCodecModeMax;
    }
}

void ImsMediaAudioUtil::AmrFmtFraming(kImsAudioFrameEntype eFrameType, kImsAudioAmrMode mode,
        uint8_t* pRawData, uint8_t* pEncodedData)
{
    int32_t i, nStartingBit;
    AmrFrameOrder* paCurFmtTable;
    /* Clear destination */
    memset(pEncodedData, 0, IMSAMR_FRAME_BYTES);
    switch (eFrameType)
    {
        case kImsAudioFrameGsmSid:
            /* Copy the sid frame to pClassA data
             */
            for (i = 0; i < 5; i++)
            {
                pEncodedData[i] = pRawData[i];
            }
            /* Set the SID type (Bit 35 = 0) */
            pEncodedData[4] &= ~0x10;
            /* Set the mode (Bit 36 - 38 = mode with bits swapped) */
            pEncodedData[4] |= (((uint8_t)mode << 3) & 0x08) | (((uint8_t)mode << 1) & 0x04) |
                    (((uint8_t)mode >> 1) & 0x02);
            break;

        case kImsAudioFrameAmrSidUpdate:
            /* Copy the sid frame to pClassA data
             */
            for (i = 0; i < 5; i++)
            {
                pEncodedData[i] = pRawData[i];
            }
#if 0
            /* Set the SID type (Bit 35 = 1)
            */
            pEncodedData[4] |= 0x10;

            /* Set the mode (Bit 36 - 38 = mode with bits swapped)
            */
            pEncodedData[4] |= (((uint8_t)mode << 3) & 0x08)
            | (((uint8_t)mode << 1) & 0x04) | (((uint8_t)mode >> 1) & 0x02);
#endif
            break;
        case kImsAudioFrameAmrSpeechGood:
            /* Clear num bits in frame */
            nStartingBit = 0;
            /* Select ordering table */
            paCurFmtTable = (AmrFrameOrder*)gaAmrFmtTable[mode];
            amrFmt_Framing(pEncodedData, &nStartingBit, pRawData, paCurFmtTable->nLenA,
                    paCurFmtTable->pClassA);
            amrFmt_Framing(pEncodedData, &nStartingBit, pRawData, paCurFmtTable->nLenB,
                    paCurFmtTable->pClassB);
            amrFmt_Framing(pEncodedData, &nStartingBit, pRawData, paCurFmtTable->nLenC,
                    paCurFmtTable->pClassC);
            break;
        default:
            break;
    }
}

void ImsMediaAudioUtil::AmrFmtDeframing(kImsAudioFrameEntype eFrameType, kImsAudioAmrMode mode,
        uint8_t* pRawData, uint8_t* pEncodedData)
{
    int32_t i, nStartingBit;
    AmrFrameOrder* paCurFmtTable = NULL;
    memset(pEncodedData, 0, IMSAMR_FRAME_BYTES);

    switch (eFrameType)
    {
        case kImsAudioFrameAmrSidFirst:
        case kImsAudioFrameAmrSidUpdate:
            /* Process SID frame */
            /* Copy the pClassA data to the sid frame*/
            for (i = 0; i < 5; i++)
            {
                pEncodedData[i] = pRawData[i];
            }
            break;
        case kImsAudioFrameAmrSpeechGood:
            /* Process AMR speech frame */
            paCurFmtTable = (AmrFrameOrder*)gaAmrFmtTable[mode];
            nStartingBit = 0;
            amrFmt_Deframing(pEncodedData, pRawData, &nStartingBit, paCurFmtTable->nLenA,
                    paCurFmtTable->pClassA);
            amrFmt_Deframing(pEncodedData, pRawData, &nStartingBit, paCurFmtTable->nLenB,
                    paCurFmtTable->pClassB);
            amrFmt_Deframing(pEncodedData, pRawData, &nStartingBit, paCurFmtTable->nLenC,
                    paCurFmtTable->pClassC);
            break;
        default:
            break;
    }
}

void ImsMediaAudioUtil::AmrWbFmtFraming(kImsAudioFrameEntype eFrameType, kImsAudioAmrWbMode mode,
        uint8_t* pRawData, uint8_t* pEncodedData)
{
    int32_t i, nStartingBit;
    AmrWbFrameOrder* paCurFmtTable;
    /* Clear destination */
    memset(pEncodedData, 0, 62);
    IMLOGD_PACKET2(
            IM_PACKET_LOG_AUDIO, "[AmrWbFmtFraming] eFrameType[%d] mode[%d]", eFrameType, mode);

    if (mode < 8)
    {
        IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO, "[AmrWbFmtFraming] return failure mode[%d]", mode);
        return;
    }

    switch (eFrameType)
    {
        case kImsAudioFrameGsmSid:
            IMLOGD_PACKET0(IM_PACKET_LOG_AUDIO, "[AmrWbFmtFraming] kImsAudioFrameGsmSid case");

            /* Copy the sid frame to pClassA data */
            for (i = 0; i < 5; i++)
            {
                pEncodedData[i] = pRawData[i];
            }

            /* Set the SID type (Bit 35 = 0) */
            pEncodedData[4] &= ~0x10;

            IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO,
                    "[AmrWbFmtFraming] kImsAudioFrameGsmSid before "
                    "pEncodedData[4][%d]",
                    pEncodedData[4]);

            /* Set the mode (Bit 36 - 38 = mode with bits swapped)
             */
            pEncodedData[4] |= (((uint8_t)mode << 3) & 0x08) | (((uint8_t)mode << 1) & 0x04) |
                    (((uint8_t)mode >> 1) & 0x02);
            IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO,
                    "[AmrWbFmtFraming] kImsAudioFrameGsmSid after "
                    "pEncodedData[4][%d]",
                    pEncodedData[4]);
            break;
        case kImsAudioFrameAmrSidUpdate:
            IMLOGD_PACKET0(
                    IM_PACKET_LOG_AUDIO, "[AmrWbFmtFraming] kImsAudioFrameAmrSidUpdate case");

            /* Copy the sid frame to pClassA data */
            for (i = 0; i < 5; i++)
            {
                pEncodedData[i] = pRawData[i];
            }
#if 1
            /* Set the SID type */
            pEncodedData[4] |= 0x10;
            IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO,
                    "[AmrWbFmtFraming] kImsAudioFrameAmrSidUpdate before "
                    "pEncodedData[4][%d]",
                    pEncodedData[4]);

            /* Set the mode (Bit 36 - 39 = mode with bits swapped) */
            pEncodedData[4] |= (0x00001001);
            IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO,
                    "[AmrWbFmtFraming] kImsAudioFrameAmrSidUpdate after pEncodedData[4][%d]",
                    pEncodedData[4]);
#endif
            break;
        case kImsAudioFrameAmrSpeechGood:
            IMLOGD_PACKET0(
                    IM_PACKET_LOG_AUDIO, "[AmrWbFmtFraming] kImsAudioFrameAmrSpeechGood case");
            /* Clear num bits in frame */
            nStartingBit = 0;
            /* Select ordering table */
            paCurFmtTable = (AmrWbFrameOrder*)gaAmrWbFmtTable[mode];
            amrWbFmt_Framing(pEncodedData, &nStartingBit, pRawData, paCurFmtTable->nLenA,
                    paCurFmtTable->pClassA);
            amrWbFmt_Framing(pEncodedData, &nStartingBit, pRawData, paCurFmtTable->nLenB,
                    paCurFmtTable->pClassB);
            amrWbFmt_Framing(pEncodedData, &nStartingBit, pRawData, paCurFmtTable->nLenC,
                    paCurFmtTable->pClassC);
            IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO,
                    "[AmrWbFmtFraming] Speech Good case startingBit[%d]", nStartingBit);
            break;
        default:
            break;
    }
}

void ImsMediaAudioUtil::AmrFmtWbDeframing(kImsAudioFrameEntype eFrameType, kImsAudioAmrWbMode mode,
        uint8_t* pRawData, uint8_t* pEncodedData)
{
    int32_t i, nStartingBit;
    AmrWbFrameOrder* paCurFmtTable = NULL;
    memset(pEncodedData, 0, 62);
    IMLOGD_PACKET2(
            IM_PACKET_LOG_AUDIO, "[AmrWbFmt_DeFraming] eFrameType[%d] mode[%d]", eFrameType, mode);

    if (mode < 8)
    {
        IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO, "[AmrWbFmt_DeFraming] return failure mode[%d]", mode);
        return;
    }

    switch (eFrameType)
    {
        case kImsAudioFrameAmrSidFirst:
        case kImsAudioFrameAmrSidUpdate:
            /* Process SID frame */
            /* Copy the pClassA data to the sid frame*/
            for (i = 0; i < 5; i++)
            {
                pEncodedData[i] = pRawData[i];
            }
            break;

        case kImsAudioFrameAmrSpeechGood:
            /* Process AMR speech frame */
            paCurFmtTable = (AmrWbFrameOrder*)gaAmrWbFmtTable[mode];
            nStartingBit = 0;
            amrWbFmt_Deframing(pEncodedData, pRawData, &nStartingBit, paCurFmtTable->nLenA,
                    paCurFmtTable->pClassA);
            amrWbFmt_Deframing(pEncodedData, pRawData, &nStartingBit, paCurFmtTable->nLenB,
                    paCurFmtTable->pClassB);
            amrWbFmt_Deframing(pEncodedData, pRawData, &nStartingBit, paCurFmtTable->nLenC,
                    paCurFmtTable->pClassC);
            IMLOGD_PACKET1(IM_PACKET_LOG_AUDIO,
                    "[AmrFmtWb_DeFraming] Speech Good case startingBit[%d]", nStartingBit);
            break;
        default:
            break;
    }
}

void ImsMediaAudioUtil::AmrFmtSwap(uint8_t* pSrc, uint8_t* pDst, uint32_t nNumOfByte)
{
    uint8_t tbyte;
    nNumOfByte++;
    while (--nNumOfByte)
    {
        tbyte = *pSrc++;
        *pDst++ = *pSrc++;
        *pDst++ = tbyte;
    }
}

uint32_t ImsMediaAudioUtil::ConvertAmrModeToLen(uint32_t mode)
{
    if (mode > kImsAudioAmrModeSID)
    {  // over SID
        return 0;
    }
    return gaAMRLen[mode];
}

uint32_t ImsMediaAudioUtil::ConvertAmrModeToBitLen(uint32_t mode)
{
    if (mode > kImsAudioAmrModeSID)
    {  // over SID
        return 0;
    }
    return gaAMRBitLen[mode];
}

uint32_t ImsMediaAudioUtil::ConvertLenToAmrMode(uint32_t nLen)
{
    uint32_t i;
    if (nLen == 0)
    {
        return 15;
    }

    for (i = 0; i <= 8; i++)
    {
        if (gaAMRLen[i] == nLen)
            return i;
    }
    return 0;
}

uint32_t ImsMediaAudioUtil::ConvertAmrWbModeToLen(uint32_t mode)
{
    if (mode == kImsAudioAmrWbModeNoData)
        return 0;
    if (mode > kImsAudioAmrWbModeSID)
        return 0;
    return gaAMRWBLen[mode];
}

uint32_t ImsMediaAudioUtil::ConvertAmrWbModeToBitLen(uint32_t mode)
{
    if (mode == kImsAudioAmrWbModeNoData)
        return 0;
    if (mode > kImsAudioAmrWbModeSID)
        return 0;
    return gaAMRWBbitLen[mode];
}

uint32_t ImsMediaAudioUtil::ConvertLenToAmrWbMode(uint32_t nLen)
{
    uint32_t i;
    if (nLen == 0)
        return kImsAudioAmrWbModeNoData;
    for (i = 0; i <= kImsAudioAmrWbModeSID; i++)
    {
        if (gaAMRWBLen[i] == nLen)
            return i;
    }
    return 0;
}

uint32_t ImsMediaAudioUtil::ConvertLenToEVSAudioMode(uint32_t nLen)
{
    uint32_t i = 0;
    if (nLen == 0)
        return kImsAudioEvsPrimaryModeNoData;
    for (i = 0; i <= kImsAudioEvsPrimaryModeSID; i++)
    {
        if (gaEVSPrimaryByteLen[i] == nLen)
            return i;
    }
    IMLOGD0("[ConvertLenToEVSAudioMode] No primery bit len found....");
    return 0;
}

uint32_t ImsMediaAudioUtil::ConvertLenToEVSAMRIOAudioMode(uint32_t nLen)
{
    uint32_t i = 0;
    if (nLen == 0)
        return kImsAudioEvsAmrWbIoModeNoData;
    for (i = 0; i <= kImsAudioEvsAmrWbIoModeSID; i++)
    {
        if (gaEVSAMRWBIOLen[i] == nLen)
            return i;
    }
    return 0;
}

uint32_t ImsMediaAudioUtil::ConvertEVSAudioModeToBitLen(uint32_t mode)
{
    if (mode == 15)
        return 0;
    if (mode > 12)
        return 0;
    return gaEVSPrimaryBitLen[mode];
}

uint32_t ImsMediaAudioUtil::ConvertEVSAMRIOAudioModeToBitLen(uint32_t mode)
{
    if (mode == 15)
        return 0;
    if (mode > 9)
        return 0;
    return gaEVSAmrWbIoBitLen[mode];
}

uint32_t ImsMediaAudioUtil::ConvertAmrModeToBitrate(int mode)
{
    switch ((kImsAudioAmrMode)mode)
    {
        case kImsAudioAmrMode475:
            return 4750;
        case kImsAudioAmrMode515:
            return 5150;
        case kImsAudioAmrMode590:
            return 5900;
        case kImsAudioAmrMode670:
            return 6700;
        case kImsAudioAmrMode740:
            return 7400;
        case kImsAudioAmrMode795:
            return 7950;
        case kImsAudioAmrMode1020:
            return 10200;
        default:
        case kImsAudioAmrMode1220:
            return 12200;
    }
}

uint32_t ImsMediaAudioUtil::ConvertAmrWbModeToBitrate(int mode)
{
    switch ((kImsAudioAmrWbMode)mode)
    {
        case kImsAudioAmrWbMode660:
            return 6600;
        case kImsAudioAmrWbMode885:
            return 8850;
        case kImsAudioAmrWbMode1265:
            return 12650;
        case kImsAudioAmrWbMode1425:
            return 14250;
        case kImsAudioAmrWbMode1585:
            return 15850;
        case kImsAudioAmrWbMode1825:
            return 18250;
        case kImsAudioAmrWbMode1985:
            return 19850;
        case kImsAudioAmrWbMode2305:
            return 23050;
        default:
        case kImsAudioAmrWbMode2385:
            return 23850;
    }
}

int32_t ImsMediaAudioUtil::ConvertEVSModeToBitRate(int32_t EvsModeToBitRate)
{
    switch (EvsModeToBitRate)
    {
        case EvsParams::EVS_MODE_0:
            return 6600;
            break;
        case EvsParams::EVS_MODE_1:
            return 8850;
            break;
        case EvsParams::EVS_MODE_2:
            return 12650;
            break;
        case EvsParams::EVS_MODE_3:
            return 14250;
            break;
        case EvsParams::EVS_MODE_4:
            return 15850;
            break;
        case EvsParams::EVS_MODE_5:
            return 18250;
            break;
        case EvsParams::EVS_MODE_6:
            return 19850;
            break;
        case EvsParams::EVS_MODE_7:
            return 23050;
            break;
        case EvsParams::EVS_MODE_8:
            return 23850;
            break;
        case EvsParams::EVS_MODE_9:
            return 5900;
            break;
        case EvsParams::EVS_MODE_10:
            return 7200;
            break;
        case EvsParams::EVS_MODE_11:
            return 8000;
            break;
        case EvsParams::EVS_MODE_12:
            return 9600;
            break;
        case EvsParams::EVS_MODE_13:
            return 13200;
            break;
        case EvsParams::EVS_MODE_14:
            return 16400;
            break;
        case EvsParams::EVS_MODE_15:
            return 24400;
            break;
        case EvsParams::EVS_MODE_16:
            return 32000;
            break;
        case EvsParams::EVS_MODE_17:
            return 48000;
            break;
        case EvsParams::EVS_MODE_18:
            return 64000;
            break;
        case EvsParams::EVS_MODE_19:
            return 96000;
            break;
        case EvsParams::EVS_MODE_20:
            return 128000;
            break;
        default:
            return 13200;
            break;
    }
}

kEvsBitrate ImsMediaAudioUtil::FindMaxEVSBitrate(
        uint32_t nEVSBitrateSet, kEvsCodecMode kEvsCodecMode)
{
    // find EVS bitrate.
    uint32_t nEVSBitrate = 0;
    uint32_t EVSBitrateInfo = nEVSBitrateSet;
    IMLOGD2("[FindMaxEVSBitrate] EVSBitrateInfo[0x%x], kEvsCodecMode[%d]", EVSBitrateInfo,
            kEvsCodecMode);

    // exception handling code, if EVSBitrateSet value is 0, EVS Bitrate set a Maxbitrate
    if (EVSBitrateInfo == 0)
    {
        if (kEvsCodecMode ==
                kEvsCodecModeAmrIo)  // AMR WB IO mode : max nEVSBitrate 8 : 23.85 kbit/s
        {                            // (kEvsAmrIoModeBitrate02385 = 8)
            nEVSBitrate = 8;
        }
        else  // Primary mode : max nEVSBitrate 8 : 128 kbit/s (kEvsPrimaryModeBitrate12800 = 20)
        {
            nEVSBitrate = 20;
        }

        IMLOGD1("[FindMaxEVSBitrate] EVSBitrateSet value is 0...nEVSBitrate set a MaxBitRate[%d]",
                nEVSBitrate);
        return ((kEvsBitrate)nEVSBitrate);
    }

    for (uint32_t i = 15; i >= 0; i--)
    {
        if ((EVSBitrateInfo & (1 << i)) != 0)
        {
            nEVSBitrate = i;
            break;
        }
    }
    IMLOGD1("[FindMaxEVSBitrate] EvsBitrate[%d]", nEVSBitrate);

    // check default mode, [ default mode  ( 16 bit ) ][ mode set (16 bit )  ]
    for (int32_t i = 31; i >= 16; i--)
    {
        if (((EVSBitrateInfo & (1 << i)) != 0) && ((EVSBitrateInfo & (1 << (i - 16))) != 0))
        {
            nEVSBitrate = i - 16;
            break;
        }
    }

    if (kEvsCodecMode == kEvsCodecModePrimary)
    {
        nEVSBitrate = nEVSBitrate + 9;  // convert EVS Primarymode bitrate
    }

    IMLOGD1("[FindMaxEVSBitrate] Selected EvsBitrate[%d]", nEVSBitrate);
    return (kEvsBitrate)nEVSBitrate;
}

kEvsCodecMode ImsMediaAudioUtil::CheckEVSCodecMode(uint32_t nAudioFrameLength)
{
    switch (nAudioFrameLength)
    {
        // EVS AMR IO Mode Case
        case 17:
        case 23:
        case 32:
        case 36:
        case 40:
        case 46:
        case 50:
        case 58:
        case 60:
        case 5:
            return kEvsCodecModeAmrIo;
        // EVS Primary Mode Case
        case 7:
        case 18:
        case 20:
        case 24:
        case 33:
        case 41:
        case 61:
        case 80:
        case 120:
        case 160:
        case 240:
        case 320:
        case 6:
        default:
            return kEvsCodecModePrimary;
    }
}

kRtpPyaloadHeaderMode ImsMediaAudioUtil::ConvertEVSPayloadMode(
        uint32_t nDataSize, kEvsCodecMode* pEVSCodecMode, uint32_t* pEVSCompactId)
{
    uint32_t i = 0;
    uint32_t nDataBitSize = 0;
    // nDataBitSize -= 2;
    nDataBitSize = nDataSize * 8;  // change byte to bit size

    // compact format & primary mode
    for (i = 0; i < EVS_COMPACT_PRIMARY_PAYLOAD_NUM; i++)
    {
        if (gaEVSPrimaryBitLen[i] == nDataBitSize)
        {
            *pEVSCodecMode = kEvsCodecModePrimary;
            *pEVSCompactId = i;
            return kRtpPyaloadHeaderModeEvsCompact;
        }
    }

    // compact format & amr-wb io mode
    for (i = 0; i < EVS_COMPACT_AMRWBIO_PAYLOAD_NUM; i++)
    {
        if (gaEVSAmrWbIoBitLen[i] == nDataBitSize)
        {
            *pEVSCodecMode = kEvsCodecModeAmrIo;
            *pEVSCompactId = i;
            return kRtpPyaloadHeaderModeEvsCompact;
        }
    }

    // TODO : need to check ID...
    *pEVSCodecMode = kEvsCodecModePrimary;
    *pEVSCompactId = EVS_COMPACT_PAYLOAD_MAX_NUM;
    return kRtpPyaloadHeaderModeEvsHeaderFull;
}

kEvsBandwidth ImsMediaAudioUtil::FindMaxEVSBandwidth(uint32_t nEVSBandwidthSet)
{
    // find EVS bandwidth.
    uint32_t nEVSBandwidth = 0;
    uint32_t EVSBandwidthInfo = nEVSBandwidthSet;
    IMLOGD1("[FindMaxEVSBandwidth] EVSBandwidthInfo[0x%x]", EVSBandwidthInfo);

    // exception handling code, if nEVSBandwidthSet value is 0, EVS Bandwidth set a Maxbandwidth
    if (EVSBandwidthInfo == 0)
    {
        nEVSBandwidth = 3;  // kEvsBandwidthFB = 3
        IMLOGD1("[FindMaxEVSBandwidth] nEVSBandwidth value is 0... nEVSBandwidth set a "
                "MaxBandwidth[%d]",
                nEVSBandwidth);
        return (kEvsBandwidth)nEVSBandwidth;
    }

    for (uint32_t i = 15; i >= 0; i--)
    {
        if ((EVSBandwidthInfo & (1 << i)) != 0)
        {
            nEVSBandwidth = i;
            break;
        }
    }
    IMLOGD1("[FindMaxEVSBandwidth] nEVSBandwidth[%d]", nEVSBandwidth);

    // check default mode, [ default mode  ( 16 bit ) ][ mode set (16 bit )  ]
    for (uint32_t i = 31; i >= 16; i--)
    {
        if (((EVSBandwidthInfo & (1 << i)) != 0) && ((EVSBandwidthInfo & (1 << (i - 16))) != 0))
        {
            nEVSBandwidth = i - 16;
            break;
        }
    }

    IMLOGD1("[FindMaxEVSBandwidth] Selected EvsBitrate[%d]", nEVSBandwidth);
    return (kEvsBandwidth)nEVSBandwidth;
}

kEvsBandwidth ImsMediaAudioUtil::FindMaxEvsBandwidthFromRange(int32_t EvsBandwidthRange)
{
    if (EvsBandwidthRange & kEvsBandwidthFB)
    {
        return kEvsBandwidthFB;
    }
    else if (EvsBandwidthRange & kEvsBandwidthSWB)
    {
        return kEvsBandwidthSWB;
    }
    else if (EvsBandwidthRange & kEvsBandwidthWB)
    {
        return kEvsBandwidthWB;
    }
    else if (EvsBandwidthRange & kEvsBandwidthNB)
    {
        return kEvsBandwidthNB;
    }
    else
    {
        return kEvsBandwidthNone;
    }
}
