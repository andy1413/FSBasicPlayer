package com.wyze.plugin.eventplayer.player.ffmpeg;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.view.SurfaceHolder;

public class WyzeBasicPlayerGLSurfaceView extends GLSurfaceView{

    private WyzeBasicRender wlRender;

    public WyzeBasicPlayerGLSurfaceView(Context context) {
        this(context, null);
    }

    public WyzeBasicPlayerGLSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setEGLContextClientVersion(2);
        wlRender = new WyzeBasicRender(context);
        setRenderer(wlRender);
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

        wlRender.setOnRenderListener(new WyzeBasicRender.OnRenderListener() {
            @Override
            public void onRender() {
                requestRender();
            }
        });
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        super.surfaceDestroyed(holder);
        wlRender.destroy();
    }

    public void setYUVData(int width, int height, byte[] y, byte[] u, byte[] v)
    {
        if(wlRender != null)
        {
            wlRender.setYUVRenderData(width, height, y, u, v);
            requestRender();
        }
    }

    public void setPicRGBData(int width, int height, byte[] rgb)
    {
        if(wlRender != null)
        {
            wlRender.setPicRGBData(width, height, rgb);
            requestRender();
        }
    }
    public WyzeBasicRender getWlRender() {
        return wlRender;
    }
}
