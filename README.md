# WZEventPlayer README

WZEventPlayer is Wyze app event player lib, it used to play event in iOS and Android.

FFplay is ffmpeg official player. It only use for Command Line.

This Lib is change ffplay.c to WZFFPlay.cpp. So it can create multi-player object. And it provide called api for iOS and android. So iOS and android can receive video frame and render by OpenGLES. So it can use ffplay to play video in WYZE iOS and anroid.

## Dependency Libraries

* `SDL` provides audio play and C++ thread manage.
* `FFmpeg` provides ffplay support.

## Directory

* `Common` is secondary modification of the ffplay.
* `iOSCode` is use WZMediaPlayerView to play video. it call WZFFplay.
* `AndroidCode` is android call example. it also call WZFFplay.

## License

This library is based on a secondary modification of the ffplay and all files follows ffplay's LGPL 2.1 license.
Please refer to the LICENSE file for detailed information.
