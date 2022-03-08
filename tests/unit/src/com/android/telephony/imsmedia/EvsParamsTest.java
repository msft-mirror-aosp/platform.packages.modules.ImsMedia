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

package com.android.telephony.imsmedia.tests;

import static com.google.common.truth.Truth.assertThat;

import android.os.Parcel;
import android.telephony.imsmedia.EvsParams;
import androidx.test.runner.AndroidJUnit4;

import org.junit.runner.RunWith;
import org.junit.Test;

@RunWith(AndroidJUnit4.class)
public class EvsParamsTest {
    private static final byte CHANNEL_AWARE_MODE = 7;
    private static final boolean USE_HEADER_FULL_ONLY_TX = true;
    private static final boolean USE_HEADER_FULL_ONLY_RX = false;

    private EvsParams createEvsParams(
            final int evsBandwidth,
            final int evsMode,
            final byte channelAwareMode,
            final boolean useHeaderFullOnlyOnTx,
            final boolean useHeaderFullOnlyOnRx) {
        return new EvsParams.Builder()
                .setEvsbandwidth(evsBandwidth)
                .setEvsMode(evsMode)
                .setChannelAwareMode(channelAwareMode)
                .setHeaderFullOnlyOnTx(useHeaderFullOnlyOnTx)
                .setHeaderFullOnlyOnRx(useHeaderFullOnlyOnRx)
                .build();
    }

    @Test
    public void testConstructorAndGetters() {
        EvsParams evs = createEvsParams(EvsParams.EVS_WIDE_BAND, EvsParams.EVS_MODE_7,
                CHANNEL_AWARE_MODE, USE_HEADER_FULL_ONLY_TX, USE_HEADER_FULL_ONLY_RX);

        assertThat(evs.getEvsBandwidth()).isEqualTo(EvsParams.EVS_WIDE_BAND);
        assertThat(evs.getEvsMode()).isEqualTo(EvsParams.EVS_MODE_7);
        assertThat(evs.getChannelAwareMode()).isEqualTo(CHANNEL_AWARE_MODE);
        assertThat(evs.getUseHeaderFullOnlyOnTx()).isEqualTo(USE_HEADER_FULL_ONLY_TX);
        assertThat(evs.getUseHeaderFullOnlyOnRx()).isEqualTo(USE_HEADER_FULL_ONLY_RX);
    }

    @Test
    public void testParcel() {
        EvsParams evs = createEvsParams(EvsParams.EVS_WIDE_BAND, EvsParams.EVS_MODE_7,
                CHANNEL_AWARE_MODE, USE_HEADER_FULL_ONLY_TX, USE_HEADER_FULL_ONLY_RX);

        Parcel parcel = Parcel.obtain();
        evs.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);

        EvsParams parcelConfig = EvsParams.CREATOR.createFromParcel(parcel);
        assertThat(evs).isEqualTo(parcelConfig);
    }

    @Test
    public void testEqual() {
        EvsParams evs1 = createEvsParams(EvsParams.EVS_WIDE_BAND, EvsParams.EVS_MODE_7,
                CHANNEL_AWARE_MODE, USE_HEADER_FULL_ONLY_TX, USE_HEADER_FULL_ONLY_RX);

        EvsParams evs2 = createEvsParams(EvsParams.EVS_WIDE_BAND, EvsParams.EVS_MODE_7,
                CHANNEL_AWARE_MODE, USE_HEADER_FULL_ONLY_TX, USE_HEADER_FULL_ONLY_RX);

        assertThat(evs1).isEqualTo(evs2);
    }

    @Test
    public void testNotEqual() {
        EvsParams evs1 = createEvsParams(EvsParams.EVS_WIDE_BAND, EvsParams.EVS_MODE_7,
                CHANNEL_AWARE_MODE, USE_HEADER_FULL_ONLY_TX, USE_HEADER_FULL_ONLY_RX);

        EvsParams evs2 = createEvsParams(EvsParams.EVS_WIDE_BAND, EvsParams.EVS_MODE_6,
                CHANNEL_AWARE_MODE, USE_HEADER_FULL_ONLY_TX, USE_HEADER_FULL_ONLY_RX);

        assertThat(evs1).isNotEqualTo(evs2);

        EvsParams evs3 = createEvsParams(EvsParams.EVS_WIDE_BAND, EvsParams.EVS_MODE_7,
                (byte)8, USE_HEADER_FULL_ONLY_TX, USE_HEADER_FULL_ONLY_RX);

        assertThat(evs1).isNotEqualTo(evs3);

        EvsParams evs4 = createEvsParams(EvsParams.EVS_WIDE_BAND, EvsParams.EVS_MODE_7,
                CHANNEL_AWARE_MODE, false, USE_HEADER_FULL_ONLY_RX);

        assertThat(evs1).isNotEqualTo(evs4);

        EvsParams evs5 = createEvsParams(EvsParams.EVS_WIDE_BAND, EvsParams.EVS_MODE_7,
                CHANNEL_AWARE_MODE, USE_HEADER_FULL_ONLY_TX, true);

        assertThat(evs1).isNotEqualTo(evs5);

        EvsParams evs6 = createEvsParams(EvsParams.EVS_SUPER_WIDE_BAND, EvsParams.EVS_MODE_7,
                CHANNEL_AWARE_MODE, USE_HEADER_FULL_ONLY_TX, USE_HEADER_FULL_ONLY_RX);

        assertThat(evs1).isNotEqualTo(evs6);
    }
}
