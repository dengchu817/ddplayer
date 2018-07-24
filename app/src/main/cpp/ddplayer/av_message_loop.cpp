//
// Created by 邓楚 on 2018/6/27.
//

#include <pthread.h>
#include "av_message_loop.h"

av_message_loop::av_message_loop():m_state(0){
    pthread_mutex_init(&m_mutex, NULL);
    pthread_cond_init(&m_cond, NULL);
}

AvMessage* av_message_loop::pop(bool wait) {
    pthread_mutex_lock(&m_mutex);
    AvMessage *msg = NULL;
    while (true) {
        if (m_state > 0) {
            msg = NULL;
            break;
        }
        if (m_msg_queue.size() > 0) {
            msg = m_msg_queue.front();
            m_msg_queue.pop();
            break;
        } else if (wait) {
            pthread_cond_wait(&m_cond, &m_mutex);
        } else{
            msg = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&m_mutex);
    return msg;
}

void av_message_loop::push(AvMessage *msg) {
    pthread_mutex_lock(&m_mutex);
    m_msg_queue.push(msg);
    pthread_cond_signal(&m_cond);
    pthread_mutex_unlock(&m_mutex);
}

void av_message_loop::quit(){
    pthread_mutex_lock(&m_mutex);
    m_state = 1;
    pthread_cond_signal(&m_cond);
    pthread_mutex_unlock(&m_mutex);
}

void av_message_loop::release(){
    pthread_mutex_lock(&m_mutex);
    while(m_msg_queue.size() > 0){
        AvMessage* msg = m_msg_queue.front();
        m_msg_queue.pop();
        if (msg && msg->data){
            delete msg->data;
            delete msg;
            msg = NULL;
        }
    }
    pthread_mutex_unlock(&m_mutex);
}

av_message_loop::~av_message_loop() {
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
}