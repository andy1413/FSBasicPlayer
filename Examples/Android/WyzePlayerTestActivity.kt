package com.wyze.plugin.camplus.thumbnail

import android.app.Activity
import android.os.Bundle
import android.os.PersistableBundle
import com.wyze.plugin.basicplayer.WyzeBasicPlayerGLSurfaceView
import com.wyze.plugin.basicplayer.WyzePlayer

/**
 * create on 2024/7/1
 */
class WyzePlayerTestActivity : Activity(), WyzePlayer.PlayListener {
    var player = WyzeMediaPlayer()
    override fun onCreate(savedInstanceState: Bundle?, persistentState: PersistableBundle?) {
        super.onCreate(savedInstanceState, persistentState)
        var glSurface = WyzeBasicPlayerGLSurfaceView(this)
        player.setGlSurfaceView(glSurface)
        player.play("http://...")
        player.setOnPlayListener(this)
        player.setMute(0)
    }

    override fun onDestroy() {
        super.onDestroy()
        player.destroyPlayer()
    }
    override fun onPlayState(p0: Int) {
    }

    override fun onPlayPicIndex(p0: Int) {
    }

    override fun onPlayProgress(p0: Long) {
    }

    override fun onSizeChange(p0: Int, p1: Int) {
    }

}
