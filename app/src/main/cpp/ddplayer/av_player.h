//
// Created by 邓楚 on 2018/6/28.
//

#ifndef SIMPLE_PLAYER_AVPLAYER_H
#define SIMPLE_PLAYER_AVPLAYER_H

#include <string>
#include "av_thread_util.h"
#include "av_read_input.h"
#include "ffplayer_option.h"
#include "av_audio_output.h"
#include "av_audio_decoder.h"
#include "av_video_decoder.h"
#include "av_video_output.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#ifdef __cplusplus
};
#endif//__cplusplus

class av_player{
public:
    av_player();
    ~av_player();
public:
    int init();
    int release();

    void start(std::string url);
    //find_stream_info callback
    static int find_stream_cbk(void* usr, AVFormatContext* ic, int stream_index);

    //av_read_frame callback
    static int read_frame_cbk(void* usr, AVPacket* st);

    void set_option(int opt_category, const char *name, const char *value);
    AVDictionary* get_opt_dict(int opt_category);

    ffplayer* get_ffplayer();
    PacketQueue* get_audio_packetqueue();
private:
    int stream_component_open(AVFormatContext* ic, int stream_index);
    int put_read_packet(AVPacket* st);
private:
    std::string         m_url;
    av_read_input*      m_read_in;

    av_audio_decoder*   m_audio_decoder;
    av_audio_output*    m_audio_out;
    av_video_decoder*   m_video_decoder;
    av_video_output*    m_video_out;


    ffplayer*    m_ffp;
    /* format/codec options */
    AVDictionary *m_format_opts;
    AVDictionary *m_codec_opts;
    AVDictionary *m_sws_dict;
    AVDictionary *m_player_opts;
    AVDictionary *m_swr_opts;
};


#endif //SIMPLE_PLAYER_AVPLAYER_H
