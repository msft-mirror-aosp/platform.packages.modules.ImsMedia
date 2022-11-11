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

package com.android.telephony.imsmedia;

import static com.google.common.truth.Truth.assertThat;

import android.os.Parcel;
import android.telephony.imsmedia.MediaQualityThreshold;
import androidx.test.filters.SmallTest;
import androidx.test.runner.AndroidJUnit4;

import org.junit.runner.RunWith;
import org.junit.Test;

@RunWith(AndroidJUnit4.class)
public class MediaQualityThresholdTest {
    private static final int RTP_TIMEOUT = 20;
    private static final int RTCP_TIMEOUT = 60;
    private static final int PACKET_LOSS_PERIOD = 120;
    private static final int PACKET_LOSS_RATE = 10;
    private static final int JITTER_PERIOD = 160;
    private static final int JITTER_THRESHOLD = 200;

    @Test
    public void testConstructorAndGetters() {
        MediaQualityThreshold threshold = createMediaQualityThreshold();

        assertThat(threshold.getRtpInactivityTimerMillis()).isEqualTo(RTP_TIMEOUT);
        assertThat(threshold.getRtcpInactivityTimerMillis()).isEqualTo(RTCP_TIMEOUT);
        assertThat(threshold.getPacketLossPeriodMillis()).isEqualTo(PACKET_LOSS_PERIOD);
        assertThat(threshold.getPacketLossThreshold()).isEqualTo(PACKET_LOSS_RATE);
        assertThat(threshold.getJitterPeriodMillis()).isEqualTo(JITTER_PERIOD);
        assertThat(threshold.getJitterThresholdMillis()).isEqualTo(JITTER_THRESHOLD);
    }

    @Test
    public void testParcel() {
        MediaQualityThreshold threshold = createMediaQualityThreshold();

        Parcel parcel = Parcel.obtain();
        threshold.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);

        MediaQualityThreshold parcelConfig = MediaQualityThreshold.CREATOR.createFromParcel(parcel);
        assertThat(threshold).isEqualTo(parcelConfig);
    }

    @Test
    public void testEqual() {
        MediaQualityThreshold threshold1 = createMediaQualityThreshold();
        MediaQualityThreshold threshold2 = createMediaQualityThreshold();

        assertThat(threshold1).isEqualTo(threshold2);
    }

    @Test
    public void testNotEqual() {
        MediaQualityThreshold threshold1 = createMediaQualityThreshold();

        MediaQualityThreshold threshold2 = new MediaQualityThreshold.Builder()
                .setRtpInactivityTimerMillis(RTP_TIMEOUT)
                .setRtcpInactivityTimerMillis(RTCP_TIMEOUT)
                .setPacketLossPeriodMillis(PACKET_LOSS_PERIOD)
                .setPacketLossThreshold(PACKET_LOSS_RATE)
                .setJitterPeriodMillis(JITTER_PERIOD)
                .setJitterThresholdMillis(JITTER_THRESHOLD+1)
                .build();

        assertThat(threshold1).isNotEqualTo(threshold2);

        MediaQualityThreshold threshold3 = new MediaQualityThreshold.Builder()
                .setRtpInactivityTimerMillis(RTP_TIMEOUT)
                .setRtcpInactivityTimerMillis(RTCP_TIMEOUT)
                .setPacketLossPeriodMillis(PACKET_LOSS_PERIOD)
                .setPacketLossThreshold(PACKET_LOSS_RATE)
                .setJitterPeriodMillis(JITTER_PERIOD+1)
                .setJitterThresholdMillis(JITTER_THRESHOLD)
                .build();

        assertThat(threshold1).isNotEqualTo(threshold3);

        MediaQualityThreshold threshold4 = new MediaQualityThreshold.Builder()
                .setRtpInactivityTimerMillis(RTP_TIMEOUT)
                .setRtcpInactivityTimerMillis(RTCP_TIMEOUT+1)
                .setPacketLossPeriodMillis(PACKET_LOSS_PERIOD)
                .setPacketLossThreshold(PACKET_LOSS_RATE)
                .setJitterPeriodMillis(JITTER_PERIOD)
                .setJitterThresholdMillis(JITTER_THRESHOLD)
                .build();

        assertThat(threshold1).isNotEqualTo(threshold4);

        MediaQualityThreshold threshold5 = new MediaQualityThreshold.Builder()
                .setRtpInactivityTimerMillis(RTP_TIMEOUT+1)
                .setRtcpInactivityTimerMillis(RTCP_TIMEOUT)
                .setPacketLossPeriodMillis(PACKET_LOSS_PERIOD)
                .setPacketLossThreshold(PACKET_LOSS_RATE)
                .setJitterPeriodMillis(JITTER_PERIOD)
                .setJitterThresholdMillis(JITTER_THRESHOLD)
                .build();

        assertThat(threshold1).isNotEqualTo(threshold5);
    }

    static MediaQualityThreshold createMediaQualityThreshold() {
        return new MediaQualityThreshold.Builder()
                .setRtpInactivityTimerMillis(RTP_TIMEOUT)
                .setRtcpInactivityTimerMillis(RTCP_TIMEOUT)
                .setPacketLossPeriodMillis(PACKET_LOSS_PERIOD)
                .setPacketLossThreshold(PACKET_LOSS_RATE)
                .setJitterPeriodMillis(JITTER_PERIOD)
                .setJitterThresholdMillis(JITTER_THRESHOLD)
                .build();
    }
}
