package com.insbyte.driversyndicate;

/**
 * Created by soap on 23.02.16.
 */
public class EqEngineActivity extends org.libsdl.app.SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] {
                "SDL2",
                "Game"
        };
    }

/*
    static {
        System.loadLibrary("SDL2");
        System.loadLibrary("eqCore");
        System.loadLibrary("Game");
    }
    */
}
