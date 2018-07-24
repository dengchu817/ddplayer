package com.example.dengchu.simple_player;

import android.graphics.Matrix;
import android.graphics.SurfaceTexture;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.TextureView;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    private VideoPlayer player;
    private TextureView mSurfaceView;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        player = new VideoPlayer();
        player.setListener(new VideoPlayer.ddPlayListener() {
            @Override
            public void dd_player_callback(int what, int arg1, int arg2, Object obj) {
                if (what == VideoPlayer.FFP_MSG_VIDEO_SIZE_CHANGED)
                    adjustAspectRatio(arg1,arg2);
            }
        });
        // Example of a call to a native method
        //TextView tv = (TextView) findViewById(R.id.sample_text);
        mSurfaceView = (TextureView)findViewById(R.id.surface_view);
        mSurfaceView.setSurfaceTextureListener(new TextureView.SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(SurfaceTexture surfaceTexture, int i, int i1) {
                player.setSurface(new Surface(surfaceTexture));
            }

            @Override
            public void onSurfaceTextureSizeChanged(SurfaceTexture surfaceTexture, int i, int i1) {

            }

            @Override
            public boolean onSurfaceTextureDestroyed(SurfaceTexture surfaceTexture) {
                return false;
            }

            @Override
            public void onSurfaceTextureUpdated(SurfaceTexture surfaceTexture) {

            }
        });


        player.setDataSource("rtmp://live.hkstv.hk.lxdns.com/live/hks");
        //player.finalize();
    }

    private void adjustAspectRatio(int videoWidth, int videoHeight) {
        int viewWidth = mSurfaceView.getWidth();
        int viewHeight = mSurfaceView.getHeight();
        double aspectRatio = (double) videoHeight / videoWidth;

        int newWidth, newHeight;
        if (viewHeight > (int) (viewWidth * aspectRatio)) {
            // limited by narrow width; restrict height
            newWidth = viewWidth;
            newHeight = (int) (viewWidth * aspectRatio);
        } else {
            // limited by short height; restrict width
            newWidth = (int) (viewHeight / aspectRatio);
            newHeight = viewHeight;
        }
        int xoff = (viewWidth - newWidth) / 2;
        int yoff = (viewHeight - newHeight) / 2;
        Log.v("lhf", "video=" + videoWidth + "x" + videoHeight + " view=" + viewWidth + "x" + viewHeight
                + " newView=" + newWidth + "x" + newHeight + " off=" + xoff + "," + yoff);

        Matrix txform = new Matrix();
        mSurfaceView.getTransform(txform);
        txform.setScale((float) newWidth / viewWidth, (float) newHeight / viewHeight);
        txform.postTranslate(xoff, yoff);
        mSurfaceView.setTransform(txform);
    }
    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
}
