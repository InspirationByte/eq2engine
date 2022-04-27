package com.insbyte.driversyndicate;

import android.content.pm.PackageManager;
import android.os.Build;

/**
 * Created by soap on 23.02.16.
 */
public class EqEngineActivity extends org.libsdl.app.SDLActivity{

    private static final int REQUEST_WRITE_PERMISSION = 786;

    @Override
    protected String[] getLibraries() {
        requestPermission();
        return new String[] {
                "SDL2",
                "DriversGame"
        };
    }

    private void requestPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(new String[]{android.Manifest.permission.WRITE_EXTERNAL_STORAGE}, REQUEST_WRITE_PERMISSION);
        }
    }

    protected String[] getArguments() {
        return new String[]{
               // "-norender",
                //"+r_drawHeightFields 0"
        };
    }
}
