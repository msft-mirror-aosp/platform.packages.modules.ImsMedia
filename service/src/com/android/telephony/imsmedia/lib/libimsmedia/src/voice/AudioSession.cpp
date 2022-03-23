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
#include <ImsMediaEventHandler.h>
#include <string>

AudioSession::AudioSession() {
    IMLOGD0("[AudioSession]");
}

AudioSession::~AudioSession() {
    IMLOGD0("[~AudioSession]");
    while (mListGraphRtpTx.size() > 0) {
        AudioStreamGraphRtpTx* graph = mListGraphRtpTx.front();
        if (graph->getState() == STATE_RUN) {
            graph->stop();
        }
        mListGraphRtpTx.pop_front();
        delete graph;
    }
    while (mListGraphRtpRx.size() > 0) {
        AudioStreamGraphRtpRx* graph = mListGraphRtpRx.front();
        if (graph->getState() == STATE_RUN) {
            graph->stop();
        }
        mListGraphRtpRx.pop_front();
        delete graph;
    }
    while (mListGraphRtcp.size() > 0) {
        AudioStreamGraphRtcp* graph = mListGraphRtcp.front();
        if (graph->getState() == STATE_RUN) {
            graph->stop();
        }
        mListGraphRtcp.pop_front();
        delete graph;
    }
}

ImsMediaResult AudioSession::startGraph(RtpConfig* config) {
    IMLOGD0("[startGraph]");
    if (config == NULL || std::strcmp(config->getRemoteAddress().c_str(), "") == 0) {
        return IMS_MEDIA_ERROR_INVALID_ARGUMENT;
    }

    ImsMediaResult ret = IMS_MEDIA_ERROR_UNKNOWN;
    IMLOGD1("[startGraph] mListGraphRtpTx size[%d]", mListGraphRtpTx.size());

    if (mListGraphRtpTx.size() != 0) {
        ret = mListGraphRtpTx.front()->update(config);
        if (ret != IMS_MEDIA_OK) {
            IMLOGE1("[startGraph] update error[%d]", ret);
            return ret;
        }
    } else {
        mListGraphRtpTx.push_back(new AudioStreamGraphRtpTx(this, mRtpFd));
        ret = mListGraphRtpTx.back()->create(config);
        if (ret == IMS_MEDIA_OK) {
            ret = mListGraphRtpTx.back()->start();
            if (ret != IMS_MEDIA_OK) {
                IMLOGE1("[startGraph] start error[%d]", ret);
                return ret;
            }
        }
    }

    IMLOGD1("[startGraph] mListGraphRtpRx size[%d]", mListGraphRtpRx.size());

    if (mListGraphRtpRx.size() != 0) {
        ret = mListGraphRtpRx.front()->update(config);
        if (ret != IMS_MEDIA_OK) {
            IMLOGE1("[startGraph] update error[%d]", ret);
            return ret;
        }
    } else {
        mListGraphRtpRx.push_back(new AudioStreamGraphRtpRx(this, mRtpFd));
        ret = mListGraphRtpRx.back()->create(config);
        if (ret == IMS_MEDIA_OK) {
            ret = mListGraphRtpRx.back()->start();
            if (ret != IMS_MEDIA_OK) {
                IMLOGE1("[startGraph] start error[%d]", ret);
                return ret;
            }
        }
    }

    IMLOGD1("[startGraph] mListGraphRtcp size[%d]", mListGraphRtcp.size());

    if (mListGraphRtcp.size() != 0) {
        ret = mListGraphRtcp.front()->update(config);
        if (ret != IMS_MEDIA_OK) {
            IMLOGE1("[startGraph] update error[%d]", ret);
            return ret;
        }
    } else {
        mListGraphRtcp.push_back(new AudioStreamGraphRtcp(this, mRtcpFd));
        ret = mListGraphRtcp.back()->create(config);
        if (ret == IMS_MEDIA_OK) {
            ret = mListGraphRtcp.back()->start();
            if (ret != IMS_MEDIA_OK) {
                IMLOGE1("[startGraph] start error[%d]", ret);
                return ret;
            }
        }
    }

    return ret;
}

ImsMediaResult AudioSession::addGraph(RtpConfig* config) {
    if (config == NULL || std::strcmp(config->getRemoteAddress().c_str(), "") == 0) {
        return IMS_MEDIA_ERROR_INVALID_ARGUMENT;
    }

    for (auto&i : mListGraphRtpTx) {
        if (i->isSameConfig(config)) {
            IMLOGE0("[addGraph] same config is exist");
            return IMS_MEDIA_ERROR_INVALID_ARGUMENT;
        }
    }

    for (auto&i : mListGraphRtpTx) {
        i->stop();
    }

    for (auto&i : mListGraphRtpRx) {
        i->stop();
    }

    for (auto&i : mListGraphRtcp) {
        if (i->getState() != STATE_RUN) {
            i->start();
        }
    }

    ImsMediaResult ret = IMS_MEDIA_ERROR_UNKNOWN;

    mListGraphRtpTx.push_back(new AudioStreamGraphRtpTx(this, mRtpFd));
    ret = mListGraphRtpTx.back()->create(config);
    if (ret == IMS_MEDIA_OK) {
        ret = mListGraphRtpTx.back()->start();
        if (ret != IMS_MEDIA_OK) {
            IMLOGE1("[addGraph] start error[%d]", ret);
            return ret;
        }
    }

    IMLOGD1("[addGraph] mListGraphTx size[%d]", mListGraphRtpTx.size());

    mListGraphRtpRx.push_back(new AudioStreamGraphRtpRx(this, mRtpFd));
    ret = mListGraphRtpRx.back()->create(config);
    if (ret == IMS_MEDIA_OK) {
        ret = mListGraphRtpRx.back()->start();
        if (ret != IMS_MEDIA_OK) {
            IMLOGE1("[addGraph] start error[%d]", ret);
            return ret;
        }
    }

    IMLOGD1("[addGraph] mListGraphRx size[%d]", mListGraphRtpRx.size());

    mListGraphRtcp.push_back(new AudioStreamGraphRtcp(this, mRtcpFd));
    ret = mListGraphRtcp.back()->create(config);
    if (ret == IMS_MEDIA_OK) {
        ret = mListGraphRtcp.back()->start();
        if (ret != IMS_MEDIA_OK) {
            IMLOGE1("[addGraph] start error[%d]", ret);
            return ret;
        }
    }

    IMLOGD1("[addGraph] mListGraphRtcp size[%d]", mListGraphRtcp.size());

    return IMS_MEDIA_OK;
}

ImsMediaResult AudioSession::confirmGraph(RtpConfig* config) {
    if (config == NULL || std::strcmp(config->getRemoteAddress().c_str(), "") == 0) {
        return IMS_MEDIA_ERROR_INVALID_ARGUMENT;
    }

    ImsMediaResult ret = IMS_MEDIA_ERROR_UNKNOWN;

    /** Stop unmatched running instances of StreamGraph. */
    for (auto&i : mListGraphRtpTx) {
        if (!i->isSameConfig(config)) {
            i->stop();
        }
    }

    for (auto&i : mListGraphRtpRx) {
        if (!i->isSameConfig(config)) {
            i->stop();
        }
    }

    for (auto&i : mListGraphRtcp) {
        if (!i->isSameConfig(config)) {
            i->stop();
        }
    }

    bool bFound = false;
    for (std::list<AudioStreamGraphRtpTx*>::iterator
        iter = mListGraphRtpTx.begin(); iter != mListGraphRtpTx.end();) {
        AudioStreamGraphRtpTx* graph = *iter;
        if (!graph->isSameConfig(config)) {
            iter = mListGraphRtpTx.erase(iter);
            delete graph;
        } else {
            if (graph->getState() != STATE_RUN) {
                ret = graph->start();
                if (ret != IMS_MEDIA_OK) {
                    IMLOGE1("[confirmGraph] start tx error[%d]", ret);
                    return ret;
                }
            }
            iter++;
            bFound = true;
        }
    }

    IMLOGD1("[confirmGraph] mListGraphTx size[%d]", mListGraphRtpTx.size());

    if (bFound == false) {
        IMLOGE0("[confirmGraph] no graph to confirm");
        return IMS_MEDIA_ERROR_INVALID_ARGUMENT;
    }

    for (std::list<AudioStreamGraphRtpRx*>::iterator
        iter = mListGraphRtpRx.begin(); iter != mListGraphRtpRx.end();) {
        AudioStreamGraphRtpRx* graph = *iter;
        if (!graph->isSameConfig(config)) {
            iter = mListGraphRtpRx.erase(iter);
            delete graph;
        } else {
            if (graph->getState() != STATE_RUN) {
                ret = graph->start();
                if (ret != IMS_MEDIA_OK) {
                    IMLOGE1("[confirmGraph] start rx error[%d]", ret);
                    return ret;
                }
            }
            iter++;
        }
    }

    IMLOGD1("[confirmGraph] mListGraphRx size[%d]", mListGraphRtpRx.size());

    for (std::list<AudioStreamGraphRtcp*>::iterator
        iter = mListGraphRtcp.begin(); iter != mListGraphRtcp.end();) {
        AudioStreamGraphRtcp* graph = *iter;
        if (!graph->isSameConfig(config)) {
            iter = mListGraphRtcp.erase(iter);
            delete graph;
        } else {
            if (graph->getState() != STATE_RUN) {
                ret = graph->start();
                if (ret != IMS_MEDIA_OK) {
                    IMLOGE1("[confirmGraph] start rtcp error[%d]", ret);
                    return ret;
                }
            }
            iter++;
        }
    }

    IMLOGD1("[confirmGraph] mListGraphRtcp size[%d]", mListGraphRtcp.size());

    return IMS_MEDIA_OK;
}

ImsMediaResult AudioSession::deleteGraph(RtpConfig* config) {
    IMLOGD0("[deleteGraph]");
    bool bFound = false;
    for (std::list<AudioStreamGraphRtpTx*>::iterator
        iter = mListGraphRtpTx.begin(); iter != mListGraphRtpTx.end();) {
        AudioStreamGraphRtpTx* graph = *iter;
        if (graph->isSameConfig(config)) {
            if (graph->getState() == STATE_RUN) {
                graph->stop();
            }
            iter = mListGraphRtpTx.erase(iter);
            delete graph;
            bFound = true;
            break;
        } else {
            iter++;
        }
    }

    if (bFound == false) {
        return IMS_MEDIA_ERROR_INVALID_ARGUMENT;
    }

    IMLOGD1("[deleteGraph] mListGraphRtpTx size[%d]", mListGraphRtpTx.size());

    for (std::list<AudioStreamGraphRtpRx*>::iterator iter =
        mListGraphRtpRx.begin(); iter != mListGraphRtpRx.end();) {
        AudioStreamGraphRtpRx* graph = *iter;
        if (graph->isSameConfig(config)) {
            if (graph->getState() == STATE_RUN) {
                graph->stop();
            }
            iter = mListGraphRtpRx.erase(iter);
            delete graph;
            break;
        } else {
            iter++;
        }
    }

    IMLOGD1("[deleteGraph] mListGraphRtpRx size[%d]", mListGraphRtpRx.size());

    for (std::list<AudioStreamGraphRtcp*>::iterator iter =
        mListGraphRtcp.begin(); iter != mListGraphRtcp.end();) {
        AudioStreamGraphRtcp* graph = *iter;
        if (graph->isSameConfig(config)) {
            if (graph->getState() == STATE_RUN) {
                graph->stop();
            }
            iter = mListGraphRtcp.erase(iter);
            delete graph;
            break;
        } else {
            iter++;
        }
    }

    IMLOGD1("[deleteGraph] mListGraphRtcp size[%d]", mListGraphRtcp.size());
    return IMS_MEDIA_OK;
}

void AudioSession::setMediaQualityThreshold(MediaQualityThreshold* threshold) {
    IMLOGD0("[setMediaQualityThreshold]");
    for (std::list<AudioStreamGraphRtpRx*>::iterator iter =
        mListGraphRtpRx.begin(); iter != mListGraphRtpRx.end(); iter++) {
        AudioStreamGraphRtpRx* graph = *iter;
        if (graph != NULL) {
            graph->setMediaQualityThreshold(*threshold);
        }
    }

    for (std::list<AudioStreamGraphRtcp*>::iterator iter =
        mListGraphRtcp.begin(); iter != mListGraphRtcp.end(); iter++) {
        AudioStreamGraphRtcp* graph = *iter;
        if (graph != NULL) {
            graph->setMediaQualityThreshold(*threshold);
        }
    }
}

void AudioSession::onEvent(ImsMediaEventType type, uint64_t param1, uint64_t param2) {
    IMLOGD3("[onEvent] type[%d], param1[%d], param2[%d]", type, param1, param2);
    switch (type) {
        case EVENT_NOTIFY_ERROR:
            break;
        case EVENT_NOTIFY_FIRST_MEDIA_PACKET_RECEIVED:
            ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT",
                FIRST_MEDIA_PACKET_IND, 0, 0);
            break;
        case EVENT_NOTIFY_HEADER_EXTENSION_RECEIVED:
            ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT",
                RTP_HEADER_EXTENSION_IND, 0, 0);
            break;
        case EVENT_NOTIFY_MEDIA_INACITIVITY:
            ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT",
                MEDIA_INACITIVITY_IND, param1, param2);
            break;
        case EVENT_NOTIFY_PACKET_LOSS:
            ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT",
                PACKET_LOSS_IND, param1, 0);
            break;
        case EVENT_NOTIFY_JITTER:
            ImsMediaEventHandler::SendEvent("VOICE_RESPONSE_EVENT",
                JITTER_IND, param1, param2);
            break;
        default:
            break;
    }
}

void AudioSession::startDtmf(char digit, int volume, int duration) {
    for (std::list<AudioStreamGraphRtpTx*>::iterator
        iter = mListGraphRtpTx.begin(); iter != mListGraphRtpTx.end(); iter++) {
        AudioStreamGraphRtpTx* graph = *iter;
        if (graph->getState() == STATE_RUN) {
            graph->startDtmf(digit, volume, duration);
        }
    }
}

void AudioSession::stopDtmf() {
    for (std::list<AudioStreamGraphRtpTx*>::iterator
        iter = mListGraphRtpTx.begin(); iter != mListGraphRtpTx.end(); iter++) {
        AudioStreamGraphRtpTx* graph = *iter;
        if (graph->getState() == STATE_RUN) {
            graph->stopDtmf();
        }
    }
}