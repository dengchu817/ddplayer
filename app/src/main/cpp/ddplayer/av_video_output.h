//
// Created by 邓楚 on 2018/7/2.
//

#ifndef SIMPLE_PLAYER_AV_VIDEO_REFRESH_H
#define SIMPLE_PLAYER_AV_VIDEO_REFRESH_H

#ifdef __cplusplus
extern "C"{
#endif
#include <libavutil/time.h>
#include <android/ijksdl_android.h>

#ifdef __cplusplus
};
#endif
#include "av_player_def.h"
#include "av_thread_util.h"

class av_video_output:av_thread_util {
public:
    av_video_output(void* player);
    ~av_video_output();

public:
    void init(dd_callback cbk, FrameQueue* ref);
    void release();
    Clock* get_av_clock(){ return &m_clock;};
    SDL_Vout* get_vout(){ return m_out;};
    void start();
protected:
    void do_cycle();

private:
    double vp_duration(Frame *vp, Frame *nextvp);
    double compute_target_delay(double delay);
    void video_refresh(double *remaining_time);
    void update_video_pts(double pts, int64_t pos, int serial);
    void stream_toggle_pause();
    void video_image_display2();
private:
    void* m_player;
    dd_callback m_cbk;
    SDL_Vout* m_out;
    FrameQueue* m_frame_queue_ref;
    Clock m_clock;
    bool m_paused;
    int m_force_refresh;
    int64_t m_frame_timer;
    int m_step;
    int m_show_mode;
    SDL_SpeedSampler m_render_fps;
    int m_first_video_frame_rendered;
};

#endif //SIMPLE_PLAYER_AV_VIDEO_REFRESH_H
