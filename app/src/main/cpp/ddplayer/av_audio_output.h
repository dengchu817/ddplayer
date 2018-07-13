//
// Created by 邓楚 on 2018/7/2.
//

#ifndef SIMPLE_PLAYER_AV_AUDIO_OUPUT_H
#define SIMPLE_PLAYER_AV_AUDIO_OUPUT_H


#include "av_thread_util.h"
#include "av_player_def.h"

#ifdef __cplusplus
extern "C"{
#endif
#include <android/ijksdl_android.h>
#include <libswresample/swresample.h>
#include <ijksdl.h>

#ifdef __cplusplus
};
#endif

class av_audio_output {

public:
    av_audio_output(void* player);
    ~av_audio_output();

public:
    void init();
    void release();
    int audio_open(int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate);

private:
    static void sdl_audio_callback(void* arg, Uint8 *stream, int len);
    void do_sdl_audio_callback(Uint8 *stream, int len);
    int audio_decode_frame(int *audio_size, int* audio_clock_serial, int64_t* audio_clock);
    int synchronize_audio( int nb_samples);
private:
    void* m_player;
    SDL_Aout* m_aout;
    FrameQueue m_frame_queue;
    Clock m_clock;
    bool m_muted;
    int m_left_volume;
    int m_right_volume;
    int m_abort_request;
    AudioParams m_audio_src;
    AudioParams m_audio_tgt;
    int m_audio_volume;
    bool m_paused;
    int m_audio_hw_buf_size;
    SwrContext* m_swr_ctx;

    uint8_t *m_audio_buf;
    uint8_t *m_audio_buf1;
    unsigned int m_audio_buf_size;
    unsigned int m_audio_buf1_size;
    int m_audio_buf_index;

};


#endif //SIMPLE_PLAYER_AV_AUDIO_OUPUT_H
