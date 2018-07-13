//
// Created by 邓楚 on 2018/7/3.
//

#ifndef SIMPLE_PLAYER_AV_CMDUTILS_H
#define SIMPLE_PLAYER_AV_CMDUTILS_H

#ifdef __cplusplus
extern "C"{
#endif

#include <libavformat/avformat.h>
#include <libavutil/opt.h>

#ifdef __cplusplus
};
#endif


int check_stream_specifier(AVFormatContext *s, AVStream *st, const char *spec);
AVDictionary* filter_codec_opts(AVDictionary *opts, enum AVCodecID codec_id,
                            AVFormatContext *s, AVStream *st, AVCodec *codec);
AVDictionary **setup_find_stream_info_opts(AVFormatContext *s,
                                       AVDictionary *codec_opts);



#endif //SIMPLE_PLAYER_AV_CMDUTILS_H
