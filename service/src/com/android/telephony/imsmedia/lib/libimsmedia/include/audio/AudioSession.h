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

#ifndef AUDIO_SESSION_H
#define AUDIO_SESSION_H

#include <ImsMediaDefine.h>
#include <BaseSession.h>
#include <AudioStreamGraphRtpTx.h>
#include <AudioStreamGraphRtpRx.h>
#include <AudioStreamGraphRtcp.h>
#include <RtpConfig.h>
#include <list>

class AudioSession : public BaseSession
{
public:
    AudioSession();
    virtual ~AudioSession();
    virtual SessionState getState();
    virtual ImsMediaResult startGraph(void* config);
    // BaseSessionCallback
    virtual void onEvent(int32_t type, uint64_t param1, uint64_t param2);
    // dtmf method
    /**
     * @brief Add and start stream graph instance of the session. It has to be called only to
     *        create new StreamGraph should be added with different RtpConfig as a argument.
     *
     * @param config The parameters to operate nodes in the StreamGraph.
     * @return ImsMediaResult result of create or start graph. If the result has no error, it
     *         returns RESULT_SUCCESS. check #ImsMediaDefine.h.
     */
    ImsMediaResult addGraph(RtpConfig* config);
    /**
     * @brief Determine to remain only one StreamGraph instance and remove other StreamGraph.
     *        If the target StreamGraph is not in RUN state, call start instance to change to
     *        RUN state. when the call session is converted to confirmed session. It has to be
     *        called with proper RtpConfig argument that can choose the StreamGraph with the
     *        config. If there is no matched StreamGraph with same RtpConfig, it returns failure
     *        of RESULT_INVALID_PARAM.
     *
     * @param config The parameters to operate nodes in the StreamGraph.
     * @return ImsMediaResult result of create or start graph. If the result has no error, it
     *         returns RESULT_SUCCESS. check #ImsMediaDefine.h.
     */
    ImsMediaResult confirmGraph(RtpConfig* config);
    /**
     * @brief Delete a StreamGraph which has a matched RtpConfig argument.
     *
     * @param config A parameter to find the matching StreamGraph instance.
     * @return ImsMediaResult A result of deleting StreamGraph instance. If the result has
     *         no error, it returns RESULT_SUCCESS. check #ImsMediaDefine.h.
     */
    ImsMediaResult deleteGraph(RtpConfig* config);
    void sendDtmf(char digit, int duration);

private:
    std::list<AudioStreamGraphRtpTx*> mListGraphRtpTx;
    std::list<AudioStreamGraphRtpRx*> mListGraphRtpRx;
    std::list<AudioStreamGraphRtcp*> mListGraphRtcp;
};

#endif