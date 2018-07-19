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

    //ffplayer option
    void set_option(int opt_category, const char *name, const char *value);
    AVDictionary* get_opt_dict(int opt_category);
    ffplayer* get_ffplayer();
    Clock* get_video_clock();
    Clock* get_audio_clock();
    SDL_Vout* get_vout();
private:
    static int dd_callback(void* player, int type, void* in_data, void** out_data);
    int do_dd_callback(int type, void* in_data, void** out_data);
    int stream_component_open(AVFormatContext* st, int stream_index);

public:
    int                 m_av_sync_type;
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
