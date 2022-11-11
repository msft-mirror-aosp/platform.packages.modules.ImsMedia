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
#include <JitterNetworkAnalyser.h>
#include <ImsMediaTimer.h>
#include <ImsMediaTrace.h>
#include <cmath>

#define PACKET_INTERVAL 20

#if defined(FEATURE_IMS_MEDIA_ATT)
#define BUFFER_REDUCE_TH         (1000 * 80)
#define STD_DISTRIBUTION_Z_VALUE 2.5
#define BUFFER_IN_DECREASE_SIZE  2
#elif defined(FEATURE_IMS_MEDIA_CMCC)
#define BUFFER_REDUCE_TH         (1000 * 240)
#define STD_DISTRIBUTION_Z_VALUE 2.4
#define BUFFER_IN_DECREASE_SIZE  1
#elif defined(FEATURE_IMS_MEDIA_JITTER_IMPROVEMENT_NOT_USED)
#define BUFFER_REDUCE_TH         (1000 * 20)
#define STD_DISTRIBUTION_Z_VALUE 2.326  // 99%, Target MOS : AMR-WB, 3.75
#define BUFFER_IN_DECREASE_SIZE  1
#else
#define BUFFER_REDUCE_TH         (1000 * 20)
#define STD_DISTRIBUTION_Z_VALUE 2.5
#define BUFFER_IN_DECREASE_SIZE  2
#endif
#define ANALYSE_PACKET_LOSS_RATE 500

JitterNetworkAnalyser::JitterNetworkAnalyser()
{
    mMinJitterBufferSize = 0;
    mMaxJitterBufferSize = 0;
    mBufferReduceTH = 1000 * 20;
    mBufferStepSize = 2;
    mBufferZValue = 2.5;
    mImprovement = false;
    Reset();
}

JitterNetworkAnalyser::~JitterNetworkAnalyser() {}

void JitterNetworkAnalyser::Reset()
{
    uint32_t i;
    mBasePacketTime = 0;
    mBaseArrivalTime = 0;
    mJitterIndex = 0;
    mNetworkStatus = NETWORK_STATUS_NORMAL;
    mGoodStatusEnteringTime = 0;
    mBadStatusChangedTime = 0;

    for (i = 0; i < JITTER_LIST_SIZE; i++)
    {
        mJitters[i] = 0;
    }
}

void JitterNetworkAnalyser::SetMinMaxJitterBufferSize(
        uint32_t nMinBufferSize, uint32_t nMaxBufferSize)
{
    mMinJitterBufferSize = nMinBufferSize;
    mMaxJitterBufferSize = nMaxBufferSize;
}

void JitterNetworkAnalyser::SetJitterOptions(
        uint32_t nReduceTH, uint32_t nStepSize, double zValue, bool bImprovement)
{
    mBufferReduceTH = nReduceTH;
    mBufferStepSize = nStepSize;
    mBufferZValue = zValue;
    mImprovement = bImprovement;

    IMLOGD4("[SetJitterOptions] ReduceTH[%d], StepSize[%d], ZValue[%.lf], bImprovement[%d]",
            mBufferReduceTH, mBufferStepSize, mBufferZValue, mImprovement);
}

int32_t JitterNetworkAnalyser::calculateTransitTimeDifference(
        uint32_t timestamp, uint32_t arrivalTime)
{
    if (mBasePacketTime == 0)
    {
        return 0;
    }

    uint32_t inputTimestampGap = timestamp - mBasePacketTime;
    uint32_t inputTimeGap = arrivalTime - mBaseArrivalTime;
    uint32_t jitter = inputTimeGap - inputTimestampGap;
    mJitters[mJitterIndex++] = jitter;

    if (mJitterIndex >= JITTER_LIST_SIZE)
    {
        mJitterIndex = 0;
    }

    return jitter;
}

double JitterNetworkAnalyser::DevCalc(double* pMean)
{
    uint32_t i;
    double dDerivation;
    double sum, mean, powersum;
    sum = 0;
    powersum = 0;

    for (i = 0; i < JITTER_LIST_SIZE; i++)
    {
        sum += mJitters[i];
    }

    mean = sum / JITTER_LIST_SIZE;

    for (i = 0; i < JITTER_LIST_SIZE; i++)
    {
        double jitter = (double)mJitters[i] - mean;
        powersum += (jitter * jitter);
    }

    dDerivation = sqrt(powersum / JITTER_LIST_SIZE);
    IMLOGD_PACKET1(IM_PACKET_LOG_JITTER, "[DevCalc] jitter deviation value %4.4f \n", dDerivation);
    *pMean = mean;
    return dDerivation;
}

uint32_t JitterNetworkAnalyser::GetMaxJitterValue()
{
    uint32_t i;
    uint32_t nMaxJitterValue = 0;

    for (i = 0; i < JITTER_LIST_SIZE; i++)
    {
        if (mJitters[i] > nMaxJitterValue)
            nMaxJitterValue = mJitters[i];
    }

    return (uint32_t)nMaxJitterValue;
}

void JitterNetworkAnalyser::BasePacketChange(uint32_t packetTime, uint32_t arrivalTime)
{
    IMLOGD_PACKET2(IM_PACKET_LOG_JITTER, "[BasePacketChange] packetTime[%d], arrivalTime[%u]",
            packetTime, arrivalTime);
    mBasePacketTime = packetTime;
    mBaseArrivalTime = arrivalTime;
}

uint32_t JitterNetworkAnalyser::GetJitterBufferSize(uint32_t nCurrJitterBufferSize)
{
    uint32_t nextJitterBuffer = nCurrJitterBufferSize;
    NETWORK_STATUS currStatus;

    if (mImprovement == false)
    {
        double dev, mean;
        // calcuatation of jitterSize
        double calcJitterSize = 0;
        uint32_t max_jitter = GetMaxJitterValue();
        uint32_t weight_value = 0;
        dev = DevCalc(&mean);

        // calcJitterSize = mean + STD_DISTRIBUTION_Z_VALUE * dev;
        calcJitterSize = mean + mBufferZValue * dev;

        IMLOGD_PACKET3(IM_PACKET_LOG_JITTER,
                "[GetJitterBufferSize] calcJitterSize %4.2f, curr[%d], MaxJitterSize %d",
                calcJitterSize, nCurrJitterBufferSize, max_jitter);

        if (calcJitterSize >= nCurrJitterBufferSize * PACKET_INTERVAL)
        {
            currStatus = NETWORK_STATUS_BAD;
            weight_value = (calcJitterSize - nCurrJitterBufferSize * PACKET_INTERVAL) / 20 + 1;

            if (weight_value >= 3)
            {
                weight_value += (weight_value + 1) / 2;
                IMLOGD1("[GetJitterBufferSize] bad network case - additional weight value[%d]",
                        weight_value);
            }
        }
        else if (calcJitterSize < ((nCurrJitterBufferSize - 1) * PACKET_INTERVAL - 10) &&
                max_jitter < ((nCurrJitterBufferSize - 1) * PACKET_INTERVAL - 10))
        {
            currStatus = NETWORK_STATUS_GOOD;
        }
        else
        {
            currStatus = NETWORK_STATUS_NORMAL;
        }

        switch (currStatus)
        {
            case NETWORK_STATUS_BAD:
            {
                uint32_t nCurrTime = ImsMediaTimer::GetTimeInMilliSeconds();

                if (mBadStatusChangedTime == 0 || (nCurrTime - mBadStatusChangedTime) >= 1000)
                {
                    nextJitterBuffer = nCurrJitterBufferSize + weight_value;
                    if (nextJitterBuffer >= mMaxJitterBufferSize)
                    {
                        nextJitterBuffer = mMaxJitterBufferSize;
                    }
                    IMLOGD_PACKET2(IM_PACKET_LOG_JITTER,
                            "[GetJitterBufferSize] Increase next[%d], curr[%d]", nextJitterBuffer,
                            nCurrJitterBufferSize);
                    mBadStatusChangedTime = nCurrTime;
                }

                break;
            }
            case NETWORK_STATUS_GOOD:
            {
                if (mNetworkStatus != NETWORK_STATUS_GOOD)
                {
                    mGoodStatusEnteringTime = ImsMediaTimer::GetTimeInMilliSeconds();
                }
                else
                {
                    uint32_t nTimeDiff =
                            ImsMediaTimer::GetTimeInMilliSeconds() - mGoodStatusEnteringTime;

                    if (nTimeDiff >= mBufferReduceTH)
                    {
                        if (nCurrJitterBufferSize > mMinJitterBufferSize)
                        {
                            nextJitterBuffer = nCurrJitterBufferSize - mBufferStepSize;
                        }

                        IMLOGD_PACKET2(IM_PACKET_LOG_JITTER,
                                "[GetJitterBufferSize] Decrease next[%d], curr[%d]",
                                nextJitterBuffer, nCurrJitterBufferSize);

                        currStatus = NETWORK_STATUS_NORMAL;  // To reset mGoodStatusEnteringTime
                    }
                }
                break;
            }
            default:
            {
                nextJitterBuffer = nCurrJitterBufferSize;
                IMLOGD_PACKET2(IM_PACKET_LOG_JITTER, "[GetJitterBufferSize] next[%d], curr[%d]",
                        nextJitterBuffer, nCurrJitterBufferSize);
            }
        }
    }
    else
    {
        double dev, mean;
        // calcuatation of jitterSize
        double calcJitterSize = 0;
        uint32_t max_jitter = GetMaxJitterValue();
        dev = DevCalc(&mean);
        // calcJitterSize = mean + STD_DISTRIBUTION_Z_VALUE * dev;
        calcJitterSize = mean + mBufferZValue * dev;
        IMLOGD_PACKET3(IM_PACKET_LOG_JITTER,
                "[GetJitterBufferSize] calcJitterSize %4.2f, curr[%d], MaxJitterSize %d",
                calcJitterSize, nCurrJitterBufferSize, max_jitter);

        if (calcJitterSize >= nCurrJitterBufferSize * PACKET_INTERVAL)
        {
            currStatus = NETWORK_STATUS_BAD;
        }
        else if (calcJitterSize < ((nCurrJitterBufferSize - 1) * PACKET_INTERVAL - 10) &&
                max_jitter < ((nCurrJitterBufferSize - 1) * PACKET_INTERVAL - 10))
        {
            currStatus = NETWORK_STATUS_GOOD;
        }
        else
        {
            currStatus = NETWORK_STATUS_NORMAL;
        }

        switch (currStatus)
        {
            case NETWORK_STATUS_BAD:
            {
                uint32_t nCurrTime = ImsMediaTimer::GetTimeInMilliSeconds();

                if (mBadStatusChangedTime == 0 || (nCurrTime - mBadStatusChangedTime) >= 1000)
                {
                    if (nCurrJitterBufferSize < mMaxJitterBufferSize)
                    {
                        nextJitterBuffer = nCurrJitterBufferSize + mBufferStepSize;
                    }
                    IMLOGD_PACKET2(IM_PACKET_LOG_JITTER,
                            "[GetJitterBufferSize] Increase next[%d], curr[%d]", nextJitterBuffer,
                            nCurrJitterBufferSize);
                    mBadStatusChangedTime = nCurrTime;
                }

                break;
            }
            case NETWORK_STATUS_GOOD:
            {
                if (mNetworkStatus != NETWORK_STATUS_GOOD)
                {
                    mGoodStatusEnteringTime = ImsMediaTimer::GetTimeInMilliSeconds();
                }
                else
                {
                    uint32_t nTimeDiff =
                            ImsMediaTimer::GetTimeInMilliSeconds() - mGoodStatusEnteringTime;
                    if (nTimeDiff >= mBufferReduceTH)
                    {
                        if (nCurrJitterBufferSize > mMinJitterBufferSize)
                            nextJitterBuffer = nCurrJitterBufferSize - mBufferStepSize;
                        IMLOGD_PACKET2(IM_PACKET_LOG_JITTER,
                                "[GetJitterBufferSize] Decrease next[%d], curr[%d]",
                                nextJitterBuffer, nCurrJitterBufferSize);
                        currStatus = NETWORK_STATUS_NORMAL;
                    }
                }

                break;
            }
            default:
            {
                nextJitterBuffer = nCurrJitterBufferSize;
                IMLOGD_PACKET2(IM_PACKET_LOG_JITTER, "[GetJitterBufferSize] next[%d], curr[%d]",
                        nextJitterBuffer, nCurrJitterBufferSize);
            }
        }
    }

    IMLOGD2("[JitterNetworkAnalyser] next[%d], curr[%d]", nextJitterBuffer, nCurrJitterBufferSize);
    mNetworkStatus = currStatus;
    return nextJitterBuffer;
}
