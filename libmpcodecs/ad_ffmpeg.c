/*
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>

#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>

#include "talloc.h"

#include "config.h"
#include "mp_msg.h"
#include "options.h"

#include "ad_internal.h"
#include "libaf/reorder_ch.h"

#include "mpbswap.h"

static const ad_info_t info =
{
    "FFmpeg/libavcodec audio decoders",
    "ffmpeg",
    "Nick Kurshev",
    "ffmpeg.sf.net",
    ""
};

LIBAD_EXTERN(ffmpeg)

struct priv {
    AVCodecContext *avctx;
    AVFrame *avframe;
    char *output;
    int output_left;
    int unitsize;
    int previous_data_left;  // input demuxer packet data
};

static int preinit(sh_audio_t *sh)
{
    return 1;
}

/* Prefer playing audio with the samplerate given in container data
 * if available, but take number the number of channels and sample format
 * from the codec, since if the codec isn't using the correct values for
 * those everything breaks anyway.
 */
static int setup_format(sh_audio_t *sh_audio,
                        const AVCodecContext *lavc_context)
{
    int sample_format = sh_audio->sample_format;
    switch (lavc_context->sample_fmt) {
    case AV_SAMPLE_FMT_U8:  sample_format = AF_FORMAT_U8;       break;
    case AV_SAMPLE_FMT_S16: sample_format = AF_FORMAT_S16_NE;   break;
    case AV_SAMPLE_FMT_S32: sample_format = AF_FORMAT_S32_NE;   break;
    case AV_SAMPLE_FMT_FLT: sample_format = AF_FORMAT_FLOAT_NE; break;
    default:
        mp_msg(MSGT_DECAUDIO, MSGL_FATAL, "Unsupported sample format\n");
        sample_format = AF_FORMAT_UNKNOWN;
    }

    bool broken_srate        = false;
    int samplerate           = lavc_context->sample_rate;
    int container_samplerate = sh_audio->container_out_samplerate;
    if (!container_samplerate && sh_audio->wf)
        container_samplerate = sh_audio->wf->nSamplesPerSec;
    if (lavc_context->codec_id == CODEC_ID_AAC
        && samplerate == 2 * container_samplerate)
        broken_srate = true;
    else if (container_samplerate)
        samplerate = container_samplerate;

    if (lavc_context->channels != sh_audio->channels ||
        samplerate != sh_audio->samplerate ||
        sample_format != sh_audio->sample_format) {
        sh_audio->channels = lavc_context->channels;
        sh_audio->samplerate = samplerate;
        sh_audio->sample_format = sample_format;
        sh_audio->samplesize = af_fmt2bits(sh_audio->sample_format) / 8;
        if (broken_srate)
            mp_msg(MSGT_DECAUDIO, MSGL_WARN,
                   "Ignoring broken container sample rate for AAC with SBR\n");
        return 1;
    }
    return 0;
}

static int init(sh_audio_t *sh_audio)
{
    struct MPOpts *opts = sh_audio->opts;
    AVCodecContext *lavc_context;
    AVCodec *lavc_codec;

    mp_msg(MSGT_DECAUDIO, MSGL_V, "FFmpeg's libavcodec audio codec\n");

    if (sh_audio->codec->dll) {
        lavc_codec = avcodec_find_decoder_by_name(sh_audio->codec->dll);
        if (!lavc_codec) {
            mp_tmsg(MSGT_DECAUDIO, MSGL_ERR,
                    "Cannot find codec '%s' in libavcodec...\n",
                    sh_audio->codec->dll);
            return 0;
        }
    } else if (!sh_audio->libav_codec_id) {
        mp_msg(MSGT_DECVIDEO, MSGL_INFO, "no lavf id\n");
        return 0;
    } else {
        lavc_codec = avcodec_find_decoder(sh_audio->libav_codec_id);
        if (!lavc_codec) {
            return 0;
            mp_msg(MSGT_DECVIDEO, MSGL_INFO, "no codec for lavf id\n");
        }
    }

    struct priv *ctx = talloc_zero(NULL, struct priv);
    sh_audio->context = ctx;
    lavc_context = avcodec_alloc_context3(lavc_codec);
    ctx->avctx = lavc_context;
    ctx->avframe = avcodec_alloc_frame();

    // Always try to set - option only exists for AC3 at the moment
    av_opt_set_double(lavc_context, "drc_scale", opts->drc_level,
                      AV_OPT_SEARCH_CHILDREN);
    lavc_context->sample_rate = sh_audio->samplerate;
    lavc_context->bit_rate = sh_audio->i_bps * 8;
    if (sh_audio->wf) {
        lavc_context->channels = sh_audio->wf->nChannels;
        lavc_context->sample_rate = sh_audio->wf->nSamplesPerSec;
        lavc_context->bit_rate = sh_audio->wf->nAvgBytesPerSec * 8;
        lavc_context->block_align = sh_audio->wf->nBlockAlign;
        lavc_context->bits_per_coded_sample = sh_audio->wf->wBitsPerSample;
    }
    lavc_context->request_channels = opts->audio_output_channels;
    lavc_context->codec_tag = sh_audio->format; //FOURCC
    lavc_context->codec_type = AVMEDIA_TYPE_AUDIO;
    lavc_context->codec_id = lavc_codec->id; // not sure if required, imho not --A'rpi

    /* alloc extra data */
    if (sh_audio->wf && sh_audio->wf->cbSize > 0) {
        lavc_context->extradata = av_mallocz(sh_audio->wf->cbSize + FF_INPUT_BUFFER_PADDING_SIZE);
        lavc_context->extradata_size = sh_audio->wf->cbSize;
        memcpy(lavc_context->extradata, sh_audio->wf + 1,
               lavc_context->extradata_size);
    }

    // for QDM2
    if (sh_audio->codecdata_len && sh_audio->codecdata &&
            !lavc_context->extradata) {
        lavc_context->extradata = av_malloc(sh_audio->codecdata_len +
                                            FF_INPUT_BUFFER_PADDING_SIZE);
        lavc_context->extradata_size = sh_audio->codecdata_len;
        memcpy(lavc_context->extradata, (char *)sh_audio->codecdata,
               lavc_context->extradata_size);
    }

    /* open it */
    if (avcodec_open2(lavc_context, lavc_codec, NULL) < 0) {
        mp_tmsg(MSGT_DECAUDIO, MSGL_ERR, "Could not open codec.\n");
        uninit(sh_audio);
        return 0;
    }
    mp_msg(MSGT_DECAUDIO, MSGL_V, "INFO: libavcodec \"%s\" init OK!\n",
           lavc_codec->name);

    if (sh_audio->format == 0x3343414D) {
        // MACE 3:1
        sh_audio->ds->ss_div = 2 * 3; // 1 samples/packet
        sh_audio->ds->ss_mul = 2 * sh_audio->wf->nChannels; // 1 byte*ch/packet
    } else if (sh_audio->format == 0x3643414D) {
        // MACE 6:1
        sh_audio->ds->ss_div = 2 * 6; // 1 samples/packet
        sh_audio->ds->ss_mul = 2 * sh_audio->wf->nChannels; // 1 byte*ch/packet
    }

    // Decode at least 1 byte:  (to get header filled)
    for (int tries = 0;;) {
        int x = decode_audio(sh_audio, sh_audio->a_buffer, 1,
                             sh_audio->a_buffer_size);
        if (x > 0) {
            sh_audio->a_buffer_len = x;
            break;
        }
        if (++tries >= 5) {
            mp_msg(MSGT_DECAUDIO, MSGL_ERR,
                   "ad_ffmpeg: initial decode failed\n");
            uninit(sh_audio);
            return 0;
        }
    }

    sh_audio->i_bps = lavc_context->bit_rate / 8;
    if (sh_audio->wf && sh_audio->wf->nAvgBytesPerSec)
        sh_audio->i_bps = sh_audio->wf->nAvgBytesPerSec;

    switch (lavc_context->sample_fmt) {
    case AV_SAMPLE_FMT_U8:
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_FLT:
        break;
    default:
        uninit(sh_audio);
        return 0;
    }
    return 1;
}

static void uninit(sh_audio_t *sh)
{
    struct priv *ctx = sh->context;
    if (!ctx)
        return;
    AVCodecContext *lavc_context = ctx->avctx;

    if (lavc_context) {
        if (avcodec_close(lavc_context) < 0)
            mp_tmsg(MSGT_DECVIDEO, MSGL_ERR, "Could not close codec.\n");
        av_freep(&lavc_context->extradata);
        av_freep(&lavc_context);
    }
    av_free(ctx->avframe);
    talloc_free(ctx);
    sh->context = NULL;
}

static int control(sh_audio_t *sh, int cmd, void *arg, ...)
{
    struct priv *ctx = sh->context;
    switch (cmd) {
    case ADCTRL_RESYNC_STREAM:
        avcodec_flush_buffers(ctx->avctx);
        ds_clear_parser(sh->ds);
        ctx->previous_data_left = 0;
        ctx->output_left = 0;
        return CONTROL_TRUE;
    }
    return CONTROL_UNKNOWN;
}

static int decode_new_packet(struct sh_audio *sh)
{
    struct priv *priv = sh->context;
    AVCodecContext *avctx = priv->avctx;
    double pts = MP_NOPTS_VALUE;
    int insize;
    bool packet_already_used = priv->previous_data_left;
    struct demux_packet *mpkt = ds_get_packet2(sh->ds,
                                               priv->previous_data_left);
    unsigned char *start;
    if (!mpkt) {
        assert(!priv->previous_data_left);
        start = NULL;
        insize = 0;
        ds_parse(sh->ds, &start, &insize, pts, 0);
        if (insize <= 0)
            return -1;  // error or EOF
    } else {
        assert(mpkt->len >= priv->previous_data_left);
        if (!priv->previous_data_left) {
            priv->previous_data_left = mpkt->len;
            pts = mpkt->pts;
        }
        insize = priv->previous_data_left;
        start = mpkt->buffer + mpkt->len - priv->previous_data_left;
        int consumed = ds_parse(sh->ds, &start, &insize, pts, 0);
        priv->previous_data_left -= consumed;
    }

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = start;
    pkt.size = insize;
    if (mpkt && mpkt->avpacket) {
        pkt.side_data = mpkt->avpacket->side_data;
        pkt.side_data_elems = mpkt->avpacket->side_data_elems;
    }
    if (pts != MP_NOPTS_VALUE && !packet_already_used) {
        sh->pts = pts;
        sh->pts_bytes = 0;
    }
    int got_frame = 0;
    int ret = avcodec_decode_audio4(avctx, priv->avframe, &got_frame, &pkt);
    // LATM may need many packets to find mux info
    if (ret == AVERROR(EAGAIN))
        return 0;
    if (ret < 0) {
        mp_msg(MSGT_DECAUDIO, MSGL_V, "lavc_audio: error\n");
        return -1;
    }
    if (!sh->parser)
        priv->previous_data_left += insize - ret;
    if (!got_frame)
        return 0;
    /* An error is reported later from output format checking, but make
     * sure we don't crash by overreading first plane. */
    if (av_sample_fmt_is_planar(avctx->sample_fmt) && avctx->channels > 1)
        return 0;
    uint64_t unitsize = (uint64_t)av_get_bytes_per_sample(avctx->sample_fmt) *
                        avctx->channels;
    if (unitsize > 100000)
        abort();
    priv->unitsize = unitsize;
    uint64_t output_left = unitsize * priv->avframe->nb_samples;
    if (output_left > 500000000)
        abort();
    priv->output_left = output_left;
    priv->output = priv->avframe->data[0];
    mp_dbg(MSGT_DECAUDIO, MSGL_DBG2, "Decoded %d -> %d  \n", insize,
           priv->output_left);
    return 0;
}


static int decode_audio(sh_audio_t *sh_audio, unsigned char *buf, int minlen,
                        int maxlen)
{
    struct priv *priv = sh_audio->context;
    AVCodecContext *avctx = priv->avctx;

    int len = -1;
    while (len < minlen) {
        if (!priv->output_left) {
            if (decode_new_packet(sh_audio) < 0)
                break;
            continue;
        }
        if (setup_format(sh_audio, avctx))
            return len;
        int size = (minlen - len + priv->unitsize - 1);
        size -= size % priv->unitsize;
        size = FFMIN(size, priv->output_left);
        if (size > maxlen)
            abort();
        memcpy(buf, priv->output, size);
        priv->output += size;
        priv->output_left -= size;
        if (avctx->channels >= 5) {
            int samplesize = av_get_bytes_per_sample(avctx->sample_fmt);
            reorder_channel_nch(buf, AF_CHANNEL_LAYOUT_LAVC_DEFAULT,
                                AF_CHANNEL_LAYOUT_MPLAYER_DEFAULT,
                                avctx->channels,
                                size / samplesize, samplesize);
        }
        if (len < 0)
            len = size;
        else
            len += size;
        buf += size;
        maxlen -= size;
        sh_audio->pts_bytes += size;
    }
    return len;
}
