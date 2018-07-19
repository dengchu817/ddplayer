//
// Created by 邓楚 on 2018/6/28.
//

#include "av_player.h"
#include "av_player_def.h"
#include "av_cmdutils.h"

extern AVPacket flush_pkt;
av_player::av_player() {
    m_read_in = NULL;
    m_url = "";
    m_ffp = NULL;
    m_format_opts = NULL;
    m_codec_opts = NULL;
    m_sws_dict = NULL;
    m_player_opts = NULL;
    m_swr_opts = NULL;
    m_av_sync_type = AV_SYNC_AUDIO_MASTER;
}

av_player::~av_player() {

}

int av_player::init() {
    avcodec_register_all();
    av_register_all();
    avformat_network_init();
    //av_log_set_callback(ffp_log_callback_brief);
    av_init_packet(&flush_pkt);
    flush_pkt.data = (uint8_t *)&flush_pkt;
    m_ffp = new ffplayer();

    m_read_in = new av_read_input(this);
    m_read_in->init(dd_callback);

    m_audio_decoder = new av_audio_decoder(this);
    m_audio_decoder->init(dd_callback);

    m_audio_out = new av_audio_output(this);
    m_audio_out->init(dd_callback, m_audio_decoder->get_frame_queue());

    m_video_decoder = new av_video_decoder(this);
    m_video_decoder->init(dd_callback);

    m_video_out = new av_video_output(this);
    m_video_out->init(dd_callback, m_video_decoder->get_frame_queue());
    return 0;
}

int av_player::release() {
    return 0;
}

void av_player::start(std::string url) {
    m_read_in->stream_open(url, NULL);
}

AVDictionary *av_player::get_opt_dict(int opt_category) {
    switch (opt_category) {
        case FFP_OPT_CATEGORY_FORMAT:   return m_format_opts;
        case FFP_OPT_CATEGORY_CODEC:    return m_codec_opts;
        case FFP_OPT_CATEGORY_SWS:      return m_sws_dict;
        case FFP_OPT_CATEGORY_PLAYER:   return m_player_opts;
        case FFP_OPT_CATEGORY_SWR:      return m_swr_opts;
        default:
            return NULL;
    }
}

void av_player::set_option(int opt_category, const char *name, const char *value) {
    AVDictionary* dict = get_opt_dict(opt_category);
    av_dict_set(&dict, name, value, 0);
}

ffplayer *av_player::get_ffplayer() {
    return m_ffp;
}

int av_player::stream_component_open(AVFormatContext* ic, int stream_index) {
    AVCodecContext *avctx;
    AVCodec *codec = NULL;
    const char *forced_codec_name = NULL;
    AVDictionary *opts = NULL;
    AVDictionaryEntry *t = NULL;
    int sample_rate, nb_channels;
    int64_t channel_layout;
    int ret = 0;
    int stream_lowres = 0;//ffp->lowres;

    if (!ic || stream_index < 0)
        return -1;
    avctx = avcodec_alloc_context3(NULL);
    if (!avctx)
        return AVERROR(ENOMEM);

    ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
    if (ret < 0)
        goto fail;
    av_codec_set_pkt_timebase(avctx, ic->streams[stream_index]->time_base);

    codec = avcodec_find_decoder(avctx->codec_id);

    if (forced_codec_name)
        codec = avcodec_find_decoder_by_name(forced_codec_name);
    if (!codec) {
        if (forced_codec_name) av_log(NULL, AV_LOG_WARNING,
                                      "No codec could be found with name '%s'\n", forced_codec_name);
        else                   av_log(NULL, AV_LOG_WARNING,
                                      "No codec could be found with id %d\n", avctx->codec_id);
        ret = AVERROR(EINVAL);
        goto fail;
    }

    avctx->codec_id = codec->id;
    if(stream_lowres > av_codec_get_max_lowres(codec)){
        av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n",
               av_codec_get_max_lowres(codec));
        stream_lowres = av_codec_get_max_lowres(codec);
    }
    av_codec_set_lowres(avctx, stream_lowres);

#if FF_API_EMU_EDGE
    if(stream_lowres) avctx->flags |= CODEC_FLAG_EMU_EDGE;
#endif
//    if (ffp->fast)
//        avctx->flags2 |= AV_CODEC_FLAG2_FAST;
#if FF_API_EMU_EDGE
    if(codec->capabilities & AV_CODEC_CAP_DR1)
        avctx->flags |= CODEC_FLAG_EMU_EDGE;
#endif

    opts = filter_codec_opts(m_codec_opts, avctx->codec_id, ic, ic->streams[stream_index], codec);
    if (!av_dict_get(opts, "threads", NULL, 0))
        av_dict_set(&opts, "threads", "auto", 0);
    if (stream_lowres)
        av_dict_set_int(&opts, "lowres", stream_lowres, 0);
    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
        av_dict_set(&opts, "refcounted_frames", "1", 0);
    if ((ret = avcodec_open2(avctx, codec, &opts)) < 0) {
        goto fail;
    }
    if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
        av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
#ifdef FFP_MERGE
        ret =  AVERROR_OPTION_NOT_FOUND;
        goto fail;
#endif
    }

    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    switch (avctx->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            sample_rate    = avctx->sample_rate;
            nb_channels    = avctx->channels;
            channel_layout = avctx->channel_layout;

            /* prepare audio output */
            if (m_audio_out)
                m_audio_out->audio_open(sample_rate, nb_channels,channel_layout);
            if (m_audio_decoder){
                m_audio_decoder->decoder_init(avctx);
                m_audio_decoder->start();
            }

            break;
        case AVMEDIA_TYPE_VIDEO:
            if (m_video_out)
                m_video_out->start();
            if (m_video_decoder){
                m_video_decoder->decoder_init(avctx);
                m_video_decoder->start(ic, ic->streams[stream_index]);
            }

            break;
        case AVMEDIA_TYPE_SUBTITLE:

            break;
        default:
            break;
    }
    goto out;

fail:
    avcodec_free_context(&avctx);
out:
    av_dict_free(&opts);

    return ret;
}

int av_player::dd_callback(void *player, int type , void* in_data, void** out_data) {
    av_player* obj = (av_player*)player;
    if (obj){
        return obj->do_dd_callback(type, in_data, out_data);
    }
    return 0;
}

int av_player::do_dd_callback(int type, void* in_data, void** out_data) {
    if (type == DD_FIND_AUDIO_STREAM || type == DD_FIND_VIDEO_STREAM) {
        AVFormatContext* ic = (AVFormatContext*) in_data;
        int stream_index = *((int*)ic->opaque);
        stream_component_open(ic, stream_index);
    }else if (type == DD_READ_AUDIO_FRAME && m_audio_decoder){
        m_audio_decoder->feed((AVPacket*)in_data);
    }else if (type == DD_READ_VIDEO_FRAME){
        m_video_decoder->feed((AVPacket*)in_data);
    }else if (type == DD_CONTINUE_READ && m_read_in) {
        m_read_in->resume_read();
    } else if (type == DD_GET_VIDEO_SDL_OUT && m_video_out){
        *out_data = get_vout();
    }
    return 0;
}

Clock *av_player::get_video_clock() {
    if (m_video_out)
        return m_video_out->get_av_clock();
    return nullptr;
}

Clock *av_player::get_audio_clock() {
    if (m_audio_out)
        return m_audio_out->get_av_clock();
    return nullptr;
}

SDL_Vout* av_player::get_vout(){
    if (m_video_out)
        return m_video_out->get_vout();
    return nullptr;
}

