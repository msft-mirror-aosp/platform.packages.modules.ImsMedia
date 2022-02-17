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

#ifndef AMRPARAMS_H
#define AMRPARAMS_H

#include <binder/Parcel.h>
#include <binder/Parcelable.h>
#include <binder/Status.h>
#include <stdint.h>

namespace android {

namespace telephony {

namespace imsmedia {

class AmrParams : public Parcelable {
public:
    enum AmrMode {
        AMR_MODE_0,
        AMR_MODE_1,
        AMR_MODE_2,
        AMR_MODE_3,
        AMR_MODE_4,
        AMR_MODE_5,
        AMR_MODE_6,
        AMR_MODE_7,
        AMR_MODE_8,
    };

    AmrParams();
    AmrParams(AmrParams& config);
    virtual ~AmrParams();
    virtual status_t writeToParcel(Parcel* parcel) const;
    virtual status_t readFromParcel(const Parcel* in);
    int32_t getAmrMode();
    bool getOctetAligned();
    int32_t getMaxRedundancyMillis();

private:
    /** mode-set: AMR codec mode to represent the bit rate */
    int32_t amrMode;
    /**
     * octet-align: If it's set to true then all fields in the AMR/AMR-WB header
     * shall be aligned to octet boundaries by adding padding bits.
     */
    bool octetAligned;
    /**
     * max-red: It’s the maximum duration in milliseconds that elapses between the
     * primary (first) transmission of a frame and any redundant transmission that
     * the sender will use. This parameter allows a receiver to have a bounded delay
     * when redundancy is used. Allowed values are between 0 (no redundancy will be
     * used) and 65535. If the parameter is omitted, no limitation on the use of
     * redundancy is present. See RFC 4867
     */
    int32_t maxRedundancyMillis;
};

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android

#endif