package com.wyze.plugin.eventplayer.player.ffmpeg;

import static com.wyze.plugin.eventplayer.player.ffmpeg.PlayerStatus.PLAYSTATUS_COMPLETED;
import static com.wyze.plugin.eventplayer.player.ffmpeg.PlayerStatus.PLAYSTATUS_ONSTOP;
import static com.wyze.plugin.eventplayer.player.ffmpeg.PlayerStatus.PLAYSTATUS_REBDERSTART;
import org.libsdl.app.SDL;

/**
 * create on 2023/8/1
 */
public class WyzeMediaPlayer {
    static {
        System.loadLibrary("wyzeplayer");
        System.loadLibrary("SDL2");
        SDL.setupJNI();
    }
    public WyzeMediaPlayer(){
    }
    private WyzeBasicPlayerGLSurfaceView glSurfaceView = null;
    private int surfaceViewW;
    private int surfaceViewH;
    private PlayListener playListener;
    public boolean isPlaying;
    public boolean isPause;
    public long mNativePtr = 0;
    public void pause(){
        isPause = true;
        pausePlayer();
    }

    public void resume(){
        isPause = false;
        pausePlayer();
    }

    public void destroyPlayer() {
        glSurfaceView = null;
        playListener = null;
        stopPlayer();
    }

    public void playPicList(String filepath) {
        isPause = false;
        preparePlayer();
        playThumbnail(filepath);
    }
    public native void setMute(int flag);
    public void play(String url) {
        play(url, 0);
    }

    public void play(String url, long startTime) {
        videoCurrentPos = 0;
        preparePlayer();
        playVideo(url, startTime);
    }
    private native void preparePlayer();

    private native void playThumbnail(String filepath);

    private native void playVideo(String url, long statTime);

    private native long getVideoCurrentPos();
    private native void pausePlayer();

    public void stopPlayer() {
        destory();
        isPlaying = false;
        if(playListener!=null){
            playListener.onPlayState(PLAYSTATUS_ONSTOP);
        }
    }

    private native void destory();

    public void setGlSurfaceView(WyzeBasicPlayerGLSurfaceView glSurfaceView) {
        this.glSurfaceView = glSurfaceView;
    }
    public void setOnPlayListener(PlayListener playListener) {
        this.playListener = playListener;
    }

    private void auto(int w, int h) {
        if (this.surfaceViewW != w || this.surfaceViewH != h) {
            if(playListener!=null){
                playListener.onSizeChange(w,h);
            }
            this.surfaceViewW = w;
            this.surfaceViewH = h;
        }
    }

    long videoCurrentPos = 0;

    public void onCallRenderPic(int w, int h, byte[] pixels) {
            glSurfaceView.getWlRender().setRenderType(WyzeBasicRender.RENDER_PIC);
            glSurfaceView.setPicRGBData(w, h, pixels);
            auto(w, h);
    }

    public interface PlayListener {
        void onPlayState(int code);

        void onPlayPicIndex(int index);

        void onPlayProgress(long currentTime);

        void onSizeChange(int w, int h);

    }

}
