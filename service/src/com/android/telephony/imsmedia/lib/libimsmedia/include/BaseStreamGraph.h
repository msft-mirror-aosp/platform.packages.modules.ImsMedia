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

#ifndef BASE_SESSION_LISTENER_H
#define BASE_SESSION_LISTENER_H

#include <ImsMediaDefine.h>
#include <StreamScheduler.h>
#include <BaseNode.h>
#include <BaseSessionCallback.h>
#include <MediaQualityThreshold.h>
#include <list>

/**
 * @class BaseStreamGraph
 */
class BaseStreamGraph {
protected:
    virtual ImsMediaResult createGraph(void* config) = 0;
    virtual ImsMediaResult updateGraph(void* config) = 0;
    virtual void AddNode(BaseNode* pNode, bool bReverse = true);
    virtual void RemoveNode(BaseNode* pNode);
    virtual ImsMediaResult startNodes();
    virtual ImsMediaResult stopNodes();

public:
    BaseStreamGraph(BaseSessionCallback* callback, int localFd = 0);
    virtual ~BaseStreamGraph();
    void setLocalFd(int localFd) {
        mLocalFd = localFd;
    }
    int getLocalFd() {
        return mLocalFd;
    }
    virtual ImsMediaResult startGraph();
    virtual ImsMediaResult stopGraph();
    void setState(StreamState state) {
        mGraphState = state;
    }
    StreamState getState() {
        return mGraphState;
    }
    virtual bool isSameConfig(RtpConfig* config) = 0;
    virtual void setMediaQualityThreshold(const MediaQualityThreshold& threshold) {
        mThreshold = threshold;
    }

protected:
    BaseSessionCallback* mCallback;
    int mLocalFd;
    StreamState mGraphState;
    std::list<BaseNode*> mListNodeToStart;
    std::list<BaseNode*> mListNodeStarted;
    MediaQualityThreshold mThreshold;
    std::unique_ptr<StreamScheduler> mScheduler;
};

#endif