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

#include <stdlib.h>
#include <string.h>

#include <rtp_pf_memory.h>

namespace RtpPfMemory
{
void* RtpMemCpy(void* dst, const void* src, int size, const char* filename, unsigned int line) {
    (void)filename;
    (void)line;
    //add debug code later
    return memcpy(dst, src, size);
}

void* RtpMemSet(void* dst, const int c, int size) {
    return memset(dst, c, size);
}

int RtpMemCmp(const void* s1, const void* s2, int size) {
    return memcmp(s1, s2, size);
}

void* RtpMemAlloc(int size, const char* filename, int line) {
    (void)filename;
    (void)line;
    //add debug code later
    void* addr = malloc(size);
    return addr;
}

void* RtpMemReAlloc(void* addr, int size) {
    return realloc(addr, size);
}

void RtpMemFree(void* addr) {
    if (addr == NULL) return;
    free(addr);
    addr = NULL;
}
}