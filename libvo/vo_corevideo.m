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
#import "sub/font_load.h"
#import "sub/sub.h"
#import "subopt-helper.h"

#import "csputils.h"
#import "libmpcodecs/vfcap.h"
#import "libmpcodecs/mp_image.h"
#import "osd.h"

#import "gl_common.h"
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

MPGLContext *mpglctx;

#define CV_VERTICES_PER_QUAD 6
#define CV_MAX_OSD_PARTS 20

GLuint osdtex[CV_MAX_OSD_PARTS];
NSRect osdtexrect[CV_MAX_OSD_PARTS];
unsigned int image_width;
unsigned int image_height;
int osdtexCnt = 0;

static struct mp_csp_details colorspace = MP_CSP_DETAILS_DEFAULTS;

static void resize(struct vo *vo, int width, int height)
{
    GL *gl = mpglctx->gl;
    int d_width, d_height;

    mp_msg(MSGT_VO, MSGL_V, "[vo_corevideo] New OpenGL Viewport (0, 0, %d, %d)\n", width, height);

    aspect(vo, &d_width, &d_height, A_WINZOOM);
    textureFrame = NSMakeRect((vo->dwidth - d_width) / 2, (vo->dheight - d_height) / 2, d_width, d_height);

    gl->Viewport(0, 0, width, height);
    gl->MatrixMode(GL_PROJECTION);
    gl->LoadIdentity();
    gl->Ortho(0, width, height, 0, -1.0, 1.0);
    gl->MatrixMode(GL_MODELVIEW);
    gl->LoadIdentity();

    force_load_font = 1;
    vo_osd_changed(OSDTYPE_OSD);

    gl->Clear(GL_COLOR_BUFFER_BIT);
    vo->want_redraw = true;
}

static int init_gl(struct vo *vo, uint32_t d_width, uint32_t d_height)
{
    GL *gl = mpglctx->gl;

    const char *vendor     = gl->GetString(GL_VENDOR);
    const char *version    = gl->GetString(GL_VERSION);
    const char *renderer   = gl->GetString(GL_RENDERER);

    mp_msg(MSGT_VO, MSGL_V, "[vo_corevideo] Running on OpenGL '%s' by '%s', version '%s'\n",
           renderer, vendor, version);

    gl->Disable(GL_BLEND);
    gl->Disable(GL_DEPTH_TEST);
    gl->DepthMask(GL_FALSE);
    gl->Disable(GL_CULL_FACE);
    gl->Enable(GL_TEXTURE_2D);
    gl->DrawBuffer(GL_BACK);
    gl->TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    resize(vo, d_width, d_height);

    gl->ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl->Clear(GL_COLOR_BUFFER_BIT);
    if (gl->SwapInterval)
        gl->SwapInterval(1);
    return 1;
}

static int config(struct vo *vo, uint32_t width, uint32_t height,
                  uint32_t d_width, uint32_t d_height, uint32_t flags,
                  uint32_t format)
{
    CVPixelBufferRelease(pixelBuffer);
    pixelBuffer = NULL;
    CVOpenGLTextureRelease(texture);
    texture = NULL;

    image_width = width;
    image_height = height;

    if (mpglctx->create_window(mpglctx, d_width, d_height, flags) < 0)
        return -1;
    if (mpglctx->setGlWindow(mpglctx) == SET_WINDOW_FAILED)
        return -1;

    init_gl(vo, vo->dwidth, vo->dheight);

    return 0;
}

static void check_events(struct vo *vo)
{
    int e = mpglctx->check_events(vo);
    if (e & VO_EVENT_RESIZE)
        resize(vo, vo->dwidth, vo->dheight);
}

static void create_osd_texture(void *ctx, int x0, int y0, int w, int h,
                               unsigned char *src, unsigned char *srca,
                               int stride)
{
    struct vo *vo = ctx;
    GL *gl = mpglctx->gl;

    if (w <= 0 || h <= 0 || stride < w) {
        mp_msg(MSGT_VO, MSGL_V, "Invalid dimensions OSD for part!\n");
        return;
    }

    if (osdtexCnt >= CV_MAX_OSD_PARTS) {
        mp_msg(MSGT_VO, MSGL_ERR, "Too many OSD parts, contact the developers!\n");
        return;
    }

    gl->GenTextures(1, &osdtex[osdtexCnt]);
    gl->BindTexture(GL_TEXTURE_2D, osdtex[osdtexCnt]);
    glCreateClearTex(gl, GL_TEXTURE_2D, GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA,
                     GL_UNSIGNED_BYTE, GL_LINEAR, w, h, 0);
    {
        int i;
        unsigned char *tmp = malloc(stride * h * 2);
        // convert alpha from weird MPlayer scale.
        for (i = 0; i < h * stride; i++) {
            tmp[i*2+0] = src[i];
            tmp[i*2+1] = -srca[i];
        }
        glUploadTex(gl, GL_TEXTURE_2D, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                    tmp, stride * 2, 0, 0, w, h, 0);
        free(tmp);
    }

    osdtexrect[osdtexCnt] = NSMakeRect(x0, y0, w, h);

    gl->BindTexture(GL_TEXTURE_2D, 0);
    osdtexCnt++;
}

static void clearOSD(struct vo *vo)
{
    GL *gl = mpglctx->gl;
    if (!osdtexCnt)
        return;
    gl->DeleteTextures(osdtexCnt, osdtex);
    osdtexCnt = 0;
}

static void draw_osd(struct vo *vo, struct osd_state *osd)
{
    GL *gl = mpglctx->gl;

    if (vo_osd_changed(0)) {
        clearOSD(vo);
        osd_draw_text_ext(osd, vo->dwidth, vo->dheight, 0, 0, 0, 0,
                          image_width, image_height, create_osd_texture, vo);
    }

    if (osdtexCnt > 0) {
        gl->Enable(GL_BLEND);

        // OSD bitmaps use premultiplied alpha.
        gl->BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        for (int n = 0; n < osdtexCnt; n++) {
            NSRect tr = osdtexrect[n];
            gl->BindTexture(GL_TEXTURE_2D, osdtex[n]);
            glDrawTex(gl, tr.origin.x, tr.origin.y, tr.size.width, tr.size.height,
                      0, 0, 1.0, 1.0, 1, 1, 0, 0, 0);
        }

        gl->Disable(GL_BLEND);
        gl->BindTexture(GL_TEXTURE_2D, 0);
    }
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
    GL *gl = mpglctx->gl;
    prepare_texture();

    gl->Clear(GL_COLOR_BUFFER_BIT);

    gl->Enable(CVOpenGLTextureGetTarget(texture));
    gl->BindTexture(CVOpenGLTextureGetTarget(texture), CVOpenGLTextureGetName(texture));

    gl->Color3f(1,1,1);
    gl->Begin(GL_QUADS);
    gl->TexCoord2f(upperLeft[0], upperLeft[1]); gl->Vertex2f(textureFrame.origin.x-(vo->panscan_x >> 1), textureFrame.origin.y-(vo->panscan_y >> 1));
    gl->TexCoord2f(lowerLeft[0], lowerLeft[1]); gl->Vertex2f(textureFrame.origin.x-(vo->panscan_x >> 1), NSMaxY(textureFrame)+(vo->panscan_y >> 1));
    gl->TexCoord2f(lowerRight[0], lowerRight[1]); gl->Vertex2f(NSMaxX(textureFrame)+(vo->panscan_x >> 1), NSMaxY(textureFrame)+(vo->panscan_y >> 1));
    gl->TexCoord2f(upperRight[0], upperRight[1]); gl->Vertex2f(NSMaxX(textureFrame)+(vo->panscan_x >> 1), textureFrame.origin.y-(vo->panscan_y >> 1));
    gl->End();
    gl->Disable(CVOpenGLTextureGetTarget(texture));
}

static void flip_page(struct vo *vo)
{
    mpglctx->swapGlBuffers(mpglctx);
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
    mpglctx->releaseGlContext(mpglctx);
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

    mpglctx = init_mpglcontext(GLTYPE_COCOA, vo);

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
            mpglctx->ontop(vo);
            return VO_TRUE;
        case VOCTRL_FULLSCREEN:
            mpglctx->fullscreen(vo);
            resize(vo, vo->dwidth, vo->dheight);
            return VO_TRUE;
        case VOCTRL_GET_PANSCAN:
            return VO_TRUE;
        case VOCTRL_SET_PANSCAN:
            panscan_calc_windowed(vo);
            return VO_TRUE;
        case VOCTRL_UPDATE_SCREENINFO:
            mpglctx->update_xinerama_info(vo);
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
