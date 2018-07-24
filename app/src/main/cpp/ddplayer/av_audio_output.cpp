//
// Created by 邓楚 on 2018/7/2.
//

#include "av_audio_output.h"
#include "ffplayer_option.h"
#include "av_player.h"
#include "av_clock.h"
#include "av_frame_queue.h"

av_audio_output::av_audio_output(void* player) {
    m_player = player;
    m_cbk = NULL;
    m_aout = NULL;
    m_frame_queue_ref = NULL;
    m_left_volume = 1;
    m_right_volume = 1;
    m_audio_buf_index = 0;
    m_audio_buf_index = 0;
    m_muted = false;
}

av_audio_output::~av_audio_output() {

}

void av_audio_output::init(dd_callback cbk, FrameQueue* ref) {
    m_cbk = cbk;
    ffplayer* ffp = ((av_player*)m_player)->get_ffplayer();
    if (ffp->opensles)
        m_aout = SDL_AoutAndroid_CreateForOpenSLES();
    else
        m_aout = SDL_AoutAndroid_CreateForAudioTrack();
    if (m_aout)
        SDL_AoutSetStereoVolume(m_aout, m_left_volume, m_right_volume);

    m_frame_queue_ref = ref;
    init_clock(&m_clock, &ref->pktq->serial);
}

void av_audio_output::release() {

}

int av_audio_output::audio_open(int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate) {
    int ret = do_audio_open(wanted_channel_layout, wanted_nb_channels, wanted_sample_rate, &m_audio_tgt);
    m_audio_src = m_audio_tgt;
    m_audio_buf_index = 0;
    m_audio_buf_index = 0;
    SDL_AoutPauseAudio(m_aout, 0);
    return ret;
}

int av_audio_output::do_audio_open(int64_t wanted_channel_layout, int wanted_nb_channels,
                                   int wanted_sample_rate, struct AudioParams *audio_hw_params) {
    SDL_AudioSpec wanted_spec, spec;
    const char *env;
    static const int next_nb_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};

    static const int next_sample_rates[] = {0, 44100, 48000};
    int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;

    env = SDL_getenv("SDL_AUDIO_CHANNELS");
    if (env) {
        wanted_nb_channels = atoi(env);
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
    }
    if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }
    wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
    wanted_spec.channels = wanted_nb_channels;
    wanted_spec.freq = wanted_sample_rate;
    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
        av_log(NULL, AV_LOG_ERROR, "Invalid sample rate or channel count!\n");
        return -1;
    }
    while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq)
        next_sample_rate_idx--;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.silence = 0;
    wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AoutGetAudioPerSecondCallBacks(m_aout)));
    wanted_spec.callback = sdl_audio_callback;
    wanted_spec.userdata = this;
    while (SDL_AoutOpenAudio(m_aout, &wanted_spec, &spec) < 0) {
        /* avoid infinity loop on exit. --by bbcallen */
        av_log(NULL, AV_LOG_WARNING, "SDL_OpenAudio (%d channels, %d Hz): %s\n",
               wanted_spec.channels, wanted_spec.freq, SDL_GetError());
        wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
        if (!wanted_spec.channels) {
            wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
            wanted_spec.channels = wanted_nb_channels;
            if (!wanted_spec.freq) {
                av_log(NULL, AV_LOG_ERROR,
                       "No more combinations to try, audio open failed\n");
                return -1;
            }
        }
        wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
    }
    if (spec.format != AUDIO_S16SYS) {
        av_log(NULL, AV_LOG_ERROR,
               "SDL advised audio format %d is not supported!\n", spec.format);
        return -1;
    }
    if (spec.channels != wanted_spec.channels) {
        wanted_channel_layout = av_get_default_channel_layout(spec.channels);
        if (!wanted_channel_layout) {
            av_log(NULL, AV_LOG_ERROR,
                   "SDL advised channel count %d is not supported!\n", spec.channels);
            return -1;
        }
    }

    audio_hw_params->fmt = AV_SAMPLE_FMT_S16;
    audio_hw_params->freq = spec.freq;
    audio_hw_params->channel_layout = wanted_channel_layout;
    audio_hw_params->channels =  spec.channels;
    audio_hw_params->frame_size = av_samples_get_buffer_size(NULL, audio_hw_params->channels, 1, audio_hw_params->fmt, 1);
    audio_hw_params->bytes_per_sec = av_samples_get_buffer_size(NULL, audio_hw_params->channels, audio_hw_params->freq, audio_hw_params->fmt, 1);
    if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0) {
        av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
        return -1;
    }

    SDL_AoutSetDefaultLatencySeconds(m_aout, ((double)(2 * spec.size)) / audio_hw_params->bytes_per_sec);
    return spec.size;
}

void av_audio_output::sdl_audio_callback(void *arg, Uint8 *stream, int len) {
    av_audio_output* pobj = (av_audio_output*)arg;
    if (pobj){
        pobj->do_sdl_audio_callback(stream, len);
    }
    return;
}

void av_audio_output::do_sdl_audio_callback(Uint8 *stream, int len) {

    int audio_size, audio_clock_serial, len1;
    double audio_clock;
    ffplayer* ffp = ((av_player*)m_player)->get_ffplayer();
    int64_t audio_callback_time = av_gettime_relative();


    if (ffp->pf_playback_rate_changed) {
        ffp->pf_playback_rate_changed = 0;
        SDL_AoutSetPlaybackRate(m_aout, ffp->pf_playback_rate);
    }
    if (ffp->pf_playback_volume_changed) {
        ffp->pf_playback_volume_changed = 0;
        SDL_AoutSetPlaybackVolume(m_aout, ffp->pf_playback_volume);
    }

    while (len > 0) {
        if (m_audio_buf_index >= m_audio_buf_size) {
            audio_decode_frame(&audio_size, &audio_clock_serial, &audio_clock);
            if (audio_size < 0) {
                /* if error, just output silence */
                m_audio_buf = NULL;
                m_audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE / m_audio_tgt.frame_size * m_audio_tgt.frame_size;
            } else {
//                if (is->show_mode != SHOW_MODE_VIDEO)
//                    update_sample_display(is, (int16_t *)is->audio_buf, audio_size);
                m_audio_buf_size = audio_size;
            }
            m_audio_buf_index = 0;
        }
        len1 = m_audio_buf_size - m_audio_buf_index;
        if (len1 > len)
            len1 = len;
        if (!m_muted && m_audio_buf /*&& m_audio_volume == SDL_MIX_MAXVOLUME*/)
            memcpy(stream, (uint8_t *)m_audio_buf + m_audio_buf_index, len1);
        else {
            memset(stream, 0, len1);
//            if (!m_muted && m_audio_buf)
//                SDL_MixAudio(stream, (uint8_t *)m_audio_buf + m_audio_buf_index, len1, m_audio_volume);
        }
        len -= len1;
        stream += len1;
        m_audio_buf_index += len1;
    }
    int audio_write_buf_size = m_audio_buf_size - m_audio_buf_index;
    /* Let's assume the audio driver that is used by SDL has two periods. */
    if (!isnan(audio_clock)) {
        set_clock_at(&m_clock, audio_clock - (double)(audio_write_buf_size) / m_audio_tgt.bytes_per_sec - SDL_AoutGetLatencySeconds(m_aout), audio_clock_serial, audio_callback_time / 1000000.0);
        //sync_clock_to_slave(&is->extclk, &is->audclk);
        av_log(NULL,AV_LOG_ERROR, "audio clock is %f\n", audio_clock - (double)(audio_write_buf_size) / m_audio_tgt.bytes_per_sec - SDL_AoutGetLatencySeconds(m_aout) );
    }
    return;
}

int av_audio_output::audio_decode_frame(int *audio_size, int* audio_clock_serial, double* audio_clock) {
    int data_size, resampled_data_size;
    int64_t dec_channel_layout;
    int wanted_nb_samples;
    Frame *af;

    do {
        if (!(af = frame_queue_peek_readable(m_frame_queue_ref)))
            return -1;
        frame_queue_next(m_frame_queue_ref);

    } while (af->serial != m_frame_queue_ref->pktq->serial);

    data_size = av_samples_get_buffer_size(NULL, af->frame->channels,
                                           af->frame->nb_samples,
                                           (AVSampleFormat)af->frame->format, 1);

    dec_channel_layout =
            (af->frame->channel_layout && af->frame->channels == av_get_channel_layout_nb_channels(af->frame->channel_layout)) ?
            af->frame->channel_layout : av_get_default_channel_layout(af->frame->channels);
    wanted_nb_samples = af->frame->nb_samples;

    if (af->frame->format        != m_audio_src.fmt            ||
        dec_channel_layout       != m_audio_src.channel_layout ||
        af->frame->sample_rate   != m_audio_src.freq           ||
        (wanted_nb_samples       != af->frame->nb_samples && !m_swr_ctx)) {
        swr_free(&m_swr_ctx);
        m_swr_ctx = swr_alloc_set_opts(NULL,
                                         m_audio_tgt.channel_layout, m_audio_tgt.fmt, m_audio_tgt.freq,
                                         dec_channel_layout,           (AVSampleFormat)af->frame->format, af->frame->sample_rate,
                                         0, NULL);
        if (!m_swr_ctx || swr_init(m_swr_ctx) < 0) {
            av_log(NULL, AV_LOG_ERROR,
                   "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
                   af->frame->sample_rate, av_get_sample_fmt_name((AVSampleFormat)af->frame->format), af->frame->channels,
                   m_audio_tgt.freq, av_get_sample_fmt_name(m_audio_tgt.fmt), m_audio_tgt.channels);
            swr_free(&m_swr_ctx);
            return -1;
        }
        m_audio_src.channel_layout = dec_channel_layout;
        m_audio_src.channels       = af->frame->channels;
        m_audio_src.freq = af->frame->sample_rate;
        m_audio_src.fmt = (AVSampleFormat)af->frame->format;
    }

    if (m_swr_ctx) {
        const uint8_t **in = (const uint8_t **)af->frame->extended_data;
        uint8_t **out = &m_audio_buf1;
        int out_count = (int64_t)wanted_nb_samples * m_audio_tgt.freq / af->frame->sample_rate + 256;
        int out_size  = av_samples_get_buffer_size(NULL, m_audio_tgt.channels, out_count, m_audio_tgt.fmt, 0);
        int len2;
        if (out_size < 0) {
            av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
            return -1;
        }
        if (wanted_nb_samples != af->frame->nb_samples) {
            if (swr_set_compensation(m_swr_ctx, (wanted_nb_samples - af->frame->nb_samples) * m_audio_tgt.freq / af->frame->sample_rate,
                                     wanted_nb_samples * m_audio_tgt.freq / af->frame->sample_rate) < 0) {
                av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
                return -1;
            }
        }
        av_fast_malloc(&m_audio_buf1, &m_audio_buf1_size, out_size);
        if (!m_audio_buf1)
            return AVERROR(ENOMEM);
        len2 = swr_convert(m_swr_ctx, out, out_count, in, af->frame->nb_samples);
        if (len2 < 0) {
            av_log(NULL, AV_LOG_ERROR, "swr_convert() failed\n");
            return -1;
        }
        if (len2 == out_count) {
            av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too small\n");
            if (swr_init(m_swr_ctx) < 0)
                swr_free(&m_swr_ctx);
        }
        m_audio_buf = m_audio_buf1;
        resampled_data_size = len2 * m_audio_tgt.channels * av_get_bytes_per_sample(m_audio_tgt.fmt);
    } else {
        m_audio_buf = af->frame->data[0];
        resampled_data_size = data_size;
    }
    *audio_size = resampled_data_size;
    /* update the audio clock with the pts */
    if (!isnan(af->pts))
        *audio_clock = af->pts + (double) af->frame->nb_samples / af->frame->sample_rate;
    else
        *audio_clock = NAN;

    *audio_clock_serial = af->serial;
    return resampled_data_size;
}


