//
// Created by 邓楚 on 2018/7/2.
//

#ifndef SIMPLE_PLAYER_AV_AUDIO_DECODER_H
#define SIMPLE_PLAYER_AV_AUDIO_DECODER_H


#include "av_decoder_util.h"
#include "av_thread_util.h"

class av_audio_decoder : public av_decoder_util,av_thread_util{
public:
    av_audio_decoder(void* player);
    ~av_audio_decoder();

public:
    void do_cycle();
};


#endif //SIMPLE_PLAYER_AV_AUDIO_DECODER_H
