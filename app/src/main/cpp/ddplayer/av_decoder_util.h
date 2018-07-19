//
// Created by 邓楚 on 2018/7/4.
//

#ifndef SIMPLE_PLAYER_AV_DECODER_UTIL_H
#define SIMPLE_PLAYER_AV_DECODER_UTIL_H

#ifdef __cplusplus
extern "C"{
#endif

#include <libavcodec/avcodec.h>

#ifdef __cplusplus
};
#endif

#include "av_packet_queue.h"
#include "ffplayer_option.h"
#include "av_player_def.h"

typedef struct Decoder {
    AVPacket pkt;
    PacketQueue *queue;
    AVCodecContext *avctx;
    int pkt_serial;
    int finished;
    int packet_pending;
    SDL_cond *empty_queue_cond;
    int64_t start_pts;
    AVRational start_pts_tb;
    int64_t next_pts;
    AVRational next_pts_tb;
    pthread_t decoder_tid;
} Decoder;

class av_decoder_util {
public:
    av_decoder_util(void* player);
    ~av_decoder_util();

public:
    void init(dd_callback decoder_cb);
    FrameQueue* get_frame_queue(){ return &m_frame_queue;};
    void decoder_init(AVCodecContext *avctx);
    int feed(AVPacket* pkt);

protected:
    int decoder_decode_frame(Decoder *d, AVFrame *frame, AVSubtitle *sub);
protected:
    Decoder m_d;
    dd_callback m_cbk;
    PacketQueue m_packet_queue;
    FrameQueue m_frame_queue;
    void* m_player;
};


#endif //SIMPLE_PLAYER_AV_DECODER_UTIL_H
