//
// Created by 邓楚 on 2018/6/27.
//

#ifndef SIMPLE_PLAYER_PLAYERDEF_H
#define SIMPLE_PLAYER_PLAYERDEF_H

#include "av_packet_queue.h"

#ifdef __cplusplus
extern "C"{
#endif
#include <libavutil/samplefmt.h>
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <ijksdl_vout.h>

#ifdef __cplusplus
};
#endif

////////////////////////////////////message/////////////////////////
#define VIDEOPLAYER_INFO                        10000
#define VIDEOPLAYER_INFO_BUFFERING_START        10001
#define VIDEOPLAYER_INFO_BUFFERING_END          10002
#define VIDEOPLAYER_INFO_VIDEO_FIRST_RENDERED   10003

#define VIDEOPLAYER_ERROR                       20000
#define VIDEOPLAYER_ERROR_DECODER               20001
#define VIDEOPLAYER_ERROR_NET                   20002
#define VIDEOPLAYER_ERROR_UNKNOW                20003


// #define VIDEO_PICTURE_QUEUE_SIZE 3
#define VIDEO_PICTURE_QUEUE_SIZE_MIN        (3)
#define VIDEO_PICTURE_QUEUE_SIZE_MAX        (16)
#define VIDEO_PICTURE_QUEUE_SIZE_DEFAULT    (VIDEO_PICTURE_QUEUE_SIZE_MIN)
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE_MAX, SUBPICTURE_QUEUE_SIZE))

#define VIDEO_MAX_FPS_DEFAULT 30

//////////////////////////////////option////////////////////////////////////
#define FFP_OPT_CATEGORY_FORMAT 1
#define FFP_OPT_CATEGORY_CODEC  2
#define FFP_OPT_CATEGORY_SWS    3
#define FFP_OPT_CATEGORY_PLAYER 4
#define FFP_OPT_CATEGORY_SWR    5


///////////////////////////////audioout///////////////////////////////////////
typedef struct AudioParams {
    int freq;
    int channels;
    int64_t channel_layout;
    AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
} AudioParams;

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512


////////////////////////////////Frame//////////////////////////////////////////
typedef struct Frame {
    AVFrame *frame;
    AVSubtitle sub;
    int serial;
    double pts;           /* presentation timestamp for the frame */
    double duration;      /* estimated duration of the frame */
    int64_t pos;          /* byte position of the frame in the input file */
#ifdef FFP_MERGE
    SDL_Texture *bmp;
#else
    SDL_VoutOverlay *bmp;
#endif
    int allocated;
    int width;
    int height;
    int format;
    AVRational sar;
    int uploaded;
} Frame;

typedef struct FrameQueue {
    Frame queue[FRAME_QUEUE_SIZE];
    int rindex;
    int windex;
    int size;
    int max_size;
    int keep_last;
    int rindex_shown;
    SDL_mutex *mutex;
    SDL_cond *cond;
    PacketQueue *pktq;
} FrameQueue;

enum {
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

///////////////////////////////clock///////////////////////////////////////////
typedef struct Clock {
    double pts;           /* clock base */
    double pts_drift;     /* clock base minus time at which we updated the clock */
    double last_updated;
    double speed;
    int serial;           /* clock is based on a packet with this serial */
    int paused;
    int *queue_serial;    /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;
#endif //SIMPLE_PLAYER_PLAYERDEF_H
