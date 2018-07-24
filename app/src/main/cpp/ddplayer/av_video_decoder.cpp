//
// Created by 邓楚 on 2018/7/2.
//

#include "av_video_decoder.h"
#include "av_clock.h"
#include "av_frame_queue.h"
#include "av_message.h.h"
#include "av_message_loop.h"

av_video_decoder::av_video_decoder(void* player) :av_decoder_util(player){

}

av_video_decoder::~av_video_decoder() {

}

void av_video_decoder::start(AVFormatContext* ic, AVStream* st) {
    m_st = st;
    m_ic = ic;
    packet_queue_start(&m_packet_queue);
    start_thread();
}

int av_video_decoder::get_video_frame(AVFrame *frame) {
    int got_picture;

    if ((got_picture = decoder_decode_frame(&m_d, frame, NULL)) < 0)
        return -1;

    if (got_picture) {

        frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(m_ic, m_st, frame);
    }

    return got_picture;
}

void av_video_decoder::alloc_picture(int frame_format) {
    Frame *vp;
    vp = &m_frame_queue.queue[m_frame_queue.windex];
    free_picture(vp);
    SDL_Vout* vout = NULL;
    m_cbk(m_player, DD_GET_VIDEO_SDL_OUT, NULL, (void**)&vout);
    ffplayer* ffp = ((av_player*)m_player)->get_ffplayer();
    if (!ffp || !vout)
        return;
    SDL_VoutSetOverlayFormat(vout, ffp->overlay_format);
    vp->bmp = SDL_Vout_CreateOverlay(vp->width, vp->height,
                                     frame_format,
                                     vout);

    /* RV16, RV32 contains only one plane */
    if (!vp->bmp || (!vp->bmp->is_private && vp->bmp->pitches[0] < vp->width)) {
        /* SDL allocates a buffer smaller than requested if the video
         * overlay hardware is unable to support the requested size. */
        av_log(NULL, AV_LOG_FATAL,
               "Error: the video system does not support an image\n"
                       "size of %dx%d pixels. Try using -lowres or -vf \"scale=w:h\"\n"
                       "to reduce the image size.\n", vp->width, vp->height );
        free_picture(vp);
    }

    SDL_LockMutex(m_frame_queue.mutex);
    vp->allocated = 1;
    SDL_CondSignal(m_frame_queue.cond);
    SDL_UnlockMutex(m_frame_queue.mutex);
}

int av_video_decoder::queue_picture( AVFrame *src_frame, double pts, double duration, int64_t pos, int serial) {
    Frame *vp;

    if (!(vp = frame_queue_peek_writable(&m_frame_queue)))
        return -1;

    vp->sar = src_frame->sample_aspect_ratio;

    /* alloc or resize hardware picture buffer */
    if (!vp->bmp || !vp->allocated ||
        vp->width  != src_frame->width ||
        vp->height != src_frame->height ||
        vp->format != src_frame->format) {

        if (vp->width != src_frame->width || vp->height != src_frame->height){
            AvMessage* msg = new AvMessage(FFP_MSG_VIDEO_SIZE_CHANGED, src_frame->width,src_frame->height,
                                           nullptr);
            m_cbk(m_player, DD_PUT_MESSAGE, msg, NULL);
        }

        vp->allocated = 0;
        vp->width = src_frame->width;
        vp->height = src_frame->height;
        vp->format = src_frame->format;

        /* the allocation must be done in the main thread to avoid
           locking problems. */
        alloc_picture(src_frame->format);

        if (m_packet_queue.abort_request)
            return -1;
    }

    /* if the frame is not skipped, then display it */
    if (vp->bmp) {
        /* get a pointer on the bitmap */
        SDL_VoutLockYUVOverlay(vp->bmp);

        // FIXME: set swscale options
        if (SDL_VoutFillFrameYUVOverlay(vp->bmp, src_frame) < 0) {
            av_log(NULL, AV_LOG_FATAL, "Cannot initialize the conversion context\n");
            exit(1);
        }
        /* update the bitmap content */
        SDL_VoutUnlockYUVOverlay(vp->bmp);

        vp->pts = pts;
        vp->duration = duration;
        vp->pos = pos;
        vp->serial = serial;
        vp->sar = src_frame->sample_aspect_ratio;
        vp->bmp->sar_num = vp->sar.num;
        vp->bmp->sar_den = vp->sar.den;

        frame_queue_push(&m_frame_queue);
    }
    return 0;
}

void av_video_decoder::do_cycle() {
    AVFrame *frame = av_frame_alloc();
    double pts;
    double duration;
    int ret;
    AVRational tb = m_st->time_base;
    AVRational frame_rate = av_guess_frame_rate(m_ic, m_st, NULL);
    if (!frame) {
        return;
    }

    for (;;) {
        ret = get_video_frame(frame);
        if (ret < 0)
            goto the_end;
        if (!ret)
            continue;

        duration = (frame_rate.num && frame_rate.den ? av_q2d((AVRational){frame_rate.den, frame_rate.num}) : 0);
        pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
        ret = queue_picture(frame, pts, duration, frame->pkt_pos, m_d.pkt_serial);
        av_frame_unref(frame);

        if (ret < 0)
            goto the_end;
    }
the_end:
    av_frame_free(&frame);
    return;
}
