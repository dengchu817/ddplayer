//
// Created by 邓楚 on 2018/7/2.
//

#include "av_audio_decoder.h"
#include "av_player_def.h"
#include "av_frame_queue.h"
#include "av_player.h"

av_audio_decoder::av_audio_decoder(void* player):av_decoder_util(player) {

}

av_audio_decoder::~av_audio_decoder() {

}

void av_audio_decoder::do_cycle() {
    AVFrame *frame = av_frame_alloc();
    Frame *af;

    int got_frame = 0;
    AVRational tb;
    int ret = 0;

    if (!frame)
        return;

    do {
        if ((got_frame = decoder_decode_frame(&m_d, frame, NULL)) < 0)
            goto the_end;
        if (got_frame) {
            tb = (AVRational) {1, frame->sample_rate};
            if (!(af = frame_queue_peek_writable(&m_frame_queue)))
                goto the_end;

            af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
            af->pos = frame->pkt_pos;
            af->serial = m_d.pkt_serial;
            af->duration = av_q2d((AVRational){frame->nb_samples, frame->sample_rate});

            av_frame_move_ref(af->frame, frame);
            frame_queue_push(&m_frame_queue);
        }

    } while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
    the_end:

    av_frame_free(&frame);
    return;
}

void av_audio_decoder::start() {
    packet_queue_start(&m_packet_queue);
    start_thread();
}
