//
// Created by 邓楚 on 2018/7/2.
//

#ifndef SIMPLE_PLAYER_AV_VIDEO_DECODER_H
#define SIMPLE_PLAYER_AV_VIDEO_DECODER_H


#include "av_decoder_util.h"
#include "av_thread_util.h"

#ifdef __cplusplus
extern "C"{
#endif

#include <libavformat/avformat.h>

#ifdef __cplusplus
};
#endif

class av_video_decoder : public av_decoder_util,public av_thread_util{
public:
    av_video_decoder(void* player);
    ~av_video_decoder();

public:
    void start(AVFormatContext* ic, AVStream* st);

protected:
    void do_cycle();

private:
    void alloc_picture(int frame_format);
    int queue_picture( AVFrame *src_frame, double pts, double duration, int64_t pos, int serial);
    int get_video_frame(AVFrame *frame);
private:
    AVFormatContext* m_ic;
    AVStream* m_st;
};


#endif //SIMPLE_PLAYER_AV_VIDEO_DECODER_H
