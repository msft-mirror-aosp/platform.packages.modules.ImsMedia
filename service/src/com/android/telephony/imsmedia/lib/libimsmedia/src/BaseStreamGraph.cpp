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

#include <ImsMediaTrace.h>
#include <BaseStreamGraph.h>

BaseStreamGraph::BaseStreamGraph(BaseSessionCallback* callback, int localFd) :
        mCallback(callback),
        mLocalFd(localFd)
{
    IMLOGD0("[BaseStreamGraph]");
    std::unique_ptr<StreamScheduler> scheduler(new StreamScheduler());
    mScheduler = std::move(scheduler);
}

BaseStreamGraph::~BaseStreamGraph()
{
    if (mListNodeToStart.size() > 0)
    {
        deleteNodes();
    }

    IMLOGD0("[~BaseStreamGraph]");
}

ImsMediaResult BaseStreamGraph::start()
{
    IMLOGD0("[start]");
    ImsMediaResult ret = startNodes();
    if (ret != RESULT_SUCCESS)
    {
        return ret;
    }
    setState(StreamState::kStreamStateRunning);
    return RESULT_SUCCESS;
}

ImsMediaResult BaseStreamGraph::stop()
{
    IMLOGD0("[stop]");
    ImsMediaResult ret = stopNodes();
    if (ret != RESULT_SUCCESS)
    {
        return ret;
    }
    setState(StreamState::kStreamStateCreated);
    return RESULT_SUCCESS;
}

void BaseStreamGraph::AddNode(BaseNode* pNode, bool bReverse)
{
    if (pNode == NULL)
    {
        return;
    }

    IMLOGD1("[AddNode] node[%s]", pNode->GetNodeName());
    if (bReverse == true)
    {
        mListNodeToStart.push_front(pNode);  // reverse direction
    }
    else
    {
        mListNodeToStart.push_back(pNode);
    }
    if (pNode->IsRunTime() == false)
    {
        IMLOGD1("[AddNode] Add to scheduler[%s]", pNode->GetNodeName());
        mScheduler->RegisterNode(pNode);
    }
}

void BaseStreamGraph::RemoveNode(BaseNode* pNode)
{
    if (pNode == NULL)
    {
        return;
    }

    if (pNode->IsRunTime() == false)
    {
        mScheduler->DeRegisterNode(pNode);
    }

    BaseNode::UnLoad(pNode);
}

ImsMediaResult BaseStreamGraph::startNodes()
{
    BaseNode* pNode = NULL;
    std::list<BaseNode*>::iterator iter;
    ImsMediaResult ret = ImsMediaResult::RESULT_NOT_READY;
    while (mListNodeToStart.size() > 0)
    {
        pNode = mListNodeToStart.front();
        IMLOGD1("[startNodes] start node[%s]", pNode->GetNodeName());
        ret = pNode->Start();
        mListNodeToStart.pop_front();
        mListNodeStarted.push_front(pNode);
        if (ret != RESULT_SUCCESS)
        {
            IMLOGE2("[startNodes] error start node[%s], ret[%d]", pNode->GetNodeName(), ret);
            return ret;
        }
    }

    mScheduler->Start();
    return RESULT_SUCCESS;
}

ImsMediaResult BaseStreamGraph::stopNodes()
{
    BaseNode* pNode;
    mScheduler->Stop();
    std::list<BaseNode*>::iterator iter;
    while (mListNodeStarted.size() > 0)
    {
        pNode = mListNodeStarted.front();
        IMLOGD1("[stopNodes] stop node[%s]", pNode->GetNodeName());
        pNode->Stop();
        mListNodeStarted.pop_front();
        mListNodeToStart.push_front(pNode);
    }
    return RESULT_SUCCESS;
}

void BaseStreamGraph::deleteNodes()
{
    BaseNode* pNode = NULL;

    if (mGraphState != StreamState::kStreamStateCreated)
    {
        stop();
    }

    if (mListNodeStarted.size() > 0)
    {
        IMLOGE1("[deleteNodes] error node remained[%d]", mListNodeStarted.size());
    }

    while (mListNodeToStart.size() > 0)
    {
        pNode = mListNodeToStart.front();
        IMLOGD1("[deleteNodes] delete node[%s]", pNode->GetNodeName());
        RemoveNode(pNode);
        mListNodeToStart.pop_front();
    }
}

void BaseStreamGraph::setMediaQualityThreshold(MediaQualityThreshold* threshold)
{
    (void)threshold;
    IMLOGW0("[setMediaQualityThreshold] base");
    // base implementation
}

void BaseStreamGraph::OnEvent(int32_t type, uint64_t param1, uint64_t param2)
{
    (void)type;
    (void)param1;
    (void)param2;
    IMLOGW0("[OnEvent] base");
}