//
// Created by 邓楚 on 2018/6/26.
//

#include <jni.h>
#include <assert.h>
#include "jvideo_player.h"
#include "jni_utils.h"

#define JNI_CLASS_VIDEOPLAYER     "com/example/dengchu/simple_player/VideoPlayer"
#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

JavaVM* g_jvm = NULL;
fields_t g_fields;


static void setVideoPlay(JNIEnv* env, jobject thiz, jvideo_player* player){
    jvideo_player* old = (jvideo_player*)env->GetLongField(thiz, g_fields.context);
    if (old != NULL){
        old->release();
        delete old;
    }
    env->SetLongField(thiz, g_fields.context, (jlong)player);
}

static jvideo_player* getVideoPlayer(JNIEnv* env, jobject thiz){
    return (jvideo_player*)env->GetLongField(thiz, g_fields.context);
}

static void Jni_setSurface(JNIEnv* env, jobject thiz, jobject obj){
    jvideo_player* player = getVideoPlayer(env, thiz);
    if (player) {
        player->setsurface(env, obj);
    }
}

static void Jni_setDataSource(JNIEnv* env, jobject thiz, jstring path, jobjectArray keys, jobjectArray values){
    jvideo_player* player = getVideoPlayer(env, thiz);
    if (player){
        const char* c_path = env->GetStringUTFChars(path, NULL);
        player->start(c_path);
        env->ReleaseStringUTFChars(path, c_path);
    }
}

static void Jni_start(JNIEnv* env, jobject thiz){
    jvideo_player* player = getVideoPlayer(env, thiz);
    AvMessage* msg = new AvMessage;
    memset(msg, 0, sizeof(AvMessage));
    player->putMessage(msg);
}

static void Jni_nativeSetup(JNIEnv* env, jobject thiz){
    jvideo_player* player = new jvideo_player(env, thiz);
    setVideoPlay(env, thiz, player);
}

static void Jni_nativeInit(JNIEnv *env){
    jclass clazz = (env)->FindClass(JNI_CLASS_VIDEOPLAYER);
    if (!clazz)
        return;
    g_fields.context = env->GetFieldID(clazz, "mNativeContext", "J");
    if (!g_fields.context)
        return;
    g_fields.postEventFromNativeID = env->GetMethodID(clazz, "postEventFromNative", "(IIILjava/lang/Object;)V");
    if (!g_fields.postEventFromNativeID)
        return;
}

static void Jni__nativeFinalize(JNIEnv* env, jobject thiz){
    setVideoPlay(env, thiz, NULL);
}

static JNINativeMethod g_methods[] = {
        {"_setDataSource", "(Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;)V", (void *) Jni_setDataSource},
        {"_setSurface", "(Landroid/view/Surface;)V", (void*) Jni_setSurface},
        {"_start", "()V", (void*)Jni_start},
        {"_nativeSetup","()V", (void*)Jni_nativeSetup},
        {"_nativeInit", "()V", (void*)Jni_nativeInit},
        {"_nativeFinalize", "()V", (void*) Jni__nativeFinalize},
};

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    g_jvm = vm;
    JNIEnv* env = NULL;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }
    assert(env != NULL);
    jclass clazz = (env)->FindClass(JNI_CLASS_VIDEOPLAYER);
    if (!clazz)
        return -1;
    env->RegisterNatives(clazz, g_methods, NELEM(g_methods));

    return JNI_VERSION_1_4;
}