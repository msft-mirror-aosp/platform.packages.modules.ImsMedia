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

#ifndef IMS_MEDIA_DATA_QUEUE_H
#define IMS_MEDIA_DATA_QUEUE_H

#include <ImsMediaDefine.h>
#include <list>

using namespace std;

struct DataEntry
{
    uint8_t* pbBuffer;
    uint32_t nBufferSize;
    uint32_t nTimestamp;
    bool bMark;
    uint16_t nSeqNum;
    int32_t nPayloadTime;
    bool bHeader;
    bool bValid;
    uint32_t nInputTime;
    ImsMediaSubType eDataType;
    ImsMediaSubType subtype;
};

/*!
 *    @class ImsMediaDataQueue
 *    @brief
 */
class ImsMediaDataQueue
{
public:
    ImsMediaDataQueue();
    virtual ~ImsMediaDataQueue();

private:
    ImsMediaDataQueue(const ImsMediaDataQueue& obj);
    ImsMediaDataQueue& operator=(const ImsMediaDataQueue& obj);

public:
    void Add(DataEntry* pEntry);
    void InsertAt(uint32_t index, DataEntry* pEntry);
    void Delete();
    void Clear();
    bool Get(DataEntry** ppEntry);
    bool GetLast(DataEntry** ppEntry);
    bool GetAt(uint32_t index, DataEntry** ppEntry);
    uint32_t GetCount();
    void SetReadPosFirst();
    bool GetNext(DataEntry** ppEntry);

private:
    list<uint8_t*> m_List;  // data list
    list<uint8_t*>::iterator m_ListIter;
};

#endif