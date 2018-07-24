//
// Created by 邓楚 on 2018/6/27.
//

#ifndef SIMPLE_PLAYER_VIDEOPLAYER_H
#define SIMPLE_PLAYER_VIDEOPLAYER_H

#include <jni.h>
#include "pthread.h"
#include "ddplayer/av_message_loop.h"
#include "ddplayer/av_thread_util.h"
#include "ddplayer/av_player.h"
#include <queue>
class jvideo_player : public av_thread_util{

public:
    jvideo_player(JNIEnv* env, jobject thiz);
    ~jvideo_player();

public:
    static void ffmpeg_init();
    void start(std::string url);
    void setsurface(JNIEnv* env, jobject obj);
    void release();
private:
    void do_cycle();
    static int play_callback(void* user, void* in_data);
    void do_play_callback(void* in_data);
    static void ffp_log_callback(void *ptr, int level, const char *fmt, va_list vl);
private:
    jobject m_object;//java对象
    jclass m_class;
    av_message_loop* m_msg_loop;
    av_player* player;
};

#endif //SIMPLE_PLAYER_VIDEOPLAYER_H
