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
#include <gtest/gtest.h>

using namespace android::telephony::imsmedia;

const int32_t kEvsBandwidth = EvsParams::EVS_BAND_NONE;
const int32_t kEvsMode = 8;
const int8_t kChannelAwareMode = 3;
const bool kUseHeaderFullOnlyOnTx = false;
const bool kUseHeaderFullOnlyOnRx = false;

TEST(EvsParamsTest, TestGetterSetter) {
    EvsParams* param = new EvsParams();
    param->setEvsBandwidth(kEvsBandwidth);
    param->setEvsMode(kEvsMode);
    param->setChannelAwareMode(kChannelAwareMode);
    param->setUseHeaderFullOnlyOnTx(kUseHeaderFullOnlyOnTx);
    param->setUseHeaderFullOnlyOnRx(kUseHeaderFullOnlyOnRx);
    EXPECT_EQ(param->getEvsMode(), kEvsMode);
    EXPECT_EQ(param->getChannelAwareMode(), kChannelAwareMode);
    EXPECT_EQ(param->getUseHeaderFullOnlyOnTx(), kUseHeaderFullOnlyOnTx);
    EXPECT_EQ(param->getUseHeaderFullOnlyOnRx(), kUseHeaderFullOnlyOnRx);
    delete param;
}

TEST(EvsParamsTest, TestParcel) {
    EvsParams* param = new EvsParams();
    param->setEvsBandwidth(kEvsBandwidth);
    param->setEvsMode(kEvsMode);
    param->setChannelAwareMode(kChannelAwareMode);
    param->setUseHeaderFullOnlyOnTx(kUseHeaderFullOnlyOnTx);
    param->setUseHeaderFullOnlyOnRx(kUseHeaderFullOnlyOnRx);

    android::Parcel parcel;
    param->writeToParcel(&parcel);
    parcel.setDataPosition(0);

    EvsParams* param2 = new EvsParams();
    param2->readFromParcel(&parcel);
    EXPECT_EQ(*param2, *param);

    delete param;
    delete param2;
}

TEST(EvsParamsTest, TestAssign) {
    EvsParams param;
    param.setEvsBandwidth(kEvsBandwidth);
    param.setEvsMode(kEvsMode);
    param.setChannelAwareMode(kChannelAwareMode);
    param.setUseHeaderFullOnlyOnTx(kUseHeaderFullOnlyOnTx);
    param.setUseHeaderFullOnlyOnRx(kUseHeaderFullOnlyOnRx);

    EvsParams param2;
    param2 = param;
    EXPECT_EQ(param, param2);
}

TEST(EvsParamsTest, TestEqual) {
    EvsParams* param = new EvsParams();
    param->setEvsBandwidth(kEvsBandwidth);
    param->setEvsMode(kEvsMode);
    param->setChannelAwareMode(kChannelAwareMode);
    param->setUseHeaderFullOnlyOnTx(kUseHeaderFullOnlyOnTx);
    param->setUseHeaderFullOnlyOnRx(kUseHeaderFullOnlyOnRx);

    EvsParams* param2 = new EvsParams();
    param2->setEvsBandwidth(kEvsBandwidth);
    param2->setEvsMode(kEvsMode);
    param2->setChannelAwareMode(kChannelAwareMode);
    param2->setUseHeaderFullOnlyOnTx(kUseHeaderFullOnlyOnTx);
    param2->setUseHeaderFullOnlyOnRx(kUseHeaderFullOnlyOnRx);

    EXPECT_EQ(*param, *param2);
    delete param;
    delete param2;
}

TEST(EvsParamsTest, TestNotEqual) {
    EvsParams* param = new EvsParams();
    param->setEvsBandwidth(kEvsBandwidth);
    param->setEvsMode(kEvsMode);
    param->setChannelAwareMode(kChannelAwareMode);
    param->setUseHeaderFullOnlyOnTx(kUseHeaderFullOnlyOnTx);
    param->setUseHeaderFullOnlyOnRx(kUseHeaderFullOnlyOnRx);

    EvsParams* param2 = new EvsParams();
    param2->setEvsBandwidth(kEvsBandwidth);
    param2->setEvsMode(5);
    param2->setChannelAwareMode(kChannelAwareMode);
    param2->setUseHeaderFullOnlyOnTx(kUseHeaderFullOnlyOnTx);
    param2->setUseHeaderFullOnlyOnRx(kUseHeaderFullOnlyOnRx);

    EvsParams* param3 = new EvsParams();
    param3->setEvsBandwidth(kEvsBandwidth);
    param3->setEvsMode(kEvsMode);
    param3->setChannelAwareMode(kChannelAwareMode);
    param3->setUseHeaderFullOnlyOnTx(true);
    param3->setUseHeaderFullOnlyOnRx(kUseHeaderFullOnlyOnRx);

    EXPECT_NE(*param, *param2);
    EXPECT_NE(*param, *param3);
    delete param;
    delete param2;
    delete param3;
}
