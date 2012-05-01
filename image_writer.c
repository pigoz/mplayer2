/*
 * This file is part of mplayer2.
 *
 * mplayer2 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mplayer2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with mplayer2.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>

#include "config.h"

#ifdef CONFIG_JPEG
#include <jpeglib.h>
#endif

#include "osdep/io.h"

#include "image_writer.h"
#include "talloc.h"
#include "libmpcodecs/img_format.h"
#include "libmpcodecs/mp_image.h"
#include "libmpcodecs/dec_video.h"
#include "libmpcodecs/vf.h"
#include "fmt-conversion.h"

//for sws_getContextFromCmdLine_hq and mp_sws_set_colorspace
#include "libmpcodecs/vf_scale.h"
#include "libvo/csputils.h"

#include "m_option.h"

// MPOpts -> image_writer_opts
// xxx these macros should take the struct as parameter, but it would be too
//     much to change right now
#undef OPT_INTRANGE
#undef OPT_STRING
#define OPT_INTRANGE(optname, varname, flags, min, max) {optname, NULL, &m_option_type_int, (flags) | CONF_RANGE, min, max, NULL, 1, offsetof(struct image_writer_opts, varname)}
#define OPT_STRING(optname, varname, flags) {optname, NULL, &m_option_type_string, flags, 0, 0, NULL, 1, offsetof(struct image_writer_opts, varname)}

const struct m_sub_options image_writer_conf = {
    .opts = (m_option_t[]) {
        OPT_INTRANGE("jpeg-quality", jpeg_quality, 0, 0, 100),
        OPT_INTRANGE("png-compression", png_compression, 0, 0, 9),
        OPT_STRING("filetype", filetype, 0),
        {0},
    },
    .size = sizeof(struct image_writer_opts),
    .defs = &(struct image_writer_opts) IMAGE_WRITER_OPTS_DEFAULTS,
};

struct img_writer {
    const char *file_ext;
    int (*write)(const struct image_writer_opts *opts,
                 mp_image_t *image,
                 FILE *fp);
};

static int write_png(const struct image_writer_opts *opts,
                     struct mp_image *image,
                     FILE *fp)
{
    void *outbuffer = NULL;
    int success = 0;
    AVFrame *pic = NULL;

    struct AVCodec *png_codec = avcodec_find_encoder(CODEC_ID_PNG);
    AVCodecContext *avctx = NULL;
    if (!png_codec)
        goto print_open_fail;
    avctx = avcodec_alloc_context3(png_codec);
    if (!avctx)
        goto print_open_fail;

    avctx->time_base = AV_TIME_BASE_Q;
    avctx->width = image->width;
    avctx->height = image->height;
    avctx->pix_fmt = PIX_FMT_RGB24;
    avctx->compression_level = opts->png_compression;

    if (avcodec_open2(avctx, png_codec, NULL) < 0) {
     print_open_fail:
        mp_msg(MSGT_CPLAYER, MSGL_INFO, "Could not open libavcodec PNG encoder"
               " for saving images\n");
        goto error_exit;
    }

    size_t outbuffer_size = image->width * image->height * 3 * 2;
    outbuffer = malloc(outbuffer_size);
    if (!outbuffer)
        goto error_exit;

    pic = avcodec_alloc_frame();
    if (!pic)
        goto error_exit;
    avcodec_get_frame_defaults(pic);
    for (int n = 0; n < 4; n++) {
        pic->data[n] = image->planes[n];
        pic->linesize[n] = image->stride[n];
    }
    int size = avcodec_encode_video(avctx, outbuffer, outbuffer_size, pic);
    if (size < 1)
        goto error_exit;

    fwrite(outbuffer, size, 1, fp);

    success = 1;
error_exit:
    if (avctx)
        avcodec_close(avctx);
    av_free(avctx);
    av_free(pic);
    free(outbuffer);
    return success;
}

#ifdef CONFIG_JPEG

static void write_jpeg_error_exit(j_common_ptr cinfo)
{
  // NOTE: do not write error message, too much effort to connect the libjpeg
  //       log callbacks with mplayer's log function mp_msp()

  // Return control to the setjmp point
  longjmp(*(jmp_buf*)cinfo->client_data, 1);
}

static int write_jpeg(const struct image_writer_opts *opts,
                      mp_image_t *image,
                      FILE *fp)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = write_jpeg_error_exit;

    jmp_buf error_return_jmpbuf;
    cinfo.client_data = &error_return_jmpbuf;
    if (setjmp(cinfo.client_data)) {
        jpeg_destroy_compress(&cinfo);
        return 0;
    }

    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);

    cinfo.image_width = image->width;
    cinfo.image_height = image->height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, opts->jpeg_quality, 1);

    jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height) {
        JSAMPROW row_pointer[1];
        row_pointer[0] = image->planes[0] +
                         cinfo.next_scanline * image->stride[0];
        jpeg_write_scanlines(&cinfo, row_pointer,1);
    }

    jpeg_finish_compress(&cinfo);

    jpeg_destroy_compress(&cinfo);

    return 1;
}

#endif

static const struct img_writer img_writers[] = {
    { "png", write_png },
#ifdef CONFIG_JPEG
    { "jpg", write_jpeg },
    { "jpeg", write_jpeg },
#endif
};

static const struct img_writer *get_writer(const struct image_writer_opts *opts)
{
    const char *type = opts->filetype;

    for (size_t n = 0; n < sizeof(img_writers) / sizeof(img_writers[0]); n++) {
        const struct img_writer *writer = &img_writers[n];
        if (type && strcmp(type, writer->file_ext) == 0)
            return writer;
    }

    return &img_writers[0];
}

const char *image_writer_file_ext(const struct image_writer_opts *opts)
{
    struct image_writer_opts defs = IMAGE_WRITER_OPTS_DEFAULTS;

    if (!opts)
        opts = &defs;

    return get_writer(opts)->file_ext;
}

int write_image(struct mp_image *image, const struct mp_csp_details *csp,
                const struct image_writer_opts *opts, const char *filename)
{
    struct mp_image *allocated_image = NULL;
    const int destfmt = IMGFMT_RGB24;
    struct image_writer_opts defs = IMAGE_WRITER_OPTS_DEFAULTS;

    if (!opts)
        opts = &defs;

    const struct img_writer *writer = get_writer(opts);

    if (image->imgfmt != destfmt) {
        struct mp_image *dst = alloc_mpi(image->w, image->h, destfmt);

        struct SwsContext *sws = sws_getContextFromCmdLine_hq(image->width,
                                                              image->height,
                                                              image->imgfmt,
                                                              dst->width,
                                                              dst->height,
                                                              dst->imgfmt);

        struct mp_csp_details colorspace = MP_CSP_DETAILS_DEFAULTS;
        if (csp)
            colorspace = *csp;
        // This is a property of the output device; images always use
        // full-range RGB.
        colorspace.levels_out = MP_CSP_LEVELS_PC;
        mp_sws_set_colorspace(sws, &colorspace);

        sws_scale(sws, (const uint8_t **)image->planes, image->stride, 0,
                  image->height, dst->planes, dst->stride);

        sws_freeContext(sws);

        allocated_image = dst;
        image = dst;
    }

    FILE *fp = fopen(filename, "wb");
    int success = 0;
    if (fp == NULL) {
        mp_msg(MSGT_CPLAYER, MSGL_ERR,
               "Error opening '%s' for writing!\n", filename);
    } else {
        success = writer->write(opts, image, fp);
        success = !fclose(fp) && success;
        if (!success)
            mp_msg(MSGT_CPLAYER, MSGL_ERR, "Error writing file '%s'!\n",
                   filename);
    }

    free_mp_image(allocated_image);

    return success;
}
