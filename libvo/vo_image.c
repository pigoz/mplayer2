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
#include <math.h>
#include <stdbool.h>
#include <sys/stat.h>

#include <libswscale/swscale.h>

#include "config.h"
#include "osdep/io.h"
#include "path.h"
#include "talloc.h"
#include "mp_msg.h"
#include "libvo/video_out.h"
#include "libvo/csputils.h"
#include "libmpcodecs/vfcap.h"
#include "libmpcodecs/mp_image.h"
#include "fmt-conversion.h"
#include "image_writer.h"
#include "m_config.h"
#include "m_option.h"

struct priv {
    struct image_writer_opts *opts;
    char *outdir;

    int frame;

    uint32_t d_width;
    uint32_t d_height;

    struct mp_csp_details colorspace;
};

static void checked_mkdir(const char *buf, int verbose) {
    struct stat stat_p;

    // hurr
    struct { char *short_name; } info = { "vo_image" };

    if ( mkdir(buf, 0755) < 0 ) {
        switch (errno) { /* use switch in case other errors need to be caught
                            and handled in the future */
            case EEXIST:
                if ( mp_stat(buf, &stat_p ) < 0 ) {
                    mp_msg(MSGT_VO, MSGL_ERR, "%s: %s: %s\n", info.short_name,
                           _("This error has occurred"), strerror(errno) );
                    mp_msg(MSGT_VO, MSGL_ERR, "%s: %s %s\n", info.short_name,
                           _("Unable to access"), buf);
                }
                if ( !S_ISDIR(stat_p.st_mode) ) {
                    mp_msg(MSGT_VO, MSGL_ERR, "%s: %s %s\n", info.short_name,
                           buf, _("already exists, but is not a directory."));
                }
                if ( !(stat_p.st_mode & S_IWUSR) ) {
                    mp_msg(MSGT_VO, MSGL_ERR, "%s: %s - %s\n", info.short_name,
                           buf, _("Output directory already exists, but is not writable."));
                }

                mp_msg(MSGT_VO, MSGL_INFO, "%s: %s - %s\n", info.short_name,
                       buf, _("Output directory already exists and is writable."));
                break;

            default:
                mp_msg(MSGT_VO, MSGL_ERR, "%s: %s: %s\n", info.short_name,
                       _("This error has occurred"), strerror(errno) );
                mp_msg(MSGT_VO, MSGL_ERR, "%s: %s - %s\n", info.short_name,
                       buf, _("Unable to create output directory."));
        } /* end switch */
    } else if ( verbose ) {
        mp_msg(MSGT_VO, MSGL_INFO, "%s: %s - %s\n", info.short_name,
               buf, _("Output directory successfully created."));
    } /* end if */
}

static int config(struct vo *vo, uint32_t width, uint32_t height,
                  uint32_t d_width, uint32_t d_height, uint32_t flags,
                  uint32_t format)
{
    struct priv *p = vo->priv;

    p->d_width = d_width;
    p->d_height = d_height;

    if (p->outdir && vo->config_count < 1)
        checked_mkdir(p->outdir, 1);

    return 0;
}

static void check_events(struct vo *vo)
{
}

static void draw_osd(struct vo *vo, struct osd_state *osd)
{
}

static void flip_page(struct vo *vo)
{
}

static uint32_t draw_image(struct vo *vo, mp_image_t *mpi)
{
    struct priv *p = vo->priv;

    mp_image_t tmp = *mpi;
    tmp.width = p->d_width;
    tmp.height = p->d_height;

    char filename[80];
    snprintf(filename, sizeof(filename), "%08d.%s", p->frame,
             image_writer_file_ext(p->opts));
    char *path = filename;
    if (p->outdir)
        path = mp_path_join(NULL, bstr(p->outdir), bstr(filename));
    printf("Save '%s'!\n", path);
    write_image(&tmp, &p->colorspace, p->opts, path);
    if (path != filename)
        talloc_free(path);

    (p->frame)++;

    return VO_TRUE;
}

static int query_format(struct vo *vo, uint32_t fmt)
{
    enum PixelFormat av_format = imgfmt2pixfmt(fmt);

    // NOTE: accept everything that can be converted by swscale. screenshot.c
    // always wants RGB (at least for now), but it probably doesn't matter
    // whether we or screenshot.c do the conversion.
    if (av_format != PIX_FMT_NONE && sws_isSupportedInput(av_format))
        return VFCAP_CSP_SUPPORTED | VFCAP_CSP_SUPPORTED_BY_HW |
               VFCAP_HWSCALE_UP | VFCAP_HWSCALE_DOWN;
    return 0;
}

static void uninit(struct vo *vo)
{
}

static int preinit(struct vo *vo, const char *arg)
{
    return 0;
}

static int control(struct vo *vo, uint32_t request, void *data)
{
    struct priv *p = vo->priv;

    switch (request) {
    case VOCTRL_QUERY_FORMAT:
        return query_format(vo, *(uint32_t *)data);
    case VOCTRL_DRAW_IMAGE:
        return draw_image(vo, data);
    case VOCTRL_SET_YUV_COLORSPACE:
        p->colorspace = *(struct mp_csp_details *)data;
        return true;
    case VOCTRL_GET_YUV_COLORSPACE:
        *(struct mp_csp_details *)data = p->colorspace;
        return true;
    // prevent random frame stepping by frontend
    case VOCTRL_REDRAW_FRAME:
        return true;
    }
    return VO_NOTIMPL;
}

const struct vo_driver video_out_image =
{
    .is_new = true,
    .info = &(const vo_info_t) {
        "Write video frames to image files",
        "image",
        "wm4",
        ""
    },
    .options = &(struct m_sub_options) {
        .size = sizeof(struct priv),
        .defs = &(struct priv) {
            .colorspace = MP_CSP_DETAILS_DEFAULTS,
        },
        .opts = (struct m_option[]) {
            {"-", (void *) &image_writer_conf, &m_option_type_subconfig_struct, M_OPT_MERGE, 0, 0, NULL,
             .new = 1, .offset = offsetof(struct priv, opts)},
            {"outdir", NULL, &m_option_type_string,
             .new = 1, .offset = offsetof(struct priv, outdir)},
            {0},
        },
    },
    .preinit = preinit,
    .config = config,
    .control = control,
    .draw_osd = draw_osd,
    .flip_page = flip_page,
    .check_events = check_events,
    .uninit = uninit,
};
