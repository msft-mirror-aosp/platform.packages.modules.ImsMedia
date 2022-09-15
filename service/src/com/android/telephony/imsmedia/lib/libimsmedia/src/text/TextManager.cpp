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

#include <TextManager.h>
#include <ImsMediaTrace.h>
#include <ImsMediaNetworkUtil.h>

using namespace android;
TextManager* TextManager::manager;

TextManager::TextManager() {}

TextManager::~TextManager() {}

TextManager* TextManager::getInstance()
{
    if (manager == NULL)
    {
        manager = new TextManager();
    }

    return manager;
}

int TextManager::getState(int sessionId)
{
    auto session = mSessions.find(sessionId);

    if (session != mSessions.end())
    {
        return (session->second)->getState();
    }
    else
    {
        return kSessionStateClosed;
    }
}

ImsMediaResult TextManager::openSession(
        const int sessionId, const int rtpFd, const int rtcpFd, TextConfig* config)
{
    IMLOGD1("[openSession] sessionId[%d]", sessionId);

    // set debug log
    ImsMediaTrace::IMSetDebugLog(ImsMediaTrace::IMGetDebugLog() | IM_PACKET_LOG_TEXT);

    if (rtpFd == -1 || rtcpFd == -1)
    {
        return RESULT_INVALID_PARAM;
    }

    if (!mSessions.count(sessionId))
    {
        TextSession* session = new TextSession();
        session->setSessionId(sessionId);
        session->setLocalEndPoint(rtpFd, rtcpFd);
        mSessions.insert(std::make_pair(sessionId, std::move(session)));
        ImsMediaResult ret = session->startGraph(config);

        if (ret != RESULT_SUCCESS)
        {
            IMLOGD1("[openSession] startGraph failed[%d]", ret);
        }
    }
    else
    {
        return RESULT_INVALID_PARAM;
    }

    return RESULT_SUCCESS;
}

ImsMediaResult TextManager::closeSession(const int sessionId)
{
    IMLOGD1("closeSession() - sessionId[%d]", sessionId);

    if (mSessions.count(sessionId))
    {
        mSessions.erase(sessionId);
        return RESULT_SUCCESS;
    }

    return RESULT_INVALID_PARAM;
}

ImsMediaResult TextManager::modifySession(const int sessionId, TextConfig* config)
{
    auto session = mSessions.find(sessionId);
    IMLOGD1("modifySession() - sessionId[%d]", sessionId);

    if (session != mSessions.end())
    {
        return (session->second)->startGraph(config);
    }
    else
    {
        IMLOGE1("modifySession() - no session id[%d]", sessionId);
        return RESULT_INVALID_PARAM;
    }
}

void TextManager::setMediaQualityThreshold(const int sessionId, MediaQualityThreshold* threshold)
{
    auto session = mSessions.find(sessionId);
    IMLOGD1("setMediaQualityThreshold() - sessionId[%d]", sessionId);

    if (session != mSessions.end())
    {
        (session->second)->setMediaQualityThreshold(*threshold);
    }
    else
    {
        IMLOGE1("setMediaQualityThreshold() - no session id[%d]", sessionId);
    }
}

ImsMediaResult TextManager::sendRtt(const int sessionId, const android::String8* text)
{
    auto session = mSessions.find(sessionId);
    IMLOGD1("sendRtt() - sessionId[%d]", sessionId);

    if (session != mSessions.end())
    {
        return (session->second)->sendRtt(text);
    }

    return RESULT_INVALID_PARAM;
}

void TextManager::sendMessage(const int sessionId, const android::Parcel& parcel)
{
    int nMsg = parcel.readInt32();
    status_t err = NO_ERROR;

    switch (nMsg)
    {
        case kTextOpenSession:
        {
            int rtpFd = parcel.readInt32();
            int rtcpFd = parcel.readInt32();
            TextConfig* config = new TextConfig();
            err = config->readFromParcel(&parcel);

            if (err != NO_ERROR && err != -ENODATA)
            {
                IMLOGE1("sendMessage() - error readFromParcel[%d]", err);
            }

            EventParamOpenSession* param = new EventParamOpenSession(rtpFd, rtcpFd, config);
            ImsMediaEventHandler::SendEvent(
                    "TEXT_REQUEST_EVENT", nMsg, sessionId, reinterpret_cast<uint64_t>(param));
        }
        break;
        case kTextCloseSession:
            ImsMediaEventHandler::SendEvent("TEXT_REQUEST_EVENT", nMsg, sessionId);
            break;
        case kTextModifySession:
        {
            TextConfig* config = new TextConfig();
            config->readFromParcel(&parcel);

            if (err != NO_ERROR)
            {
                IMLOGE1("sendMessage() - error readFromParcel[%d]", err);
            }

            ImsMediaEventHandler::SendEvent(
                    "TEXT_REQUEST_EVENT", nMsg, sessionId, reinterpret_cast<uint64_t>(config));
        }
        break;
        case kTextSetMediaQualityThreshold:
        {
            MediaQualityThreshold* threshold = new MediaQualityThreshold();
            threshold->readFromParcel(&parcel);
            ImsMediaEventHandler::SendEvent(
                    "TEXT_REQUEST_EVENT", nMsg, sessionId, reinterpret_cast<uint64_t>(threshold));
        }
        break;
        case kTextSendRtt:
        {
            android::String16 text;
            parcel.readString16(&text);
            android::String8* rttText = new String8(text.string());
            ImsMediaEventHandler::SendEvent(
                    "TEXT_REQUEST_EVENT", nMsg, sessionId, reinterpret_cast<uint64_t>(rttText));
        }
        break;
        default:
            break;
    }
}

TextManager::RequestHandler::RequestHandler() :
        ImsMediaEventHandler("TEXT_REQUEST_EVENT")
{
}

TextManager::RequestHandler::~RequestHandler() {}

void TextManager::RequestHandler::processEvent(
        uint32_t event, uint64_t sessionId, uint64_t paramA, uint64_t paramB)
{
    IMLOGD4("[processEvent] event[%d], sessionId[%d], paramA[%d], paramB[%d]", event, sessionId,
            paramA, paramB);
    ImsMediaResult result = RESULT_SUCCESS;

    switch (event)
    {
        case kTextOpenSession:
        {
            EventParamOpenSession* param = reinterpret_cast<EventParamOpenSession*>(paramA);
            if (param != NULL)
            {
                TextConfig* pConfig = reinterpret_cast<TextConfig*>(param->mConfig);
                result = TextManager::getInstance()->openSession(
                        static_cast<int>(sessionId), param->rtpFd, param->rtcpFd, pConfig);

                if (result == RESULT_SUCCESS)
                {
                    ImsMediaEventHandler::SendEvent(
                            "TEXT_RESPONSE_EVENT", kTextOpenSessionSuccess, sessionId);
                }
                else
                {
                    ImsMediaEventHandler::SendEvent(
                            "TEXT_RESPONSE_EVENT", kTextOpenSessionFailure, sessionId, result);
                }

                delete param;

                if (pConfig != NULL)
                {
                    delete pConfig;
                }
            }
            else
            {
                ImsMediaEventHandler::SendEvent("TEXT_RESPONSE_EVENT", kTextOpenSessionFailure,
                        sessionId, RESULT_INVALID_PARAM);
            }
        }
        break;
        case kTextCloseSession:
            TextManager::getInstance()->closeSession(static_cast<int>(sessionId));
            ImsMediaEventHandler::SendEvent(
                    "TEXT_RESPONSE_EVENT", kTextSessionClosed, sessionId, 0, 0);
            break;
        case kTextModifySession:
        {
            TextConfig* config = reinterpret_cast<TextConfig*>(paramA);
            result = TextManager::getInstance()->modifySession(static_cast<int>(sessionId), config);
            ImsMediaEventHandler::SendEvent(
                    "TEXT_RESPONSE_EVENT", kTextModifySessionResponse, sessionId, result, paramA);
        }
        break;
        case kTextSetMediaQualityThreshold:
        {
            MediaQualityThreshold* threshold = reinterpret_cast<MediaQualityThreshold*>(paramA);

            if (threshold != NULL)
            {
                TextManager::getInstance()->setMediaQualityThreshold(
                        static_cast<int>(sessionId), threshold);
                delete threshold;
            }
        }
        break;
        case kTextSendRtt:
        {
            android::String8* text = reinterpret_cast<android::String8*>(paramA);

            if (text != NULL)
            {
                TextManager::getInstance()->sendRtt(static_cast<int>(sessionId), text);
                delete text;
            }
        }
        break;
        default:
            break;
    }
}

TextManager::ResponseHandler::ResponseHandler() :
        ImsMediaEventHandler("TEXT_RESPONSE_EVENT")
{
}

TextManager::ResponseHandler::~ResponseHandler() {}

void TextManager::ResponseHandler::processEvent(
        uint32_t event, uint64_t sessionId, uint64_t paramA, uint64_t paramB)
{
    IMLOGD4("[processEvent] event[%d], sessionId[%d], paramA[%d], paramB[%d]", event, sessionId,
            paramA, paramB);
    android::Parcel parcel;
    switch (event)
    {
        case kTextOpenSessionSuccess:
        case kTextOpenSessionFailure:
            parcel.writeInt32(event);
            parcel.writeInt32(static_cast<int>(sessionId));

            if (event == kTextOpenSessionFailure)
            {
                // fail reason
                parcel.writeInt32(static_cast<int>(paramA));
            }

            TextManager::getInstance()->sendResponse(
                    reinterpret_cast<uint64_t>(TextManager::getInstance()), parcel);
            break;
        case kTextModifySessionResponse:  // fall through
        {
            parcel.writeInt32(event);
            parcel.writeInt32(paramA);  // result
            TextConfig* config = reinterpret_cast<TextConfig*>(paramB);

            if (config != NULL)
            {
                config->writeToParcel(&parcel);
                TextManager::getInstance()->sendResponse(
                        reinterpret_cast<uint64_t>(TextManager::getInstance()), parcel);
                delete config;
            }
        }
        break;
        case kTextSessionChangedInd:
            parcel.writeInt32(event);
            parcel.writeInt32(static_cast<int>(sessionId));
            parcel.writeInt32(paramA);  // state
            TextManager::getInstance()->sendResponse(
                    reinterpret_cast<uint64_t>(TextManager::getInstance()), parcel);
            break;
        case kTextMediaInactivityInd:
            parcel.writeInt32(event);
            parcel.writeInt32(static_cast<int>(paramA));  // type
            parcel.writeInt32(static_cast<int>(paramB));  // duration
            TextManager::getInstance()->sendResponse(
                    reinterpret_cast<uint64_t>(TextManager::getInstance()), parcel);
            break;
        case kTextRttReceived:
        {
            parcel.writeInt32(event);
            android::String8* text = reinterpret_cast<String8*>(paramA);

            if (text != NULL)
            {
                String16 rttText(*text);
                parcel.writeString16(rttText);
                TextManager::getInstance()->sendResponse(
                        reinterpret_cast<uint64_t>(TextManager::getInstance()), parcel);
                delete text;
            }
        }
        break;
        case kTextSessionClosed:
            parcel.writeInt32(event);
            parcel.writeInt32(static_cast<int>(sessionId));
            TextManager::getInstance()->sendResponse(
                    reinterpret_cast<uint64_t>(TextManager::getInstance()), parcel);
            break;
        default:
            break;
    }
}
