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

#include <ImsMediaBitReader.h>
#include <ImsMediaTrace.h>
#include <string.h>

ImsMediaBitReader::ImsMediaBitReader()
{
    m_pbBuffer = NULL;
    m_nMaxBufferSize = 0;
    m_nBytePos = 0;
    m_nBitPos = 0;
    m_nBitBuffer = 0;
    m_bBufferEOF = false;
}

ImsMediaBitReader::~ImsMediaBitReader() {}

void ImsMediaBitReader::SetBuffer(uint8_t* pbBuffer, uint32_t nBufferSize)
{
    m_nBytePos = 0;
    m_nBitPos = 32;
    m_nBitBuffer = 0;
    m_bBufferEOF = false;
    m_pbBuffer = pbBuffer;
    m_nMaxBufferSize = nBufferSize;
}

uint32_t ImsMediaBitReader::Read(uint32_t nSize)
{
    uint32_t value;
    if (nSize == 0)
        return 0;
    if (m_pbBuffer == NULL || nSize > 24 || m_bBufferEOF)
    {
        IMLOGE2("[ImsMediaBitReader::GetBit] nSize[%d], bBufferEOF[%d]", nSize, m_bBufferEOF);
        return 0;
    }

    // read from byte buffer
    while ((32 - m_nBitPos) < nSize)
    {
        if (m_nBytePos >= m_nMaxBufferSize)
        {
            m_bBufferEOF = true;
            IMLOGE2("[ImsMediaBitReader::GetBit] End of Buffer : nBytePos[%d], nMaxBufferSize[%d]",
                    m_nBytePos, m_nMaxBufferSize);
            return 0;
        }

        m_nBitPos -= 8;
        m_nBitBuffer <<= 8;
        m_nBitBuffer += m_pbBuffer[m_nBytePos++];
    }

    // read from bit buffer
    value = m_nBitBuffer << m_nBitPos >> (32 - nSize);
    m_nBitPos += nSize;
    return value;
}

void ImsMediaBitReader::ReadByteBuffer(uint8_t* pbDst, uint32_t nBitSize)
{
    uint32_t dst_pos = 0;
    uint32_t nByteSize;
    uint32_t nRemainBitSize;
    nByteSize = nBitSize >> 3;
    nRemainBitSize = nBitSize & 0x07;

    if (m_nBitPos == 32)
    {
        memcpy(pbDst, m_pbBuffer + m_nBytePos, nByteSize);
        m_nBytePos += nByteSize;
        dst_pos += nByteSize;
    }
    else
    {
        for (dst_pos = 0; dst_pos < nByteSize; dst_pos++)
        {
            pbDst[dst_pos] = Read(8);
        }
    }

    if (nRemainBitSize > 0)
    {
        uint32_t v;
        v = Read(nRemainBitSize);
        v <<= (8 - nRemainBitSize);
        pbDst[dst_pos] = (unsigned char)v;
    }
}

uint32_t ImsMediaBitReader::ReadByUEMode()
{
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t k = 1;
    uint32_t result = 0;

    while (Read(1) == 0 && m_bBufferEOF == false)
    {
        i++;
    }

    j = Read(i);
    result = j - 1 + (k << i);
    return result;
}