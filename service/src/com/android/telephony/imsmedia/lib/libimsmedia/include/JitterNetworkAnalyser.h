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

#ifndef JITTERNETWORKANALYSER_H_INCLUDED
#define JITTERNETWORKANALYSER_H_INCLUDED
#define JITTER_LIST_SIZE 50

#include <stdint.h>

enum NETWORK_STATUS
{
    NETWORK_STATUS_BAD,
    NETWORK_STATUS_NORMAL,
    NETWORK_STATUS_GOOD
};

class JitterNetworkAnalyser
{
public:
    JitterNetworkAnalyser();
    virtual ~JitterNetworkAnalyser();
    void Reset();
    // initialze network analyser
    void SetMinMaxJitterBufferSize(uint32_t nMinBufferSize, uint32_t nMaxBufferSize);
    void SetJitterOptions(uint32_t nReduceTH, uint32_t nStepSize, double zValue, bool bImprovement);
    // input data of analyser
    void OnInputData(uint32_t nTimestamp, bool bMark, uint32_t nSeqNum, uint32_t nInputTime);
    void OnFrameLoss();
    void BasePacketChange(uint32_t nTimestamp, uint32_t nInputTime);
    // output data of analyzer
    uint32_t GetJitterBufferSize(uint32_t nCurrJitterBufferSize);
    // Analyse packet lossRate
    void LossPacketRateAnalyser();
    void PrintPacketStatus();

private:
    bool JitterCalc(uint32_t nTimestamp, uint32_t nInputTime);
    double DevCalc(double* pMean);
    uint32_t GetMaxJitterValue();

    uint32_t mMinJitterBufferSize;
    uint32_t mMaxJitterBufferSize;
    uint32_t mReadCount;
    uint32_t mLossPacketCount;
    uint32_t mBaseInputTimestamp;
    uint32_t mBaseInputTime;
    uint32_t m_pnJitter[JITTER_LIST_SIZE];
    uint32_t mJitterIndex;
    NETWORK_STATUS m_NetworkStatus;
    uint32_t mGoodStatusEnteringTime;
    uint32_t mBadStatusChangedTime;
    uint32_t mBufferReduceTH;
    uint32_t mBufferStepSize;
    double m_fBufferZValue;
    bool mImprovement;
};

#endif  // JITTERNETWORKANALYSER_H_INCLUDED
