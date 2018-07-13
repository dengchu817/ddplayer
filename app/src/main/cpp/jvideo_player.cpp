//
// Created by 邓楚 on 2018/6/27.
//

#include "jvideo_player.h"
#include "jni_utils.h"
#include <android/log.h>
extern JavaVM* g_jvm;
extern fields_t g_fields;

jvideo_player::jvideo_player(JNIEnv* env, jobject thiz){
    if (!thiz)
        return;
    jclass classz = env->GetObjectClass(thiz);
    if (!classz)
        return;
    m_class = (jclass)env->NewGlobalRef(classz);
    m_object = env->NewGlobalRef(thiz);
    m_msg_loop = new av_message_loop();
    this->start_thread();

    player = new av_player();
    player->init();
}

jvideo_player::~jvideo_player(){}

void jvideo_player::release(){

    JavaVM* vm = g_jvm;
    JNIEnv* env = NULL;
    if (vm->GetEnv((void**)&env,JNI_VERSION_1_4) != JNI_OK)
        vm->AttachCurrentThread((JNIEnv**)&env, NULL);
    if (m_msg_loop)
        m_msg_loop->quit();
    this->stop_thread();
    if (m_msg_loop) {
        m_msg_loop->release();
        delete m_msg_loop;
        m_msg_loop = NULL;
    }
    env->DeleteGlobalRef(m_object);
    env->DeleteGlobalRef(m_class);
}

void jvideo_player::putMessage(AvMessage* msg) {
    if (m_msg_loop)
        m_msg_loop->push(msg);
    __android_log_print(ANDROID_LOG_ERROR, "=====dc","putMessage");
}

void jvideo_player::do_cycle() {

    JavaVM* vm = g_jvm;
    JNIEnv* env = NULL;
    if (vm->GetEnv((void**)&env,JNI_VERSION_1_4) != JNI_OK)
        vm->AttachCurrentThread((JNIEnv**)&env, NULL);
    while(m_abort_request == 0) {
        AvMessage* msg = m_msg_loop->pop(true);

        if (msg) {
            env->CallVoidMethod(
                    m_object,
                    g_fields.postEventFromNativeID,
                    msg->what,
                    msg->arg1,
                    msg->arg2,
                    msg->obj);

            env->DeleteGlobalRef(msg->obj);
            delete msg;
            msg = NULL;
        }
    }
}

void jvideo_player::start(std::string url) {
    player->start(url);
}
