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

#ifndef EVSPARAMS_H
#define EVSPARAMS_H

#include <binder/Parcel.h>
#include <binder/Parcelable.h>
#include <binder/Status.h>
#include <stdint.h>

namespace android {

namespace telephony {

namespace imsmedia {

class EvsParams : public Parcelable {
public:
    enum EvsMode {
        EVS_MODE_0,
        EVS_MODE_1,
        EVS_MODE_2,
        EVS_MODE_3,
        EVS_MODE_4,
        EVS_MODE_5,
        EVS_MODE_6,
        EVS_MODE_7,
        EVS_MODE_8,
        EVS_MODE_9,
        EVS_MODE_10,
        EVS_MODE_11,
        EVS_MODE_12,
        EVS_MODE_13,
        EVS_MODE_14,
        EVS_MODE_15,
        EVS_MODE_16,
        EVS_MODE_17,
        EVS_MODE_18,
        EVS_MODE_19,
        EVS_MODE_20,
    };

    EvsParams();
    EvsParams(EvsParams& params);
    virtual ~EvsParams();
    virtual status_t writeToParcel(Parcel* parcel) const;
    virtual status_t readFromParcel(const Parcel* in);
    int32_t getEvsMode();
    int8_t getChannelAwareMode();
    bool getUseHeaderFullOnlyOnTx();
    bool getMaxRedundancyMillis();

private:
    /** mode-set: EVS codec mode to represent the bit rate */
    int32_t evsMode;
    /**
     * ch-aw-recv: Channel aware mode for the receive direction. Permissible values
     * are -1, 0, 2, 3, 5, and 7. If -1, channel-aware mode is disabled in the
     * session for the receive direction. If 0 or not present, partial redundancy
     * (channel-aware mode) is not used at the start of the session for the receive
     * direction. If positive (2, 3, 5, or 7), partial redundancy (channel-aware
     * mode) is used at the start of the session for the receive direction using the
     * value as the offset, See 3GPP TS 26.445 section 4.4.5
     */
    int8_t channelAwareMode;
    /**
     * hf-only: Header full only is used for the outgoing packets. If it's true then
     * the session shall support header full format only else the session could
     * support both header full format and compact format.
     */
    bool useHeaderFullOnlyOnTx;
    /**
     * hf-only: Header full only used on the incoming packets. If it's true then the
     * session shall support header full format only else the session could support
     * both header full format and compact format.
     */
    bool useHeaderFullOnlyOnRx;
};

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android

#endif