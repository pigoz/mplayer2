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
 *
 * You can alternatively redistribute this file and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

//xxx hack for using GL functions without mplayer's function loader
#if 0
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>
#include "config.h"

#include <libavutil/avutil.h>

#ifdef CONFIG_LCMS2
#include <lcms2.h>
#include "stream/stream.h"
#endif

#include "talloc.h"
#include "bstr.h"
#include "mp_msg.h"
#include "subopt-helper.h"
#include "video_out.h"
#include "libmpcodecs/vfcap.h"
#include "libmpcodecs/mp_image.h"
#include "geometry.h"
#include "osd.h"
#include "sub/font_load.h"
#include "sub/sub.h"
#include "eosd_packer.h"

#include "gl_common.h"
#include "filter_kernels.h"
#include "aspect.h"
#include "fastmemcpy.h"
#include "sub/ass_mp.h"

// generated from libvo/vo_gl3_shaders.glsl
#include "libvo/vo_gl3_shaders.h"

//! How many parts the OSD may consist of at most
#define MAX_OSD_PARTS 20

// Pixel width of 1D lookup textures.
#define LOOKUP_TEXTURE_SIZE 256

// Texture units 0-2 are used by the video, with unit 0 for free use.
// Units 3-4 are used for scaler LUTs.
#define TEXUNIT_SCALERS 3
#define TEXUNIT_3DLUT 5
#define TEXUNIT_DITHER 6

// lscale/cscale arguments that map directly to shader filter routines.
// Note that the convolution filters are not included in this list.
static const char *fixed_scale_filters[] = {
    "bilinear",
    "bicubic_fast",
    "sharpen3",
    "sharpen5",
    NULL
};

struct lut_tex_format {
    int pixels;
    GLint internal_format;
    GLenum format;
};

// Indexed with filter_kernel->size.
// This must match the weightsN functions in the shader.
// Each entry uses (size+3)/4 pixels per LUT entry, and size/pixels components
// per pixel.
struct lut_tex_format lut_tex_formats[] = {
    [2] =  {1, GL_RG16F,   GL_RG},
    [4] =  {1, GL_RGBA16F, GL_RGBA},
    [6] =  {2, GL_RGB16F,  GL_RGB},
    [8] =  {2, GL_RGBA16F, GL_RGBA},
    [12] = {3, GL_RGBA16F, GL_RGBA},
    [16] = {4, GL_RGBA16F, GL_RGBA},
};

// must be sorted, and terminated with 0
static const int filter_sizes[] = {2, 4, 6, 8, 12, 16, 0};

struct vertex {
    float position[2];
    uint8_t color[4];
    float texcoord[2];
};

#define VERTEX_ATTRIB_POSITION 0
#define VERTEX_ATTRIB_COLOR 1
#define VERTEX_ATTRIB_TEXCOORD 2

#define VERTICES_PER_QUAD 6

struct texplane {
    // chroma shifts
    // e.g. get the plane's width in pixels with (priv->src_width >> shift_x)
    int shift_x, shift_y;
    // GL state
    GLuint gl_texture;
    // temporary locking during uploading the frame (e.g. for draw_slice)
    int gl_buffer;
    int buffer_size;
    void *buffer_ptr;
    // value used to clear the image with memset (YUV chroma planes do not use
    // the value 0 for this)
    uint8_t clear_val;
};

struct scaler {
    int index;
    const char *name;
    float params[2];
    struct filter_kernel *kernel;
    GLuint gl_lut;
    const char *lut_name;

    // kernel points here
    struct filter_kernel kernel_storage;
};

struct fbotex {
    GLuint fbo;
    GLuint texture;
    int tex_w, tex_h;           // size of .texture
    int vp_w, vp_h;             // viewport of fbo / used part of the texture
};

struct gl_priv {
    struct vo *vo;
    MPGLContext *glctx;
    GL *gl;

    int gl_debug;
    int force_gl2;

    GLuint vertex_buffer;
    GLuint vao;

    GLuint osd_program, eosd_program;
    GLuint indirect_program, scale_sep_program, final_program;

    //! Textures for OSD
    GLuint osdtex[MAX_OSD_PARTS];
    int osdtex_count;
    GLuint eosd_texture;
    GLuint eosd_buffer;
    int eosd_texture_width, eosd_texture_height;
    struct eosd_packer *eosd;
    struct vertex *eosd_va;
    struct vertex osd_va[MAX_OSD_PARTS * VERTICES_PER_QUAD];
    int osd_color;

    GLuint lut_3d_texture;
    int lut_3d_w, lut_3d_h, lut_3d_d;
    void *lut_3d_data;

    GLuint dither_texture;
    float dither_quantization;
    float dither_multiply;

    int use_indirect;           // convert YUV to RGB texture first
    int use_gamma;
    int use_srgb;
    int use_scale_sep;
    int use_fancy_downscaling;
    int use_lut_3d;
    int dither_depth;
    struct mp_csp_details colorspace;
    bool is_yuv;
    bool is_linear_rgb;
    float filter_strength;
    int use_npot;
    uint32_t image_width;
    uint32_t image_height;
    uint32_t image_format;
    uint32_t image_d_width;
    uint32_t image_d_height;
    int use_pbo;
    int use_glFinish;
    int swap_interval;
    GLint fbo_format;

    // per pixel (full pixel when packed, each component when planar)
    int plane_bytes;
    int plane_bits;

    GLint gl_internal_format;
    GLenum gl_format;
    GLenum gl_type;

    int plane_count;
    struct texplane planes[3];

    struct fbotex indirect_fbo;         // RGB target
    struct fbotex scale_sep_fbo;        // first pass when doing 2 pass scaling

    // state for luma and chroma scalers
    struct scaler scalers[2];
    float scaler_params[2];

    int mipmap_gen;
    int stereo_mode;

    struct mp_csp_equalizer video_eq;

    int texture_width;
    int texture_height;
    int mpi_flipped;
    int vo_flipped;

    struct vo_rect src_rect;    // displayed part of the source video
    struct vo_rect dst_rect;    // video rectangle on output window
    int border_x, border_y;     // OSD borders
    int vp_x, vp_y, vp_w, vp_h; // GL viewport
};

struct fmt_entry {
    int mp_format;
    GLint internal_format;
    GLenum format;
    GLenum type;
};

static const struct fmt_entry mp_to_gl_formats[] = {
    {IMGFMT_RGB48NE, GL_RGB16, GL_RGB,  GL_UNSIGNED_SHORT},
    {IMGFMT_RGB24,   GL_RGB,   GL_RGB,  GL_UNSIGNED_BYTE},
    {IMGFMT_RGBA,    GL_RGBA,  GL_RGBA, GL_UNSIGNED_BYTE},
    {IMGFMT_RGB15,   GL_RGBA,  GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV},
    {IMGFMT_RGB16,   GL_RGB,   GL_RGB,  GL_UNSIGNED_SHORT_5_6_5_REV},
    {IMGFMT_BGR15,   GL_RGBA,  GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV},
    {IMGFMT_BGR16,   GL_RGB,   GL_RGB,  GL_UNSIGNED_SHORT_5_6_5},
    {IMGFMT_BGR24,   GL_RGB,   GL_BGR,  GL_UNSIGNED_BYTE},
    {IMGFMT_BGRA,    GL_RGBA,  GL_BGRA, GL_UNSIGNED_BYTE},
    {0},
};

// Return the high byte of the value that represents white in chroma (U/V)
static int get_chroma_clear_val(int bit_depth)
{
    return 1 << (bit_depth - 1 & 7);
}

static bool init_format(int fmt, struct gl_priv *init)
{
    bool supported = false;
    struct gl_priv dummy;
    if (!init)
        init = &dummy;

    mp_image_t dummy_img = {0};
    mp_image_setfmt(&dummy_img, fmt);

    init->image_format = fmt;

    // RGB/packed formats
    for (const struct fmt_entry *e = mp_to_gl_formats; e->mp_format; e++) {
        if (e->mp_format == fmt) {
            supported = true;
            init->plane_bits = dummy_img.bpp;
            init->gl_format = e->format;
            init->gl_internal_format = e->internal_format;
            init->gl_type = e->type;
            break;
        }
    }

    // YUV/planar formats
    if (!supported && mp_get_chroma_shift(fmt, NULL, NULL, &init->plane_bits)) {
        init->gl_format = GL_RED;
        if (init->plane_bits == 8) {
            supported = true;
            init->gl_internal_format = GL_RED;
            init->gl_type = GL_UNSIGNED_BYTE;
        } else if (IMGFMT_IS_YUVP16_NE(fmt)) {
            supported = true;
            init->gl_internal_format = GL_R16;
            init->gl_type = GL_UNSIGNED_SHORT;
        }
    }

    if (!supported)
        return false;

    init->plane_bytes = (init->plane_bits + 7) / 8;
    init->is_yuv = dummy_img.flags & MP_IMGFLAG_YUV;
    init->is_linear_rgb = false;

    // NOTE: we throw away the additional alpha plane, if one exists.
    init->plane_count = dummy_img.num_planes > 2 ? 3 : 1;
    assert(dummy_img.num_planes >= init->plane_count);
    assert(dummy_img.num_planes <= init->plane_count + 1);

    for (int n = 0; n < init->plane_count; n++) {
        struct texplane *plane = &init->planes[n];

        plane->shift_x = n > 0 ? dummy_img.chroma_x_shift : 0;
        plane->shift_y = n > 0 ? dummy_img.chroma_y_shift : 0;
        plane->clear_val = n > 0 && init->is_yuv
                           ? get_chroma_clear_val(init->plane_bits) : 0;
    }

    return true;
}

static void default_tex_params(struct GL *gl, GLenum target, GLint filter)
{
    gl->TexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
    gl->TexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
    gl->TexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->TexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

static void tex_size(struct gl_priv *p, int w, int h, int *texw, int *texh)
{
    if (p->use_npot) {
        *texw = w;
        *texh = h;
    } else {
        *texw = 32;
        while (*texw < w)
            *texw *= 2;
        *texh = 32;
        while (*texh < h)
            *texh *= 2;
    }
}

static void matrix_ortho2d(float m[3][3], float x0, float x1,
                           float y0, float y1)
{
    memset(m, 0, 9 * sizeof(float));
    m[0][0] = 2.0f / (x1 - x0);
    m[1][1] = 2.0f / (y1 - y0);
    m[2][0] = -(x1 + x0) / (x1 - x0);
    m[2][1] = -(y1 + y0) / (y1 - y0);
    m[2][2] = 1.0f;
}

static bool can_use_filter_kernel(const struct filter_kernel *kernel)
{
    if (!kernel)
        return false;
    struct filter_kernel k = *kernel;
    return mp_init_filter(&k, filter_sizes, 1);
}

static void draw_triangles(struct gl_priv *p, struct vertex *vb, int vert_count)
{
    GL *gl = p->gl;

    assert(vert_count % 3 == 0);

    gl->BindBuffer(GL_ARRAY_BUFFER, p->vertex_buffer);
    gl->BufferData(GL_ARRAY_BUFFER, vert_count * sizeof(struct vertex), vb,
                   GL_DYNAMIC_DRAW);
    gl->BindBuffer(GL_ARRAY_BUFFER, 0);

    gl->BindVertexArray(p->vao);
    gl->DrawArrays(GL_TRIANGLES, 0, vert_count);
    gl->BindVertexArray(0);

    glCheckError(gl, "after rendering");
}

// Write a textured quad to a vertex array.
// va = destination vertex array, VERTICES_PER_QUAD entries will be overwritten
// x0, y0, x1, y1 = destination coordinates of the quad
// tx0, ty0, tx1, ty1 = source texture coordinates (usually in pixels)
// texture_w, texture_h = size of the texture, or an inverse factor
// color = optional color for all vertices, NULL for opaque white
// flip = flip vertically
static void write_quad(struct vertex *va,
                       float x0, float y0, float x1, float y1,
                       float tx0, float ty0, float tx1, float ty1,
                       float texture_w, float texture_h,
                       const uint8_t color[4], bool flip)
{
    static const uint8_t white[4] = { 255, 255, 255, 255 };

    if (!color)
        color = white;

    tx0 /= texture_w;
    ty0 /= texture_h;
    tx1 /= texture_w;
    ty1 /= texture_h;

    if (flip) {
        float tmp = ty0;
        ty0 = ty1;
        ty1 = tmp;
    }

#define COLOR_INIT {color[0], color[1], color[2], color[3]}
    va[0] = (struct vertex) { {x0, y0}, COLOR_INIT, {tx0, ty0} };
    va[1] = (struct vertex) { {x0, y1}, COLOR_INIT, {tx0, ty1} };
    va[2] = (struct vertex) { {x1, y0}, COLOR_INIT, {tx1, ty0} };
    va[3] = (struct vertex) { {x1, y1}, COLOR_INIT, {tx1, ty1} };
    va[4] = va[2];
    va[5] = va[1];
#undef COLOR_INIT
}

static void fbotex_init(struct gl_priv *p, struct fbotex *fbo, int w, int h)
{
    GL *gl = p->gl;

    assert(!fbo->fbo);
    assert(!fbo->texture);

    tex_size(p, w, h, &fbo->tex_w, &fbo->tex_h);

    fbo->vp_w = w;
    fbo->vp_h = h;

    mp_msg(MSGT_VO, MSGL_V, "[gl] Create FBO: %dx%d\n", fbo->tex_w, fbo->tex_h);

    gl->GenFramebuffers(1, &fbo->fbo);
    gl->GenTextures(1, &fbo->texture);
    gl->BindTexture(GL_TEXTURE_2D, fbo->texture);
    gl->TexImage2D(GL_TEXTURE_2D, 0, p->fbo_format, fbo->tex_w, fbo->tex_h, 0,
                   GL_RGB, GL_UNSIGNED_BYTE, NULL);
    default_tex_params(gl, GL_TEXTURE_2D, GL_LINEAR);
    gl->BindFramebuffer(GL_FRAMEBUFFER, fbo->fbo);
    gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, fbo->texture, 0);

    if (gl->CheckFramebufferStatus(GL_FRAMEBUFFER)
        != GL_FRAMEBUFFER_COMPLETE)
    {
        mp_msg(MSGT_VO, MSGL_ERR, "[gl] Error: framebuffer completeness "
                                    "check failed!\n");
    }

    gl->BindFramebuffer(GL_FRAMEBUFFER, 0);

    glCheckError(gl, "after creating framebuffer & associated texture");
}

static void fbotex_uninit(struct gl_priv *p, struct fbotex *fbo)
{
    GL *gl = p->gl;

    gl->DeleteFramebuffers(1, &fbo->fbo);
    fbo->fbo = 0;
    gl->DeleteTextures(1, &fbo->texture);
    fbo->texture = 0;
    fbo->tex_w = fbo->tex_h = 0;
    fbo->vp_w = fbo->vp_h = 0;
}

static void update_uniforms(struct gl_priv *p, GLuint program)
{
    GL *gl = p->gl;
    GLint loc;

    if (program == 0)
        return;

    gl->UseProgram(program);

    struct mp_csp_params cparams = {
        .colorspace = p->colorspace,
        .input_bits = p->plane_bits,
        .texture_bits = (p->plane_bits + 7) & ~7,
    };
    mp_csp_copy_equalizer_values(&cparams, &p->video_eq);

    loc = gl->GetUniformLocation(program, "transform");
    if (loc >= 0) {
        float matrix[3][3];
        matrix_ortho2d(matrix, 0, p->vp_w, p->vp_h, 0);
        gl->UniformMatrix3fv(loc, 1, GL_FALSE, &matrix[0][0]);
    }

    loc = gl->GetUniformLocation(program, "colormatrix");
    if (loc >= 0) {
        float yuv2rgb[3][4];
        mp_get_yuv2rgb_coeffs(&cparams, yuv2rgb);
        gl->UniformMatrix4x3fv(loc, 1, GL_TRUE, &yuv2rgb[0][0]);
    }

    loc = gl->GetUniformLocation(program, "inv_gamma");
    if (loc >= 0) {
        gl->Uniform3f(loc, 1.0 / cparams.rgamma, 1.0 / cparams.ggamma,
                      1.0 / cparams.bgamma);
    }

    loc = gl->GetUniformLocation(program, "texture1");
    if (loc >= 0)
        gl->Uniform1i(loc, 0);
    loc = gl->GetUniformLocation(program, "texture2");
    if (loc >= 0)
        gl->Uniform1i(loc, 1);
    loc = gl->GetUniformLocation(program, "texture3");
    if (loc >= 0)
        gl->Uniform1i(loc, 2);

    loc = gl->GetUniformLocation(program, "lut_3d");
    if (loc >= 0)
        gl->Uniform1i(loc, TEXUNIT_3DLUT);

    for (int n = 0; n < 2; n++) {
        const char *lut = p->scalers[n].lut_name;
        if (lut) {
            GLint loc = gl->GetUniformLocation(program, lut);
            if (loc >= 0)
                gl->Uniform1i(loc, TEXUNIT_SCALERS + n);
        }
    }

    loc = gl->GetUniformLocation(program, "dither");
    if (loc >= 0)
        gl->Uniform1i(loc, TEXUNIT_DITHER);

    loc = gl->GetUniformLocation(program, "dither_quantization");
    if (loc >= 0)
        gl->Uniform1f(loc, p->dither_quantization);

    loc = gl->GetUniformLocation(program, "dither_multiply");
    if (loc >= 0)
        gl->Uniform1f(loc, p->dither_multiply);

    loc = gl->GetUniformLocation(program, "filter_strength");
    if (loc >= 0)
        gl->Uniform1f(loc, p->filter_strength);

    gl->UseProgram(0);

    glCheckError(gl, "update_uniforms()");
}

static void update_all_uniforms(struct gl_priv *p)
{
    update_uniforms(p, p->osd_program);
    update_uniforms(p, p->eosd_program);
    update_uniforms(p, p->indirect_program);
    update_uniforms(p, p->scale_sep_program);
    update_uniforms(p, p->final_program);
}

static void update_window_sized_objects(struct gl_priv *p)
{
    if (p->scale_sep_program) {
        if (p->dst_rect.height > p->scale_sep_fbo.tex_h) {
            fbotex_uninit(p, &p->scale_sep_fbo);
            // Round up to an arbitrary alignment to make window resizing or
            // panscan controls smoother (less texture reallocations).
            int height = FFALIGN(p->dst_rect.height, 256);
            fbotex_init(p, &p->scale_sep_fbo, p->image_width, height);
        }
        p->scale_sep_fbo.vp_w = p->image_width;
        p->scale_sep_fbo.vp_h = p->dst_rect.height;
    }
}

static void reinit_rendering(struct gl_priv *p);
static void init_scaler(struct gl_priv *p, struct scaler *scaler);

static double get_scale_factor(struct gl_priv *p)
{
    double sx = p->dst_rect.width / (double)p->src_rect.width;
    double sy = p->dst_rect.height / (double)p->src_rect.height;
    // xxx: actually we should use different scalers in X/Y directions if the
    // scale factors are different due to anamorphic content
    return FFMIN(sx, sy);
}

static bool update_scale_factor(struct gl_priv *p, struct filter_kernel *kernel)
{
    double scale = get_scale_factor(p);
    if (!p->use_fancy_downscaling && scale < 1.0)
        scale = 1.0;
    return mp_init_filter(kernel, filter_sizes, FFMAX(1.0, 1.0/scale));
}

static void resize(struct gl_priv *p)
{
    GL *gl = p->gl;
    struct vo *vo = p->vo;

    mp_msg(MSGT_VO, MSGL_V, "[gl] Resize: %dx%d\n", vo->dwidth, vo->dheight);
    p->vp_x = 0, p->vp_y = 0;
    if (WinID >= 0) {
        int w = vo->dwidth, h = vo->dheight;
        int old_y = vo->dheight;
        geometry(&p->vp_x, &p->vp_y, &w, &h, vo->dwidth, vo->dheight);
        p->vp_y = old_y - h - p->vp_y;
    }
    p->vp_w = vo->dwidth, p->vp_h = vo->dheight;
    gl->Viewport(p->vp_x, p->vp_y, p->vp_w, p->vp_h);

    struct vo_rect borders;
    calc_src_dst_rects(vo, p->image_width, p->image_height, &p->src_rect,
                       &p->dst_rect, &borders, NULL);
    p->border_x = borders.left;
    p->border_y = borders.top;

    bool need_scaler_reinit = false;    // filter size change needed
    bool need_scaler_update = false;    // filter LUT change needed
    bool too_small = false;
    for (int n = 0; n < 2; n++) {
        if (p->scalers[n].kernel) {
            struct filter_kernel tkernel = *p->scalers[n].kernel;
            struct filter_kernel old = tkernel;
            bool ok = update_scale_factor(p, &tkernel);
            too_small |= !ok;
            need_scaler_reinit |= (tkernel.size != old.size);
            need_scaler_update |= (tkernel.inv_scale != old.inv_scale);
        }
    }
    if (need_scaler_reinit) {
        reinit_rendering(p);
    } else if (need_scaler_update) {
        init_scaler(p, &p->scalers[0]);
        init_scaler(p, &p->scalers[1]);
    }
    if (too_small)
        mp_msg(MSGT_VO, MSGL_WARN, "[gl] Can't downscale that much, window "
               "output may look suboptimal.\n");

    update_window_sized_objects(p);
    update_all_uniforms(p);

#ifdef CONFIG_FREETYPE
    // adjust font size to display size
    force_load_font = 1;
#endif
    vo_osd_changed(OSDTYPE_OSD);

    gl->Clear(GL_COLOR_BUFFER_BIT);
    vo->want_redraw = true;
}

/**
 * \brief remove all OSD textures and display-lists, thus clearing it.
 */
static void clear_osd(struct gl_priv *p)
{
    GL *gl = p->gl;

    if (!p->osdtex_count)
        return;
    gl->DeleteTextures(p->osdtex_count, p->osdtex);
    p->osdtex_count = 0;
}

/**
 * \brief construct display list from ass image list
 * \param img image list to create OSD from.
 */
static void gen_eosd(struct gl_priv *p, mp_eosd_images_t *imgs)
{
    GL *gl = p->gl;

    bool need_repos, need_upload, need_allocate;
    eosd_packer_generate(p->eosd, imgs, &need_repos, &need_upload,
                         &need_allocate);

    if (!need_repos)
        return;

    if (!p->eosd_texture) {
        gl->GenTextures(1, &p->eosd_texture);
        gl->GenBuffers(1, &p->eosd_buffer);
    }

    gl->BindTexture(GL_TEXTURE_2D, p->eosd_texture);

    if (need_allocate) {
        tex_size(p, p->eosd->surface.w, p->eosd->surface.h,
                 &p->eosd_texture_width, &p->eosd_texture_height);
        gl->TexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                       p->eosd_texture_width, p->eosd_texture_height, 0,
                       GL_RED, GL_UNSIGNED_BYTE, NULL);
        default_tex_params(gl, GL_TEXTURE_2D, GL_NEAREST);
        gl->BindBuffer(GL_PIXEL_UNPACK_BUFFER, p->eosd_buffer);
        gl->BufferData(GL_PIXEL_UNPACK_BUFFER,
                       p->eosd->surface.w * p->eosd->surface.h,
                       NULL,
                       GL_DYNAMIC_COPY);
        gl->BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

    // 2 triangles primitives per quad = 6 vertices per quad
    // not using GL_QUADS, as it is deprecated in OpenGL 3.x and later
    p->eosd_va = talloc_realloc_size(p->eosd, p->eosd_va,
                                     p->eosd->targets_count
                                     * sizeof(struct vertex)
                                     * VERTICES_PER_QUAD);

    if (need_upload && p->use_pbo) {
        gl->BindBuffer(GL_PIXEL_UNPACK_BUFFER, p->eosd_buffer);
        char *data = gl->MapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        if (!data) {
            mp_msg(MSGT_VO, MSGL_FATAL, "[gl] Error: can't upload subtitles! "
                                        "Subtitles will look corrupted.\n");
        } else {
            for (int n = 0; n < p->eosd->targets_count; n++) {
                struct eosd_target *target = &p->eosd->targets[n];
                ASS_Image *i = target->ass_img;

                void *pdata = data + target->source.y0 * p->eosd->surface.w
                              + target->source.x0;

                memcpy_pic(pdata, i->bitmap, i->w, i->h,
                           p->eosd->surface.w, i->stride);
            }
            if (!gl->UnmapBuffer(GL_PIXEL_UNPACK_BUFFER))
                mp_msg(MSGT_VO, MSGL_FATAL, "[gl] EOSD PBO upload failed. "
                       "Remove the 'pbo' suboption.\n");
            struct eosd_rect rc;
            eosd_packer_calculate_source_bb(p->eosd, &rc);
            glUploadTex(gl, GL_TEXTURE_2D, GL_RED, GL_UNSIGNED_BYTE, NULL,
                        p->eosd->surface.w, rc.x0, rc.y0,
                        rc.x1 - rc.x0, rc.y1 - rc.y0, 0);
        }
        gl->BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    } else if (need_upload) {
        // non-PBO upload
        for (int n = 0; n < p->eosd->targets_count; n++) {
            struct eosd_target *target = &p->eosd->targets[n];
            ASS_Image *i = target->ass_img;

            glUploadTex(gl, GL_TEXTURE_2D, GL_RED, GL_UNSIGNED_BYTE, i->bitmap,
                        i->stride, target->source.x0, target->source.y0,
                        i->w, i->h, 0);
        }
    }

    gl->BindTexture(GL_TEXTURE_2D, 0);

    glCheckError(gl, "EOSD upload");

    for (int n = 0; n < p->eosd->targets_count; n++) {
        struct eosd_target *target = &p->eosd->targets[n];
        ASS_Image *i = target->ass_img;
        uint8_t color[4] = { i->color >> 24, (i->color >> 16) & 0xff,
                            (i->color >> 8) & 0xff, 255 - (i->color & 0xff) };

        write_quad(&p->eosd_va[n * VERTICES_PER_QUAD],
                   target->dest.x0, target->dest.y0,
                   target->dest.x1, target->dest.y1,
                   target->source.x0, target->source.y0,
                   target->source.x1, target->source.y1,
                   p->eosd_texture_width, p->eosd_texture_height,
                   color, false);
    }
}

static void draw_eosd(struct gl_priv *p, mp_eosd_images_t *imgs)
{
    GL *gl = p->gl;

    gen_eosd(p, imgs);

    if (p->eosd->targets_count == 0)
        return;

    gl->Enable(GL_BLEND);
    gl->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl->BindTexture(GL_TEXTURE_2D, p->eosd_texture);
    gl->UseProgram(p->eosd_program);
    draw_triangles(p, p->eosd_va, p->eosd->targets_count * VERTICES_PER_QUAD);
    gl->UseProgram(0);
    gl->BindTexture(GL_TEXTURE_2D, 0);
    gl->Disable(GL_BLEND);
}

static void delete_program(GL *gl, GLuint *prog)
{
    gl->DeleteProgram(*prog);
    *prog = 0;
}

static void delete_shaders(struct gl_priv *p)
{
    GL *gl = p->gl;

    delete_program(gl, &p->osd_program);
    delete_program(gl, &p->eosd_program);
    delete_program(gl, &p->indirect_program);
    delete_program(gl, &p->scale_sep_program);
    delete_program(gl, &p->final_program);
}

static void uninit_rendering(struct gl_priv *p)
{
    GL *gl = p->gl;

    delete_shaders(p);

    for (int n = 0; n < 2; n++) {
        gl->DeleteTextures(1, &p->scalers->gl_lut);
        p->scalers->gl_lut = 0;
        p->scalers->lut_name = NULL;
        p->scalers->kernel = NULL;
    }

    gl->DeleteTextures(1, &p->dither_texture);
    p->dither_texture = 0;
}

// Free video resources etc.
static void uninit_video(struct gl_priv *p)
{
    GL *gl = p->gl;

    uninit_rendering(p);

    for (int n = 0; n < 3; n++) {
        struct texplane *plane = &p->planes[n];

        gl->DeleteTextures(1, &plane->gl_texture);
        plane->gl_texture = 0;
        gl->DeleteBuffers(1, &plane->gl_buffer);
        plane->gl_buffer = 0;
        plane->buffer_ptr = NULL;
        plane->buffer_size = 0;
    }

    fbotex_uninit(p, &p->indirect_fbo);
    fbotex_uninit(p, &p->scale_sep_fbo);
}

/**
 * \brief uninitialize OpenGL context, freeing textures, buffers etc.
 */
static void uninit_gl(struct gl_priv *p)
{
    GL *gl = p->gl;

    // NOTE: GL functions might not be loaded yet
    if (!(p->glctx && p->gl->DeleteTextures))
        return;

    uninit_video(p);

    gl->DeleteVertexArrays(1, &p->vao);
    p->vao = 0;
    gl->DeleteBuffers(1, &p->vertex_buffer);
    p->vertex_buffer = 0;

    clear_osd(p);
    gl->DeleteTextures(1, &p->eosd_texture);
    p->eosd_texture = 0;
    gl->DeleteBuffers(1, &p->eosd_buffer);
    p->eosd_buffer = 0;
    eosd_packer_reinit(p->eosd, 0, 0);

    gl->DeleteTextures(1, &p->lut_3d_texture);
    p->lut_3d_texture = 0;
}

#define SECTION_HEADER "#!section "

static char *get_section(void *talloc_ctx, struct bstr source,
                         const char *section)
{
    char *res = talloc_strdup(talloc_ctx, "");
    bool copy = false;
    while (source.len) {
        struct bstr line = bstr_getline(source, &source);
        if (bstr_eatstart(&line, bstr(SECTION_HEADER))) {
            copy = bstrcmp0(line, section) == 0;
        } else {
            if (copy)
                res = talloc_asprintf_append_buffer(res, "%.*s\n", BSTR_P(line));
        }
    }
    return res;
}

static GLuint create_shader(GL *gl, GLenum type, const char *header,
                            const char *source)
{
    void *ctx = talloc_new(NULL);
    const char *full_source = talloc_asprintf(ctx, "%s%s", header, source);

    GLuint shader = gl->CreateShader(type);
    gl->ShaderSource(shader, 1, &full_source, NULL);
    gl->CompileShader(shader);
    GLint status;
    gl->GetShaderiv(shader, GL_COMPILE_STATUS, &status);
    GLint log_length;
    gl->GetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

    int pri = status ? (log_length > 1 ? MSGL_V : MSGL_DBG2) : MSGL_ERR;
    const char *typestr = type == GL_VERTEX_SHADER ? "vertex" : "fragment";
    if (mp_msg_test(MSGT_VO, pri)) {
        mp_msg(MSGT_VO, pri, "[gl] %s shader source:\n", typestr);
        mp_log_source(MSGT_VO, pri, full_source);
    }
    if (log_length > 1) {
        GLchar *log = talloc_zero_size(ctx, log_length + 1);
        gl->GetShaderInfoLog(shader, log_length, NULL, log);
        mp_msg(MSGT_VO, pri, "[gl] %s shader compile log (status=%d):\n%s\n",
               typestr, status, log);
    }

    talloc_free(ctx);

    return shader;
}

static void prog_create_shader(GL *gl, GLuint program, GLenum type,
                               const char *header, const char *source)
{
    GLuint shader = create_shader(gl, type, header, source);
    gl->AttachShader(program, shader);
    gl->DeleteShader(shader);
}

static void bind_attrib_locs(GL *gl, GLuint program)
{
    gl->BindAttribLocation(program, VERTEX_ATTRIB_POSITION, "vertex_position");
    gl->BindAttribLocation(program, VERTEX_ATTRIB_COLOR, "vertex_color");
    gl->BindAttribLocation(program, VERTEX_ATTRIB_TEXCOORD, "vertex_texcoord");
}

static void link_shader(GL *gl, GLuint program)
{
    gl->LinkProgram(program);
    GLint status;
    gl->GetProgramiv(program, GL_LINK_STATUS, &status);
    GLint log_length;
    gl->GetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

    int pri = status ? (log_length > 1 ? MSGL_V : MSGL_DBG2) : MSGL_ERR;
    if (mp_msg_test(MSGT_VO, pri)) {
        GLchar *log = talloc_zero_size(NULL, log_length + 1);
        gl->GetProgramInfoLog(program, log_length, NULL, log);
        mp_msg(MSGT_VO, pri, "[gl] shader link log (status=%d): %s\n",
               status, log);
        talloc_free(log);
    }
}

static GLuint create_program(GL *gl, const char *name, const char *header,
                             const char *vertex, const char *frag)
{
    mp_msg(MSGT_VO, MSGL_V, "[gl] compiling shader program '%s'\n", name);
    mp_msg(MSGT_VO, MSGL_V, "[gl] header:\n");
    mp_log_source(MSGT_VO, MSGL_V, header);
    GLuint prog = gl->CreateProgram();
    prog_create_shader(gl, prog, GL_VERTEX_SHADER, header, vertex);
    prog_create_shader(gl, prog, GL_FRAGMENT_SHADER, header, frag);
    bind_attrib_locs(gl, prog);
    link_shader(gl, prog);
    return prog;
}

static void shader_def(char **shader, const char *name,
                       const char *value)
{
    *shader = talloc_asprintf_append(*shader, "#define %s %s\n", name, value);
}

static void shader_def_opt(char **shader, const char *name, bool b)
{
    if (b)
        shader_def(shader, name, "1");
}

static void shader_setup_scaler(char **shader, struct scaler *scaler, int pass)
{
    const char *target = scaler->index == 0 ? "SAMPLE_L" : "SAMPLE_C";
    if (!scaler->kernel) {
        *shader = talloc_asprintf_append(*shader, "#define %s sample_%s\n",
                                         target, scaler->name);
    } else {
        int size = scaler->kernel->size;
        if (pass != -1) {
            // The direction/pass assignment is rather arbitrary, but fixed in
            // other parts of the code (like FBO setup).
            const char *direction = pass == 0 ? "0, 1" : "1, 0";
            *shader = talloc_asprintf_append(*shader, "#define %s(p0, p1) "
                "sample_convolution_sep%d(vec2(%s), %s, p0, p1)\n",
                target, size, direction, scaler->lut_name);
        } else {
            *shader = talloc_asprintf_append(*shader, "#define %s(p0, p1) "
                "sample_convolution%d(%s, p0, p1)\n",
                target, size, scaler->lut_name);
        }
    }
}

// return false if RGB or 4:4:4 YUV
static bool input_is_subsampled(struct gl_priv *p)
{
    for (int i = 0; i < p->plane_count; i++)
        if (p->planes[i].shift_x || p->planes[i].shift_y)
            return true;
    return false;
}

static void compile_shaders(struct gl_priv *p)
{
    GL *gl = p->gl;

    delete_shaders(p);

    void *tmp = talloc_new(NULL);

    struct bstr src = { (char*)vo_gl3_shaders, sizeof(vo_gl3_shaders) };
    char *vertex_shader = get_section(tmp, src, "vertex_all");
    char *shader_prelude = get_section(tmp, src, "prelude");
    char *s_video = get_section(tmp, src, "frag_video");
    char *s_eosd = get_section(tmp, src, "frag_eosd");
    char *s_osd = get_section(tmp, src, "frag_osd");

    char *header = talloc_strdup(tmp, shader_prelude);

    char *header_eosd = talloc_strdup(tmp, header);
    shader_def_opt(&header_eosd, "USE_3DLUT", p->use_lut_3d);

    p->eosd_program =
        create_program(gl, "eosd", header_eosd, vertex_shader, s_eosd);

    p->osd_program =
        create_program(gl, "osd", header, vertex_shader, s_osd);

    char *header_conv = talloc_strdup(tmp, "");
    char *header_final = talloc_strdup(tmp, "");
    char *header_sep = NULL;

    shader_def_opt(&header_conv, "USE_PLANAR", p->plane_count > 1);
    shader_def_opt(&header_conv, "USE_YGRAY", p->is_yuv && p->plane_count == 1);
    shader_def_opt(&header_conv, "USE_COLORMATRIX", p->is_yuv);
    shader_def_opt(&header_conv, "USE_LINEAR_CONV",
                   !p->is_linear_rgb && (p->use_srgb || p->use_lut_3d));

    shader_def_opt(&header_final, "USE_LINEAR_CONV_INV", p->use_lut_3d);
    shader_def_opt(&header_final, "USE_GAMMA_POW", p->use_gamma);
    shader_def_opt(&header_final, "USE_3DLUT", p->use_lut_3d);
    shader_def_opt(&header_final, "USE_DITHER", p->dither_texture != 0);

    if (p->use_scale_sep && p->scalers[0].kernel) {
        header_sep = talloc_strdup(tmp, "");
        shader_def_opt(&header_sep, "FIXED_SCALE", true);
        shader_setup_scaler(&header_sep, &p->scalers[0], 0);
        shader_setup_scaler(&header_final, &p->scalers[0], 1);
    } else {
        shader_setup_scaler(&header_final, &p->scalers[0], -1);
    }

    bool use_indirect = p->use_indirect;

    // It doesn't make sense to scale the chroma with cscale in the 1. scale
    // step and with lscale in the 2. step. Also, even with 4:4:4 YUV or planar
    // RGB, the indirection might be faster: the shader can't use one scaler for
    // sampling from 3 textures, but has to fetch the coefficients for each
    // texture separately, even though they're the same (this is not an inherent
    // restriction, but would require to restructure the shader).
    if (header_sep && p->plane_count > 1)
        use_indirect = true;

    if (input_is_subsampled(p)) {
        shader_setup_scaler(&header_conv, &p->scalers[1], -1);
    } else {
        // Force using the luma scaler on chroma. If the "indirect" stage is
        // used, the actual scaling will happen in the next stage.
        shader_def(&header_conv, "SAMPLE_C",
                   use_indirect ? "sample_bilinear" : "SAMPLE_L");
    }

    // We want to do scaling in linear light. Scaling is closely connected to
    // texture sampling due to how the shader is structured (or if GL bilinear
    // scaling is used). The purpose of the "indirect" pass is to convert the
    // input video to linear RGB.
    // Another purpose is reducing input to a single texture for scaling.
    if (use_indirect) {
        // We don't use filtering for the Y-plane (luma), because it's never
        // scaled in this scenario.
        shader_def(&header_conv, "SAMPLE_L", "sample_bilinear");
        shader_def_opt(&header_conv, "FIXED_SCALE", true);
        header_conv = talloc_asprintf(tmp, "%s%s", header, header_conv);
        p->indirect_program =
            create_program(gl, "indirect", header_conv, vertex_shader, s_video);
    } else if (header_sep) {
        header_sep = talloc_asprintf(tmp, "%s%s", header_sep, header_conv);
    } else {
        header_final = talloc_asprintf(tmp, "%s%s", header_final, header_conv);
    }

    if (header_sep) {
        header_sep = talloc_asprintf(tmp, "%s%s", header, header_sep);
        p->scale_sep_program =
            create_program(gl, "scale_sep", header_sep, vertex_shader, s_video);
    }

    header_final = talloc_asprintf(tmp, "%s%s", header, header_final);
    p->final_program =
        create_program(gl, "final", header_final, vertex_shader, s_video);

    glCheckError(gl, "shader compilation");

    talloc_free(tmp);
}


static void setup_vertex_array(GL *gl)
{
    size_t stride = sizeof(struct vertex);

    gl->EnableVertexAttribArray(VERTEX_ATTRIB_POSITION);
    gl->VertexAttribPointer(VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE,
                            stride, (void*)offsetof(struct vertex, position));

    gl->EnableVertexAttribArray(VERTEX_ATTRIB_COLOR);
    gl->VertexAttribPointer(VERTEX_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                            stride, (void*)offsetof(struct vertex, color));

    gl->EnableVertexAttribArray(VERTEX_ATTRIB_TEXCOORD);
    gl->VertexAttribPointer(VERTEX_ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE,
                            stride, (void*)offsetof(struct vertex, texcoord));
}

// First-time initialization of the GL state.
static int init_gl(struct gl_priv *p)
{
    GL *gl = p->gl;

    glCheckError(p->gl, "before init_gl");

    const char *vendor     = gl->GetString(GL_VENDOR);
    const char *version    = gl->GetString(GL_VERSION);
    const char *renderer   = gl->GetString(GL_RENDERER);
    const char *glsl       = gl->GetString(GL_SHADING_LANGUAGE_VERSION);
    mp_msg(MSGT_VO, MSGL_V, "[gl] GL_RENDERER='%s', GL_VENDOR='%s', "
                            "GL_VERSION='%s', GL_SHADING_LANGUAGE_VERSION='%s'"
                            "\n", renderer, vendor, version, glsl);
    mp_msg(MSGT_VO, MSGL_V, "[gl] Display depth: R=%d, G=%d, B=%d\n",
           p->glctx->depth_r, p->glctx->depth_g, p->glctx->depth_b);

    gl->Disable(GL_DITHER);
    gl->Disable(GL_BLEND);
    gl->Disable(GL_DEPTH_TEST);
    gl->DepthMask(GL_FALSE);
    gl->Disable(GL_CULL_FACE);
    gl->DrawBuffer(GL_BACK);

    gl->GenBuffers(1, &p->vertex_buffer);
    gl->GenVertexArrays(1, &p->vao);

    gl->BindBuffer(GL_ARRAY_BUFFER, p->vertex_buffer);
    gl->BindVertexArray(p->vao);
    setup_vertex_array(gl);
    gl->BindBuffer(GL_ARRAY_BUFFER, 0);
    gl->BindVertexArray(0);

    GLint max_texture_size;
    gl->GetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    eosd_packer_reinit(p->eosd, max_texture_size, max_texture_size);

    gl->ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl->Clear(GL_COLOR_BUFFER_BIT);
    if (gl->SwapInterval && p->swap_interval >= 0)
        gl->SwapInterval(p->swap_interval);

    glCheckError(gl, "after init_gl");

    return 1;
}

static void init_scaler(struct gl_priv *p, struct scaler *scaler)
{
    GL *gl = p->gl;

    assert(scaler->name);

    const struct filter_kernel *t_kernel = mp_find_filter_kernel(scaler->name);
    if (!t_kernel)
        return;

    scaler->kernel_storage = *t_kernel;
    scaler->kernel = &scaler->kernel_storage;

    for (int n = 0; n < 2; n++) {
        if (!isnan(p->scaler_params[n]))
            scaler->kernel->params[n] = p->scaler_params[n];
    }

    update_scale_factor(p, scaler->kernel);

    int size = scaler->kernel->size;
    assert(size < FF_ARRAY_ELEMS(lut_tex_formats));
    struct lut_tex_format *fmt = &lut_tex_formats[size];
    bool use_2d = fmt->pixels > 1;
    bool is_luma = scaler->index == 0;
    scaler->lut_name = use_2d
                       ? (is_luma ? "lut_l_2d" : "lut_c_2d")
                       : (is_luma ? "lut_l_1d" : "lut_c_1d");

    gl->ActiveTexture(GL_TEXTURE0 + TEXUNIT_SCALERS + scaler->index);
    GLenum target = use_2d ? GL_TEXTURE_2D : GL_TEXTURE_1D;

    if (!scaler->gl_lut)
        gl->GenTextures(1, &scaler->gl_lut);

    gl->BindTexture(target, scaler->gl_lut);
    gl->PixelStorei(GL_UNPACK_ALIGNMENT, 4);
    gl->PixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    float *weights = talloc_array(NULL, float, LOOKUP_TEXTURE_SIZE * size);
    mp_compute_lut(scaler->kernel, LOOKUP_TEXTURE_SIZE, weights);
    if (use_2d) {
        gl->TexImage2D(GL_TEXTURE_2D, 0, fmt->internal_format, fmt->pixels,
                       LOOKUP_TEXTURE_SIZE, 0, fmt->format, GL_FLOAT,
                       weights);
    } else {
        gl->TexImage1D(GL_TEXTURE_1D, 0, fmt->internal_format,
                       LOOKUP_TEXTURE_SIZE, 0, fmt->format, GL_FLOAT,
                       weights);
    }
    talloc_free(weights);

    gl->TexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl->TexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl->TexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->TexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    gl->ActiveTexture(GL_TEXTURE0);

    glCheckError(gl, "after initializing scaler");
}

static void make_dither_matrix(unsigned char *m, int size)
{
    m[0] = 0;
    for (int sz = 1; sz < size; sz *= 2) {
        int offset[] = {sz*size, sz, sz * (size+1), 0};
        for (int i = 0; i < 4; i++)
            for (int y = 0; y < sz * size; y += size)
                for (int x = 0; x < sz; x++)
                    m[x+y+offset[i]] = m[x+y] * 4 + (3-i) * 256/size/size;
    }
}

static void init_dither(struct gl_priv *p)
{
    GL *gl = p->gl;

    // Assume 8 bits per component if unknown.
    int dst_depth = p->glctx->depth_g ? p->glctx->depth_g : 8;
    if (p->dither_depth > 0)
        dst_depth = p->dither_depth;

    // not sure how to get per-component bit depth for RGB formats
    int src_depth = p->is_yuv ? p->plane_bits : -1;
    if (p->use_lut_3d)
        src_depth = 16;

    if (dst_depth >= src_depth || p->dither_depth < 0 || src_depth < 0)
        return;

    mp_msg(MSGT_VO, MSGL_V, "[gl] Dither %d->%d.\n", src_depth, dst_depth);

    // This defines how many bits are considered significant for output on
    // screen. The superfluous bits will be used for rounded according to the
    // dither matrix. The precision of the source implicitly decides how many
    // dither patterns can be visible.
    p->dither_quantization = (1 << dst_depth) - 1;
    int size = 8;
    p->dither_multiply = p->dither_quantization + 1. / (size*size);
    unsigned char dither[256];
    make_dither_matrix(dither, size);

    gl->ActiveTexture(GL_TEXTURE0 + TEXUNIT_DITHER);
    gl->GenTextures(1, &p->dither_texture);
    gl->BindTexture(GL_TEXTURE_2D, p->dither_texture);
    gl->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gl->PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    gl->TexImage2D(GL_TEXTURE_2D, 0, GL_RED, size, size, 0, GL_RED,
                   GL_UNSIGNED_BYTE, dither);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    gl->ActiveTexture(GL_TEXTURE0);
}

static void reinit_rendering(struct gl_priv *p)
{
    GL *gl = p->gl;

    mp_msg(MSGT_VO, MSGL_V, "[gl] Reinit rendering.\n");

    glCheckError(gl, "before scaler initialization");

    uninit_rendering(p);

    init_dither(p);

    init_scaler(p, &p->scalers[0]);
    init_scaler(p, &p->scalers[1]);

    compile_shaders(p);

    if (p->indirect_program && !p->indirect_fbo.fbo)
        fbotex_init(p, &p->indirect_fbo, p->texture_width, p->texture_height);
}

static void init_lut_3d(struct gl_priv *p)
{
    GL *gl = p->gl;

    mp_msg(MSGT_VO, MSGL_V, "[gl] upload 3dlut\n");

    gl->GenTextures(1, &p->lut_3d_texture);
    gl->ActiveTexture(GL_TEXTURE0 + TEXUNIT_3DLUT);
    gl->BindTexture(GL_TEXTURE_3D, p->lut_3d_texture);
    gl->PixelStorei(GL_UNPACK_ALIGNMENT, 4);
    gl->PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    gl->TexImage3D(GL_TEXTURE_3D, 0, GL_RGB16, p->lut_3d_w, p->lut_3d_h,
                   p->lut_3d_d, 0, GL_RGB, GL_UNSIGNED_SHORT, p->lut_3d_data);
    gl->TexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl->TexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl->TexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->TexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->TexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    gl->ActiveTexture(GL_TEXTURE0);

    mp_msg(MSGT_VO, MSGL_V, "[gl] end upload 3dlut\n");

    glCheckError(gl, "after 3d lut creation");
}

static void init_video(struct gl_priv *p)
{
    GL *gl = p->gl;

    if (p->use_lut_3d && !p->lut_3d_texture)
        init_lut_3d(p);

    init_format(p->image_format, p);

    if (!p->is_yuv && (p->use_srgb || p->use_lut_3d)) {
        p->is_linear_rgb = true;
        p->gl_internal_format = GL_SRGB;
    }

    int eq_caps = MP_CSP_EQ_CAPS_GAMMA;
    if (p->is_yuv)
        eq_caps |= MP_CSP_EQ_CAPS_COLORMATRIX;
    p->video_eq.capabilities = eq_caps;

    glCheckError(gl, "before video texture creation");

    tex_size(p, p->image_width, p->image_height,
             &p->texture_width, &p->texture_height);

    void *tmp = NULL;
    if (!p->use_npot)
        tmp = malloc(p->texture_width * p->texture_height * p->plane_bytes);

    gl->PixelStorei(GL_UNPACK_ALIGNMENT, 4);
    gl->PixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    for (int n = 0; n < p->plane_count; n++) {
        struct texplane *plane = &p->planes[n];

        int w = p->texture_width >> plane->shift_x;
        int h = p->texture_height >> plane->shift_y;

        mp_msg(MSGT_VO, MSGL_V, "[gl] Texture for plane %d: %dx%d\n", n, w, h);

        gl->ActiveTexture(GL_TEXTURE0 + n);
        gl->GenTextures(1, &plane->gl_texture);
        gl->BindTexture(GL_TEXTURE_2D, plane->gl_texture);

        if (tmp)
            memset(tmp, plane->clear_val, w * h * p->plane_bytes);
        gl->TexImage2D(GL_TEXTURE_2D, 0, p->gl_internal_format, w, h, 0,
                       p->gl_format, p->gl_type, tmp);
        default_tex_params(gl, GL_TEXTURE_2D, GL_LINEAR);
        if (p->mipmap_gen)
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                              GL_LINEAR_MIPMAP_NEAREST);
    }
    gl->ActiveTexture(GL_TEXTURE0);

    free(tmp);

    glCheckError(gl, "after video texture creation");

    reinit_rendering(p);
}


static bool config_window(struct gl_priv *p, uint32_t d_width,
                          uint32_t d_height, uint32_t flags)
{
    if (p->stereo_mode == GL_3D_QUADBUFFER)
        flags |= VOFLAG_STEREO;

    int mpgl_version = p->force_gl2 ? MPGL_VER(2, 1) : MPGL_VER(3, 2);
    int mpgl_flags = 0;
    if (p->gl_debug)
        mpgl_flags |= MPGLFLAG_DEBUG;

    if (create_mpglcontext(p->glctx, mpgl_flags, mpgl_version, d_width,
                           d_height, flags) == SET_WINDOW_FAILED)
        return false;

    if (!p->vertex_buffer)
        init_gl(p);

    return true;
}

static int config(struct vo *vo, uint32_t width, uint32_t height,
                  uint32_t d_width, uint32_t d_height, uint32_t flags,
                  uint32_t format)
{
    struct gl_priv *p = vo->priv;

    if (!config_window(p, d_width, d_height, flags))
        return -1;

    p->image_d_width = d_width;
    p->image_d_height = d_height;
    p->vo_flipped = !!(flags & VOFLAG_FLIPPING);

    if (p->image_format != format || p->image_width != width
        || p->image_height != height)
    {
        uninit_video(p);
        p->image_height = height;
        p->image_width = width;
        p->image_format = format;
        init_video(p);
    }

    resize(p);

    return 0;
}

static void check_events(struct vo *vo)
{
    struct gl_priv *p = vo->priv;

    int e = p->glctx->check_events(vo);
    if (e & VO_EVENT_REINIT) {
        uninit_gl(p);
        init_gl(p);
        init_video(p);
        resize(p);
    }
    if (e & VO_EVENT_RESIZE)
        resize(p);
    if (e & VO_EVENT_EXPOSE)
        vo->want_redraw = true;
}

/**
 * Creates the textures and the display list needed for displaying
 * an OSD part.
 * Callback function for osd_draw_text_ext().
 */
static void create_osd_texture(void *ctx, int x0, int y0, int w, int h,
                               unsigned char *src, unsigned char *srca,
                               int stride)
{
    struct gl_priv *p = ctx;
    GL *gl = p->gl;

    // initialize to 8 to avoid special-casing on alignment
    int sx = 8, sy = 8;

    if (w <= 0 || h <= 0 || stride < w) {
        mp_msg(MSGT_VO, MSGL_V, "Invalid dimensions OSD for part!\n");
        return;
    }
    tex_size(p, w, h, &sx, &sy);

    if (p->osdtex_count >= MAX_OSD_PARTS) {
        mp_msg(MSGT_VO, MSGL_ERR, "Too many OSD parts, contact the developers!\n");
        return;
    }

    gl->GenTextures(1, &p->osdtex[p->osdtex_count]);
    gl->BindTexture(GL_TEXTURE_2D, p->osdtex[p->osdtex_count]);
    gl->TexImage2D(GL_TEXTURE_2D, 0, GL_RG, sx, sy, 0, GL_RG, GL_UNSIGNED_BYTE,
                   NULL);
    default_tex_params(gl, GL_TEXTURE_2D, GL_NEAREST);
    {
        int i;
        unsigned char *tmp = malloc(stride * h * 2);
        // convert alpha from weird MPlayer scale.
        for (i = 0; i < h * stride; i++) {
            tmp[i*2+0] = src[i];
            tmp[i*2+1] = -srca[i];
        }
        glUploadTex(gl, GL_TEXTURE_2D, GL_RG, GL_UNSIGNED_BYTE, tmp, stride * 2,
                    0, 0, w, h, 0);
        free(tmp);
    }

    gl->BindTexture(GL_TEXTURE_2D, 0);

    uint8_t color[4] = {(p->osd_color >> 16) & 0xff, (p->osd_color >> 8) & 0xff,
                        p->osd_color & 0xff, 0xff - (p->osd_color >> 24)};

    write_quad(&p->osd_va[p->osdtex_count * VERTICES_PER_QUAD],
               x0, y0, x0 + w, y0 + h, 0, 0, w, h,
               sx, sy, color, false);

    p->osdtex_count++;
}

static void draw_osd(struct vo *vo, struct osd_state *osd)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    if (vo_osd_changed(0)) {
        clear_osd(p);
        osd_draw_text_ext(osd, vo->dwidth, vo->dheight, p->border_x,
                          p->border_y, p->border_x,
                          p->border_y, p->image_width,
                          p->image_height, create_osd_texture, p);
    }

    if (p->osdtex_count > 0) {
        gl->Enable(GL_BLEND);
        // OSD bitmaps use premultiplied alpha.
        gl->BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        gl->UseProgram(p->osd_program);

        for (int n = 0; n < p->osdtex_count; n++) {
            gl->BindTexture(GL_TEXTURE_2D, p->osdtex[n]);
            draw_triangles(p, &p->osd_va[n * VERTICES_PER_QUAD],
                           VERTICES_PER_QUAD);
        }

        gl->UseProgram(0);

        gl->Disable(GL_BLEND);
        gl->BindTexture(GL_TEXTURE_2D, 0);
    }
}

static void render_to_fbo(struct gl_priv *p, struct fbotex *fbo, int w, int h,
                          int tex_w, int tex_h)
{
    GL *gl = p->gl;

    gl->Viewport(0, 0, fbo->vp_w, fbo->vp_h);
    gl->BindFramebuffer(GL_FRAMEBUFFER, fbo->fbo);

    struct vertex vb[VERTICES_PER_QUAD];
    write_quad(vb, -1, -1, 1, 1,
               0, 0, w, h,
               tex_w, tex_h,
               NULL, false);
    draw_triangles(p, vb, VERTICES_PER_QUAD);

    gl->BindFramebuffer(GL_FRAMEBUFFER, 0);
    gl->Viewport(p->vp_x, p->vp_y, p->vp_w, p->vp_h);

}

static void handle_pass(struct gl_priv *p, struct fbotex **source,
                        struct fbotex *fbo, GLuint program)
{
    GL *gl = p->gl;

    if (!program)
        return;

    gl->BindTexture(GL_TEXTURE_2D, (*source)->texture);
    gl->UseProgram(program);
    render_to_fbo(p, fbo, (*source)->vp_w, (*source)->vp_h,
                  (*source)->tex_w, (*source)->tex_h);
    *source = fbo;
}

static void do_render(struct gl_priv *p)
{
    GL *gl = p->gl;
    struct vertex vb[VERTICES_PER_QUAD];
    bool is_flipped = p->mpi_flipped ^ p->vo_flipped;

    // Order of processing:
    //  [indirect -> [scale_sep ->]] final

    struct fbotex dummy = {
        .vp_w = p->image_width, .vp_h = p->image_height,
        .tex_w = p->texture_width, .tex_h = p->texture_height,
        .texture = p->planes[0].gl_texture,
    };
    struct fbotex *source = &dummy;

    handle_pass(p, &source, &p->indirect_fbo, p->indirect_program);
    handle_pass(p, &source, &p->scale_sep_fbo, p->scale_sep_program);

    gl->BindTexture(GL_TEXTURE_2D, source->texture);
    gl->UseProgram(p->final_program);

    float final_texw = p->image_width * source->tex_w / (float)source->vp_w;
    float final_texh = p->image_height * source->tex_h / (float)source->vp_h;

    if (p->use_srgb && !p->use_lut_3d)
        gl->Enable(GL_FRAMEBUFFER_SRGB);

    if (p->stereo_mode) {
        int w = p->src_rect.width;
        int imgw = p->image_width;

        glEnable3DLeft(gl, p->stereo_mode);

        write_quad(vb,
                   p->dst_rect.left, p->dst_rect.top,
                   p->dst_rect.right, p->dst_rect.bottom,
                   p->src_rect.left / 2, p->src_rect.top,
                   p->src_rect.left / 2 + w / 2, p->src_rect.bottom,
                   final_texw, final_texh,
                   NULL, is_flipped);
        draw_triangles(p, vb, VERTICES_PER_QUAD);

        glEnable3DRight(gl, p->stereo_mode);

        write_quad(vb,
                   p->dst_rect.left, p->dst_rect.top,
                   p->dst_rect.right, p->dst_rect.bottom,
                   p->src_rect.left / 2 + imgw / 2, p->src_rect.top,
                   p->src_rect.left / 2 + imgw / 2 + w / 2, p->src_rect.bottom,
                   final_texw, final_texh,
                   NULL, is_flipped);
        draw_triangles(p, vb, VERTICES_PER_QUAD);

        glDisable3D(gl, p->stereo_mode);
    } else {
        write_quad(vb,
                   p->dst_rect.left, p->dst_rect.top,
                   p->dst_rect.right, p->dst_rect.bottom,
                   p->src_rect.left, p->src_rect.top,
                   p->src_rect.right, p->src_rect.bottom,
                   final_texw, final_texh,
                   NULL, is_flipped);
        draw_triangles(p, vb, VERTICES_PER_QUAD);
    }

    gl->Disable(GL_FRAMEBUFFER_SRGB);

    gl->UseProgram(0);

    glCheckError(gl, "after video rendering");
}

static void flip_page(struct vo *vo)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    if (p->use_glFinish)
        gl->Finish();
    p->glctx->swapGlBuffers(p->glctx);
    if (aspect_scaling())
        gl->Clear(GL_COLOR_BUFFER_BIT);
}

static int draw_slice(struct vo *vo, uint8_t *src[], int stride[], int w, int h,
                      int x, int y)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    p->mpi_flipped = stride[0] < 0;

    for (int n = 0; n < p->plane_count; n++) {
        gl->ActiveTexture(GL_TEXTURE0 + n);
        gl->BindTexture(GL_TEXTURE_2D, p->planes[n].gl_texture);
        int xs = p->planes[n].shift_x, ys = p->planes[n].shift_y;
        glUploadTex(gl, GL_TEXTURE_2D, p->gl_format, p->gl_type, src[n],
                    stride[n], x >> xs, y >> ys, w >> xs, h >> ys, 0);
    }
    gl->ActiveTexture(GL_TEXTURE0);

    return 0;
}

static uint32_t get_image(struct vo *vo, mp_image_t *mpi)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    if (!p->use_pbo)
        return VO_FALSE;

    // We don't support alpha planes. (Disabling PBOs with normal draw calls is
    // an undesired, but harmless side-effect.)
    if (mpi->num_planes != p->plane_count)
        return VO_FALSE;

    if (mpi->flags & MP_IMGFLAG_READABLE)
        return VO_FALSE;
    if (mpi->type != MP_IMGTYPE_STATIC && mpi->type != MP_IMGTYPE_TEMP &&
        (mpi->type != MP_IMGTYPE_NUMBERED || mpi->number))
        return VO_FALSE;
    mpi->flags &= ~MP_IMGFLAG_COMMON_PLANE;
    for (int n = 0; n < p->plane_count; n++) {
        struct texplane *plane = &p->planes[n];
        mpi->stride[n] = (mpi->width >> plane->shift_x) * p->plane_bytes;
        int needed_size = (mpi->height >> plane->shift_y) * mpi->stride[n];
        if (!plane->gl_buffer)
            gl->GenBuffers(1, &plane->gl_buffer);
        gl->BindBuffer(GL_PIXEL_UNPACK_BUFFER, plane->gl_buffer);
        if (needed_size > plane->buffer_size) {
            plane->buffer_size = needed_size;
            gl->BufferData(GL_PIXEL_UNPACK_BUFFER, plane->buffer_size,
                           NULL, GL_DYNAMIC_DRAW);
        }
        if (!plane->buffer_ptr)
            plane->buffer_ptr = gl->MapBuffer(GL_PIXEL_UNPACK_BUFFER,
                                              GL_WRITE_ONLY);
        mpi->planes[n] = plane->buffer_ptr;
        gl->BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
    mpi->flags |= MP_IMGFLAG_DIRECT;
    return VO_TRUE;
}

static uint32_t draw_image(struct gl_priv *p, mp_image_t *mpi)
{
    GL *gl = p->gl;
    int n;

    assert(mpi->num_planes >= p->plane_count);

    mp_image_t mpi2 = *mpi;
    int w = mpi->w, h = mpi->h;
    if (mpi->flags & MP_IMGFLAG_DRAW_CALLBACK)
        goto skip_upload;
    mpi2.flags = 0;
    mpi2.type = MP_IMGTYPE_TEMP;
    mpi2.width = mpi2.w;
    mpi2.height = mpi2.h;
    if (!(mpi->flags & MP_IMGFLAG_DIRECT)
        && !p->planes[0].buffer_ptr
        && get_image(p->vo, &mpi2) == VO_TRUE)
    {
        for (n = 0; n < p->plane_count; n++) {
            struct texplane *plane = &p->planes[n];
            int xs = plane->shift_x, ys = plane->shift_y;
            int line_bytes = (mpi->w >> xs) * p->plane_bytes;
            memcpy_pic(mpi2.planes[n], mpi->planes[n], line_bytes, mpi->h >> ys,
                       mpi2.stride[n], mpi->stride[n]);
        }
        mpi = &mpi2;
    }
    p->mpi_flipped = mpi->stride[0] < 0;
    for (n = 0; n < p->plane_count; n++) {
        struct texplane *plane = &p->planes[n];
        int xs = plane->shift_x, ys = plane->shift_y;
        void *plane_ptr = mpi->planes[n];
        if (mpi->flags & MP_IMGFLAG_DIRECT) {
            gl->BindBuffer(GL_PIXEL_UNPACK_BUFFER, plane->gl_buffer);
            if (!gl->UnmapBuffer(GL_PIXEL_UNPACK_BUFFER))
                mp_msg(MSGT_VO, MSGL_FATAL, "[gl] Video PBO upload failed. "
                       "Remove the 'pbo' suboption.\n");
            plane->buffer_ptr = NULL;
            plane_ptr = NULL; // PBO offset 0
        }
        gl->ActiveTexture(GL_TEXTURE0 + n);
        gl->BindTexture(GL_TEXTURE_2D, plane->gl_texture);
        glUploadTex(gl, GL_TEXTURE_2D, p->gl_format, p->gl_type, plane_ptr,
                    mpi->stride[n], 0, 0, w >> xs, h >> ys, 0);
    }
    gl->ActiveTexture(GL_TEXTURE0);
    gl->BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
skip_upload:
    if (p->mipmap_gen) {
        for (n = 0; n < p->plane_count; n++) {
            gl->ActiveTexture(GL_TEXTURE0 + n);
            gl->GenerateMipmap(GL_TEXTURE_2D);
        }
        gl->ActiveTexture(GL_TEXTURE0);
    }
    do_render(p);
    return VO_TRUE;
}

static mp_image_t *get_screenshot(struct gl_priv *p)
{
    GL *gl = p->gl;

    mp_image_t *image = alloc_mpi(p->texture_width, p->texture_height,
                                  p->image_format);

    // NOTE about image formats with alpha plane: we don't even have the alpha
    // anymore. We never upload it to any texture, as it would be a waste of
    // time. On the other hand, we can't find a "similar", non-alpha image
    // format easily. So we just leave the alpha plane of the newly allocated
    // image as-is, and hope that the alpha is ignored by the receiver of the
    // screenshot. (If not, code should be added to make it fully opaque.)

    for (int n = 0; n < p->plane_count; n++) {
        gl->ActiveTexture(GL_TEXTURE0 + n);
        gl->BindTexture(GL_TEXTURE_2D, p->planes[n].gl_texture);
        glDownloadTex(gl, GL_TEXTURE_2D, p->gl_format, p->gl_type,
                      image->planes[n], image->stride[n]);
    }
    gl->ActiveTexture(GL_TEXTURE0);

    image->width = p->image_width;
    image->height = p->image_height;

    image->w = p->image_d_width;
    image->h = p->image_d_height;

    return image;
}

static mp_image_t *get_window_screenshot(struct gl_priv *p)
{
    GL *gl = p->gl;

    GLint vp[4]; //x, y, w, h
    gl->GetIntegerv(GL_VIEWPORT, vp);
    mp_image_t *image = alloc_mpi(vp[2], vp[3], IMGFMT_RGB24);
    gl->PixelStorei(GL_PACK_ALIGNMENT, 4);
    gl->PixelStorei(GL_PACK_ROW_LENGTH, 0);
    gl->ReadBuffer(GL_FRONT);
    //flip image while reading
    for (int y = 0; y < vp[3]; y++) {
        gl->ReadPixels(vp[0], vp[1] + vp[3] - y - 1, vp[2], 1,
                       GL_RGB, GL_UNSIGNED_BYTE,
                       image->planes[0] + y * image->stride[0]);
    }
    return image;
}

static int query_format(uint32_t format)
{
    int caps = VFCAP_CSP_SUPPORTED | VFCAP_CSP_SUPPORTED_BY_HW | VFCAP_FLIP |
               VFCAP_HWSCALE_UP | VFCAP_HWSCALE_DOWN | VFCAP_ACCEPT_STRIDE |
               VFCAP_OSD | VFCAP_EOSD | VFCAP_EOSD_UNSCALED;
    if (!init_format(format, NULL))
        return 0;
    return caps;
}

static void uninit(struct vo *vo)
{
    struct gl_priv *p = vo->priv;

    uninit_gl(p);
    uninit_mpglcontext(p->glctx);
    p->glctx = NULL;
    p->gl = NULL;
}

#if 0
static void print_scalers(void)
{
    mp_msg(MSGT_VO, MSGL_INFO, "Available scalers:\n");
    for (const char **e = fixed_scale_filters; *e; e++) {
        mp_msg(MSGT_VO, MSGL_INFO, "    %s\n", *e);
    }
    for (const struct filter_kernel *e = mp_filter_kernels; e->name; e++) {
        if (can_use_filter_kernel(e))
            mp_msg(MSGT_VO, MSGL_INFO, "    %s\n", e->name);
    }
}
#endif

static const char* handle_scaler_opt(const char *name)
{
    const struct filter_kernel *kernel = mp_find_filter_kernel(name);
    if (can_use_filter_kernel(kernel))
        return kernel->name;

    for (const char **filter = fixed_scale_filters; *filter; filter++) {
        if (strcmp(*filter, name) == 0)
            return *filter;
    }

    return NULL;
}

static int scaler_valid(void *arg)
{
    return handle_scaler_opt(*(const char **)arg) != NULL;
}

static int backend_valid(void *arg)
{
    return mpgl_find_backend(*(const char **)arg) >= 0;
}

struct fbo_format {
    const char *name;
    GLint format;
};

const struct fbo_format fbo_formats[] = {
    {"rgb",    GL_RGB},
    {"rgba",   GL_RGBA},
    {"rgb8",   GL_RGB8},
    {"rgb16",  GL_RGB16},
    {"rgb16f", GL_RGB16F},
    {"rgb32f", GL_RGB32F},
    {0}
};

static GLint find_fbo_format(const char *name)
{
    for (const struct fbo_format *fmt = fbo_formats; fmt->name; fmt++) {
        if (strcmp(fmt->name, name) == 0)
            return fmt->format;
    }
    return -1;
}

static int fbo_format_valid(void *arg)
{
    return find_fbo_format(*(const char **)arg) >= 0;
}

#ifdef CONFIG_LCMS2

static void lcms2_error_handler(cmsContext ctx, cmsUInt32Number code,
                                const char *msg)
{
    mp_msg(MSGT_VO, MSGL_ERR, "[gl] lcms2: %s\n", msg);
}

static struct bstr load_file(struct gl_priv *p, void *talloc_ctx,
                             const char *filename)
{
    struct bstr res = {0};
    stream_t *s = open_stream(filename, p->vo->opts, NULL);
    if (s) {
        res = stream_read_complete(s, talloc_ctx, 1000000000, 0);
        free_stream(s);
    }
    return res;
}

#define LUT3D_CACHE_HEADER "mplayer2 2dlut cache 1.0\n"

static bool load_icc(struct gl_priv *p, const char *icc_file,
                     const char *icc_cache, int icc_intent,
                     int s_r, int s_g, int s_b)
{
    void *tmp = talloc_new(p);
    uint16_t *output = talloc_array(tmp, uint16_t, s_r * s_g * s_b * 3);

    if (icc_intent == -1)
        icc_intent = INTENT_ABSOLUTE_COLORIMETRIC;

    mp_msg(MSGT_VO, MSGL_INFO, "[gl] Opening ICC profile '%s'\n", icc_file);
    struct bstr iccdata = load_file(p, tmp, icc_file);
    if (!iccdata.len)
        goto error_exit;

    char *cache_info = talloc_asprintf(tmp, "intent=%d, size=%dx%dx%d\n",
                                       icc_intent, s_r, s_g, s_b);

    // check cache
    if (icc_cache) {
        mp_msg(MSGT_VO, MSGL_INFO, "[gl] Opening 3D LUT cache in file '%s'.\n",
               icc_cache);
        struct bstr cachedata = load_file(p, tmp, icc_cache);
        if (bstr_eatstart(&cachedata, bstr(LUT3D_CACHE_HEADER))
            && bstr_eatstart(&cachedata, bstr(cache_info))
            && bstr_eatstart(&cachedata, iccdata)
            && cachedata.len == talloc_get_size(output))
        {
            memcpy(output, cachedata.start, cachedata.len);
            goto done;
        } else {
            mp_msg(MSGT_VO, MSGL_WARN, "[gl] 3D LUT cache invalid!\n");
        }
    }

    cmsSetLogErrorHandler(lcms2_error_handler);

    cmsHPROFILE profile = cmsOpenProfileFromMem(iccdata.start, iccdata.len);
    if (!profile)
        goto error_exit;

    cmsCIExyY d65;
    cmsWhitePointFromTemp(&d65, 6504);
    static const cmsCIExyYTRIPLE bt709prim = {
        .Red   = {0.64, 0.33, 1.0},
        .Green = {0.30, 0.60, 1.0},
        .Blue  = {0.15, 0.06, 1.0},
    };
    cmsToneCurve *tonecurve = cmsBuildGamma(NULL, 2.2);
    cmsHPROFILE vid_profile = cmsCreateRGBProfile(&d65, &bt709prim,
                        (cmsToneCurve*[3]){tonecurve, tonecurve, tonecurve});
    cmsFreeToneCurve(tonecurve);
    cmsHTRANSFORM trafo = cmsCreateTransform(vid_profile, TYPE_RGB_16,
                                             profile, TYPE_RGB_16,
                                             icc_intent,
                                             cmsFLAGS_HIGHRESPRECALC);
    cmsCloseProfile(profile);
    cmsCloseProfile(vid_profile);

    if (!trafo)
        goto error_exit;

    mp_msg(MSGT_VO, MSGL_V, "[gl] DoTransform\n");

    // transform a (s_r)x(s_g)x(s_b) cube, with 3 components per channel
    uint16_t *input = talloc_array(tmp, uint16_t, s_r * 3);
    for (int b = 0; b < s_b; b++) {
        for (int g = 0; g < s_g; g++) {
            for (int r = 0; r < s_r; r++) {
                input[r * 3 + 0] = r * 65535 / (s_r - 1);
                input[r * 3 + 1] = g * 65535 / (s_g - 1);
                input[r * 3 + 2] = b * 65535 / (s_b - 1);
            }
            size_t base = (b * s_r * s_g + g * s_r) * 3;
            cmsDoTransform(trafo, input, output + base, s_r);
        }
    }

    mp_msg(MSGT_VO, MSGL_V, "[gl] end DoTransform\n");

    cmsDeleteTransform(trafo);

    if (icc_cache) {
        FILE *out = fopen(icc_cache, "wb");
        if (out) {
            fprintf(out, "%s%s", LUT3D_CACHE_HEADER, cache_info);
            fwrite(iccdata.start, iccdata.len, 1, out);
            fwrite(output, talloc_get_size(output), 1, out);
            fclose(out);
        }
    }

done:

    p->lut_3d_data = talloc_steal(p, output);
    p->lut_3d_w = s_r, p->lut_3d_h = s_g, p->lut_3d_d = s_b;
    p->use_lut_3d = true;
    p->use_indirect = true;

    talloc_free(tmp);
    return true;

error_exit:
    mp_msg(MSGT_VO, MSGL_FATAL, "[gl] Error loading ICC profile.\n");
    talloc_free(tmp);
    return false;
}

#else /* CONFIG_LCMS2 */

static bool load_icc(struct gl_priv *p, ...)
{
    mp_msg(MSGT_VO, MSGL_FATAL, "[gl] LCMS2 support not compiled.\n");
    return false;
}

#endif /* CONFIG_LCMS2 */

static bool parse_3dlut_size(const char *s, int *p1, int *p2, int *p3)
{
    if (sscanf(s, "%dx%dx%d", p1, p2, p3) != 3)
        return false;
    for (int n = 0; n < 3; n++) {
        int s = ((int[]) { *p1, *p2, *p3 })[n];
        if (s < 2 || s > 256 || ((s - 1) & s))
            return false;
    }
    return true;
}

static int lut3d_size_valid(void *arg)
{
    char *s = *(char **)arg;
    int p1, p2, p3;
    return parse_3dlut_size(s, &p1, &p2, &p3);
}

static int preinit(struct vo *vo, const char *arg)
{
    struct gl_priv *p = talloc_zero(vo, struct gl_priv);
    vo->priv = p;

    *p = (struct gl_priv) {
        .vo = vo,
        .colorspace = MP_CSP_DETAILS_DEFAULTS,
        .filter_strength = 0.5,
        .use_npot = 1,
        .use_pbo = 0,
        .swap_interval = 1,
        .osd_color = 0xffffff,
        .fbo_format = GL_RGB16,
        .use_scale_sep = 1,
        .use_fancy_downscaling = 1,
        .scalers = {
            { .index = 0, .name = "lanczos2" },
            { .index = 1, .name = "bilinear" },
        },
        .scaler_params = {NAN, NAN},
    };


    char *scalers[2] = {0};
    char *backend_arg = NULL;
    char *fbo_format = NULL;
    char *icc_profile = NULL;
    char *icc_cache = NULL;
    int icc_intent = -1;
    char *icc_size_str = NULL;

    const opt_t subopts[] = {
        {"gamma",        OPT_ARG_BOOL, &p->use_gamma,    NULL},
        {"srgb",         OPT_ARG_BOOL, &p->use_srgb,     NULL},
        {"npot",         OPT_ARG_BOOL, &p->use_npot,     NULL},
        {"filter-strength", OPT_ARG_FLOAT, &p->filter_strength, NULL},
        {"pbo",          OPT_ARG_BOOL, &p->use_pbo,      NULL},
        {"glfinish",     OPT_ARG_BOOL, &p->use_glFinish, NULL},
        {"swapinterval", OPT_ARG_INT,  &p->swap_interval,NULL},
        {"mipmapgen",    OPT_ARG_BOOL, &p->mipmap_gen,   NULL},
        {"osdcolor",     OPT_ARG_INT,  &p->osd_color,    NULL},
        {"stereo",       OPT_ARG_INT,  &p->stereo_mode,  NULL},
        {"lscale",       OPT_ARG_MSTRZ,&scalers[0],      scaler_valid},
        {"cscale",       OPT_ARG_MSTRZ,&scalers[1],      scaler_valid},
        {"lparam1",      OPT_ARG_FLOAT,&p->scaler_params[0], NULL},
        {"lparam2",      OPT_ARG_FLOAT,&p->scaler_params[1], NULL},
        {"fancy-downscaling", OPT_ARG_BOOL, &p->use_fancy_downscaling, NULL},
        {"debug",        OPT_ARG_BOOL, &p->gl_debug,     NULL},
        {"force-gl2",    OPT_ARG_BOOL, &p->force_gl2,    NULL},
        {"indirect",     OPT_ARG_BOOL, &p->use_indirect, NULL},
        {"scale-sep",    OPT_ARG_BOOL, &p->use_scale_sep, NULL},
        {"fbo-format",   OPT_ARG_MSTRZ,&fbo_format,      fbo_format_valid},
        {"backend",      OPT_ARG_MSTRZ,&backend_arg,     backend_valid},
        {"icc-profile",  OPT_ARG_MSTRZ,&icc_profile,     NULL},
        {"icc-cache",    OPT_ARG_MSTRZ,&icc_cache,       NULL},
        {"icc-intent",   OPT_ARG_INT,  &icc_intent,      NULL},
        {"3dlut-size",   OPT_ARG_MSTRZ,&icc_size_str,    lut3d_size_valid},
        {"dither-depth", OPT_ARG_INT,  &p->dither_depth, NULL},
        {NULL}
    };

    if (subopt_parse(arg, subopts) != 0) {
        mp_msg(MSGT_VO, MSGL_FATAL,
               "\n--vo=gl3 command line help:\n"
               "Example: mplayer --vo=gl3:scale-sep:lscale=lanczos2\n"
               "\nOptions:\n"
               "  lscale=<filter>\n"
               "    Set the scaling filter. Possible choices:\n"
               "    bilinear: bilinear texture filtering (fastest).\n"
               "    bicubic_fast: bicubic filter (without lookup texture).\n"
               "    sharpen3: unsharp masking (sharpening) with radius=3.\n"
               "    sharpen5: unsharp masking (sharpening) with radius=5.\n"
               "    lanczos2: Lanczos with radius=2 (default, recommended).\n"
               "    lanczos3: Lanczos with radius=3.\n"
               "    Default: lanczos2\n"
               "  filter-strength=<value>\n"
               "    Set the effect strength for the sharpen4/sharpen5 filters.\n"
               "    Default: 0.5\n"
               "  lparam1=<value> / lparam2=<value>\n"
               "    Set parameters for configurable filters. Currently only the\n"
               "    filters 'mitchell' (Mitchell-Netravali) and 'kaiser' support\n"
               "    this. Affects chroma scaler as well.\n"
               "  osdcolor=<0xAARRGGBB>\n"
               "    Use the given color for the OSD.\n"
               "  stereo=<n>\n"
               "    0: normal display\n"
               "    1: side-by-side to red-cyan stereo\n"
               "    2: side-by-side to green-magenta stereo\n"
               "    3: side-by-side to quadbuffer stereo\n"
               "  srgb\n"
               "    Enable gamma-correct scaling by working in linear light. This\n"
               "    makes use of sRGB textures and framebuffers.\n"
               "    This option forces the options 'indirect' and 'gamma'.\n"
               "    NOTE: for BT.709 colorspaces, a gamma of 2.35 is assumed. For\n"
               "    other YUV colorspaces, 2.2 is assumed. RGB input is always\n"
               "    assumed to be in sRGB.\n"
               "  pbo\n"
               "    Enable use of PBOs. This is faster, but can sometimes lead to\n"
               "    sparodic and temporary image corruption.\n"
               "  dither-depth=<n>\n"
               "    Positive non-zero values select the target bit depth.\n"
               "    -1: Disable any dithering done by mplayer.\n"
               "     0: Automatic selection. If output bit depth can't be detected,\n"
               "        8 bits per component are assumed.\n"
               "     8: Dither to 8 bit output.\n"
               "    Default: 0.\n"
               "    Note that dithering will always be disabled if the bit depth\n"
               "    of the video is lower or qual to the detected dither-depth.\n"
               "    If color management is enabled, input depth is assumed to be\n"
               "    16 bits, because the 3D LUT output is 16 bit wide.\n"
               "Less useful options:\n"
               "  swapinterval=<n>\n"
               "    Interval in displayed frames between to buffer swaps.\n"
               "    1 is equivalent to enable VSYNC, 0 to disable VSYNC.\n"
               "  no-scale-sep\n"
               "    When using a separable scale filter for luma, usually two filter\n"
               "    passes are done. This is often faster. However, it forces\n"
               "    conversion to RGB in an extra pass, so it can actually be slower\n"
               "    if used with fast filters on small screen resolutions. Using\n"
               "    this options will make rendering a single operation.\n"
               "    Note that chroma scalers are always done as 1-pass filters.\n"
               "  cscale=<n>\n"
               "    As lscale but for chroma (2x slower with little visible effect).\n"
               "    Note that with some scaling filters, upscaling is always done\n"
               "    in RGB. Setting this option doesn't necessarily help with\n"
               "    anything.\n"
               "  no-fancy-downscaling\n"
               "    When using convolution based filters, don't extend the filter\n"
               "    size when downscaling. Trades downscaling performance for\n"
               "    reduced quality.\n"
               "  no-npot\n"
               "    Force use of power-of-2 texture sizes.\n"
               "  glfinish\n"
               "    Call glFinish() before swapping buffers\n"
               "  backend=<sys>\n"
               "    auto: auto-select (default)\n"
               "    cocoa: Cocoa/OSX\n"
               "    win: Win32/WGL\n"
               "    x11: X11/GLX\n"
               "  mipmapgen\n"
               "    generate mipmaps for the video image (helps with downscaling)\n"
               "  indirect\n"
               "    Do YUV conversion and scaling as separate passes. This will\n"
               "    first render the video into a video-sized RGB texture, and\n"
               "    draw the result on screen. The luma scaler is used to scale\n"
               "    the RGB image when rendering to screen. The chroma scaler\n"
               "    is used only on YUV conversion, and only if the video uses\n"
               "    chroma-subsampling.\n"
               "    This mechanism is disabled on RGB input.\n"
               "  fbo-format=<fmt>\n"
               "    Selects the internal format of any FBO textures used.\n"
               "    fmt can be one of: rgb, rgba, rgb8, rgb16, rgb16f, rgb32f\n"
               "    Default: rgb16.\n"
               "  gamma\n"
               "    Always enable gamma control. (Disables delayed enabling.)\n"
               "  force-gl2\n"
               "    Create a legacy GL context. This will randomly malfunction\n"
               "    if the proper extensions are not supported.\n"
               "  debug\n"
               "    Request debug OpenGL context. Does nothing.\n"
               "Color management:\n"
               "  icc-profile=<file>\n"
               "    Load an ICC profile and use it to transform linear RGB to\n"
               "    screen output. Needs LittleCMS2 support compiled in.\n"
               "  icc-cache=<file>\n"
               "    Store and load the 3D LUT created from the ICC profile in\n"
               "    this file. This can be used to speed up loading, since\n"
               "    LittleCMS2 can take a while to create the 3D LUT.\n"
               "    Note that this file will be at most about 100 MB big.\n"
               "  icc-intent=<value>\n"
               "    0: perceptual\n"
               "    1: relative colorimetric\n"
               "    2: saturation\n"
               "    3: absolute colorimetric (default)\n"
               "  3dlut-size=<r>x<g>x<b>\n"
               "    Size of the 3D LUT generated from the ICC profile in each\n"
               "    dimension. Default is 128x256x64.\n"
               "    Sizes must be a power of two, and 256 at most.\n"
               "\n");
        goto err_out;
    }

    if (p->use_srgb)
        p->use_indirect = 1;

    int backend = backend_arg ? mpgl_find_backend(backend_arg) : GLTYPE_AUTO;
    free(backend_arg);

    if (fbo_format)
        p->fbo_format = find_fbo_format(fbo_format);
    free(fbo_format);

    for (int n = 0; n < 2; n++) {
        if (scalers[n])
            p->scalers[n].name = handle_scaler_opt(scalers[n]);
        free(scalers[n]);
    }

    int s_r = 128, s_g = 256, s_b = 64;
    if (icc_size_str)
        parse_3dlut_size(icc_size_str, &s_r, &s_g, &s_b);
    free(icc_size_str);

    bool success = true;
    if (icc_profile) {
        success = load_icc(p, icc_profile, icc_cache, icc_intent,
                           s_r, s_g, s_b);
    }
    free(icc_profile);
    free(icc_cache);

    if (!success)
        goto err_out;

    p->eosd = eosd_packer_create(vo);

    p->glctx = init_mpglcontext(backend, vo);
    if (!p->glctx)
        goto err_out;
    p->gl = p->glctx->gl;

    if (true) {
        if (!config_window(p, 320, 200, VOFLAG_HIDDEN))
            goto err_out;
        // We created a window to test whether the GL context could be
        // created and so on. Destroy that window to make sure all state
        // associated with it is lost.
        uninit(vo);
        p->glctx = init_mpglcontext(backend, vo);
        if (!p->glctx)
            goto err_out;
        p->gl = p->glctx->gl;
    }

    return 0;

err_out:
    uninit(vo);
    return -1;
}

static int control(struct vo *vo, uint32_t request, void *data)
{
    struct gl_priv *p = vo->priv;

    switch (request) {
    case VOCTRL_QUERY_FORMAT:
        return query_format(*(uint32_t *)data);
    case VOCTRL_GET_IMAGE:
        return get_image(vo, data);
    case VOCTRL_DRAW_IMAGE:
        return draw_image(p, data);
    case VOCTRL_DRAW_EOSD:
        if (!data)
            return VO_FALSE;
        draw_eosd(p, data);
        return VO_TRUE;
    case VOCTRL_GET_EOSD_RES: {
        mp_eosd_res_t *r = data;
        r->w = vo->dwidth;
        r->h = vo->dheight;
        r->mt = r->mb = r->ml = r->mr = 0;
        if (aspect_scaling()) {
            r->ml = r->mr = p->border_x;
            r->mt = r->mb = p->border_y;
        }
        return VO_TRUE;
    }
    case VOCTRL_ONTOP:
        if (!p->glctx->ontop)
            break;
        p->glctx->ontop(vo);
        return VO_TRUE;
    case VOCTRL_FULLSCREEN:
        p->glctx->fullscreen(vo);
        resize(p);
        return VO_TRUE;
    case VOCTRL_BORDER:
        if (!p->glctx->border)
            break;
        p->glctx->border(vo);
        resize(p);
        return VO_TRUE;
    case VOCTRL_GET_PANSCAN:
        return VO_TRUE;
    case VOCTRL_SET_PANSCAN:
        resize(p);
        return VO_TRUE;
    case VOCTRL_GET_EQUALIZER: {
        struct voctrl_get_equalizer_args *args = data;
        return mp_csp_equalizer_get(&p->video_eq, args->name, args->valueptr)
                >= 0 ? VO_TRUE : VO_NOTIMPL;
    }
    case VOCTRL_SET_EQUALIZER: {
        struct voctrl_set_equalizer_args *args = data;
        if (mp_csp_equalizer_set(&p->video_eq, args->name, args->value) < 0)
            return VO_NOTIMPL;
        if (!p->use_gamma && p->video_eq.values[MP_CSP_EQ_GAMMA] != 0) {
            mp_msg(MSGT_VO, MSGL_V, "[gl] Auto-enabling gamma.\n");
            p->use_gamma = true;
            compile_shaders(p);
        }
        update_all_uniforms(p);
        vo->want_redraw = true;
        return VO_TRUE;
    }
    case VOCTRL_SET_YUV_COLORSPACE: {
        if (p->is_yuv) {
            p->colorspace = *(struct mp_csp_details *)data;
            update_all_uniforms(p);
            vo->want_redraw = true;
        }
        return VO_TRUE;
    }
    case VOCTRL_GET_YUV_COLORSPACE:
        *(struct mp_csp_details *)data = p->colorspace;
        return VO_TRUE;
    case VOCTRL_UPDATE_SCREENINFO:
        if (!p->glctx->update_xinerama_info)
            break;
        p->glctx->update_xinerama_info(vo);
        return VO_TRUE;
    case VOCTRL_SCREENSHOT: {
        struct voctrl_screenshot_args *args = data;
        if (args->full_window)
            args->out_image = get_window_screenshot(p);
        else
            args->out_image = get_screenshot(p);
        return true;
    }
    case VOCTRL_REDRAW_FRAME:
        do_render(p);
        return true;
    }
    return VO_NOTIMPL;
}

const struct vo_driver video_out_gl3 = {
    .is_new = true,
    .info = &(const vo_info_t) {
        "OpenGL",
        "gl3",
        "Reimar Doeffinger <Reimar.Doeffinger@gmx.de> and others",
        ""
    },
    .preinit = preinit,
    .config = config,
    .control = control,
    .draw_slice = draw_slice,
    .draw_osd = draw_osd,
    .flip_page = flip_page,
    .check_events = check_events,
    .uninit = uninit,
};
