package com.example.dengchu.simple_player;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.Nullable;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceView;


/**
 * Created by dengchu on 2018/6/26.
 */

public class VideoPlayer {

    public static int FFP_MSG_VIDEO_SIZE_CHANGED =         400;
    private final String TAG = "=====dc";
    private String mPath;
    private ddPlayListener mListener;
    private EvenHander mEventHandler;

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
        mEventHandler = new EvenHander(Looper.getMainLooper());
    }

    public void setListener(ddPlayListener listener){
        mListener = listener;
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

    interface ddPlayListener{
        void dd_player_callback(int what, int arg1, int arg2, Object obj);
    }

    private class EvenHander extends Handler {
        public EvenHander (Looper looper){
            super(looper);
        }

        @Override
        public void handleMessage(Message msg){
            mListener.dd_player_callback(msg.what, msg.arg1, msg.arg2, msg.obj);
        }
    }
//////////////////////////////called by native//////////////////////////////////////////////////////////////
    private void postEventFromNative(int what, int arg1, int arg2, Object obj) {
        if(mListener != null){
            Message m = mEventHandler.obtainMessage(what, arg1, arg2, obj);
            mEventHandler.sendMessage(m);
        }
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
