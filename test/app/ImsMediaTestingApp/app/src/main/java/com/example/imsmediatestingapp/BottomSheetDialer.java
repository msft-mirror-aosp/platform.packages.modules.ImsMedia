package com.example.imsmediatestingapp;

import android.content.Context;
import android.widget.TextView;
import androidx.annotation.NonNull;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import java.util.EventListener;

public class BottomSheetDialer extends BottomSheetDialog implements EventListener {
    private boolean isOpen = false;
    private TextView dtmfInput;

    public BottomSheetDialer(@NonNull Context context) {
        super(context);
    }

    @Override
    public void onStart() {
        super.onStart();
        isOpen = true;
        dtmfInput = findViewById(R.id.dtmfInput);
    }

    @Override
    public void dismiss() {
        super.dismiss();
        isOpen = false;
    }

    public boolean isOpen() {
        return isOpen;
    }

    public TextView getDtmfInput() {
        return dtmfInput;
    }
}
