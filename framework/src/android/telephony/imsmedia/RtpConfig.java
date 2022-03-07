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

import android.annotation.IntDef;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.os.Parcel;
import android.os.Parcelable;
import android.telephony.AccessNetworkConstants.AccessNetworkType;
import android.net.InetAddresses;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.net.InetSocketAddress;
import java.net.UnknownHostException;
import java.util.Objects;

/**
 * Class to encapsulate RTP (Real Time Protocol) configurations
 *
 * @hide
 */
public abstract class RtpConfig implements Parcelable {
    /** Device neither transmits nor receives any media */
    public static final int MEDIA_DIRECTION_NO_FLOW = 0;
    /**
     * Device transmits outgoing media but but doesn't receive incoming media.
     * Eg. Other party muted the call
     */
    public static final int MEDIA_DIRECTION_TRANSMIT_ONLY = 1;
    /**
     * Device receives the incoming media but doesn't transmit any outgoing media.
     * Eg. User muted the call
     */
    public static final int MEDIA_DIRECTION_RECEIVE_ONLY = 2;
    /** Device transmits and receives media in both the directions */
    public static final int MEDIA_DIRECTION_TRANSMIT_RECEIVE = 3;
    /* definition of uninitialized port number*/
    public static final int UNINITIALIZED_PORT = -1;

    /** @hide */
    @IntDef(
        value = {
           MEDIA_DIRECTION_NO_FLOW,
           MEDIA_DIRECTION_TRANSMIT_ONLY,
           MEDIA_DIRECTION_RECEIVE_ONLY,
           MEDIA_DIRECTION_TRANSMIT_RECEIVE,
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface MediaDirection {}

    private @MediaDirection int direction;
    private int accessNetwork;
    @Nullable
    private InetSocketAddress remoteRtpAddress;
    @Nullable
    private RtcpConfig rtcpConfig;
    private int maxMtuBytes;
    private int dscp;
    private int rxPayloadTypeNumber;
    private int txPayloadTypeNumber;
    private byte samplingRateKHz;

    /** @hide */
    RtpConfig(Parcel in) {
        direction = in.readInt();
        accessNetwork = in.readInt();
        remoteRtpAddress = readSocketAddress(in);
        rtcpConfig = in.readParcelable(RtcpConfig.class.getClassLoader(), RtcpConfig.class);
        maxMtuBytes = in.readInt();
        dscp = in.readInt();
        rxPayloadTypeNumber = in.readInt();
        txPayloadTypeNumber = in.readInt();
        samplingRateKHz = in.readByte();
    }

    /** @hide **/
    RtpConfig(AbstractBuilder builder) {
        direction = builder.direction;
        accessNetwork = builder.accessNetwork;
        remoteRtpAddress = builder.remoteRtpAddress;
        rtcpConfig = builder.rtcpConfig;
        maxMtuBytes = builder.maxMtuBytes;
        dscp = builder.dscp;
        rxPayloadTypeNumber = builder.rxPayloadTypeNumber;
        txPayloadTypeNumber = builder.txPayloadTypeNumber;
        samplingRateKHz = builder.samplingRateKHz;
    }

    private @NonNull InetSocketAddress readSocketAddress(final Parcel in) {
        final String address = in.readString();
        final int port = in.readInt();
        if(address != null && port != UNINITIALIZED_PORT) {
            return new InetSocketAddress(
                InetAddresses.parseNumericAddress(address), port);
        }
        return null;
    }

    public int getMediaDirection() {
        return direction;
    }

    public void setMediaDirection(final @MediaDirection int direction) {
        this.direction = direction;
    }

    public int getAccessNetwork() {
        return accessNetwork;
    }

    public void setAccessNetwork(final int accessNetwork) {
        this.accessNetwork = accessNetwork;
    }

    public InetSocketAddress getRemoteRtpAddress() {
        return remoteRtpAddress;
    }

    public void setRemoteRtpAddress(final InetSocketAddress remoteRtpAddress) {
        this.remoteRtpAddress = remoteRtpAddress;
    }

    public RtcpConfig getRtcpConfig() {
        return rtcpConfig;
    }

    public void setRtcpConfig(final RtcpConfig rtcpConfig) {
        this.rtcpConfig = rtcpConfig;
    }

    public int getmaxMtuBytes() {
        return maxMtuBytes;
    }

    public void setMaxMtuBytes(final int maxMtuBytes) {
        this.maxMtuBytes = maxMtuBytes;
    }

    public int getDscp() {
        return dscp;
    }

    public void setDscp(final int dscp) {
        this.dscp = dscp;
    }

    public int getRxPayloadTypeNumber() {
        return rxPayloadTypeNumber;
    }

    public void setRxPayloadTypeNumber(final int rxPayloadTypeNumber) {
        this.rxPayloadTypeNumber = rxPayloadTypeNumber;
    }

    public int getTxPayloadTypeNumber() {
        return txPayloadTypeNumber;
    }

    public void setTxPayloadTypeNumber(final int txPayloadTypeNumber) {
        this.txPayloadTypeNumber = txPayloadTypeNumber;
    }

    public byte getSamplingRateKHz() {
        return samplingRateKHz;
    }

    public void setSamplingRateKHz(final byte samplingRateKHz) {
        this.samplingRateKHz = samplingRateKHz;
    }

    @NonNull
    @Override
    public String toString() {
        return "RtpConfig: {direction=" + direction
                + ", accessNetwork=" + accessNetwork
                + ", remoteRtpAddress=" + remoteRtpAddress
                + ", rtcpConfig=" + rtcpConfig
                + ", maxMtuBytes=" + maxMtuBytes
                + ", dscp=" + dscp
                + ", rxPayloadTypeNumber=" + rxPayloadTypeNumber
                + ", txPayloadTypeNumber=" + txPayloadTypeNumber
                + ", samplingRateKHz=" + samplingRateKHz
                + " }";
    }

    @Override
    public int hashCode() {
        return Objects.hash(direction, accessNetwork, remoteRtpAddress, rtcpConfig, maxMtuBytes,
                dscp, rxPayloadTypeNumber, txPayloadTypeNumber, samplingRateKHz);
    }

    @Override
    public boolean equals(@Nullable Object o) {
        if (o == null || !(o instanceof RtpConfig) || hashCode() != o.hashCode()) {
            return false;
        }

        if (this == o) {
            return true;
        }

        RtpConfig s = (RtpConfig) o;

        return (direction == s.direction
                && accessNetwork == s.accessNetwork
                && Objects.equals(remoteRtpAddress, s.remoteRtpAddress)
                && Objects.equals(rtcpConfig, s.rtcpConfig)
                && maxMtuBytes == s.maxMtuBytes
                && dscp == s.dscp
                && rxPayloadTypeNumber == s.rxPayloadTypeNumber
                && txPayloadTypeNumber == s.txPayloadTypeNumber
                && samplingRateKHz == s.samplingRateKHz);
    }

    /**
     * {@link Parcelable#describeContents}
     */
    public int describeContents() {
        return 0;
    }

    /**
     * Used by child classes for parceling.
     *
     * @hide
     */
    @CallSuper
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(direction);
        dest.writeInt(accessNetwork);
        if (remoteRtpAddress == null) {
            dest.writeString(null);
            dest.writeInt(UNINITIALIZED_PORT);
        } else {
            dest.writeString(remoteRtpAddress.getAddress().getHostAddress());
            dest.writeInt(remoteRtpAddress.getPort());
        }
        dest.writeParcelable(rtcpConfig, flags);
        dest.writeInt(maxMtuBytes);
        dest.writeInt(dscp);
        dest.writeInt(rxPayloadTypeNumber);
        dest.writeInt(txPayloadTypeNumber);
        dest.writeByte(samplingRateKHz);
    }

    public static final @NonNull Parcelable.Creator<RtpConfig>
        CREATOR = new Parcelable.Creator() {
        public RtpConfig createFromParcel(Parcel in) {
            /** TODO use builder class so it will validate */
            /** TODO: Fix for other types */
            return new AudioConfig(in);
        }

        public RtpConfig[] newArray(int size) {
            return new RtpConfig[size];
        }
    };

    /**
     * Provides a convenient way to set the fields of a {@link RtpConfig}
     * when creating a new instance.
     */
    public static abstract class AbstractBuilder<T extends AbstractBuilder<T>> {
        private @MediaDirection int direction;
        private int accessNetwork;
        @Nullable
        private InetSocketAddress remoteRtpAddress;
        @Nullable
        private RtcpConfig rtcpConfig;
        private int maxMtuBytes;
        private int dscp;
        private int rxPayloadTypeNumber;
        private int txPayloadTypeNumber;
        private byte samplingRateKHz;

        AbstractBuilder() {}

        /** Returns {@code this} */
        abstract T self();

        public T setMediaDirection(@MediaDirection int direction) {
            this.direction = direction;
            return self();
        }

        public T setAccessNetwork(int accessNetwork) {
            this.accessNetwork = accessNetwork;
            return self();
        }

        public T setRemoteRtpAddress(InetSocketAddress remoteRtpAddress) {
            this.remoteRtpAddress = remoteRtpAddress;
            return self();
        }

        public T setRtcpConfig(RtcpConfig rtcpConfig) {
            this.rtcpConfig = rtcpConfig;
            return self();
        }

        public T setMaxMtuBytes(int maxMtuBytes) {
            this.maxMtuBytes = maxMtuBytes;
            return self();
        }

        public T setDscp(int dscp) {
            this.dscp = dscp;
            return self();
        }

        public T setRxPayloadTypeNumber(int rxPayloadTypeNumber) {
            this.rxPayloadTypeNumber = rxPayloadTypeNumber;
            return self();
        }

        public T setTxPayloadTypeNumber(int txPayloadTypeNumber) {
            this.txPayloadTypeNumber = txPayloadTypeNumber;
            return self();
        }

        public T setSamplingRateKHz(byte samplingRateKHz) {
            this.samplingRateKHz = samplingRateKHz;
            return self();
        }
    }
}

