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

#include <StreamScheduler.h>
#include <ImsMediaTrace.h>
#include <stdint.h>
#include <chrono>
#include <thread>

using namespace std::chrono;

#define RUN_WAIT_TIMEOUT 9

StreamScheduler::StreamScheduler() {
    mbStarted = false;
    mbTerminate = false;
    mbStartPending = false;
}

StreamScheduler::~StreamScheduler() {

}

void StreamScheduler::RegisterNode(BaseNode* pNode) {
    mMutex.lock();
    if (pNode->IsSourceNode()) {
        mlistSourceNode.push_back(pNode);
    } else {
        mlistRegisteredNode.push_back(pNode);
    }

    mMutex.unlock();
    if (mbStartPending == true) {
        IMLOGD1("[AddNode] [%p] Start Pending thread", this);
        Start();
    }
}

void StreamScheduler::DeRegisterNode(BaseNode* pNode) {
    IMLOGD_PACKET1(IM_PACKET_LOG_SCHEDULER,
        "[DeRegisterNode] [%p] enter", this);
    mMutex.lock();
    IMLOGD_PACKET1(IM_PACKET_LOG_SCHEDULER,
        "[DeRegisterNode] [%p] enter critical section", this);

    if (pNode->IsSourceNode()) {
        mlistSourceNode.remove(pNode);
    } else {
        mlistRegisteredNode.remove(pNode);
    }

    mMutex.unlock();
    if (mbStarted) {
        uint32_t nNumOfRegisteredNode = 0;
        nNumOfRegisteredNode += mlistSourceNode.size();
        nNumOfRegisteredNode += mlistRegisteredNode.size();
        if (nNumOfRegisteredNode == 0) {
            IMLOGD1("[DeRegisterNode] [%p] Stop thread and goto pending state", this);
            Stop();
            mbStartPending = true;
        }
    }

    IMLOGD_PACKET1(IM_PACKET_LOG_SCHEDULER, "[DeRegisterNode] [%p] exit", this);
}

void StreamScheduler::Start() {
    uint32_t nNumOfRegisteredNode = 0;
    IMLOGD1("[Start] [%p] enter", this);
    if (mbStarted) {
        IMLOGD1("[Start] [%p] exit - scheduler is already started", this);
        return;
    }

    for (auto&i:mlistSourceNode) {
        nNumOfRegisteredNode++;
        IMLOGD2("[Start] [%p] registered source node [%s]",
            this, i->GetNodeName());
    }

    for (auto&i:mlistRegisteredNode) {
        nNumOfRegisteredNode++;
        IMLOGD2("[Start] [%p] registered node [%s]",
            this, i->GetNodeName());
    }

    if (nNumOfRegisteredNode > 0) {
        IMLOGD1("[Start] [%p] Start thread", this);
        mbStartPending = false;
        mbTerminate = false;
        mbStarted = true;
        for (int i = 0; i < 3; i++) {
            if (StartThread()) break;
            std::this_thread::sleep_for(10ms);
        }
    } else {
        IMLOGD1("[Start] [%p] no node to run, goto pending state", this);
        mbStartPending = true;
    }
    IMLOGD1("[Start] [%p] exit", this);
}

void StreamScheduler::Stop() {
    IMLOGD1("[Stop] [%p] enter", this);
    if (mbStarted) {
        mbTerminate = true;
        mbStarted = false;
        Awake();
        mCondExit.wait_timeout(RUN_WAIT_TIMEOUT * 2);
    }

    IMLOGD1("[Stop] [%p] exit", this);
}

void StreamScheduler::Awake() {
    IMLOGD_PACKET0(IM_PACKET_LOG_SCHEDULER, "[Awake]");
    mCondMain.signal();
}

BaseNode* StreamScheduler::DeterminProcessingNode(uint32_t* pnMaxDataInNode) {
    BaseNode* pRetNode = NULL;
    uint32_t nMaxDataInNode = 0;
    for (auto&i:mlistNodeToRun) {
        uint32_t nDataInNode;
        if (mbTerminate) {
            pRetNode = NULL;
            break;
        }

        nDataInNode = i->GetDataCount();
        if (nDataInNode > 0 && nDataInNode >= nMaxDataInNode) {
            pRetNode = i;
            nMaxDataInNode = nDataInNode;
        }
    }
    *pnMaxDataInNode = nMaxDataInNode;
    return pRetNode;
}

void StreamScheduler::RunRegisteredNode() {
    BaseNode* pNode;
    uint32_t nMaxDataInNode;
    IMLOGD_PACKET1(IM_PACKET_LOG_SCHEDULER,
        "[RunRegisteredNode] Run Source Nodes [%p]", this);
    // run source nodes
    for (auto&i:mlistSourceNode) {
        if (i->GetState() == NODESTATE_RUNNING) {
            i->ProcessData();
        }
    }

    // run nodes
    for (auto&i:mlistRegisteredNode) {
        mlistNodeToRun.push_back(i);
    }

    for (;;) {
        pNode = DeterminProcessingNode(&nMaxDataInNode);
        if (pNode == NULL || mbTerminate) break;
        if (pNode->GetState() == NODESTATE_RUNNING) {
            pNode->ProcessData();
        }
        if (mbTerminate) break;
        mlistNodeToRun.remove(pNode);
    };

    mlistNodeToRun.clear();
}

void* StreamScheduler::run() {
    IMLOGD1("[run] [%p] enter", this);
    while (!mbTerminate) {
        mMutex.lock();
        RunRegisteredNode();
        mMutex.unlock();

        if (mbTerminate) break;
        if (mlistSourceNode.size() > 0) {
            mCondMain.wait_timeout(RUN_WAIT_TIMEOUT / 2);
        } else {
            mCondMain.wait_timeout(RUN_WAIT_TIMEOUT / 2);
        }

        IMLOGD_PACKET1(IM_PACKET_LOG_SCHEDULER,
            "[run] [%p] mCondMain.wait_timeout return", this);
    }
    IMLOGD1("[run] [%p] send exit signal", this);
    mCondExit.signal();
    IMLOGD1("[run] [%p] exit", this);
    return NULL;
}
