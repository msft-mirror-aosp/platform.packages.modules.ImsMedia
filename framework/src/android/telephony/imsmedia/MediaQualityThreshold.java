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

package android.telephony.imsmedia;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.os.Parcel;
import android.os.Parcelable;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Objects;

/**
 * Class to set the threshold for media quality notifications
 *
 * @hide
 */
public final class MediaQualityThreshold implements Parcelable {
    private final int rtpInactivityTimerMillis;
    private final int rtcpInactivityTimerMillis;
    private final int packetLossPeriodMillis;
    private final int packetLossThreshold;
    private final int jitterPeriodMillis;
    private final int jitterThresholdMillis;

    /** @hide **/
    public MediaQualityThreshold(Parcel in) {
        rtpInactivityTimerMillis = in.readInt();
        rtcpInactivityTimerMillis = in.readInt();
        packetLossPeriodMillis = in.readInt();
        packetLossThreshold = in.readInt();
        jitterPeriodMillis = in.readInt();
        jitterThresholdMillis = in.readInt();
    }

    /** @hide **/
    public MediaQualityThreshold(Builder builder) {
        rtpInactivityTimerMillis = builder.rtpInactivityTimerMillis;
        rtcpInactivityTimerMillis = builder.rtcpInactivityTimerMillis;
        packetLossPeriodMillis = builder.packetLossPeriodMillis;
        packetLossThreshold = builder.packetLossThreshold;
        jitterPeriodMillis = builder.jitterPeriodMillis;
        jitterThresholdMillis = builder.jitterThresholdMillis;
    }

    /** @hide **/
    public int getRtpInactivityTimerMillis() {
        return rtpInactivityTimerMillis;
    }

    /** @hide **/
    public int getRtcpInactivityTimerMillis() {
        return rtcpInactivityTimerMillis;
    }

    /** @hide **/
    public int getPacketLossPeriodMillis() {
        return packetLossPeriodMillis;
    }

    /** @hide **/
    public int getPacketLossThreshold() {
        return packetLossThreshold;
    }

    /** @hide **/
    public int getJitterPeriodMillis() {
        return jitterPeriodMillis;
    }

    /** @hide **/
    public int getJitterThresholdMillis() {
        return jitterThresholdMillis;
    }

    @NonNull
    @Override
    public String toString() {
        return "MediaQualityThreshold: {rtpInactivityTimerMillis=" + rtpInactivityTimerMillis
                + ", rtcpInactivityTimerMillis=" + rtcpInactivityTimerMillis
                + ", packetLossPeriodMillis=" + packetLossPeriodMillis
                + ", packetLossThreshold=" + packetLossThreshold
                + ", jitterPeriodMillis =" + jitterPeriodMillis
                + ", jitterThresholdMillis=" + jitterThresholdMillis
                + " }";
    }

    @Override
    public int hashCode() {
        return Objects.hash(rtpInactivityTimerMillis, rtcpInactivityTimerMillis,
                packetLossPeriodMillis, packetLossThreshold, jitterPeriodMillis,
                jitterThresholdMillis);
    }

    @Override
    public boolean equals(@Nullable Object o) {
        if (o == null || !(o instanceof MediaQualityThreshold) || hashCode() != o.hashCode()) {
            return false;
        }

        if (this == o) {
            return true;
        }

        MediaQualityThreshold s = (MediaQualityThreshold) o;

        return (rtpInactivityTimerMillis == s.rtpInactivityTimerMillis
                && rtcpInactivityTimerMillis == s.rtcpInactivityTimerMillis
                && packetLossPeriodMillis == s.packetLossPeriodMillis
                && packetLossThreshold == s.packetLossThreshold
                && jitterPeriodMillis == s.jitterPeriodMillis
                && jitterThresholdMillis == s.jitterThresholdMillis);
    }

    /**
     * {@link Parcelable#describeContents}
     */
    public int describeContents() {
        return 0;
    }

    /**
     * {@link Parcelable#writeToParcel}
     */
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(rtpInactivityTimerMillis);
        dest.writeInt(rtcpInactivityTimerMillis);
        dest.writeInt(packetLossPeriodMillis);
        dest.writeInt(packetLossThreshold);
        dest.writeInt(jitterPeriodMillis);
        dest.writeInt(jitterThresholdMillis);
    }

    public static final @NonNull Parcelable.Creator<MediaQualityThreshold>
        CREATOR = new Parcelable.Creator() {
        public MediaQualityThreshold createFromParcel(Parcel in) {
            // TODO use builder class so it will validate
            return new MediaQualityThreshold(in);
        }

        public MediaQualityThreshold[] newArray(int size) {
            return new MediaQualityThreshold[size];
        }
    };

    /**
     * Provides a convenient way to set the fields of a {@link MediaQualityThreshold}
     * when creating a new instance.
     */
    public static final class Builder {
        private int rtpInactivityTimerMillis;
        private int rtcpInactivityTimerMillis;
        private int packetLossPeriodMillis;
        private int packetLossThreshold;
        private int jitterPeriodMillis;
        private int jitterThresholdMillis;

        /**
         * Default constructor for Builder.
         */
        public Builder() {
        }

        /**
         * Set the timer in milliseconds for monitoring RTP inactivity
         *
         * @param timer The timer value in milliseconds
         *
         * @return The same instance of the builder
         */
        public @NonNull Builder setRtpInactivityTimerMillis(final int timer) {
            this.rtpInactivityTimerMillis = timer;
            return this;
        }

        /**
         * Set the timer in milliseconds for monitoring RTCP inactivity
         *
         * @param timer The timer value in milliseconds
         *
         * @return The same instance of the builder
         */
        public @NonNull Builder setRtcpInactivityTimerMillis(final int timer) {
            this.rtcpInactivityTimerMillis = timer;
            return this;
        }

        /**
         * Set the duration in milliseconds for monitoring the RTP packet loss rate
         *
         * @param packetLossPeriod The duration in milliseconds
         *
         * @return The same instance of the builder
         */
        public @NonNull Builder setPacketLossPeriodMillis(final int packetLossPeriod) {
            this.packetLossPeriodMillis = packetLossPeriod;
            return this;
        }

        /**
         * Set the RTP packet loss rate threshold in percentage
         *
         * Packet loss rate = (Number of packets lost / number of packets expected) * 100
         *
         * @param packetLossRate The packet loss rate
         *
         * @return The same instance of the builder
         */
        public @NonNull Builder setPacketLossThreshold(final int packetLossRate) {
            this.packetLossThreshold = packetLossRate;
            return this;
        }

        /**
         * Set the duration in milliseconds for monitoring the jitter for RTP traffic
         *
         * @param jitterPeriod The jitter monitoring duration
         *
         * @return The same instance of the builder
         */
        public @NonNull Builder setJitterPeriodMillis(final int jitterPeriod) {
            this.jitterPeriodMillis = jitterPeriod;
            return this;
        }

        /**
         * Set the RTP jitter threshold in milliseconds
         *
         * @param jitter The jitter threshold
         *
         * @return The same instance of the builder
         */
        public @NonNull Builder setJitterThresholdMillis(final int jitter) {
            this.jitterThresholdMillis = jitter;
            return this;
        }

        /**
         * Build the MediaQualityThreshold.
         *
         * @return the MediaQualityThreshold object.
         */
        public @NonNull MediaQualityThreshold build() {
            // TODO validation
            return new MediaQualityThreshold(this);
        }
    }
}

