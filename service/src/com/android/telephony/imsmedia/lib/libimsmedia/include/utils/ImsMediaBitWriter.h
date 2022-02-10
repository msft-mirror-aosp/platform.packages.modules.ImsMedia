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

#ifndef IMS_MEDIA_BITWRITER_H
#define IMS_MEDIA_BITWRITER_H

#include <stdint.h>

class ImsMediaBitWriter
{
private:
    uint8_t* m_pbBuffer;
    uint32_t m_nMaxBufferSize;
    uint32_t m_nBytePos;
    uint32_t m_nBitPos;
    uint32_t m_nBitBuffer;
    bool m_bBufferFull;

public:
    ImsMediaBitWriter();
    ~ImsMediaBitWriter();
    void SetBuffer(uint8_t* pbBuffer, uint32_t nBufferSize);
    void Write(uint32_t nValue, uint32_t nSize);
    void WriteByteBuffer(uint8_t* pbSrc, uint32_t nBitSize);
    void WriteByteBuffer(uint32_t value);
    void Seek(uint32_t nSize);
    void AddPadding();
    uint32_t GetBufferSize();
    void Flush();
};

#endif