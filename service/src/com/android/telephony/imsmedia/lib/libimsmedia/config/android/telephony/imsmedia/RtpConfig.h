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

#ifndef RTPCONFIG_H
#define RTPCONFIG_H

#include <binder/Parcel.h>
#include <binder/Parcelable.h>
#include <binder/Status.h>

namespace android {

namespace telephony {

namespace imsmedia {

class RtpConfig : public Parcelable {
private:
    int mDirection;
    String16 mRemoteAddress;
    int mRemotePort;

public:
    RtpConfig();
    RtpConfig(RtpConfig& config);
    RtpConfig(int direction, String16& remoteAddress, int remotePort);
    virtual ~RtpConfig();
    virtual status_t writeToParcel(Parcel* out) const;
    virtual status_t readFromParcel(const Parcel* in);
    int getMediaDirection();
    void setMediaDirection(int direction);
    String16 getRemoteAddress();
    int getRemotePort();
};

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android

#endif