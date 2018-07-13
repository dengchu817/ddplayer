//
// Created by 邓楚 on 2018/6/27.
//

#ifndef SIMPLE_PLAYER_AVMESSAGE_H
#define SIMPLE_PLAYER_AVMESSAGE_H


#include <jni.h>
#include <sys/types.h>
#include <queue>

struct AvMessage{
    int what;
    int arg1;
    int arg2;
    jobject obj;
};

class av_message_loop {
public:
    av_message_loop();
    ~av_message_loop();
    AvMessage* pop(bool wait);
    void push(AvMessage* msg);
    void quit();
    void release();
private:
    int m_state;
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
    std::queue<AvMessage*> m_msg_queue;
};


#endif //SIMPLE_PLAYER_AVMESSAGE_H
