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

#ifndef DTMFSENDERNODE_H_INCLUDED
#define DTMFSENDERNODE_H_INCLUDED

#include <ImsMediaDefine.h>
#include <BaseNode.h>

class DtmfSenderNode : public BaseNode {
private:
    DtmfSenderNode();
    ~DtmfSenderNode();

public:
    static BaseNode* GetInstance();
    static void ReleaseInstance(BaseNode* pNode);
    virtual BaseNodeID GetNodeID();
    virtual ImsMediaResult Start();
    virtual void Stop();
    virtual bool IsRunTime();
    virtual bool IsSourceNode();
    virtual void ProcessData();
    void SetInterval(uint32_t interval);

private:
    uint32_t mNextTime;
    uint32_t mPrevTime;
    uint32_t mInterval; // msec unit, interval of two DTMF signals
};


#endif // DTMFSENDERNODE_H_INCLUDED
