//
// Created by 邓楚 on 2018/7/5.
//

#ifndef SIMPLE_PLAYER_AV_FRAME_QUEUE_H
#define SIMPLE_PLAYER_AV_FRAME_QUEUE_H

#include "av_player_def.h"

void frame_queue_unref_item(Frame *vp);

int frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last);

void frame_queue_destory(FrameQueue *f);

void frame_queue_signal(FrameQueue *f);

Frame *frame_queue_peek(FrameQueue *f);

Frame *frame_queue_peek_next(FrameQueue *f);

Frame *frame_queue_peek_last(FrameQueue *f);

Frame *frame_queue_peek_writable(FrameQueue *f);

Frame *frame_queue_peek_readable(FrameQueue *f);

void frame_queue_push(FrameQueue *f);

void frame_queue_next(FrameQueue *f);

/* return the number of undisplayed frames in the queue */
int frame_queue_nb_remaining(FrameQueue *f);

/* return last shown position */
int64_t frame_queue_last_pos(FrameQueue *f);

void free_picture(Frame *vp);

#endif //SIMPLE_PLAYER_AV_FRAME_QUEUE_H

