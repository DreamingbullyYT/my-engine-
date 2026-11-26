package dev.dreamengine.host;

import com.google.androidgamesdk.GameActivity;

/** Minimal Java host; the engine's native android_main owns the game loop. */
public final class MainActivity extends GameActivity {
    static {
        System.loadLibrary("dream_engine_android_host");
    }
}
