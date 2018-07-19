package com.example.dengchu.simple_player;

import android.support.annotation.Nullable;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceView;

/**
 * Created by dengchu on 2018/6/26.
 */

public class VideoPlayer {
    private final String TAG = "=====dc";
    private String mPath;
    static {
        //System.loadLibrary("ijkj4a");
        //System.loadLibrary("yuv_shared");
        //System.loadLibrary("ijkffmpeg");
        System.loadLibrary("ijksdl");
        System.loadLibrary("native-player");
        _nativeInit();
    }
    private long mNativeContext = 0;
//////////////////////////////////java method///////////////////////////////////////////////////////////////
    public VideoPlayer(){
        _nativeSetup();
    }

    public void setDataSource(String path){
        mPath = path;
        _setDataSource(mPath, null,null);
    }

    public void setSurface(Surface surface){
        _setSurface(surface);
    }

    public void start(){
        _start();
    }

    public void finalize(){
        _nativeFinalize();
    }
//////////////////////////////called by native//////////////////////////////////////////////////////////////
    private void postEventFromNative(int what, int arg1, int arg2, @Nullable Object obj) {
        Log.e(TAG, "postEventFromNative"+what+arg1+arg2);
    }
////////////////////////////native method//////////////////////////////////////////////////////
    private native final void _setDataSource(String path, String[] keys, String[] values);

    private native final void _setSurface(Surface surface);

    private native final void _start();

    private native final void _nativeSetup();

    private static native final void _nativeInit();

    private native final void _nativeFinalize();
}
