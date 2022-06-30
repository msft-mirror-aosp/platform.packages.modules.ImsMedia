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

#include "ImsMediaImageRotate.h"

void ImsMediaImageRotate::YUV420_Planar_Rotate90_Flip(
        uint8_t* pbDst, uint8_t* pbSrc, uint32_t nSrcWidth, uint32_t nSrcHeight)
{
    uint32_t srcIdx, dstIdx, x, y;
    const uint32_t size = nSrcWidth * nSrcHeight;
    dstIdx = size - 1;

    // Rotate Y buffer
    for (y = 0; y < nSrcWidth; y++)
    {
        srcIdx = y;
        for (x = 0; x < nSrcHeight; x++)
        {
            pbDst[dstIdx] = pbSrc[srcIdx];  // Y

            srcIdx += nSrcWidth;
            dstIdx--;
        }
    }

    dstIdx = (size * 1.5f) - 1;
    const uint32_t usize = size / 4;
    nSrcWidth /= 2;
    nSrcHeight /= 2;

    // Rotate UV buffer
    for (y = 0; y < nSrcWidth; y++)
    {
        srcIdx = size + y;
        for (x = 0; x < nSrcHeight; x++)
        {
            pbDst[dstIdx - usize] = pbSrc[srcIdx];  // U
            pbDst[dstIdx] = pbSrc[usize + srcIdx];  // V

            srcIdx += nSrcWidth;
            dstIdx--;
        }
    }
}

void ImsMediaImageRotate::YUV420_SP_Rotate90_Flip(uint8_t* pbDst, uint8_t* pYPlane,
        uint8_t* pUVPlane, uint32_t nSrcWidth, uint32_t nSrcHeight)
{
    uint32_t srcIdx, dstIdx, x, y;
    const uint32_t size = nSrcWidth * nSrcHeight;
    dstIdx = size - 1;

    // Rotate Y buffer
    for (y = 0; y < nSrcWidth; y++)
    {
        srcIdx = y;
        for (x = 0; x < nSrcHeight; x++)
        {
            pbDst[dstIdx] = pYPlane[srcIdx];  // Y
            srcIdx += nSrcWidth;
            dstIdx--;
        }
    }

    dstIdx = (size * 1.5f) - 1;
    nSrcWidth /= 2;
    nSrcHeight /= 2;

    // Rotate UV buffer
    for (y = 0; y < nSrcWidth; y++)
    {
        srcIdx = y * 2;
        for (x = 0; x < nSrcHeight; x++)
        {
            pbDst[dstIdx--] = pUVPlane[srcIdx + 1];  // V
            pbDst[dstIdx--] = pUVPlane[srcIdx];      // U
            srcIdx += nSrcWidth * 2;
        }
    }
}

void ImsMediaImageRotate::YUV420_SP_Rotate270(uint8_t* pbDst, uint8_t* pYPlane, uint8_t* pUVPlane,
        uint32_t nSrcWidth, uint32_t nSrcHeight)
{
    uint32_t srcIdx, dstIdx, x, y;
    const uint32_t size = nSrcWidth * nSrcHeight;
    dstIdx = size - 1;

    // Rotate Y buffer
    for (y = 0; y < nSrcWidth; y++)
    {
        srcIdx = size - nSrcWidth + y;
        for (x = 0; x < nSrcHeight; x++)
        {
            pbDst[dstIdx] = pYPlane[srcIdx];  // Y
            srcIdx -= nSrcWidth;
            dstIdx--;
        }
    }

    dstIdx = (size * 1.5f) - 1;
    nSrcWidth /= 2;
    nSrcHeight /= 2;

    // Rotate UV buffer
    for (y = 0; y < nSrcWidth; y++)
    {
        srcIdx = (size / 2) - (nSrcWidth - y) * 2;
        for (x = 0; x < nSrcHeight; x++)
        {
            pbDst[dstIdx--] = pUVPlane[srcIdx + 1];  // V
            pbDst[dstIdx--] = pUVPlane[srcIdx];      // U
            srcIdx -= nSrcWidth * 2;
        }
    }
}