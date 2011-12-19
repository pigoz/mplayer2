/*
 * CoreVideo video output driver
 * Copyright (c) 2005 Nicolas Plourde <nicolasplourde@gmail.com>
 *
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

#import "vo_corevideo.h"

// mplayer includes
#import "fastmemcpy.h"
#import "video_out.h"
#import "aspect.h"
#import "sub/sub.h"
#import "subopt-helper.h"

#import "csputils.h"
#import "libmpcodecs/vfcap.h"
#import "libmpcodecs/mp_image.h"
#import "osd.h"

#import "cocoa_common.h"

OSType pixelFormat;

CVPixelBufferRef pixelBuffer;
CVOpenGLTextureCacheRef textureCache;
CVOpenGLTextureRef texture;
NSRect textureFrame;

GLfloat lowerLeft[2];
GLfloat lowerRight[2];
GLfloat upperRight[2];
GLfloat upperLeft[2];

static struct mp_csp_details colorspace = MP_CSP_DETAILS_DEFAULTS;

static void resize(struct vo *vo, int width, int height)
{
    int d_width, d_height;

    mp_msg(MSGT_VO, MSGL_V, "[vo_corevideo] New OpenGL Viewport (0, 0, %d, %d)\n", width, height);

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    aspect(vo, &d_width, &d_height, A_WINZOOM);
    textureFrame = NSMakeRect((vo->dwidth - d_width) / 2, (vo->dheight - d_height) / 2, d_width, d_height);
}

static void prepare_opengl(struct vo *vo)
{
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    resize(vo, vo->dwidth, vo->dheight);
}

static int config(struct vo *vo, uint32_t width, uint32_t height,
                  uint32_t d_width, uint32_t d_height, uint32_t flags,
                  uint32_t format)
{
    CVPixelBufferRelease(pixelBuffer);
    pixelBuffer = NULL;
    CVOpenGLTextureRelease(texture);
    texture = NULL;

    vo_cocoa_create_window(vo, d_width, d_height, flags);
    prepare_opengl(vo);
    vo_cocoa_swap_interval(1);

    return 0;
}

static void check_events(struct vo *vo)
{
    int e = vo_cocoa_check_events(vo);
    if (e & VO_EVENT_RESIZE)
        resize(vo, vo->dwidth, vo->dheight);
}

static void draw_osd(struct vo *vo, struct osd_state *osd)
{
    //osd_draw_text(osd, image_width, image_height, draw_alpha, vo);
}

static void prepare_texture(void)
{
    CVReturn error;
    CVOpenGLTextureRelease(texture);
    error = CVOpenGLTextureCacheCreateTextureFromImage(NULL, textureCache, pixelBuffer, 0, &texture);
    if(error != kCVReturnSuccess)
        mp_msg(MSGT_VO, MSGL_ERR,"[vo_corevideo] Failed to create OpenGL texture(%d)\n", error);

    CVOpenGLTextureGetCleanTexCoords(texture, lowerLeft, lowerRight, upperRight, upperLeft);
}

static void do_render(struct vo *vo)
{
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(CVOpenGLTextureGetTarget(texture));
    glBindTexture(CVOpenGLTextureGetTarget(texture), CVOpenGLTextureGetName(texture));

    glColor3f(1,1,1);
    glBegin(GL_QUADS);
    glTexCoord2f(upperLeft[0], upperLeft[1]); glVertex2i(textureFrame.origin.x-(vo->panscan_x >> 1), textureFrame.origin.y-(vo->panscan_y >> 1));
    glTexCoord2f(lowerLeft[0], lowerLeft[1]); glVertex2i(textureFrame.origin.x-(vo->panscan_x >> 1), NSMaxY(textureFrame)+(vo->panscan_y >> 1));
    glTexCoord2f(lowerRight[0], lowerRight[1]); glVertex2i(NSMaxX(textureFrame)+(vo->panscan_x >> 1), NSMaxY(textureFrame)+(vo->panscan_y >> 1));
    glTexCoord2f(upperRight[0], upperRight[1]); glVertex2i(NSMaxX(textureFrame)+(vo->panscan_x >> 1), textureFrame.origin.y-(vo->panscan_y >> 1));
    glEnd();
    glDisable(CVOpenGLTextureGetTarget(texture));
}

static void flip_page(struct vo *vo)
{
    if (vo_doublebuffering) {
        vo_cocoa_swap_buffers();
    } else {
        mp_msg(MSGT_VO, MSGL_ERR,"[vo_corevideo] Shit happening!\n");
    }
}

static uint32_t draw_image(struct vo *vo, mp_image_t *mpi)
{
    CVReturn error;

    if (!textureCache || !pixelBuffer) {
        error = CVOpenGLTextureCacheCreate(NULL, 0, vo_cocoa_cgl_context(), vo_cocoa_cgl_pixel_format(), 0, &textureCache);
        if(error != kCVReturnSuccess)
            mp_msg(MSGT_VO, MSGL_ERR,"[vo_corevideo] Failed to create OpenGL texture Cache(%d)\n", error);

        error = CVPixelBufferCreateWithBytes(NULL, mpi->width, mpi->height, pixelFormat,
                                             mpi->planes[0], mpi->width * mpi->bpp / 8, NULL, NULL, NULL, &pixelBuffer);
        if(error != kCVReturnSuccess)
            mp_msg(MSGT_VO, MSGL_ERR,"[vo_corevideo] Failed to create Pixel Buffer(%d)\n", error);
    }

    prepare_texture();
    do_render(vo);

    return VO_TRUE;
}

static int query_format(uint32_t format)
{
    const int supportflags = VFCAP_CSP_SUPPORTED | VFCAP_CSP_SUPPORTED_BY_HW |
                             VFCAP_OSD | VFCAP_HWSCALE_UP | VFCAP_HWSCALE_DOWN |
                             VOCAP_NOSLICES;

    switch(format)
    {
        case IMGFMT_YUY2:
            pixelFormat = kYUVSPixelFormat;
            return supportflags;

        case IMGFMT_RGB24:
            pixelFormat = k24RGBPixelFormat;
            return supportflags;

        case IMGFMT_ARGB:
            pixelFormat = k32ARGBPixelFormat;
            return supportflags;

        case IMGFMT_BGRA:
            pixelFormat = k32BGRAPixelFormat;
            return supportflags;
    }
    return 0;
}

static void uninit(struct vo *vo)
{
    vo_cocoa_uninit(vo);
}

static const opt_t subopts[] = {
{NULL}
};

static int preinit(struct vo *vo, const char *arg)
{
    if (subopt_parse(arg, subopts) != 0) {
        mp_msg(MSGT_VO, MSGL_FATAL,
                "\n-vo corevideo command line help:\n"
                "Example: mplayer -vo corevideo\n"
                "\n" );
        return -1;
    }

    vo_cocoa_init(vo);
    return 0;
}

static CFStringRef get_cv_csp_matrix(void)
{
    switch (colorspace.format) {
        case MP_CSP_BT_601:
            return kCVImageBufferYCbCrMatrix_ITU_R_601_4;
        case MP_CSP_BT_709:
            return kCVImageBufferYCbCrMatrix_ITU_R_709_2;
        case MP_CSP_SMPTE_240M:
            return kCVImageBufferYCbCrMatrix_SMPTE_240M_1995;
    }
    return kCVImageBufferYCbCrMatrix_ITU_R_601_4;
}

static void set_yuv_colorspace(struct vo *vo)
{
    CVBufferSetAttachment(pixelBuffer,
                          kCVImageBufferYCbCrMatrixKey, get_cv_csp_matrix(),
                          kCVAttachmentMode_ShouldPropagate);
    vo->want_redraw = true;
}

static int control(struct vo *vo, uint32_t request, void *data)
{
    switch (request) {
        case VOCTRL_DRAW_IMAGE:
            return draw_image(vo, data);
        case VOCTRL_QUERY_FORMAT:
            return query_format(*(uint32_t*)data);
        case VOCTRL_ONTOP:
            vo_cocoa_ontop(vo);
            return VO_TRUE;
        case VOCTRL_FULLSCREEN:
            vo_cocoa_fullscreen(vo);
            resize(vo, vo->dwidth, vo->dheight);
            return VO_TRUE;
        case VOCTRL_GET_PANSCAN:
            return VO_TRUE;
        case VOCTRL_SET_PANSCAN:
            panscan_calc_windowed(vo);
            return VO_TRUE;
        case VOCTRL_UPDATE_SCREENINFO:
            vo_cocoa_update_xinerama_info(vo);
            return VO_TRUE;
        case VOCTRL_SET_YUV_COLORSPACE:
            colorspace.format = ((struct mp_csp_details *)data)->format;
            set_yuv_colorspace(vo);
            return VO_TRUE;
        case VOCTRL_GET_YUV_COLORSPACE:
            *(struct mp_csp_details *)data = colorspace;
            return VO_TRUE;
    }
    return VO_NOTIMPL;
}

const struct vo_driver video_out_corevideo = {
    .is_new = true,
    .info = &(const vo_info_t) {
        "Mac OS X Core Video",
        "corevideo",
        "Nicolas Plourde <nicolas.plourde@gmail.com> and others",
        ""
    },
    .preinit = preinit,
    .config = config,
    .control = control,
    .draw_osd = draw_osd,
    .flip_page = flip_page,
    .check_events = check_events,
    .uninit = uninit,
};
