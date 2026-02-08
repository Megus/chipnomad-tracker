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

        // Copy bundled assets on first run
        copyAssets();
        Log.d(TAG, "onCreate: after copyAssets()");
    }
    
    private void copyAssets() {
        // TODO: Implement asset copying for bundled content
        // This will copy fonts, sample projects, etc. to app storage
    }
}