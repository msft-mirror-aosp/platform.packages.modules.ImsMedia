package com.example.imsmediatestingapp;

import android.content.Context;
import android.telephony.imsmedia.EvsParams;
import android.view.View;
import android.widget.AutoCompleteTextView;
import android.widget.PopupMenu;
import androidx.annotation.NonNull;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.textfield.TextInputLayout;
import java.util.EventListener;
import java.util.Objects;

/**
 * The BottomSheetAudioCodecSettings class extends the BottomSheetDialog class. It is used when
 * changing the audio codec during an open IMS audio session.
 */
public class BottomSheetAudioCodecSettings extends BottomSheetDialog implements EventListener {

    private boolean isOpen = false;
    AutoCompleteTextView audioCodecDropDown;
    AutoCompleteTextView evsBandDropdown;
    AutoCompleteTextView audioCodecModeDropdown;
    TextInputLayout audioCodecModeDropdownLayout;
    TextInputLayout evsBandDropdownLayout;
    private int audioCodec;
    private int evsMode;
    private int evsBand;
    private int amrMode;

    /**
     * Constructor for the BottomSheetAudioCodecSettings
     * @param context The context that the BottomSheetDialog will be displayed on top of.
     */
    public BottomSheetAudioCodecSettings(@NonNull Context context) {
        super(context);
    }

    /**
     * Sets up the three dropdown lists' onClickListeners
     */
    @Override
    public void onStart() {
        super.onStart();
        isOpen = true;

        audioCodecDropDown = findViewById(R.id.selectAudioCodec);
        Objects.requireNonNull(audioCodecDropDown).setOnClickListener(
            view -> openCodecSelectionMenu());

        evsBandDropdown = findViewById(R.id.selectEvsBand);
        Objects.requireNonNull(evsBandDropdown).setOnClickListener(
            view -> openEvsBandwidthMenu());

        audioCodecModeDropdown = findViewById(R.id.selectAudioCodecMode);
        Objects.requireNonNull(audioCodecModeDropdown).setOnClickListener(
            view -> openAmrModeSelectionMenu());
    }

    /**
     * Changes the isOpen boolean to false after the BottomSheetDialog is dismissed.
     */
    @Override
    public void dismiss() {
        super.dismiss();
        isOpen = false;
    }

    /**
     * Gets if the BottomSheetDialong is open
     * @return if the BottomSheetDialog is currently on screen
     */
    public boolean isOpen() {
        return isOpen;
    }

    /**
     * Displays a PopupMenu filled with the audio codecs from the "audio_codecs_menu.xml" and
     * configures the onClickListener to save the user's selection. Also contains the logic to
     * determine which menu should be shown for each selection.
     */
    public void openCodecSelectionMenu() {
        PopupMenu audioCodecMenu = new PopupMenu(getContext(), findViewById(R.id.selectAudioCodec));
        audioCodecMenu.getMenuInflater()
            .inflate(R.menu.audio_codecs_menu, audioCodecMenu.getMenu());
        audioCodecMenu.setOnMenuItemClickListener(item -> {
            audioCodecDropDown.setText(item.getTitle());
            audioCodec = Integer.parseInt(String.valueOf(item.getTitleCondensed()));
            audioCodecModeDropdownLayout = findViewById(R.id.selectAudioCodecModeLayout);
            evsBandDropdownLayout = findViewById(R.id.selectEvsBandLayout);

            switch (item.getItemId()) {

                case R.id.amrCodec:
                case R.id.amrWbCodec:
                    Objects.requireNonNull(audioCodecModeDropdown).setOnClickListener(
                        view -> openAmrModeSelectionMenu());
                    evsBandDropdownLayout.setVisibility(View.GONE);
                    audioCodecModeDropdownLayout.setVisibility(View.VISIBLE);
                    break;

                case R.id.evsCodec:
                    evsBandDropdownLayout.setVisibility(View.VISIBLE);
                    audioCodecModeDropdownLayout.setVisibility(View.GONE);
                    break;

                case R.id.pcmaCodec:
                case R.id.pcmuCodec:
                    evsBandDropdownLayout.setVisibility(View.GONE);
                    audioCodecModeDropdownLayout.setVisibility(View.GONE);
                    break;

                default:
                    return false;
            }

            return true;
        });
        audioCodecMenu.show();
    }

    /**
     * Displays a PopupMenu filled with the EVS bandwidths from the "evs_band_menu.xml".
     * Also, configures the menu's onClickListener to save the user's selection.
     */
    public void openEvsBandwidthMenu() {
        PopupMenu evsBandMenu = new PopupMenu(getContext(), findViewById(R.id.selectEvsBand));
        evsBandMenu.getMenuInflater().inflate(R.menu.evs_band_menu, evsBandMenu.getMenu());
        evsBandMenu.setOnMenuItemClickListener(item -> {
            evsBandDropdown.setText(item.getTitle());
            Objects.requireNonNull(audioCodecModeDropdown).setOnClickListener(
                view -> openEvsModeSelectionMenu());
            audioCodecModeDropdownLayout.setVisibility(View.VISIBLE);
            evsBand = Integer.parseInt(String.valueOf(item.getTitleCondensed()));
            switch (item.getItemId()) {
                case R.id.evsBandNone:
                    evsBand = EvsParams.EVS_BAND_NONE;
                    break;

                case R.id.evsBandNarrow:
                    evsBand = EvsParams.EVS_NARROW_BAND;
                    break;

                case R.id.evsBandWide:
                    evsBand = EvsParams.EVS_WIDE_BAND;
                    break;

                case R.id.evsBandSuperWide:
                    evsBand = EvsParams.EVS_SUPER_WIDE_BAND;
                    break;

                case R.id.evsBandFull:
                    evsBand = EvsParams.EVS_FULL_BAND;
                    break;
            }

            return true;
        });
        evsBandMenu.show();
    }

    /**
     * Displays a PopupMenu filled with the AMR modes from the "amr_modes_menu.xml" and configures
     * the menu's onClickListener to save the user's selection.
     */
    public void openAmrModeSelectionMenu() {
        PopupMenu amrModeMenu = new PopupMenu(getContext(),
            findViewById(R.id.selectAudioCodecMode));
        amrModeMenu.getMenuInflater().inflate(R.menu.amr_modes_menu, amrModeMenu.getMenu());
        amrModeMenu.setOnMenuItemClickListener(item -> {
            audioCodecModeDropdown.setText(item.getTitle());
            amrMode = Integer.parseInt(String.valueOf(item.getTitleCondensed()));

            return true;
        });
        amrModeMenu.show();
    }

    /**
     * Displays a PopupMenu filled with the EVS modes from the "evs_modes_menu.xml" and configures
     * the menu's onClickListener to save the user's selection.
     */
    public void openEvsModeSelectionMenu() {
        PopupMenu evsModeMenu = new PopupMenu(getContext(),
            findViewById(R.id.selectAudioCodecMode));
        evsModeMenu.getMenuInflater().inflate(R.menu.evs_modes_menu, evsModeMenu.getMenu());
        evsModeMenu.setOnMenuItemClickListener(item -> {
            audioCodecModeDropdown.setText(item.getTitle());
            evsMode = Integer.parseInt(String.valueOf(item.getTitleCondensed()));

            return true;
        });
        evsModeMenu.show();
    }

    /**
     * @return Integer value of the selected EVS mode.
     */
    public int getEvsMode() {
        return evsMode;
    }

    /**
     * @return Integer value of the selected EVS band.
     */
    public int getEvsBand() {
        return evsBand;
    }

    /**
     * @return Integer value of the selected AMR mode.
     */
    public int getAmrMode() {
        return amrMode;
    }

    /**
     * @return Integer value of the selected audio codec type.
     */
    public int getAudioCodec() {
        return audioCodec;
    }
}
