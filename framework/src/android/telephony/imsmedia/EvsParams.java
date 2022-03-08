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
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.ParcelFileDescriptor;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Objects;

/**
 * The class represents EVS (Enhanced Voice Services) codec parameters.
 *
 * @hide
 */
public final class EvsParams implements Parcelable {
    /** EVS band not specified */
    public static final int EVS_BAND_NONE = 0;
    /** EVS narrow band */
    public static final int EVS_NARROW_BAND = 1 << 0;
    /** EVS wide band */
    public static final int EVS_WIDE_BAND = 1 << 1;
    /** EVS super wide band */
    public static final int EVS_SUPER_WIDE_BAND = 1 << 2;
    /** EVS full band */
    public static final int EVS_FULL_BAND = 1 << 3;

    /** @hide */
    @IntDef(
        value = {
           EVS_BAND_NONE,
           EVS_NARROW_BAND,
           EVS_WIDE_BAND,
           EVS_SUPER_WIDE_BAND,
           EVS_FULL_BAND,
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface EvsBandwidth {}

    /** 6.6 kbps for EVS AMR-WB IO */
    public static final int EVS_MODE_0 = 0;
    /** 8.855 kbps for AMR-WB IO */
    public static final int EVS_MODE_1 = 1;
    /** 12.65 kbps for AMR-WB IO */
    public static final int EVS_MODE_2 = 2;
    /** 14.25 kbps for AMR-WB IO */
    public static final int EVS_MODE_3 = 3;
    /** 15.85 kbps for AMR-WB IO */
    public static final int EVS_MODE_4 = 4;
    /** 18.25 kbps for AMR-WB IO */
    public static final int EVS_MODE_5 = 5;
    /** 19.85 kbps for AMR-WB IO */
    public static final int EVS_MODE_6 = 6;
    /** 23.05 kbps for AMR-WB IO */
    public static final int EVS_MODE_7 = 7;
    /** 23.85 kbps for AMR-WB IO */
    public static final int EVS_MODE_8 = 8;
    /** 5.9 kbps for EVS primary */
    public static final int EVS_MODE_9 = 9;
    /** 7.2 kbps for EVS primary */
    public static final int EVS_MODE_10 = 10;
    /** 8.0 kbps for EVS primary */
    public static final int EVS_MODE_11 = 11;
    /** 9.6 kbps for EVS primary */
    public static final int EVS_MODE_12 = 12;
    /** 13.2 kbps for EVS primary */
    public static final int EVS_MODE_13 = 13;
    /** 16.4 kbps for EVS primary */
    public static final int EVS_MODE_14 = 14;
    /** 24.4 kbps for EVS primary */
    public static final int EVS_MODE_15 = 15;
    /** 32.0 kbps for EVS primary */
    public static final int EVS_MODE_16 = 16;
    /** 48.0 kbps for EVS primary */
    public static final int EVS_MODE_17 = 17;
    /** 64.0 kbps for EVS primary */
    public static final int EVS_MODE_18 = 18;
    /** 96.0 kbps for EVS primary */
    public static final int EVS_MODE_19 = 19;
    /** 128.0 kbps for EVS primary */
    public static final int EVS_MODE_20 = 20;

    /** @hide */
    @IntDef(
        value = {
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
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface EvsMode {}

    private final @EvsBandwidth int evsBandwidth;
    private final @EvsMode int evsMode;
    private final byte channelAwareMode;
    private final boolean useHeaderFullOnlyOnTx;
    private final boolean useHeaderFullOnlyOnRx;


    /** @hide **/
    public EvsParams(Parcel in) {
        evsBandwidth = in.readInt();
        evsMode = in.readInt();
        channelAwareMode = in.readByte();
        useHeaderFullOnlyOnTx = in.readBoolean();
        useHeaderFullOnlyOnRx = in.readBoolean();
    }

    private EvsParams(final @EvsBandwidth int evsBandwidth, final @EvsMode int evsMode,
            final byte channelAwareMode, final boolean useHeaderFullOnlyOnTx,
            final boolean useHeaderFullOnlyOnRx) {
        this.evsBandwidth = evsBandwidth;
        this.evsMode = evsMode;
        this.channelAwareMode = channelAwareMode;
        this.useHeaderFullOnlyOnTx = useHeaderFullOnlyOnTx;
        this.useHeaderFullOnlyOnRx = useHeaderFullOnlyOnRx;
    }

    /** @hide **/
    public @EvsBandwidth int getEvsBandwidth() {
        return evsBandwidth;
    }

    /** @hide **/
    public @EvsMode int getEvsMode() {
        return evsMode;
    }

    /** @hide **/
    public byte getChannelAwareMode() {
        return channelAwareMode;
    }

    /** @hide **/
    public boolean getUseHeaderFullOnlyOnTx() {
        return useHeaderFullOnlyOnTx;
    }

    /** @hide **/
    public boolean getUseHeaderFullOnlyOnRx() {
        return useHeaderFullOnlyOnRx;
    }

    @NonNull
    @Override
    public String toString() {
        return "EvsParams: {evsBandwidth=" + evsBandwidth
                + ", evsMode=" + evsMode
                + ", channelAwareMode=" + channelAwareMode
                + ", useHeaderFullOnlyOnTx=" + useHeaderFullOnlyOnTx
                + ", useHeaderFullOnlyOnRx=" + useHeaderFullOnlyOnRx
                + " }";
    }

    @Override
    public int hashCode() {
        return Objects.hash(evsBandwidth, evsMode, channelAwareMode,
                useHeaderFullOnlyOnTx, useHeaderFullOnlyOnRx);
    }

    @Override
    public boolean equals(@Nullable Object o) {
        if (o == null || !(o instanceof EvsParams) || hashCode() != o.hashCode()) {
            return false;
        }

        if (this == o) {
            return true;
        }

        EvsParams s = (EvsParams) o;

        return (evsBandwidth == s.evsBandwidth
                && evsMode == s.evsMode
                && channelAwareMode == s.channelAwareMode
                && useHeaderFullOnlyOnTx == s.useHeaderFullOnlyOnTx
                && useHeaderFullOnlyOnRx == s.useHeaderFullOnlyOnRx);
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
        dest.writeInt(evsBandwidth);
        dest.writeInt(evsMode);
        dest.writeByte(channelAwareMode);
        dest.writeBoolean(useHeaderFullOnlyOnTx);
        dest.writeBoolean(useHeaderFullOnlyOnRx);
    }

    public static final @NonNull Parcelable.Creator<EvsParams>
        CREATOR = new Parcelable.Creator() {
        public EvsParams createFromParcel(Parcel in) {
            // TODO use builder class so it will validate
            return new EvsParams(in);
        }

        public EvsParams[] newArray(int size) {
            return new EvsParams[size];
        }
    };

    /**
     * Provides a convenient way to set the fields of a {@link EvsParams}
     * when creating a new instance.
     */
    public static final class Builder {
        private @EvsBandwidth int evsBandwidth;
        private @EvsMode int evsMode;
        private byte channelAwareMode;
        private boolean useHeaderFullOnlyOnTx;
        private boolean useHeaderFullOnlyOnRx;

        /**
         * Default constructor for Builder.
         */
        public Builder() {
        }

        /**
         * Set the EVS speech codec bandwidth, See 3gpp spec 26.441 Table 1
         *
         * @param evsBandwidth EVS codec bandwidth
         * @return The same instance of the builder
         */
        public @NonNull Builder setEvsbandwidth(final @EvsBandwidth int evsBandwidth) {
            this.evsBandwidth = evsBandwidth;
            return this;
        }

        /**
         * Set the EVS codec mode to represent the bit rate
         *
         * @param evsMode EVS codec mode
         * @return The same instance of the builder
         */
        public @NonNull Builder setEvsMode(final @EvsMode int evsMode) {
            this.evsMode = evsMode;
            return this;
        }

        /**
         * Set the channel aware mode for the receive direction
         *
         * Permissible values are -1, 0, 2, 3, 5, and 7. If -1, channel-aware mode
         * is disabled in the session for the receive direction. If 0 or not present,
         * partial redundancy (channel-aware mode) is not used at the start of the
         * session for the receive  direction. If positive (2, 3, 5, or 7), partial
         * redundancy (channel-aware  mode) is used at the start of the session for
         * the receive direction using the value as the offset, See 3GPP TS 26.445
         * section 4.4.5
         *
         * @param channelAwareMode channel aware mode
         * @return The same instance of the builder
         */
        public @NonNull Builder setChannelAwareMode(final byte channelAwareMode) {
            this.channelAwareMode = channelAwareMode;
            return this;
        }

        /**
         * Set header full only mode the outgoing packets
         *
         * hf-only: Header full only is used for the outgoing packets. If it's true
         * then the session shall support header full format only else the session
         * could support both header full format and compact format.
         *
         * @param useHeaderFullOnlyOnTx {@code true} if header full only needs to enabled
         * @return The same instance of the builder.
         */
        public @NonNull Builder setHeaderFullOnlyOnTx(final boolean useHeaderFullOnlyOnTx) {
            this.useHeaderFullOnlyOnTx = useHeaderFullOnlyOnTx;
            return this;
        }

        /**
         * Set the header full only for the incoming packets
         *
         * hf-only: Header full only used on the incoming packets. If it's true then the
         * session shall support header full format only else the session could support
         * both header full format and compact format.
         *
         * @param useHeaderFullOnlyOnRx {@code true} if header full only needs to enabled
         * @return The same instance of the builder.
         */
        public @NonNull Builder setHeaderFullOnlyOnRx(final boolean useHeaderFullOnlyOnRx) {
            this.useHeaderFullOnlyOnRx = useHeaderFullOnlyOnRx;
            return this;
        }

        /**
         * Build the EvsParams.
         *
         * @return the EvsParams object.
         */
        public @NonNull EvsParams build() {
            // TODO validation
            return new EvsParams(evsBandwidth, evsMode, channelAwareMode,
                    useHeaderFullOnlyOnTx, useHeaderFullOnlyOnRx);
        }
    }
}

