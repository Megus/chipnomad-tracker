package org.chipnomad.tracker;

import org.libsdl.app.SDLActivity;
import android.util.Log;

public class ChipNomadActivity extends SDLActivity {
    private static final String TAG = "ChipNomadActivity";

    @Override
    protected String[] getLibraries() {
        return new String[] {
            "SDL2",
            "chipnomad"
        };
    }

    @Override
    protected void onCreate(android.os.Bundle savedInstanceState) {
        Log.d(TAG, "onCreate: before super.onCreate()");
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate: after super.onCreate()");
    }
}