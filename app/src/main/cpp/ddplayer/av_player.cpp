//
// Created by 邓楚 on 2018/6/28.
//

#include "av_player.h"
#include "av_player_def.h"

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
    m_read_in->init(find_stream_cbk, read_frame_cbk);

    m_audio_decoder = new av_audio_decoder(this);
    m_audio_decoder->init();

    m_audio_out = new av_audio_output(this);
    m_audio_out->init();
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

int av_player::read_frame_cbk(void *usr, AVPacket *pkt) {
    av_player* pobj = (av_player*)usr;
    if (pobj){
        return pobj->put_read_packet(pkt);
    }
    return -1;
}

int av_player::put_read_packet(AVPacket *st) {
    return 0;
}

int av_player::find_stream_cbk(void *usr,AVFormatContext* ic, int stream_index) {
    av_player* pobj = (av_player*)usr;
    if (pobj){
        return pobj->stream_component_open(ic, stream_index);
    }
    return -1;
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

    if (stream_index < 0 || stream_index >= ic->nb_streams)
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
#if CONFIG_AVFILTER
            {
            AVFilterContext *sink;

            is->audio_filter_src.freq           = avctx->sample_rate;
            is->audio_filter_src.channels       = avctx->channels;
            is->audio_filter_src.channel_layout = get_valid_channel_layout(avctx->channel_layout, avctx->channels);
            is->audio_filter_src.fmt            = avctx->sample_fmt;
            SDL_LockMutex(ffp->af_mutex);
            if ((ret = configure_audio_filters(ffp, ffp->afilters, 0)) < 0) {
                SDL_UnlockMutex(ffp->af_mutex);
                goto fail;
            }
            ffp->af_changed = 0;
            SDL_UnlockMutex(ffp->af_mutex);
            sink = is->out_audio_filter;
            sample_rate    = av_buffersink_get_sample_rate(sink);
            nb_channels    = av_buffersink_get_channels(sink);
            channel_layout = av_buffersink_get_channel_layout(sink);
        }
#else
            sample_rate    = avctx->sample_rate;
            nb_channels    = avctx->channels;
            channel_layout = avctx->channel_layout;
#endif

            /* prepare audio output */
            m_audio_out->audio_open(sample_rate, nb_channels,channel_layout);
            break;
        case AVMEDIA_TYPE_VIDEO:


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

PacketQueue* av_player::get_audio_packetqueue() {
    if (m_audio_decoder)
        return m_audio_decoder->get_packetqueue();
    return NULL;
}

