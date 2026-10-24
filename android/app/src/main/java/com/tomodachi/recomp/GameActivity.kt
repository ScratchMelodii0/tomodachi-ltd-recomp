package com.tomodachi.recomp

import android.os.Bundle
import android.view.WindowManager
import androidx.games.activity.GameActivity

/**
 * Full-screen native rendering activity.
 *
 * Delegates all lifecycle events to the native layer via GameActivity's C callback mechanism
 * (android_main entry point in tomodachi_android.cpp).  The activity itself is kept thin: it
 * only enforces fullscreen flags and keeps the screen on while the game runs.
 */
class GameActivity : GameActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        // Keep screen on and go truly fullscreen before super.onCreate so the native
        // window is already sized correctly when android_main runs.
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        @Suppress("DEPRECATION")
        window.addFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN or
            WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN
        )
        super.onCreate(savedInstanceState)
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        TomodachiNative.onWindowFocusChanged(hasFocus)
    }
}
