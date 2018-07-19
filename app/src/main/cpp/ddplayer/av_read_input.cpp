//
// Created by 邓楚 on 2018/6/29.
//
#include "av_cmdutils.h"
#include "av_read_input.h"
#include "av_player.h"
#include "av_player_def.h"

av_read_input::av_read_input(void* player) {
    m_player = player;
    m_read_input_cbk = NULL;
    m_iformat = NULL;
    m_url = "";
    m_seek_pos = 0;
    m_realtime = 0;
    m_wait_mutex = SDL_CreateMutex();
    m_continue_read_thread = SDL_CreateCond();
}

av_read_input::~av_read_input() {

}

void av_read_input::init(dd_callback cbk) {
    m_read_input_cbk = cbk;
}

int av_read_input::is_realtime(AVFormatContext *s) {
    if(   !strcmp(s->iformat->name, "rtp")
          || !strcmp(s->iformat->name, "rtsp")
          || !strcmp(s->iformat->name, "sdp")
            )
        return 1;

    if(s->pb && (   !strncmp(s->filename, "rtp:", 4)
                    || !strncmp(s->filename, "udp:", 4)
    )
            )
        return 1;
    return 0;
}

int av_read_input::decode_interrupt_cb(void* usr) {
    av_read_input* pobj = (av_read_input*)usr;
    if (pobj){
        return pobj->m_abort_request;
    }
    return 0;
}

void av_read_input::do_cycle() {
    int err, i, ret;
    int64_t stream_start_time;
    const char* filename = m_url.data();
    AVFormatContext *ic = NULL;
    int st_index[AVMEDIA_TYPE_NB];
    AVPacket pkt1, *pkt = &pkt1;
    int pkt_in_play_range = 1;
    AVDictionaryEntry *t = NULL;
    AVDictionary *format_opts = NULL, *codec_opts = NULL;
    int seek_by_bytes = -1;
    int video_stream =-1, audio_stream = -1, eof = 0;
    int scan_all_pmts_set = 0;
    int64_t pkt_ts;
    int seek_flags = 0;
    if (!m_read_input_cbk || !m_player)
        goto fail;

    format_opts = ((av_player *) m_player)->get_opt_dict(FFP_OPT_CATEGORY_FORMAT);
    codec_opts = ((av_player *) m_player)->get_opt_dict(FFP_OPT_CATEGORY_CODEC);
    memset(st_index, -1, sizeof(st_index));
    ic = avformat_alloc_context();
    if (!ic) {
        av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
        goto fail;
    }
    ic->interrupt_callback.callback = decode_interrupt_cb;
    ic->interrupt_callback.opaque = this;

    if (!av_dict_get(format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
        av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
        scan_all_pmts_set = 1;
    }
    if (av_stristart(filename, "rtmp", NULL) ||
        av_stristart(filename, "rtsp", NULL)) {
        // There is total different meaning for 'timeout' option in rtmp
        av_log(NULL, AV_LOG_WARNING, "remove 'timeout' option for rtmp.\n");
        av_dict_set(&format_opts, "timeout", NULL, 0);
    }

    err = avformat_open_input(&ic,filename, m_iformat, &format_opts);
    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_open_input failed filename:%s, err:%d" ,filename, err);
        goto fail;
    }
    //ffp_notify_msg1(ffp, FFP_MSG_OPEN_INPUT);
    if (scan_all_pmts_set)
        av_dict_set(&format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);

    if ((t = av_dict_get(format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
        av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
        goto fail;
    }

    av_format_inject_global_side_data(ic);

    {
        AVDictionary **opts = setup_find_stream_info_opts(ic, codec_opts);
        int orig_nb_streams = ic->nb_streams;

        err = avformat_find_stream_info(ic, opts);

        for (i = 0; i < orig_nb_streams; i++)
            av_dict_free(&opts[i]);
        av_freep(&opts);

        if (err < 0) {
            av_log(NULL, AV_LOG_WARNING,
                   "%s: could not find codec parameters\n", filename);
            goto fail;
        }
    }

    if (ic->pb)
        ic->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end

    if (seek_by_bytes < 0)
        seek_by_bytes = !!(ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", ic->iformat->name);

    m_realtime = is_realtime(ic);
    av_dump_format(ic, 0, filename, 0);

    st_index[AVMEDIA_TYPE_VIDEO] =
            av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO,
                                st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);

    st_index[AVMEDIA_TYPE_AUDIO] =
            av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO,
                                st_index[AVMEDIA_TYPE_AUDIO],
                                st_index[AVMEDIA_TYPE_VIDEO],
                                NULL, 0);

    st_index[AVMEDIA_TYPE_SUBTITLE] =
            av_find_best_stream(ic, AVMEDIA_TYPE_SUBTITLE,
                                st_index[AVMEDIA_TYPE_SUBTITLE],
                                (st_index[AVMEDIA_TYPE_AUDIO] >= 0 ?
                                 st_index[AVMEDIA_TYPE_AUDIO] :
                                 st_index[AVMEDIA_TYPE_VIDEO]),
                                NULL, 0);


    /* open the streams */

    if (st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
        ic->opaque = &st_index[AVMEDIA_TYPE_AUDIO];
        m_read_input_cbk(m_player, DD_FIND_AUDIO_STREAM, ic, NULL);
        audio_stream = st_index[AVMEDIA_TYPE_AUDIO];
    }

    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
        ic->opaque = &st_index[AVMEDIA_TYPE_VIDEO];
        m_read_input_cbk(m_player, DD_FIND_VIDEO_STREAM, ic, NULL);
        video_stream = st_index[AVMEDIA_TYPE_VIDEO];
    }

    if (video_stream < 0 && audio_stream < 0) {
        goto fail;
    }

    for(;;){
        if (m_abort_request)
            break;

        if (m_seek_pos) {
            int64_t seek_target = m_seek_pos;
            int64_t seek_min    = INT64_MIN;
            int64_t seek_max    = INT64_MAX;

            seek_flags &= ~AVSEEK_FLAG_BYTE;
            if (seek_by_bytes)
                seek_flags |= AVSEEK_FLAG_BYTE;

            ret = avformat_seek_file(ic, -1, seek_min, seek_target, seek_max, seek_flags);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR,
                       "%s: error while seeking\n", ic->filename);
            } else {
                if (audio_stream >= 0) {
                    //packet_queue_flush(&is->audioq);
                    //packet_queue_put(&is->audioq, &flush_pkt);
                }

                if (video_stream >= 0) {
                    //packet_queue_flush(&is->videoq);
                    //packet_queue_put(&is->videoq, &flush_pkt);
                }
//                if (seek_flags & AVSEEK_FLAG_BYTE) {
//                    set_clock(&is->extclk, NAN, 0);
//                } else {
//                    set_clock(&is->extclk, seek_target / (double)AV_TIME_BASE, 0);
//                }
            }

            eof = 0;
//            if (is->paused)
//                step_to_next_frame(is);
        }

        ret = av_read_frame(ic, pkt);
        if (ret < 0) {
            if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !eof) {
                if (video_stream >= 0)
                    m_read_input_cbk(m_player, DD_READ_VIDEO_FRAME, pkt, NULL);
                    //packet_queue_put_nullpacket(&is->videoq, is->video_stream);
                if (audio_stream >= 0)
                    m_read_input_cbk(m_player, DD_READ_AUDIO_FRAME, pkt, NULL);
                    //packet_queue_put_nullpacket(&is->audioq, is->audio_stream);

                eof = 1;
            }
            if (ic->pb && ic->pb->error)
                break;
            SDL_LockMutex(m_wait_mutex);
            SDL_CondWaitTimeout(m_continue_read_thread, m_wait_mutex, 10);
            SDL_UnlockMutex(m_wait_mutex);
            continue;
        } else {
            eof = 0;
        }

        if (pkt->stream_index == audio_stream || pkt->stream_index == video_stream) {
            m_read_input_cbk(m_player, pkt->stream_index == audio_stream?DD_READ_AUDIO_FRAME:DD_READ_VIDEO_FRAME, pkt, NULL);
        } else {
            av_packet_unref(pkt);
        }
    }

fail:
    return ;
}

void av_read_input::stream_open(std::string url, AVInputFormat *iformat) {
    m_url = url;
    m_iformat = iformat;
    start_thread();
}

void av_read_input::seek(int64_t pos) {
    m_seek_pos = pos;
}

void av_read_input::stream_close() {

}

int av_read_input::resume_read() {
    SDL_CondSignal(m_continue_read_thread);
    return 0;
}
