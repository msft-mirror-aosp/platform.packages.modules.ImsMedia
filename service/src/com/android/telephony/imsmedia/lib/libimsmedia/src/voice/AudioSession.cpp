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

#include <AudioSession.h>
#include <ImsMediaTrace.h>

AudioSession::AudioSession() {
}

AudioSession::~AudioSession() {
}

ImsMediaResult AudioSession::startGraph(RtpConfig* config) {
    IMLOGD0("[startGraph]");
    bool isSameConfigExist = false;
    ImsMediaResult ret;
    for (std::list<AudioStreamGraphRtpTx*>::iterator
        iter = mListGraphRtpTx.begin(); iter != mListGraphRtpTx.end(); iter++) {
        AudioStreamGraphRtpTx* graph = *iter;
        if (graph->isSameConfig(config)) {
            //ret = graph->updateGraph(config);
            isSameConfigExist = true;
            break;
        }
    }

    if (isSameConfigExist == false) {
        mListGraphRtpTx.push_back(new AudioStreamGraphRtpTx(this, mRtpFd));
        ret = mListGraphRtpTx.back()->createGraph(config);
        if (ret == IMS_MEDIA_OK) {
            ret = mListGraphRtpTx.back()->startGraph();
            if (ret != IMS_MEDIA_OK) {
                IMLOGE1("[startGraph] error[%d]", ret);
            }
        }
    }

    IMLOGD1("[startGraph] mListGraphRtpTx size[%d]", mListGraphRtpTx.size());

    isSameConfigExist = false;
    for (std::list<AudioStreamGraphRtpRx*>::iterator
        iter = mListGraphRtpRx.begin(); iter != mListGraphRtpRx.end(); iter++) {
        AudioStreamGraphRtpRx* graph = *iter;
        if (graph->isSameConfig(config)) {
            //ret = graph->updateGraph(config);
            isSameConfigExist = true;
            break;
        }
    }

    if (isSameConfigExist == false) {
        mListGraphRtpRx.push_back(new AudioStreamGraphRtpRx(this, mRtpFd));
        ret = mListGraphRtpRx.back()->createGraph(config);
        if (ret == IMS_MEDIA_OK) {
            ret = mListGraphRtpRx.back()->startGraph();
            if (ret != IMS_MEDIA_OK) {
                IMLOGE1("[startGraph] error[%d]", ret);
            }
        }
    }

    IMLOGD1("[startGraph] mListGraphRtpRx size[%d]", mListGraphRtpRx.size());

    isSameConfigExist = false;
    for (std::list<AudioStreamGraphRtcp*>::iterator
        iter = mListGraphRtcp.begin(); iter != mListGraphRtcp.end(); iter++) {
        AudioStreamGraphRtcp* graph = *iter;
        if (graph->isSameConfig(config)) {
            //ret = graph->updateGraph(config);
            isSameConfigExist = true;
            break;
        }
    }

    if (isSameConfigExist == false) {
        mListGraphRtcp.push_back(new AudioStreamGraphRtcp(this, mRtcpFd));
        ret = mListGraphRtcp.back()->createGraph(config);
        if (ret == IMS_MEDIA_OK) {
            mListGraphRtcp.back()->startGraph();
        }
    }

    IMLOGD1("[startGraph] mListGraphRtcp size[%d]", mListGraphRtcp.size());

    return ret;
}

ImsMediaResult AudioSession::addGraph(RtpConfig* config) {
    //stop rtp tx/rx and start rtcp
    //create graph for new config
    (void)config;
    return IMS_MEDIA_OK;
}

ImsMediaResult AudioSession::confirmGraph(RtpConfig* config) {
    //stop rtp tx/rx/rtcp and delete other graph
    (void)config;
    return IMS_MEDIA_OK;
}

ImsMediaResult AudioSession::deleteGraph(RtpConfig* config) {
    (void)config;
    IMLOGD0("[deleteGraph]");
    for (std::list<AudioStreamGraphRtpTx*>::iterator
        iter = mListGraphRtpTx.begin(); iter != mListGraphRtpTx.end(); iter++) {
        AudioStreamGraphRtpTx* graph = *iter;
        //test
        if (true) { //(graph->isSameConfig(config)) {
            if (graph->getState() == STATE_RUN) {
                graph->stopGraph();
            }
            mListGraphRtpTx.erase(iter);
            //break;
        }
    }

    IMLOGD1("[deleteGraph] mListGraphRtpTx size[%d]", mListGraphRtpTx.size());

    for (std::list<AudioStreamGraphRtpRx*>::iterator iter =
        mListGraphRtpRx.begin(); iter != mListGraphRtpRx.end(); iter++) {
        AudioStreamGraphRtpRx* graph = *iter;
        //test
        if (true) { //(graph->isSameConfig(config)) {
            if (graph->getState() == STATE_RUN) {
                graph->stopGraph();
            }
            mListGraphRtpRx.erase(iter);
            //break;
        }
    }

    IMLOGD1("[deleteGraph] mListGraphRtpRx size[%d]", mListGraphRtpRx.size());

    for (std::list<AudioStreamGraphRtcp*>::iterator iter =
        mListGraphRtcp.begin(); iter != mListGraphRtcp.end(); iter++) {
        AudioStreamGraphRtcp* graph = *iter;
        //test
        if (true) { //(graph->isSameConfig(config)) {
            if (graph->getState() == STATE_RUN) {
                graph->stopGraph();
            }
            mListGraphRtcp.erase(iter);
            //break;
        }
    }

    IMLOGD1("[deleteGraph] mListGraphRtcp size[%d]", mListGraphRtcp.size());

    return IMS_MEDIA_OK;
}

void AudioSession::onEvent(ImsMediaEventType type, uint64_t param1, uint64_t param2) {
    //opreate event
    (void)type;
    (void)param1;
    (void)param2;
}

void AudioSession::startDtmf(char digit, int volume, int duration) {
    (void)digit;
    (void)volume;
    (void)duration;
}

void AudioSession::stopDtmf() {

}