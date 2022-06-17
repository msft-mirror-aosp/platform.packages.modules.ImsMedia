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

#include <EvsParams.h>

namespace android
{

namespace telephony
{

namespace imsmedia
{

/** Native representation of android.telephony.imsmedia.EvsParams */
EvsParams::EvsParams()
{
    this->evsBandwidth = 0;
    this->evsMode = 0;
    this->channelAwareMode = 0;
    this->useHeaderFullOnly = false;
}

EvsParams::EvsParams(EvsParams& params)
{
    this->evsBandwidth = params.evsBandwidth;
    this->evsMode = params.evsMode;
    this->channelAwareMode = params.channelAwareMode;
    this->useHeaderFullOnly = params.useHeaderFullOnly;
}

EvsParams::~EvsParams() {}

EvsParams& EvsParams::operator=(const EvsParams& param)
{
    this->evsBandwidth = param.evsBandwidth;
    this->evsMode = param.evsMode;
    this->channelAwareMode = param.channelAwareMode;
    this->useHeaderFullOnly = param.useHeaderFullOnly;
    return *this;
}

bool EvsParams::operator==(const EvsParams& param) const
{
    return (this->evsBandwidth == param.evsBandwidth && this->evsMode == param.evsMode &&
            this->channelAwareMode == param.channelAwareMode &&
            this->useHeaderFullOnly == param.useHeaderFullOnly);
}

bool EvsParams::operator!=(const EvsParams& param) const
{
    return (this->evsBandwidth != param.evsBandwidth || this->evsMode != param.evsMode ||
            this->channelAwareMode != param.channelAwareMode ||
            this->useHeaderFullOnly != param.useHeaderFullOnly);
}

status_t EvsParams::writeToParcel(Parcel* out) const
{
    status_t err;
    if (out == NULL)
    {
        return BAD_VALUE;
    }

    err = out->writeInt32(evsBandwidth);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = out->writeInt32(evsMode);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = out->writeByte(channelAwareMode);
    if (err != NO_ERROR)
    {
        return err;
    }

    int32_t value = 0;
    useHeaderFullOnly ? value = 1 : value = 0;
    err = out->writeInt32(value);
    if (err != NO_ERROR)
    {
        return err;
    }

    return NO_ERROR;
}

status_t EvsParams::readFromParcel(const Parcel* in)
{
    status_t err;
    if (in == NULL)
    {
        return BAD_VALUE;
    }

    err = in->readInt32(&evsBandwidth);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readInt32(&evsMode);
    if (err != NO_ERROR)
    {
        return err;
    }

    err = in->readByte(&channelAwareMode);
    if (err != NO_ERROR)
    {
        return err;
    }

    int32_t value = 0;
    err = in->readInt32(&value);
    if (err != NO_ERROR)
    {
        return err;
    }

    value == 0 ? useHeaderFullOnly = false : useHeaderFullOnly = true;

    return NO_ERROR;
}

void EvsParams::setEvsBandwidth(const int32_t bandwidth)
{
    evsBandwidth = bandwidth;
}

int32_t EvsParams::getEvsBandwidth()
{
    return evsBandwidth;
}

void EvsParams::setEvsMode(const int32_t mode)
{
    evsMode = mode;
}

int32_t EvsParams::getEvsMode()
{
    return evsMode;
}

void EvsParams::setChannelAwareMode(int8_t mode)
{
    channelAwareMode = mode;
}

int8_t EvsParams::getChannelAwareMode()
{
    return channelAwareMode;
}

void EvsParams::setUseHeaderFullOnly(const bool enable)
{
    useHeaderFullOnly = enable;
}

bool EvsParams::getUseHeaderFullOnly()
{
    return useHeaderFullOnly;
}

}  // namespace imsmedia

}  // namespace telephony

}  // namespace android
