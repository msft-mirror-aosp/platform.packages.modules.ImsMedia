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

#include <ImsMediaDataQueue.h>
#include <string.h>

ImsMediaDataQueue::ImsMediaDataQueue() {
}

ImsMediaDataQueue::~ImsMediaDataQueue() {
    while(GetCount() > 0) Delete();
}

void ImsMediaDataQueue::Add(DataEntry* pEntry) {
    uint8_t* pbData = NULL;
    pbData = (uint8_t*)malloc(sizeof(DataEntry) + pEntry->nBufferSize);

    if (pbData == NULL) return;

    memcpy(pbData, pEntry, sizeof(DataEntry));

    if (pEntry->nBufferSize > 0 && pEntry->pbBuffer) {
        memcpy(pbData+sizeof(DataEntry), pEntry->pbBuffer, pEntry->nBufferSize);
    }

    ((DataEntry*)pbData)->pbBuffer = pbData+sizeof(DataEntry);

    m_List.push_back(pbData);
}

void ImsMediaDataQueue::InsertAt(uint32_t index, DataEntry* pEntry) {
    uint8_t* pbData;
    pbData = (uint8_t*)malloc(sizeof(DataEntry) + pEntry->nBufferSize);

    if (pbData == NULL) return;

    memcpy(pbData, pEntry, sizeof(DataEntry));

    if (pEntry->nBufferSize > 0 && pEntry->pbBuffer) {
        memcpy(pbData + sizeof(DataEntry), pEntry->pbBuffer, pEntry->nBufferSize);
    }

    ((DataEntry*)pbData)->pbBuffer = pbData + sizeof(DataEntry);

    std::list<uint8_t*>::iterator iter = m_List.begin();
    advance(iter, index);
    m_List.insert(iter, pbData);
    //m_List.InsertAt(index, pbData);
}

void ImsMediaDataQueue::Delete() {
    if (!m_List.empty()) {
        uint8_t* pbData = m_List.front();
        free((uint8_t*)pbData);
        pbData = NULL;
        m_List.pop_front();
    }
}

void ImsMediaDataQueue::Clear() {
    while(GetCount() > 0) Delete();
}

bool ImsMediaDataQueue::Get(DataEntry** ppEntry) {
    //get first data in the queue
    uint8_t* pbData = m_List.front();

    if (pbData != NULL) {
        *ppEntry = (DataEntry*)pbData;
        return true;
    } else {
        *ppEntry = NULL;
        return false;
    }
}

bool ImsMediaDataQueue::GetLast(DataEntry** ppEntry) {
    //get last data in the queue
    if (GetCount() > 0) {
        uint8_t* pbData = m_List.back();
        *ppEntry = (DataEntry*)pbData;
        return true;
    } else {
        *ppEntry = NULL;
        return false;
    }
}

bool ImsMediaDataQueue::GetAt(uint32_t index, DataEntry** ppEntry) {
    if (GetCount() > index) {
        std::list<uint8_t*>::iterator iter = m_List.begin();
        advance(iter, index);
        uint8_t* pbData = *(iter);
        *ppEntry = (DataEntry*)pbData;
        return true;
    } else {
        *ppEntry = NULL;
        return false;
    }
}

uint32_t ImsMediaDataQueue::GetCount() {
    return m_List.size();
}

void ImsMediaDataQueue::SetReadPosFirst() {
    m_ListIter = m_List.begin();
}

bool ImsMediaDataQueue::GetNext(DataEntry** ppEntry) {
    if (++m_ListIter != m_List.end()) {
        uint8_t* pbData = *m_ListIter;
        *ppEntry = (DataEntry*)pbData;
        return true;
    } else {
        *ppEntry = NULL;
        return false;
    }
}
