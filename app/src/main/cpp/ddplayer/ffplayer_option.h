//
// Created by 邓楚 on 2018/7/2.
//

#ifndef SIMPLE_PLAYER_FFPLAYER_OPTION_H
#define SIMPLE_PLAYER_FFPLAYER_OPTION_H

#ifdef __cplusplus
extern "C"{
#endif
#include <libavutil/log.h>

#ifdef __cplusplus
};
#endif

struct ffplayer{

    ffplayer(){
        decoder_reorder_pts = -1;
        opensles = false;
        pf_playback_rate_changed = 0;
        pf_playback_rate = 1.0f;
        pf_playback_volume_changed = 0;
        pf_playback_volume = 1.0f;
    }

    const AVClass *av_class;
    int audio_disable;
    int video_disable;
    int display_disable;
    int fast;
    int skip_calc_frame_rate;
    char *iformat_name;
    int genpts;
    int find_stream_info;
    int decoder_reorder_pts;
    bool opensles;
    int pf_playback_rate_changed;
    float pf_playback_rate;
    int pf_playback_volume_changed;
    float pf_playback_volume;

};

#endif //SIMPLE_PLAYER_FFPLAYER_OPTION_H
