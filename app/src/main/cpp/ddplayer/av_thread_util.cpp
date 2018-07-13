//
// Created by 邓楚 on 2018/6/28.
//

#include <pthread.h>
#include "av_thread_util.h"

av_thread_util::av_thread_util():m_abort_request(0) {

}

av_thread_util::~av_thread_util() {

}

void av_thread_util::start_thread() {
    pthread_create(&m_thread_handle, NULL, cycle, (void*)this);
}

void* av_thread_util::cycle(void* arg) {
    av_thread_util* pObj = (av_thread_util*)arg;
    if (pObj){
        pObj->do_cycle();
    }
    return NULL;
}

void av_thread_util::stop_thread() {
    m_abort_request = 1;
    if (m_thread_handle){
        void *thread_res;
        pthread_join(m_thread_handle, &thread_res);
        m_thread_handle = NULL;
    }
}