//
// Created by 邓楚 on 2018/7/3.
//

#ifndef SIMPLE_PLAYER_AV_PACKET_QUEUE_H
#define SIMPLE_PLAYER_AV_PACKET_QUEUE_H

#ifdef __cplusplus
extern "C"{
#endif

#include <libavcodec/avcodec.h>
#include <ijksdl.h>

#ifdef __cplusplus
};
#endif

typedef struct MyAVPacketList {
    AVPacket pkt;
    struct MyAVPacketList *next;
    int serial;
} MyAVPacketList;

typedef struct PacketQueue {
    MyAVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    int64_t duration;
    int abort_request;
    int serial;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;
int packet_queue_put(PacketQueue *q, AVPacket *pkt);
int packet_queue_put_nullpacket(PacketQueue *q, int stream_index);
int packet_queue_init(PacketQueue *q);
void packet_queue_flush(PacketQueue *q);
void packet_queue_destroy(PacketQueue *q);
void packet_queue_abort(PacketQueue *q);
void packet_queue_start(PacketQueue *q);
int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial);
#endif //SIMPLE_PLAYER_AV_PACKET_QUEUE_H
