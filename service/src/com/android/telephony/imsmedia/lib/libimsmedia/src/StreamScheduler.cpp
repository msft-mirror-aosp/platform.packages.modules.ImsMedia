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

#define RUN_WAIT_TIMEOUT 6

StreamScheduler::StreamScheduler() {}

StreamScheduler::~StreamScheduler()
{
    Stop();
}

void StreamScheduler::RegisterNode(BaseNode* pNode)
{
    if (pNode == NULL)
    {
        return;
    }

    IMLOGD2("[RegisterNode] [%p], node[%s]", this, pNode->GetNodeName());
    std::lock_guard<std::mutex> guard(mMutex);

    if (pNode->IsSourceNode())
    {
        mlistSourceNode.push_back(pNode);
    }
    else
    {
        mlistRegisteredNode.push_back(pNode);
    }
}

void StreamScheduler::DeRegisterNode(BaseNode* pNode)
{
    if (pNode == NULL)
    {
        return;
    }

    IMLOGD2("[DeRegisterNode] [%p], node[%s]", this, pNode->GetNodeName());
    std::lock_guard<std::mutex> guard(mMutex);

    if (pNode->IsSourceNode())
    {
        mlistSourceNode.remove(pNode);
    }
    else
    {
        mlistRegisteredNode.remove(pNode);
    }
}

void StreamScheduler::Start()
{
    uint32_t nNumOfRegisteredNode = 0;
    IMLOGD1("[Start] [%p] enter", this);

    for (auto& node : mlistSourceNode)
    {
        if (node != NULL)
        {
            nNumOfRegisteredNode++;
            IMLOGD2("[Start] [%p] registered source node [%s]", this, node->GetNodeName());
        }
    }

    for (auto& node : mlistRegisteredNode)
    {
        if (node != NULL)
        {
            nNumOfRegisteredNode++;
            IMLOGD2("[Start] [%p] registered node [%s]", this, node->GetNodeName());
        }
    }

    if (nNumOfRegisteredNode > 0)
    {
        IMLOGD1("[Start] [%p] Start thread", this);
        StartThread();
    }

    IMLOGD1("[Start] [%p] exit", this);
}

void StreamScheduler::Stop()
{
    IMLOGD1("[Stop] [%p] enter", this);

    StopThread();
    Awake();
    mConditionExit.wait_timeout(RUN_WAIT_TIMEOUT * 2);

    IMLOGD1("[Stop] [%p] exit", this);
}

void StreamScheduler::Awake()
{
    mConditionMain.signal();
}

BaseNode* StreamScheduler::DeterminProcessingNode(uint32_t* pnMaxDataInNode)
{
    if (IsThreadStopped())
    {
        return NULL;
    }

    BaseNode* pRetNode = NULL;
    uint32_t nMaxDataInNode = 0;

    for (auto& node : mlistNodeToRun)
    {
        uint32_t nDataInNode;

        if (node != NULL)
        {
            nDataInNode = node->GetDataCount();

            if (nDataInNode > 0 && nDataInNode >= nMaxDataInNode)
            {
                pRetNode = node;
                nMaxDataInNode = nDataInNode;
            }
        }
    }

    *pnMaxDataInNode = nMaxDataInNode;
    return pRetNode;
}

void StreamScheduler::RunRegisteredNode()
{
    BaseNode* pNode;
    uint32_t nMaxDataInNode;

    // run source nodes
    for (auto& node : mlistSourceNode)
    {
        if (node != NULL && node->GetState() == kNodeStateRunning)
        {
            node->ProcessData();
        }
    }

    // run nodes
    for (auto& node : mlistRegisteredNode)
    {
        if (node != NULL)
        {
            mlistNodeToRun.push_back(node);
        }
    }

    for (;;)
    {
        pNode = DeterminProcessingNode(&nMaxDataInNode);

        if (pNode == NULL)
        {
            break;
        }

        if (pNode->GetState() == kNodeStateRunning)
        {
            pNode->ProcessData();
        }

        if (IsThreadStopped())
        {
            break;
        }

        mlistNodeToRun.remove(pNode);
    };

    mlistNodeToRun.clear();
}

void* StreamScheduler::run()
{
    IMLOGD1("[run] [%p] enter", this);

    while (!IsThreadStopped())
    {
        mMutex.lock();
        RunRegisteredNode();
        mMutex.unlock();

        if (IsThreadStopped())
        {
            break;
        }

        if (mlistSourceNode.size() > 0)
        {
            mConditionMain.wait_timeout(RUN_WAIT_TIMEOUT / 2);
        }
        else
        {
            mConditionMain.wait_timeout(RUN_WAIT_TIMEOUT / 2);
        }
    }

    mConditionExit.signal();
    IMLOGD1("[run] [%p] exit", this);
    return NULL;
}
