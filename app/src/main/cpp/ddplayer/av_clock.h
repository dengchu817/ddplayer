//
// Created by 邓楚 on 2018/7/11.
//

#ifndef SIMPLE_PLAYER_AV_CLOCK_H
#define SIMPLE_PLAYER_AV_CLOCK_H

#ifdef __cplusplus
extern "C"{
#endif

#include <libavutil/time.h>

#ifdef __cplusplus
};
#endif

#include "av_player_def.h"

double get_clock(Clock *c);

void set_clock_at(Clock *c, double pts, int serial, double time);

void set_clock(Clock *c, double pts, int serial);

void set_clock_speed(Clock *c, double speed);

void init_clock(Clock *c, int *queue_serial);

#endif //SIMPLE_PLAYER_AV_CLOCK_H
