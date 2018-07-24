//
// Created by 邓楚 on 2018/6/27.
//

#include "jvideo_player.h"
#include "jni_utils.h"
#include <android/log.h>
extern JavaVM* g_jvm;
extern fields_t g_fields;
extern AVPacket flush_pkt;

inline static int log_level_av_to_ijk(int av_level) {
    int ijk_level = IJK_LOG_VERBOSE;
    if      (av_level <= AV_LOG_PANIC)      ijk_level = IJK_LOG_FATAL;
    else if (av_level <= AV_LOG_FATAL)      ijk_level = IJK_LOG_FATAL;
    else if (av_level <= AV_LOG_ERROR)      ijk_level = IJK_LOG_ERROR;
    else if (av_level <= AV_LOG_WARNING)    ijk_level = IJK_LOG_WARN;
    else if (av_level <= AV_LOG_INFO)       ijk_level = IJK_LOG_INFO;
        // AV_LOG_VERBOSE means detailed info
    else if (av_level <= AV_LOG_VERBOSE)    ijk_level = IJK_LOG_INFO;
    else if (av_level <= AV_LOG_DEBUG)      ijk_level = IJK_LOG_DEBUG;
    else if (av_level <= AV_LOG_TRACE)      ijk_level = IJK_LOG_VERBOSE;
    else                                    ijk_level = IJK_LOG_VERBOSE;
    return ijk_level;
}

inline static int log_level_ijk_to_av(int ijk_level) {
    int av_level = IJK_LOG_VERBOSE;
    if      (ijk_level >= IJK_LOG_SILENT)   av_level = AV_LOG_QUIET;
    else if (ijk_level >= IJK_LOG_FATAL)    av_level = AV_LOG_FATAL;
    else if (ijk_level >= IJK_LOG_ERROR)    av_level = AV_LOG_ERROR;
    else if (ijk_level >= IJK_LOG_WARN)     av_level = AV_LOG_WARNING;
    else if (ijk_level >= IJK_LOG_INFO)     av_level = AV_LOG_INFO;
        // AV_LOG_VERBOSE means detailed info
    else if (ijk_level >= IJK_LOG_DEBUG)    av_level = AV_LOG_DEBUG;
    else if (ijk_level >= IJK_LOG_VERBOSE)  av_level = AV_LOG_TRACE;
    else if (ijk_level >= IJK_LOG_DEFAULT)  av_level = AV_LOG_TRACE;
    else if (ijk_level >= IJK_LOG_UNKNOWN)  av_level = AV_LOG_TRACE;
    else                                    av_level = AV_LOG_TRACE;
    return av_level;
}

void jvideo_player::ffmpeg_init() {
    avcodec_register_all();
    av_register_all();
    avformat_network_init();
    av_log_set_callback(ffp_log_callback);
    av_log_set_level(AV_LOG_ERROR);
    av_init_packet(&flush_pkt);
    flush_pkt.data = (uint8_t *)&flush_pkt;
}

void jvideo_player::ffp_log_callback(void *ptr, int level, const char *fmt, va_list vl) {
    if (level > av_log_get_level())
        return;

    int ffplv __unused  = log_level_av_to_ijk(level);
    VLOG(ffplv, "=====dc", fmt, vl);
}

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
    player = new av_player(this);
    player->init(play_callback);
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
                    NULL);
            if (msg->data)
                delete msg->data;
            delete msg;
            msg = NULL;
        }
    }
}

void jvideo_player::start(std::string url) {
    player->start(url);
}

void jvideo_player::setsurface(JNIEnv *env, jobject obj) {
    SDL_Vout* vout = player->get_vout();
    SDL_VoutAndroid_SetAndroidSurface(env, vout, obj);
}

int jvideo_player::play_callback(void *user, void *in_data) {
    jvideo_player* obj = (jvideo_player*)user;
    if (obj){
        obj->do_play_callback(in_data);
    }
    return 0;
}

void jvideo_player::do_play_callback(void *in_data) {
    AvMessage* msg = (AvMessage*)in_data;
    if (msg && m_msg_loop){
        m_msg_loop->push(msg);
        __android_log_print(ANDROID_LOG_ERROR, "=====dc","putMessage");
    }
}