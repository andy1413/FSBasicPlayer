#include <jni.h>
#include <string>
#include <map>
#include "WZFFPlay.hpp"
std::map<WZFFPlay*, CallJava*> calljavaMap;
_JavaVM *javaVM = NULL;
CallJava *callJava = NULL;
long dashStartTime = 0;

WZFFPlay * getFsPtrFromJava(JNIEnv *env , jobject jobj){
    jclass jlz = env->GetObjectClass(jobj);
    jfieldID fieldId = env->GetFieldID(jlz, "mNativePtr", "J");
    jlong value = env->GetLongField(jobj, fieldId);
    WZFFPlay  *playPtr = (WZFFPlay*)value;
    return playPtr;
}
CallJava* getCallJavaPtrFromVoid(void* ctx){
    CallJava *&pJava = calljavaMap[reinterpret_cast<WZFFPlay*>(ctx)];
    return pJava;
}


void callback_yuv(void *ctx,VideoFrame *videoFrame) {
    CallJava * pJava = getCallJavaPtrFromVoid(ctx);
    if(videoFrame){
        if(videoFrame->format==AV_PIX_FMT_YUV420P){
            if(pJava!=NULL){
                pJava->onCallRenderYUV(videoFrame->width, videoFrame->height,
                                          videoFrame->pixels[0], videoFrame->pixels[1],
                                          videoFrame->pixels[2]);
            }
        }else if(videoFrame->format == AV_PIX_FMT_RGB24){
            if(pJava!=NULL){
                pJava->onCallRenderPic(videoFrame->width, videoFrame->height,
                                          videoFrame->pixels[0]);
            }
        }
    }

}
void setMute(JNIEnv *jniEnv, jobject jobject1, jint flag){
    WZFFPlay * ptrP = getFsPtrFromJava(jniEnv,jobject1);
    if(ptrP!=NULL){
        ptrP->ffplay_setMuted(flag);
    }
}
static const JNINativeMethod playerJaM[] = {
        {"setMute", "(I)V", (void*)setMute},
};
extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    jint result = -1;
    javaVM = vm;
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return result;
    }
    jclass jniClass = env->FindClass("com/wyze/plugin/eventplayer/player/ffmpeg/WyzeMediaPlayerEx");
    env->RegisterNatives(jniClass, playerJaM,sizeof(playerJaM) / sizeof(JNINativeMethod));
    return JNI_VERSION_1_4;
}

void initContactParam (void * c){
    WZFFPlay  *  plPtr = reinterpret_cast<WZFFPlay*>(c);
    plPtr->set_input_format( "concat");
    plPtr->set_format_opt("safe", "0");
    plPtr->set_format_opt("auto_convert","1");
    plPtr->set_format_opt("protocol_whitelist", "file,http,https,crypto,data,tls,tcp");
}

void initDashParam (void * c){
    WZFFPlay  *  plPtr = reinterpret_cast<WZFFPlay*>(c);
    plPtr->start_time = 1000 * dashStartTime;
    plPtr->audio_disable = 0;
    plPtr->opt_sync(NULL, "sync", "video");
}
void playCallBack (void * c , enum  WZFFPlayStatus status){
    CallJava * pJava = getCallJavaPtrFromVoid(c);
    if(pJava!=NULL){
        pJava->onPlayCallBack(static_cast<int>(status));
    }
}
void logCallBack(void *, int level, const char * format, va_list args){
    char log[1024];
    vsnprintf(log, 1024, format, args);
    if(callJava!=NULL){
        callJava->logPrintNative(level,log);
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_wyze_plugin_eventplayer_player_ffmpeg_WyzeMediaPlayerEx_playThumbnail(JNIEnv *env, jobject thiz,
                                                                               jstring filepath) {
    const char *fileUrl = env->GetStringUTFChars(filepath, NULL);
    WZFFPlay  *playPtr = getFsPtrFromJava(env,thiz);
    playPtr->ffplay_main(playPtr, const_cast<char *>(fileUrl), initContactParam, callback_yuv);
    playPtr->statusChangeCallback = playCallBack;
    env->ReleaseStringUTFChars(filepath, fileUrl);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wyze_plugin_eventplayer_player_ffmpeg_WyzeMediaPlayerEx_playVideo(JNIEnv *env, jobject thiz,
                                                                           jstring filepath, jlong startTime) {
    const char *fileUrl = env->GetStringUTFChars(filepath, NULL);
    dashStartTime = startTime;
    WZFFPlay  *playPtr = getFsPtrFromJava(env,thiz);
    playPtr->ffplay_main(playPtr, const_cast<char *>(fileUrl), initDashParam, callback_yuv);
    playPtr->statusChangeCallback = playCallBack;
    env->ReleaseStringUTFChars(filepath, fileUrl);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_wyze_plugin_eventplayer_player_ffmpeg_WyzeMediaPlayerEx_destory(JNIEnv *env, jobject thiz) {
    WZFFPlay  *playPtr = getFsPtrFromJava(env,thiz);
    if(playPtr !=NULL){
        playPtr->ffplay_stream_close();
    }
    dashStartTime = 0;
    if(!calljavaMap.empty()){
        calljavaMap.erase(playPtr);
        CallJava * callJ = calljavaMap[playPtr];
        delete callJ;
        callJ = NULL;
    }
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_wyze_plugin_eventplayer_player_ffmpeg_WyzeMediaPlayerEx_getVideoCurrentPos(JNIEnv *env, jobject thiz) {
    WZFFPlay * ptrP = getFsPtrFromJava(env,thiz);
    if(ptrP!=NULL){
       long mills =  ptrP->ffplay_getTimeIntervalMS();
       return mills;
    }
    return  0;

}
extern "C"
JNIEXPORT void JNICALL
Java_com_wyze_plugin_eventplayer_player_ffmpeg_WyzeMediaPlayerEx_pausePlayer(JNIEnv *env, jobject thiz) {
    WZFFPlay  *playPtr = getFsPtrFromJava(env,thiz);
    if(playPtr!=NULL){
        playPtr->ffplay_pause();
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_wyze_plugin_eventplayer_player_ffmpeg_WyzeMediaPlayerEx_preparePlayer(JNIEnv *env, jobject thiz) {
    WZFFPlay * ffPlay = new WZFFPlay();
    ffPlay->ffplay_stream_close();
    jclass jc = env->GetObjectClass(thiz);
    jfieldID jf = env->GetFieldID(jc, "mNativePtr", "J");
    env->SetLongField(thiz, jf, (jlong)ffPlay);
    fs_log_set_callback(logCallBack);
    callJava = new CallJava(javaVM,env,&thiz);
    calljavaMap[ffPlay]  = callJava;
}