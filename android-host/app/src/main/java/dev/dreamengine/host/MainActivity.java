package dev.dreamengine.host;

import android.os.Bundle;
import android.util.Log;

import com.google.androidgamesdk.GameActivity;

/** Minimal Java host; the engine's native android_main owns the game loop. */
public final class MainActivity extends GameActivity {
    private static final String TAG = "DreamEngineBoot";

    static {
        Log.i(TAG, "MainActivity class initialized; loading dream_engine_android_host");
        System.loadLibrary("dream_engine_android_host");
        Log.i(TAG, "dream_engine_android_host loaded successfully");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "MainActivity.onCreate entered; calling GameActivity.onCreate");
        super.onCreate(savedInstanceState);
        Log.i(TAG, "GameActivity.onCreate returned");
    }
}
