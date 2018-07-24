//
// Created by 邓楚 on 2018/7/2.
//


#include "av_video_output.h"
#include "av_frame_queue.h"
#include "av_clock.h"

av_video_output::av_video_output(void* player) {
    m_player = player;
    m_paused = false;
    m_force_refresh = 1;
    m_step = 0;
}

av_video_output::~av_video_output() {

}

void av_video_output::init(dd_callback cbk, FrameQueue *ref) {
    m_cbk = cbk;
    m_frame_queue_ref = ref;
    SDL_SpeedSamplerReset(&m_render_fps);
    m_out = SDL_VoutAndroid_CreateForAndroidSurface();
    init_clock(&m_clock, &ref->pktq->serial);
}

double av_video_output::vp_duration(Frame *vp, Frame *nextvp) {
    if (vp->serial == nextvp->serial) {
        double duration = nextvp->pts - vp->pts;
        if (isnan(duration) || duration <= 0 || duration > 10.0)
            return vp->duration;
        else
            return duration;
    } else {
        return 0.0;
    }
}

double av_video_output::compute_target_delay(double delay) {
    double sync_threshold, diff = 0;

    /* update delay to follow master synchronisation source */
    if (get_master_sync_type((av_player*)m_player) != AV_SYNC_VIDEO_MASTER) {
        /* if video is slave, we try to correct big delays by
           duplicating or deleting a frame */
        diff = get_clock(&m_clock) - get_master_clock((av_player*)m_player);

        /* skip or repeat frame. We take into account the
           delay to compute the threshold. I still don't know
           if it is the best guess */
        sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        /* -- by bbcallen: replace is->max_frame_duration with AV_NOSYNC_THRESHOLD */
        if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD) {
            if (diff <= -sync_threshold)
                delay = FFMAX(0, delay + diff);
            else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
                delay = delay + diff;
            else if (diff >= sync_threshold)
                delay = 2 * delay;
        }
    }

#ifdef FFP_SHOW_AUDIO_DELAY
    av_log(NULL, AV_LOG_TRACE, "video: delay=%0.3f A-V=%f\n",
            delay, -diff);
#endif

    return delay;
}

void av_video_output::do_cycle() {
    m_first_video_frame_rendered = 0;
    double remaining_time = 0.0;
    while (!m_abort_request) {
        if (remaining_time > 0.0)
            av_usleep((int) (int64_t) (remaining_time * 1000000.0));
        remaining_time = REFRESH_RATE;
        if ((!m_paused || m_force_refresh))
            video_refresh(&remaining_time);
    }
}

void av_video_output::video_refresh(double *remaining_time) {
    double time;
    Frame *sp, *sp2;

    ffplayer* ffp = ((av_player*)m_player)->get_ffplayer();

retry:
    if (frame_queue_nb_remaining(m_frame_queue_ref) == 0) {
        // nothing to do, no picture to display in the queue
    } else {
        double last_duration, duration, delay;
        Frame *vp, *lastvp;

        /* dequeue the picture */
        lastvp = frame_queue_peek_last(m_frame_queue_ref);
        vp = frame_queue_peek(m_frame_queue_ref);

        if (vp->serial != m_frame_queue_ref->pktq->serial) {
            frame_queue_next(m_frame_queue_ref);
            goto retry;
        }

        if (lastvp->serial != vp->serial)
            m_frame_timer = av_gettime_relative() / 1000000.0;

        if (m_paused)
            goto display;

        /* compute nominal last_duration */
        last_duration = vp_duration( lastvp, vp);
        delay = compute_target_delay(last_duration);
        av_log(NULL, AV_LOG_ERROR, "video clock last_duration is = %f, delay = %f\n", last_duration, delay);
        time= av_gettime_relative()/1000000.0;
        if (isnan(m_frame_timer) || time < m_frame_timer)
            m_frame_timer = time;
        if (time < m_frame_timer + delay) {
            *remaining_time = FFMIN(m_frame_timer + delay - time, *remaining_time);
            goto display;
        }

        m_frame_timer += delay;
        if (delay > 0 && time - m_frame_timer > AV_SYNC_THRESHOLD_MAX)
            m_frame_timer = time;

        SDL_LockMutex(m_frame_queue_ref->mutex);
        if (!isnan(vp->pts))
            update_video_pts(vp->pts, vp->pos, vp->serial);
        SDL_UnlockMutex(m_frame_queue_ref->mutex);

        if (frame_queue_nb_remaining(m_frame_queue_ref) > 1) {
            Frame *nextvp = frame_queue_peek_next(m_frame_queue_ref);
            duration = vp_duration(vp, nextvp);
            if(!m_step && (ffp->framedrop > 0 || (ffp->framedrop && get_master_sync_type((av_player*)m_player) != AV_SYNC_VIDEO_MASTER)) && time > m_frame_timer + duration) {
                frame_queue_next(m_frame_queue_ref);
                goto retry;
            }
        }

        frame_queue_next(m_frame_queue_ref);
        m_force_refresh = 1;

        //SDL_LockMutex(ffp->is->play_mutex);
        if (m_step && !m_paused) {
            stream_toggle_pause();
        }
       //SDL_UnlockMutex(ffp->is->play_mutex);
    }
    display:
    /* display picture */
    if (!ffp->display_disable && m_force_refresh && m_frame_queue_ref->rindex_shown)
        video_image_display2();

    m_force_refresh = 0;
}

void av_video_output::video_image_display2() {
    Frame *vp;
    Frame *sp = NULL;

    vp = frame_queue_peek_last(m_frame_queue_ref);

    if (vp->bmp) {

        SDL_VoutDisplayYUVOverlay(m_out, vp->bmp);
        int fps = SDL_SpeedSamplerAdd(&m_render_fps, false, "vfps[ffplay]");
        if (!m_first_video_frame_rendered) {
            m_first_video_frame_rendered = 1;
            //ffp_notify_msg1(ffp, FFP_MSG_VIDEO_RENDERING_START);
        }

    }
}

void av_video_output::update_video_pts(double pts, int64_t pos, int serial) {
    set_clock(&m_clock, pts, serial);
    av_log(NULL,AV_LOG_ERROR, "video clock is %f\n",pts);
}

void av_video_output::stream_toggle_pause() {

}

void av_video_output::start() {
    start_thread();
}
