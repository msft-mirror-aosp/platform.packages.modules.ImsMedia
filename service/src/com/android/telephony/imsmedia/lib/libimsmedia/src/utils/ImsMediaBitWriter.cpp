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

#include <ImsMediaBitWriter.h>
#include <ImsMediaTrace.h>
#include <string.h>

ImsMediaBitWriter::ImsMediaBitWriter()
{
    m_pbBuffer = NULL;
    m_nMaxBufferSize = 0;
    m_nBytePos = 0;
    m_nBitPos = 0;
    m_nBitBuffer = 0;
    m_bBufferFull = false;
}

ImsMediaBitWriter::~ImsMediaBitWriter() {}

void ImsMediaBitWriter::SetBuffer(uint8_t* pbBuffer, uint32_t nBufferSize)
{
    m_nBytePos = 0;
    m_nBitPos = 0;
    m_nBitBuffer = 0;
    m_bBufferFull = false;
    m_pbBuffer = pbBuffer;
    m_nMaxBufferSize = nBufferSize;
}

void ImsMediaBitWriter::Write(uint32_t nValue, uint32_t nSize)
{
    if (nSize == 0)
        return;

    if (m_pbBuffer == NULL || nSize > 24 || m_bBufferFull)
    {
        IMLOGE2("[ImsMediaBitWriter::PutBit] nSize[%d], bBufferFull[%d]", nSize, m_bBufferFull);
        return;
    }

    // write to bit buffer
    m_nBitBuffer += (nValue << (32 - nSize) >> m_nBitPos);
    m_nBitPos += nSize;

    // write to byte buffer
    while (m_nBitPos >= 8)
    {
        m_pbBuffer[m_nBytePos++] = (uint8_t)(m_nBitBuffer >> 24);
        m_nBitBuffer <<= 8;
        m_nBitPos -= 8;
    }

    if (m_nBytePos >= m_nMaxBufferSize)
    {
        m_bBufferFull = true;
    }
}

void ImsMediaBitWriter::WriteByteBuffer(uint8_t* pbSrc, uint32_t nBitSize)
{
    uint32_t nByteSize;
    uint32_t nRemainBitSize;
    nByteSize = nBitSize >> 3;
    nRemainBitSize = nBitSize & 0x07;

    if (m_nBitPos == 0)
    {
        memcpy(m_pbBuffer + m_nBytePos, pbSrc, nByteSize);
        m_nBytePos += nByteSize;
    }
    else
    {
        uint32_t i;

        for (i = 0; i < nByteSize; i++)
        {
            Write(pbSrc[i], 8);
        }
    }

    if (nRemainBitSize > 0)
    {
        uint32_t v = pbSrc[nByteSize];
        v >>= (8 - nRemainBitSize);
        Write(v, nRemainBitSize);
    }
}

void ImsMediaBitWriter::WriteByteBuffer(uint32_t value)
{
    uint32_t nRemainBitSize = 32;

    for (uint32_t i = 0; i < 4; i++)
    {
        nRemainBitSize -= 8;
        uint8_t v = (value >> nRemainBitSize) & 0x00ff;
        Write(v, 8);
    }
}

void ImsMediaBitWriter::Seek(uint32_t nSize)
{
    Flush();
    m_nBitPos += nSize;

    while (m_nBitPos >= 8)
    {
        m_nBytePos++;
        m_nBitPos -= 8;
    }
}

void ImsMediaBitWriter::AddPadding()
{
    if (m_nBitPos > 0)
    {
        Write(0, 8 - m_nBitPos);
    }
}

uint32_t ImsMediaBitWriter::GetBufferSize()
{
    uint32_t nSize;
    nSize = (m_nBitPos + 7) >> 3;
    nSize += m_nBytePos;
    return nSize;
}

void ImsMediaBitWriter::Flush()
{
    if (m_nBitPos > 0)
    {
        m_pbBuffer[m_nBytePos] += (uint8_t)(m_nBitBuffer >> 24);
        m_nBitBuffer = 0;
    }
}