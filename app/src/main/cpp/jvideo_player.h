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
    void start(std::string url);
    void release();
    void putMessage(AvMessage* msg);

public:
    void do_cycle();
private:
    jobject m_object;//java对象
    jclass m_class;
    av_message_loop* m_msg_loop;
    av_player* player;
};


#endif //SIMPLE_PLAYER_VIDEOPLAYER_H