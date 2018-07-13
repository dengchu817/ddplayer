//
// Created by 邓楚 on 2018/6/28.
//

#ifndef SIMPLE_PLAYER_THREADUTILS_H
#define SIMPLE_PLAYER_THREADUTILS_H


#include <sys/types.h>

class av_thread_util {
public:
    av_thread_util();
    ~av_thread_util();

public:
    void start_thread();
    void stop_thread();
public:
    virtual void do_cycle() = 0;
public:
    int m_abort_request;
private:
    static void* cycle(void* arg);
private:
    pthread_t m_thread_handle;
};


#endif //SIMPLE_PLAYER_THREADUTILS_H
