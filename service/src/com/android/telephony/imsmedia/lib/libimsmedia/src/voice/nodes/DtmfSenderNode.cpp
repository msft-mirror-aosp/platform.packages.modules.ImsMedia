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

#include <ImsMediaDefine.h>
#include <ImsMediaTrace.h>
#include <DtmfSenderNode.h>
#include <ImsMediaTimer.h>
#include <AudioConfig.h>

DtmfSenderNode::DtmfSenderNode() {
    mNextTime = 0;
    mPrevTime = 0;
}

DtmfSenderNode::~DtmfSenderNode() {
}

BaseNode* DtmfSenderNode::GetInstance() {
    BaseNode* pNode;
    pNode = new DtmfSenderNode();

    if (pNode == NULL) {
        IMLOGE0("[GetInstance] Can't create DtmfSenderNode");
    }

    return pNode;
}

void DtmfSenderNode::ReleaseInstance(BaseNode* pNode) {
    delete (DtmfSenderNode*)pNode;
}

BaseNodeID DtmfSenderNode::GetNodeID() {
    return NODEID_DTMFSENDER;
}

ImsMediaResult DtmfSenderNode::Start() {
    mNextTime = 0;
    mNodeState = NODESTATE_RUNNING;
    return IMS_MEDIA_OK;
}

void DtmfSenderNode::Stop() {
    mNextTime = 0;
    mNodeState = NODESTATE_STOPPED;
}

bool DtmfSenderNode::IsRunTime() {
    return false;
}

bool DtmfSenderNode::IsSourceNode() {
    return false;
}

void DtmfSenderNode::ProcessData() {
    ImsMediaSubType subtype;
    uint8_t* pData;
    uint32_t nDataSize;
    uint32_t nTimeStamp;
    bool bMark;
    uint32_t nCurrTime;

    if (GetData(&subtype, &pData, &nDataSize, &nTimeStamp, &bMark, NULL, NULL) == false) {
        return;
    }

    nCurrTime = ImsMediaTimer::GetTimeInMilliSeconds();

    if (mNextTime &&  !(nCurrTime >= mNextTime
        || nCurrTime < mPrevTime || mNextTime < mPrevTime)) {
        mPrevTime = nCurrTime;
        return;
    }

    if (subtype == MEDIASUBTYPE_DTMFSTART) {
        SendDataToRearNode(subtype, pData,
            nDataSize, nTimeStamp, bMark, 0);
        DeleteData();
        mNextTime = nCurrTime;

        // send the first dtmf packet
        if (GetData(&subtype, &pData, &nDataSize, &nTimeStamp, &bMark, NULL, NULL)
            && subtype == MEDIASUBTYPE_DTMF_PAYLOAD) {
            SendDataToRearNode(subtype, pData, nDataSize, nTimeStamp, bMark, 0);

            if (nDataSize >= 4) {
                IMLOGD4("[ProcessData] Send DTMF packet %02X %02X %02X %02X",
                    pData[0], pData[1], pData[2], pData[3]);
            }

            DeleteData();
            mNextTime += 20;
        }
    }
    else if (subtype == MEDIASUBTYPE_DTMFEND) {
        SendDataToRearNode(subtype, pData, nDataSize, nTimeStamp, bMark, 0);
        DeleteData();
        mNextTime += mInterval;
    } else {
        SendDataToRearNode(subtype, pData, nDataSize, nTimeStamp, bMark, 0);

        if (nDataSize >= 4) {
            IMLOGD4("[ProcessData] Send DTMF packet %02X %02X %02X %02X",
                pData[0], pData[1], pData[2], pData[3]);
        }

        DeleteData();
        mNextTime += 20;
    }

    mPrevTime = nCurrTime;
}

void DtmfSenderNode::SetInterval(uint32_t interval) {
    mInterval = interval;
}