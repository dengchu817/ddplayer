//
// Created by 邓楚 on 2018/6/29.
//

#ifndef SIMPLE_PLAYER_AV_READ_THREAD_H
#define SIMPLE_PLAYER_AV_READ_THREAD_H

#include <string>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avstring.h>
#include <libavutil/opt.h>
#include <ijksdl.h>
#ifdef __cplusplus
};
#endif//__cplusplus

#include "av_thread_util.h"
#include "av_player_def.h"

class av_read_input : public av_thread_util {
public:
    av_read_input(void* player);
    ~av_read_input();

public:
    void stream_open(std::string url, AVInputFormat *iformat);
    void seek(int64_t pos);
    void stream_close();
    void init(dd_callback cbk);
    int resume_read();

protected:
    void do_cycle();
private:
    int is_realtime(AVFormatContext *s);
    static int decode_interrupt_cb(void* usr);
private:
    dd_callback m_read_input_cbk;
    void* m_player;
    std::string m_url;
    AVInputFormat* m_iformat;
    int64_t m_seek_pos;
    int m_realtime;

    SDL_mutex *m_wait_mutex;
    SDL_cond *m_continue_read_thread;
};


#endif //SIMPLE_PLAYER_AV_READ_THREAD_H
