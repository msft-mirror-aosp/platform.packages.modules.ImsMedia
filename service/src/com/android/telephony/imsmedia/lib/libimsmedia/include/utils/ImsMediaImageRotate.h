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

#ifndef IMS_MEDIA_IMAGE_ROTATE
#define IMS_MEDIA_IMAGE_ROTATE

#include <string.h>

class ImsMediaImageRotate
{
public:
    /**
     * @brief Rotates YUVImage_420_Planar Image by 90 degrees and flips.
     * Supports zero pixel and zero row stride.
     *
     *  Source Image    Destination Image
     *  + - - - - +     + - - - - +
     *  | 1  2  3 |     | 9  6  3 |
     *  | 4  5  6 |     | 8  5  2 |
     *  | 7  8  9 |     | 7  4  1 |
     *  + - - - - +     + - - - - +
     *
     * @param pbDst Destination buffer with size nDstWidth*nDstHeight*1.5.
     * @param pbSrc Source buffer with size nDstWidth*nDstHeight*1.5.
     * @param nSrcWidth Source Image width.
     * @param nSrcHeight Source Image height.
     */
    static void YUV420_Planar_Rotate90_Flip(
            uint8_t* pbDst, uint8_t* pbSrc, uint32_t nSrcWidth, uint32_t nSrcHeight);

    /**
     * @brief Rotates YUVImage_420_888 Image by 90 degrees.
     * Supports zero pixel stride and zero row stride.
     *
     *  Source Image    Destination Image
     *  + - - - - +     + - - - - +
     *  | 1  2  3 |     | 7  4  1 |
     *  | 4  5  6 |     | 8  5  2 |
     *  | 7  8  9 |     | 9  6  3 |
     *  + - - - - +     + - - - - +
     *
     * @param pbDst Destination buffer with size nDstWidth*nDstHeight*1.5.
     * @param pYPlane Y-Plane data of size nDstWidth*nDstHeight.
     * @param pUVPlane UV-Plane data of size (nDstWidth*nDstHeight)/2.
     * @param nSrcWidth Source Image width.
     * @param nSrcHeight Source Image height.
     */
    static void YUV420_SP_Rotate90(uint8_t* pbDst, uint8_t* pYPlane, uint8_t* pUVPlane,
            uint32_t nSrcWidth, uint32_t nSrcHeight);

    /**
     * @brief Rotates YUVImage_420_888 Image by 90 degrees and flip.
     * Supports zero pixel stride and zero row stride.
     *
     *  Source Image    Destination Image
     *  + - - - - +     + - - - - +
     *  | 1  2  3 |     | 9  6  3 |
     *  | 4  5  6 |     | 8  5  2 |
     *  | 7  8  9 |     | 7  4  1 |
     *  + - - - - +     + - - - - +
     *
     * @param pbDst Destination buffer with size nDstWidth*nDstHeight*1.5.
     * @param pYPlane Y-Plane data of size nDstWidth*nDstHeight.
     * @param pUVPlane UV-Plane data of size (nDstWidth*nDstHeight)/2.
     * @param nSrcWidth Source Image width.
     * @param nSrcHeight Source Image height.
     */
    static void YUV420_SP_Rotate90_Flip(uint8_t* pbDst, uint8_t* pYPlane, uint8_t* pUVPlane,
            uint32_t nSrcWidth, uint32_t nSrcHeight);

    /**
     * @brief Rotates YUVImage_420_888 Image by 270 degrees.
     * Supports zero pixel stride and zero row stride.
     *
     *  Source Image    Destination Image
     *  + - - - - +     + - - - - +
     *  | 1  2  3 |     | 3  6  9 |
     *  | 4  5  6 |     | 2  5  8 |
     *  | 7  8  9 |     | 1  4  7 |
     *  + - - - - +     + - - - - +
     *
     * @param pbDst Destination buffer with size nDstWidth*nDstHeight*1.5.
     * @param pYPlane Y-Plane data of size nDstWidth*nDstHeight.
     * @param pUVPlane UV-Plane data of size (nDstWidth*nDstHeight)/2.
     * @param nSrcWidth Source Image width.
     * @param nSrcHeight Source Image height.
     */
    static void YUV420_SP_Rotate270(uint8_t* pbDst, uint8_t* pYPlane, uint8_t* pUVPlane,
            uint32_t nSrcWidth, uint32_t nSrcHeight);
};

#endif  // IMS_MEDIA_IMAGE_ROTATE