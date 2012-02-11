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

#include <libavutil/avutil.h>

#include "config.h"
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

// lscale/cscale arguments that map directly to shader filter routines.
// Note that the convolution filters are not included in this list.
static const char *fixed_scale_filters[] = {
    "bilinear",
    "bicubic_fast",
    "sharpen3",
    "sharpen5",
    NULL
};

struct convolution_filters {
    char *shader_fn, *shader_fn_sep;
    bool use_2d;
    GLint internal_format;
    GLenum format;
};

// indexed with filter_kernel->size
struct convolution_filters convolution_filters[] = {
    [2] = {"sample_convolution2", "sample_convolution_sep2", false, GL_RG16F,   GL_RG},
    [4] = {"sample_convolution4", "sample_convolution_sep4", false, GL_RGBA16F, GL_RGBA},
    [6] = {"sample_convolution6", "sample_convolution_sep6", true,  GL_RGB16F,  GL_RGB},
    [8] = {"sample_convolution8", "sample_convolution_sep8", true,  GL_RGBA16F, GL_RGBA},
};

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
    // (this can be false even for plane 1+2 if the format is planar RGB)
    bool is_chroma;
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

struct vertex_array {
    GLuint buffer;
    GLuint vao;
    int vertex_count;
};

struct scaler {
    int id;
    char *name;
    struct filter_kernel *kernel;
    GLuint gl_lut;
    int texunit;
    const char *lut_name;
};

struct fbotex {
    GLuint fbo;
    GLuint texture;
    int tex_w, tex_h;           // size of .texture
    int vp_w, vp_h;             // viewport of fbo / used part of the texture
};

struct gl_priv {
    MPGLContext *glctx;
    GL *gl;

    int gl_debug;
    int force_gl2;

    struct vertex_array va_osd, va_eosd, va_tmp;
    GLuint osd_program, eosd_program;
    GLuint indirect_program, scale_sep_program, final_program;

    //! Textures for OSD
    GLuint osdtex[MAX_OSD_PARTS];
    GLuint eosd_texture;
    GLuint eosd_buffer;
    int eosd_texture_width, eosd_texture_height;
    struct eosd_packer *eosd;
    struct vertex *eosd_va;
    struct vertex osd_va[MAX_OSD_PARTS * VERTICES_PER_QUAD];
    //! How many parts the OSD currently consists of
    int osdtexCnt;
    int osd_color;

    int use_indirect;           // convert YUV to RGB texture first
    int use_gamma;
    int use_srgb;
    int use_scale_sep;
    struct mp_csp_details colorspace;
    int is_yuv;
    float filter_strength;
    int use_rectangle;
    uint32_t image_width;
    uint32_t image_height;
    uint32_t image_format;
    uint32_t image_d_width;
    uint32_t image_d_height;
    int use_pbo;
    int use_glFinish;
    int swap_interval;

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


static void texSize(struct vo *vo, int w, int h, int *texw, int *texh)
{
    struct gl_priv *p = vo->priv;

    if (p->use_rectangle) {
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

static bool can_use_filter_kernel(struct filter_kernel *kernel)
{
    if (!kernel)
        return false;
    if (kernel->size >= FF_ARRAY_ELEMS(convolution_filters))
        return false;
    return !!convolution_filters[kernel->size].shader_fn;
}

static void vertex_array_init(GL *gl, struct vertex_array * va)
{
    size_t stride = sizeof(struct vertex);

    *va = (struct vertex_array) {0};

    gl->GenBuffers(1, &va->buffer);
    gl->GenVertexArrays(1, &va->vao);

    gl->BindBuffer(GL_ARRAY_BUFFER, va->buffer);
    gl->BindVertexArray(va->vao);

    gl->EnableVertexAttribArray(VERTEX_ATTRIB_POSITION);
    gl->VertexAttribPointer(VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE,
                            stride, (void*)offsetof(struct vertex, position));

    gl->EnableVertexAttribArray(VERTEX_ATTRIB_COLOR);
    gl->VertexAttribPointer(VERTEX_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                            stride, (void*)offsetof(struct vertex, color));

    gl->EnableVertexAttribArray(VERTEX_ATTRIB_TEXCOORD);
    gl->VertexAttribPointer(VERTEX_ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE,
                            stride, (void*)offsetof(struct vertex, texcoord));

    gl->BindBuffer(GL_ARRAY_BUFFER, 0);
    gl->BindVertexArray(0);
}

static void vertex_array_uninit(GL *gl, struct vertex_array *va)
{
    gl->DeleteVertexArrays(1, &va->vao);
    gl->DeleteBuffers(1, &va->buffer);
    *va = (struct vertex_array) {0};
}

static void vertex_array_upload(GL *gl, struct vertex_array *va,
                                struct vertex *vb, int vertex_count)
{
    gl->BindBuffer(GL_ARRAY_BUFFER, va->buffer);
    gl->BufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(struct vertex), vb,
                   GL_DYNAMIC_DRAW);
    gl->BindBuffer(GL_ARRAY_BUFFER, 0);
    va->vertex_count = vertex_count;
}

static void vertex_array_draw(GL *gl, struct vertex_array *va)
{
    gl->BindVertexArray(va->vao);
    gl->DrawArrays(GL_TRIANGLES, 0, va->vertex_count);
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

static void fbotex_init(struct vo *vo, struct fbotex *fbo, int w, int h)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    texSize(vo, w, h, &fbo->tex_w, &fbo->tex_h);

    fbo->vp_w = w;
    fbo->vp_h = h;

    gl->GenFramebuffers(1, &fbo->fbo);
    gl->GenTextures(1, &fbo->texture);
    gl->BindTexture(GL_TEXTURE_2D, fbo->texture);
    // We use a 16 bit format, because 8 bit is not really enough for
    // storing linear RGB without loss.
    glCreateClearTex(gl, GL_TEXTURE_2D, GL_RGBA16, GL_RGB, GL_UNSIGNED_BYTE,
                     GL_LINEAR, fbo->tex_w, fbo->tex_h, 0);
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

static void fbotex_uninit(struct vo *vo, struct fbotex *fbo)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    gl->DeleteFramebuffers(1, &fbo->fbo);
    fbo->fbo = 0;
    gl->DeleteTextures(1, &fbo->texture);
    fbo->texture = 0;
    fbo->tex_w = fbo->tex_h = 0;
    fbo->vp_w = fbo->vp_h = 0;
}

static void scaler_texture(struct vo *vo, GLuint program, struct scaler *scaler)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    if (!scaler->kernel)
        return;

    int size = scaler->kernel->size;
    struct convolution_filters *entry = &convolution_filters[size];

    GLint loc = gl->GetUniformLocation(program, scaler->lut_name);
    if (loc < 0)
        return;

    gl->Uniform1i(loc, scaler->texunit);

    gl->ActiveTexture(GL_TEXTURE0 + scaler->texunit);
    GLenum target = entry->use_2d ? GL_TEXTURE_2D : GL_TEXTURE_1D;

    if (scaler->gl_lut) {
        gl->BindTexture(target, scaler->gl_lut);
    } else {
        gl->GenTextures(1, &scaler->gl_lut);
        gl->BindTexture(target, scaler->gl_lut);
        gl->PixelStorei(GL_UNPACK_ALIGNMENT, 4);
        gl->PixelStorei(GL_UNPACK_ROW_LENGTH, 0);

        float *weights = talloc_array(NULL, float, LOOKUP_TEXTURE_SIZE * size);
        mp_compute_lut(scaler->kernel, LOOKUP_TEXTURE_SIZE, weights);
        if (entry->use_2d) {
            gl->TexImage2D(GL_TEXTURE_2D, 0, entry->internal_format, 2,
                           LOOKUP_TEXTURE_SIZE, 0, entry->format, GL_FLOAT,
                           weights);
        } else {
            gl->TexImage1D(GL_TEXTURE_1D, 0, entry->internal_format,
                           LOOKUP_TEXTURE_SIZE, 0, entry->format, GL_FLOAT,
                           weights);
        }
        talloc_free(weights);

        gl->TexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl->TexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl->TexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl->TexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    gl->ActiveTexture(GL_TEXTURE0);
}

static void update_uniforms(struct vo *vo, GLuint program)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;
    GLint loc;

    if (program == 0)
        return;

    gl->UseProgram(program);

    struct mp_csp_params cparams = {
        .colorspace = p->colorspace,
        .input_shift = -p->plane_bits & 7,
    };
    mp_csp_copy_equalizer_values(&cparams, &p->video_eq);

    loc = gl->GetUniformLocation(program, "transform");
    if (loc >= 0) {
        float matrix[3][3];
        matrix_ortho2d(matrix, 0, vo->dwidth, vo->dheight, 0);
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
        float factor = 1.0;
        if (p->use_srgb && p->is_yuv)
            factor = mp_csp_gamma(p->colorspace.format) / 2.2;
        gl->Uniform3f(loc, factor / cparams.rgamma, factor / cparams.ggamma,
                      factor / cparams.bgamma);
    }

    loc = gl->GetUniformLocation(program, "conv_gamma");
    if (loc >= 0)
        gl->Uniform1f(loc, mp_csp_gamma(p->colorspace.format));

    loc = gl->GetUniformLocation(program, "texture1");
    if (loc >= 0)
        gl->Uniform1i(loc, 0);
    loc = gl->GetUniformLocation(program, "texture2");
    if (loc >= 0)
        gl->Uniform1i(loc, 1);
    loc = gl->GetUniformLocation(program, "texture3");
    if (loc >= 0)
        gl->Uniform1i(loc, 2);

    scaler_texture(vo, program, &p->scalers[0]);
    scaler_texture(vo, program, &p->scalers[1]);

    loc = gl->GetUniformLocation(program, "filter_strength");
    if (loc >= 0)
        gl->Uniform1f(loc, p->filter_strength);

    gl->UseProgram(0);

    glCheckError(gl, "update_uniforms()");
}

static void update_all_uniforms(struct vo *vo)
{
    struct gl_priv *p = vo->priv;

    update_uniforms(vo, p->osd_program);
    update_uniforms(vo, p->eosd_program);
    update_uniforms(vo, p->indirect_program);
    update_uniforms(vo, p->scale_sep_program);
    update_uniforms(vo, p->final_program);
}

static void update_window_sized_objects(struct vo *vo)
{
    struct gl_priv *p = vo->priv;

    if (p->scale_sep_program) {
        if (p->dst_rect.height > p->scale_sep_fbo.tex_h) {
            fbotex_uninit(vo, &p->scale_sep_fbo);
            // Round up to an arbitrary alignment to make window resizing or
            // panscan controls smoother (less texture reallocations).
            int height = FFALIGN(p->dst_rect.height, 256);
            fbotex_init(vo, &p->scale_sep_fbo, p->image_width, height);
        }
        p->scale_sep_fbo.vp_w = p->image_width;
        p->scale_sep_fbo.vp_h = p->dst_rect.height;
    }
}

static void resize(struct vo *vo)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

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

    update_window_sized_objects(vo);

    update_all_uniforms(vo);

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
static void clearOSD(struct vo *vo)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    if (!p->osdtexCnt)
        return;
    gl->DeleteTextures(p->osdtexCnt, p->osdtex);
    p->osdtexCnt = 0;
}

/**
 * \brief construct display list from ass image list
 * \param img image list to create OSD from.
 */
static void genEOSD(struct vo *vo, mp_eosd_images_t *imgs)
{
    struct gl_priv *p = vo->priv;
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
        texSize(vo, p->eosd->surface.w, p->eosd->surface.h,
                &p->eosd_texture_width, &p->eosd_texture_height);
        // xxx it doesn't need to be cleared, that's a waste of time
        glCreateClearTex(gl, GL_TEXTURE_2D, GL_RED, GL_RED,
                         GL_UNSIGNED_BYTE, GL_NEAREST,
                         p->eosd_texture_width, p->eosd_texture_height, 0);
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
            gl->UnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
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

    vertex_array_upload(gl, &p->va_eosd, p->eosd_va,
                        p->eosd->targets_count * VERTICES_PER_QUAD);
}

static void draw_eosd(struct vo *vo, mp_eosd_images_t *imgs)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    genEOSD(vo, imgs);

    if (p->eosd->targets_count == 0)
        return;

    gl->Enable(GL_BLEND);
    gl->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl->BindTexture(GL_TEXTURE_2D, p->eosd_texture);
    gl->UseProgram(p->eosd_program);
    vertex_array_draw(gl, &p->va_eosd);
    gl->UseProgram(0);
    gl->BindTexture(GL_TEXTURE_2D, 0);
    gl->Disable(GL_BLEND);
}

static void delete_program(GL *gl, GLuint *prog)
{
    gl->DeleteProgram(*prog);
    *prog = 0;
}

static void delete_shaders(struct vo *vo)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    delete_program(gl, &p->osd_program);
    delete_program(gl, &p->eosd_program);
    delete_program(gl, &p->indirect_program);
    delete_program(gl, &p->scale_sep_program);
    delete_program(gl, &p->final_program);
}

// Free video resources etc.
static void uninitVideo(struct vo *vo)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    delete_shaders(vo);

    for (int n = 0; n < 2; n++) {
        gl->DeleteTextures(1, &p->scalers->gl_lut);
        p->scalers->gl_lut = 0;
    }

    for (int n = 0; n < 3; n++) {
        struct texplane *plane = &p->planes[n];

        gl->DeleteTextures(1, &plane->gl_texture);
        plane->gl_texture = 0;
        gl->DeleteBuffers(1, &plane->gl_buffer);
        plane->gl_buffer = 0;
        plane->buffer_ptr = NULL;
        plane->buffer_size = 0;
    }

    fbotex_uninit(vo, &p->indirect_fbo);
    fbotex_uninit(vo, &p->scale_sep_fbo);
}

/**
 * \brief uninitialize OpenGL context, freeing textures, buffers etc.
 */
static void uninitGL(struct vo *vo)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    uninitVideo(vo);

    vertex_array_uninit(gl, &p->va_osd);
    vertex_array_uninit(gl, &p->va_eosd);
    vertex_array_uninit(gl, &p->va_tmp);

    clearOSD(vo);
    gl->DeleteTextures(1, &p->eosd_texture);
    gl->DeleteBuffers(1, &p->eosd_buffer);
    eosd_packer_reinit(p->eosd, 0, 0);
    p->eosd_texture = 0;
}

// Return the high byte of the value that represents white in chroma (U/V)
static int get_chroma_clear_val(int bit_depth)
{
    return 1 << (bit_depth - 1 & 7);
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
    const char *target = scaler->id == 0 ? "SAMPLE_L" : "SAMPLE_C";
    if (!scaler->kernel) {
        *shader = talloc_asprintf_append(*shader, "#define %s sample_%s\n",
                                         target, scaler->name);
    } else {
        const struct convolution_filters *entry =
            &convolution_filters[scaler->kernel->size];
        if (pass != -1) {
            // strictly needed for the first pass
            if (pass == 0)
                shader_def_opt(shader, "FIXED_SCALE", true);
            // the direction/pass assignment is rather arbitrary
            const char *direction = pass == 0 ? "0, 1" : "1, 0";
            *shader = talloc_asprintf_append(*shader,
                "#define %s(p0, p1) %s(vec2(%s), %s, p0, p1)\n", target,
                entry->shader_fn_sep, direction, scaler->lut_name);
        } else {
            *shader = talloc_asprintf_append(*shader,
                "#define %s(p0, p1) %s(%s, p0, p1)\n", target, entry->shader_fn,
                scaler->lut_name);
        }
    }
}

static void compile_shaders(struct vo *vo)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    delete_shaders(vo);

    void *tmp = talloc_new(NULL);
    struct bstr src = { (char*)vo_gl3_shaders, sizeof(vo_gl3_shaders) };

    char *vertex_shader = get_section(tmp, src, "vertex_all");
    char *shader_prelude = get_section(tmp, src, "prelude");

    char *header = talloc_strdup(tmp, shader_prelude);

    p->eosd_program =
        create_program(gl, "eosd", header, vertex_shader,
            get_section(tmp, src, "frag_eosd"));

    p->osd_program =
        create_program(gl, "osd", header, vertex_shader,
            get_section(tmp, src, "frag_osd"));

    char *header_conv = talloc_strdup(tmp, "");
    char *header_final = talloc_strdup(tmp, "");
    char *header_sep = NULL;

    shader_def_opt(&header_conv, "USE_PLANAR", p->plane_count > 1);
    shader_def_opt(&header_conv, "USE_YGRAY", p->is_yuv && p->plane_count == 1);
    shader_def_opt(&header_conv, "USE_COLORMATRIX", p->is_yuv);
    shader_def_opt(&header_final, "USE_GAMMA_POW", p->use_gamma);

    if (p->use_indirect && p->use_scale_sep && p->scalers[0].kernel) {
        header_sep = talloc_strdup(tmp, "");
        shader_setup_scaler(&header_sep, &p->scalers[0], 0);
        shader_setup_scaler(&header_final, &p->scalers[0], 1);
    } else {
        shader_setup_scaler(&header_final, &p->scalers[0], -1);
    }

    shader_setup_scaler(&header_conv, &p->scalers[1], -1);

    if (p->use_indirect && p->is_yuv) {
        // We don't use filtering for the Y-plane (luma), because it's never
        // scaled in this scenario.
        shader_def(&header_conv, "SAMPLE_L", "sample_bilinear");
        shader_def_opt(&header_conv, "USE_LINEAR_CONV", p->use_srgb);
        shader_def_opt(&header_conv, "FIXED_SCALE", true);
        header_conv = talloc_asprintf(tmp, "%s%s", header, header_conv);
        header_final = talloc_asprintf(tmp, "%s%s", header, header_final);
        p->indirect_program =
            create_program(gl, "indirect", header_conv, vertex_shader,
                get_section(tmp, src, "frag_video"));
    } else {
        header_final = talloc_asprintf(tmp, "%s%s%s", header, header_conv,
                                       header_final);
    }

    if (header_sep) {
        header_sep = talloc_asprintf(tmp, "%s%s", header, header_sep);
        p->scale_sep_program =
            create_program(gl, "scale_sep", header_sep, vertex_shader,
                get_section(tmp, src, "frag_video"));
    }

    p->final_program =
        create_program(gl, "final", header_final, vertex_shader,
            get_section(tmp, src, "frag_video"));

    glCheckError(gl, "shader compilation");

    talloc_free(tmp);
}


// First-time initialization of the GL state.
static int initGL(struct vo *vo)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    glCheckError(p->gl, "before initGL");

    const char *vendor     = gl->GetString(GL_VENDOR);
    const char *version    = gl->GetString(GL_VERSION);
    const char *renderer   = gl->GetString(GL_RENDERER);
    const char *glsl       = gl->GetString(GL_SHADING_LANGUAGE_VERSION);
    mp_msg(MSGT_VO, MSGL_V, "[gl] GL_RENDERER='%s', GL_VENDOR='%s', "
                            "GL_VERSION='%s', GL_SHADING_LANGUAGE_VERSION='%s'"
                            "\n", renderer, vendor, version, glsl);

    gl->Disable(GL_BLEND);
    gl->Disable(GL_DEPTH_TEST);
    gl->DepthMask(GL_FALSE);
    gl->Disable(GL_CULL_FACE);
    gl->DrawBuffer(GL_BACK);

    vertex_array_init(gl, &p->va_eosd);
    vertex_array_init(gl, &p->va_osd);
    vertex_array_init(gl, &p->va_tmp);

    GLint max_texture_size;
    gl->GetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    eosd_packer_reinit(p->eosd, max_texture_size, max_texture_size);

    gl->ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl->Clear(GL_COLOR_BUFFER_BIT);
    if (gl->SwapInterval && p->swap_interval >= 0)
        gl->SwapInterval(p->swap_interval);

    if (p->use_srgb) {
        GLboolean b = 0;
        gl->GetBooleanv(GL_FRAMEBUFFER_SRGB_CAPABLE_EXT, &b);
        if (!b) {
            mp_msg(MSGT_VO, MSGL_ERR, "[gl]no sRGB framebuffer! Disabling any "
                                      "sRGB use.\n");
            p->use_srgb = 0;
        }
    }

    glCheckError(gl, "after initGL");

    return 1;
}

static void initVideo(struct vo *vo)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    int xs, ys, depth;
    p->is_yuv = mp_get_chroma_shift(p->image_format, &xs, &ys, &depth) > 0;
    glFindFormat(p->image_format, true, NULL, &p->gl_internal_format,
                 &p->gl_format, &p->gl_type);

    // fix for legacy crap from gl_common.c
    if (p->gl_internal_format == 1) p->gl_internal_format = GL_RED;
    if (p->gl_internal_format == 2) p->gl_internal_format = GL_RG;
    if (p->gl_internal_format == 3) p->gl_internal_format = GL_RGB;
    if (p->gl_internal_format == 4) p->gl_internal_format = GL_RGBA;
    if (p->gl_format == GL_LUMINANCE)
        p->gl_format = GL_RED;

    if (!p->is_yuv && p->use_srgb)
        p->gl_internal_format = GL_SRGB;

    if (!p->is_yuv) {
        // xxx mp_image_setfmt calculates this as well
        depth = glFmt2bpp(p->gl_format, p->gl_type) * 8;
    }

    p->plane_bits = depth;
    p->plane_bytes = (depth + 7) / 8;

    p->plane_count = p->is_yuv ? 3 : 1;
    if (p->image_format == IMGFMT_Y800)
        p->plane_count = 1;

    for (int n = 0; n < p->plane_count; n++) {
        struct texplane *plane = &p->planes[n];

        plane->is_chroma = n > 0;
        plane->shift_x = n > 0 ? xs : 0;
        plane->shift_y = n > 0 ? ys : 0;
        plane->clear_val = n > 0 ? get_chroma_clear_val(p->plane_bits) : 0;
    }

    int eq_caps = 0;
    if (p->is_yuv)
        eq_caps |= MP_CSP_EQ_CAPS_COLORMATRIX;
    if (p->use_gamma)
        eq_caps |= MP_CSP_EQ_CAPS_GAMMA;
    p->video_eq.capabilities = eq_caps;

    compile_shaders(vo);

    glCheckError(gl, "before video texture creation");

    texSize(vo, p->image_width, p->image_height,
            &p->texture_width, &p->texture_height);

    int scale_type = p->mipmap_gen ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR;

    for (int n = 0; n < p->plane_count; n++) {
        struct texplane *plane = &p->planes[n];

        int w = p->texture_width >> plane->shift_x;
        int h = p->texture_height >> plane->shift_y;

        mp_msg(MSGT_VO, MSGL_V, "[gl] Texture for plane %d: %dx%d\n", n, w, h);

        gl->ActiveTexture(GL_TEXTURE0 + n);
        gl->GenTextures(1, &plane->gl_texture);
        gl->BindTexture(GL_TEXTURE_2D, plane->gl_texture);

        glCreateClearTex(gl, GL_TEXTURE_2D, p->gl_internal_format, p->gl_format,
                         p->gl_type, scale_type, w, h, plane->clear_val);
    }
    gl->ActiveTexture(GL_TEXTURE0);

    glCheckError(gl, "after video texture creation");

    if (p->indirect_program)
        fbotex_init(vo, &p->indirect_fbo, p->texture_width, p->texture_height);
}


static int create_window(struct vo *vo, uint32_t d_width, uint32_t d_height,
                         uint32_t flags)
{
    struct gl_priv *p = vo->priv;

    if (p->stereo_mode == GL_3D_QUADBUFFER)
        flags |= VOFLAG_STEREO;

    int mpgl_version = p->force_gl2 ? MPGL_VER(2, 1) : MPGL_VER(3, 0);
    int mpgl_flags = 0;
    if (p->gl_debug)
        mpgl_flags |= MPGLFLAG_DEBUG;

    return create_mpglcontext(p->glctx, mpgl_flags, mpgl_version, d_width,
                              d_height, flags);
}

static int config(struct vo *vo, uint32_t width, uint32_t height,
                  uint32_t d_width, uint32_t d_height, uint32_t flags,
                  uint32_t format)
{
    struct gl_priv *p = vo->priv;

    if (create_window(vo, d_width, d_height, flags) == SET_WINDOW_FAILED)
        return -1;

    if (!vo->config_count)
        initGL(vo);

    p->image_d_width = d_width;
    p->image_d_height = d_height;
    p->vo_flipped = !!(flags & VOFLAG_FLIPPING);

    if (p->image_format != format || p->image_width != width
        || p->image_height != height)
    {
        uninitVideo(vo);
        p->image_height = height;
        p->image_width = width;
        p->image_format = format;
        initVideo(vo);
    }

    resize(vo);

    return 0;
}

static void check_events(struct vo *vo)
{
    struct gl_priv *p = vo->priv;

    int e = p->glctx->check_events(vo);
    if (e & VO_EVENT_REINIT) {
        uninitGL(vo);
        initGL(vo);
        initVideo(vo);
        resize(vo);
    }
    if (e & VO_EVENT_RESIZE)
        resize(vo);
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
    struct vo *vo = ctx;
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    // initialize to 8 to avoid special-casing on alignment
    int sx = 8, sy = 8;
    GLint scale_type = GL_NEAREST;

    if (w <= 0 || h <= 0 || stride < w) {
        mp_msg(MSGT_VO, MSGL_V, "Invalid dimensions OSD for part!\n");
        return;
    }
    texSize(vo, w, h, &sx, &sy);

    if (p->osdtexCnt >= MAX_OSD_PARTS) {
        mp_msg(MSGT_VO, MSGL_ERR, "Too many OSD parts, contact the developers!\n");
        return;
    }

    gl->GenTextures(1, &p->osdtex[p->osdtexCnt]);
    gl->BindTexture(GL_TEXTURE_2D, p->osdtex[p->osdtexCnt]);
    glCreateClearTex(gl, GL_TEXTURE_2D, GL_RG, GL_RG, GL_UNSIGNED_BYTE,
                     scale_type, sx, sy, 0);
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

    write_quad(&p->osd_va[p->osdtexCnt * VERTICES_PER_QUAD],
               x0, y0, x0 + w, y0 + h, 0, 0, w, h,
               sx, sy, color, false);

    p->osdtexCnt++;
}

static void draw_osd(struct vo *vo, struct osd_state *osd)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    if (vo_osd_changed(0)) {
        clearOSD(vo);
        osd_draw_text_ext(osd, vo->dwidth, vo->dheight, p->border_x,
                          p->border_y, p->border_x,
                          p->border_y, p->image_width,
                          p->image_height, create_osd_texture, vo);
        vertex_array_upload(gl, &p->va_osd, &p->osd_va[0],
                            p->osdtexCnt * VERTICES_PER_QUAD);
    }

    if (p->osdtexCnt > 0) {
        gl->Enable(GL_BLEND);
        // OSD bitmaps use premultiplied alpha.
        gl->BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        gl->UseProgram(p->osd_program);
        gl->BindVertexArray(p->va_osd.vao);

        for (int n = 0; n < p->osdtexCnt; n++) {
            gl->BindTexture(GL_TEXTURE_2D, p->osdtex[n]);
            gl->DrawArrays(GL_TRIANGLES, n * VERTICES_PER_QUAD,
                           VERTICES_PER_QUAD);
        }

        gl->BindVertexArray(0);
        gl->UseProgram(0);

        gl->Disable(GL_BLEND);
        gl->BindTexture(GL_TEXTURE_2D, 0);
    }
}

static void render_to_fbo(struct vo *vo, struct fbotex *fbo, int w, int h,
                          int tex_w, int tex_h)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    gl->Viewport(0, 0, fbo->vp_w, fbo->vp_h);
    gl->BindFramebuffer(GL_FRAMEBUFFER, fbo->fbo);

    struct vertex vb[VERTICES_PER_QUAD];
    write_quad(vb, -1, -1, 1, 1,
               0, 0, w, h,
               tex_w, tex_h,
               NULL, false);
    vertex_array_upload(gl, &p->va_tmp, vb, VERTICES_PER_QUAD);
    vertex_array_draw(gl, &p->va_tmp);

    gl->BindFramebuffer(GL_FRAMEBUFFER, 0);
    gl->Viewport(p->vp_x, p->vp_y, p->vp_w, p->vp_h);

}

static void handle_pass(struct vo *vo, struct fbotex **source,
                        struct fbotex *fbo, GLuint program)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    if (!program)
        return;

    gl->BindTexture(GL_TEXTURE_2D, (*source)->texture);
    gl->UseProgram(program);
    render_to_fbo(vo, fbo, (*source)->vp_w, (*source)->vp_h,
                  (*source)->tex_w, (*source)->tex_h);
    *source = fbo;
}

static void do_render(struct vo *vo)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;
    struct vertex vb[VERTICES_PER_QUAD * 2];
    bool is_flipped = p->mpi_flipped ^ p->vo_flipped;

    // Order of processing:
    //  [indirect -> [scale_sep ->]] final

    struct fbotex dummy = {
        .vp_w = p->image_width, .vp_h = p->image_height,
        .tex_w = p->texture_width, .tex_h = p->texture_height,
        .texture = p->planes[0].gl_texture,
    };
    struct fbotex *source = &dummy;

    handle_pass(vo, &source, &p->indirect_fbo, p->indirect_program);
    handle_pass(vo, &source, &p->scale_sep_fbo, p->scale_sep_program);

    gl->BindTexture(GL_TEXTURE_2D, source->texture);
    gl->UseProgram(p->final_program);

    float final_texw = p->image_width * source->tex_w / (float)source->vp_w;
    float final_texh = p->image_height * source->tex_h / (float)source->vp_h;

    if (p->use_srgb)
        gl->Enable(GL_FRAMEBUFFER_SRGB);

    if (p->stereo_mode) {
        int w = p->src_rect.width;
        int imgw = p->image_width;
        write_quad(vb,
                   p->dst_rect.left, p->dst_rect.top,
                   p->dst_rect.right, p->dst_rect.bottom,
                   p->src_rect.left / 2, p->src_rect.top,
                   p->src_rect.left / 2 + w / 2, p->src_rect.bottom,
                   final_texw, final_texh,
                   NULL, is_flipped);
        write_quad(vb + VERTICES_PER_QUAD,
                   p->dst_rect.left, p->dst_rect.top,
                   p->dst_rect.right, p->dst_rect.bottom,
                   p->src_rect.left / 2 + imgw / 2, p->src_rect.top,
                   p->src_rect.left / 2 + imgw / 2 + w / 2, p->src_rect.bottom,
                   final_texw, final_texh,
                   NULL, is_flipped);

        vertex_array_upload(gl, &p->va_tmp, vb, VERTICES_PER_QUAD * 2);

        gl->BindVertexArray(p->va_tmp.vao);

        glEnable3DLeft(gl, p->stereo_mode);
        gl->DrawArrays(GL_TRIANGLES, 0, VERTICES_PER_QUAD);
        glEnable3DRight(gl, p->stereo_mode);
        gl->DrawArrays(GL_TRIANGLES, VERTICES_PER_QUAD, VERTICES_PER_QUAD);
        glDisable3D(gl, p->stereo_mode);

        gl->BindVertexArray(0);
    } else {
        write_quad(vb,
                   p->dst_rect.left, p->dst_rect.top,
                   p->dst_rect.right, p->dst_rect.bottom,
                   p->src_rect.left, p->src_rect.top,
                   p->src_rect.right, p->src_rect.bottom,
                   final_texw, final_texh,
                   NULL, is_flipped);

        vertex_array_upload(gl, &p->va_tmp, vb, VERTICES_PER_QUAD);
        vertex_array_draw(gl, &p->va_tmp);
    }

    if (p->use_srgb)
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

    assert(mpi->num_planes >= p->plane_count);

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

static uint32_t draw_image(struct vo *vo, mp_image_t *mpi)
{
    struct gl_priv *p = vo->priv;
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
        && get_image(vo, &mpi2) == VO_TRUE)
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
            gl->UnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
            plane->buffer_ptr = NULL;
            plane_ptr = NULL; // PBO offset 0
        }
        gl->ActiveTexture(GL_TEXTURE0 + n);
        gl->BindTexture(GL_TEXTURE_2D, plane->gl_texture);
        glUploadTex(gl, GL_TEXTURE_2D, p->gl_format, p->gl_type, plane_ptr,
                    mpi->stride[n], mpi->x >> xs, mpi->y >> ys, w >> xs,
                    h >> ys, 0);
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
    do_render(vo);
    return VO_TRUE;
}

static mp_image_t *get_screenshot(struct vo *vo)
{
    struct gl_priv *p = vo->priv;
    GL *gl = p->gl;

    mp_image_t *image = alloc_mpi(p->texture_width, p->texture_height,
                                  p->image_format);
    assert(image->num_planes >= p->plane_count);
    // Account for alpha plane.
    // NOTE about image formats with alpha plane: we don't even have the alpha
    // anymore. We never upload it to any texture, as it would be a waste of
    // time. On the other hand, we can't find a "similar", non-alpha image
    // format easily. So we just leave the alpha plane of the newly allocated
    // image as-is, and hope that the alpha is ignored by the receiver of the
    // screenshot. (If not, code should be added to make it fully opaque.)
    assert(image->num_planes <= p->plane_count + 1);

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

static mp_image_t *get_window_screenshot(struct vo *vo)
{
    struct gl_priv *p = vo->priv;
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

static int query_format(struct vo *vo, uint32_t format)
{
    int depth;
    int caps = VFCAP_CSP_SUPPORTED | VFCAP_CSP_SUPPORTED_BY_HW | VFCAP_FLIP |
               VFCAP_HWSCALE_UP | VFCAP_HWSCALE_DOWN | VFCAP_ACCEPT_STRIDE |
               VFCAP_OSD | VFCAP_EOSD | VFCAP_EOSD_UNSCALED;
    if (mp_get_chroma_shift(format, NULL, NULL, &depth) &&
        (IMGFMT_IS_YUVP16_NE(format) || !IMGFMT_IS_YUVP16(format)))
        return caps;
    // xxx glFindFormat reports this as supported
    if (format == IMGFMT_UYVY || format == IMGFMT_YVYU)
        return 0;
    if (glFindFormat(format, true, NULL, NULL, NULL, NULL))
        return caps;
    return 0;
}

static void uninit(struct vo *vo)
{
    struct gl_priv *p = vo->priv;

    // NOTE: GL functions might not be loaded yet
    if (p->glctx && p->gl->DeleteTextures)
        uninitGL(vo);
    uninit_mpglcontext(p->glctx);
    p->glctx = NULL;
    p->gl = NULL;
}

static bool handle_scaler_opt(struct vo *vo, struct scaler *scaler)
{
    if (!scaler->name || scaler->name[0] == '\0')
        scaler->name = talloc_strdup(vo, "bilinear");

    if (strcmp(scaler->name, "help") == 0) {
        mp_msg(MSGT_VO, MSGL_INFO, "Available scalers:\n");
        for (const char **e = fixed_scale_filters; *e; e++) {
            mp_msg(MSGT_VO, MSGL_INFO, "    %s\n", *e);
        }
        for (const struct filter_kernel *e = mp_filter_kernels; e->name; e++) {
            mp_msg(MSGT_VO, MSGL_INFO, "    %s\n", e->name);
        }
        return false;
    }

    scaler->kernel = NULL;
    for (const char **filter = fixed_scale_filters; *filter; filter++) {
        if (strcmp(*filter, scaler->name) == 0)
            return true;
    }

    scaler->kernel = mp_find_filter_kernel(scaler->name);
    if (can_use_filter_kernel(scaler->kernel)) {
        const struct convolution_filters *entry =
            &convolution_filters[scaler->kernel->size];
        bool is_luma = scaler->id == 0;
        scaler->lut_name = entry->use_2d
                           ? (is_luma ? "lut_l_2d" : "lut_c_2d")
                           : (is_luma ? "lut_l_1d" : "lut_c_1d");
        return true;
    }

    mp_msg(MSGT_VO, MSGL_FATAL, "[gl] Error: scaler '%s' not found.\n",
           scaler->name);
    return false;
}

static int backend_valid(void *arg)
{
    return mpgl_find_backend(*(const char **)arg) >= 0;
}

static int preinit(struct vo *vo, const char *arg)
{
    struct gl_priv *p = talloc_zero(vo, struct gl_priv);
    vo->priv = p;

    *p = (struct gl_priv) {
        .colorspace = MP_CSP_DETAILS_DEFAULTS,
        .filter_strength = 0.5,
        .use_rectangle = 1,
        .use_pbo = 1,
        .swap_interval = 1,
        .osd_color = 0xffffff,
        .scalers = {
            { .id = 0, .texunit = 5 },
            { .id = 1, .texunit = 6 },
        },
    };

    p->eosd = eosd_packer_create(vo);

    char *lscale = NULL;
    char *cscale = NULL;
    char *backend_arg = NULL;

    const opt_t subopts[] = {
        {"gamma",        OPT_ARG_BOOL, &p->use_gamma,    NULL},
        {"srgb",         OPT_ARG_BOOL, &p->use_srgb,     NULL},
        {"rectangle",    OPT_ARG_INT,  &p->use_rectangle,int_non_neg},
        {"filter-strength", OPT_ARG_FLOAT, &p->filter_strength, NULL},
        {"pbo",          OPT_ARG_BOOL, &p->use_pbo,      NULL},
        {"glfinish",     OPT_ARG_BOOL, &p->use_glFinish, NULL},
        {"swapinterval", OPT_ARG_INT,  &p->swap_interval,NULL},
        {"mipmapgen",    OPT_ARG_BOOL, &p->mipmap_gen,   NULL},
        {"osdcolor",     OPT_ARG_INT,  &p->osd_color,    NULL},
        {"stereo",       OPT_ARG_INT,  &p->stereo_mode,  NULL},
        {"lscale",       OPT_ARG_MSTRZ,&lscale,          NULL},
        {"cscale",       OPT_ARG_MSTRZ,&cscale,          NULL},
        {"debug",        OPT_ARG_BOOL, &p->gl_debug,     NULL},
        {"force-gl2",    OPT_ARG_BOOL, &p->force_gl2,    NULL},
        {"indirect",     OPT_ARG_BOOL, &p->use_indirect, NULL},
        {"scale-sep",    OPT_ARG_BOOL, &p->use_scale_sep, NULL},
        {"backend",      OPT_ARG_MSTRZ,&backend_arg,     backend_valid},
        {NULL}
    };

    if (subopt_parse(arg, subopts) != 0) {
        mp_msg(MSGT_VO, MSGL_FATAL,
               "\n-vo gl command line help:\n"
               "Example: mplayer -vo gl:yuv=2\n"
               "\nOptions:\n"
               "  gamma\n"
               "    Enable gamma control.\n"
               "  srgb\n"
               "    Gamma-correct scaling by working in linear colorspace. This\n"
               "    makes use of sRGB textures and framebuffers.\n"
               "    This option forces the options 'indirect' and 'gamma'.\n"
               "  rectangle=<0,1,2>\n"
               "    0: use power-of-two textures\n"
               "    1 and 2: use texture_non_power_of_two\n"
               "  no-pbo\n"
               "    Disable use of PBOs.\n"
               "  glfinish\n"
               "    Call glFinish() before swapping buffers\n"
               "  swapinterval=<n>\n"
               "    Interval in displayed frames between to buffer swaps.\n"
               "    1 is equivalent to enable VSYNC, 0 to disable VSYNC.\n"
               "    Requires GLX_SGI_swap_control support to work.\n"
               "  lscale=<filter>\n"
               "    bilinear: use standard bilinear scaling for luma.\n"
               "    bicubic_fast: bicubic filter (without lookup texture).\n"
               "    sharpen3: unsharp masking (sharpening) with radius=3.\n"
               "    sharpen5: unsharp masking (sharpening) with radius=5.\n"
               "    lanczos2: Lanczos with radius=2.\n"
               "    lanczos3: Lanczos with radius=3.\n"
               "  There are more filters - print a list with lscale=help.\n"
               "  cscale=<n>\n"
               "    as lscale but for chroma (2x slower with little visible effect).\n"
               "  scale-sep\n"
               "    When using a separable scale filter for luma, do two filter\n"
               "    passes. This is often faster. Note that chroma scalers will\n"
               "    still be done as 1-pass filter.\n"
               "    Forces the option 'indirect', and scaling will be done in RGB.\n"
               "  filter-strength=<value>\n"
               "    set the effect strength for the sharpen4/sharpen5 filters\n"
               "  mipmapgen\n"
               "    generate mipmaps for the video image (helps with downscaling)\n"
               "  osdcolor=<0xAARRGGBB>\n"
               "    use the given color for the OSD\n"
               "  stereo=<n>\n"
               "    0: normal display\n"
               "    1: side-by-side to red-cyan stereo\n"
               "    2: side-by-side to green-magenta stereo\n"
               "    3: side-by-side to quadbuffer stereo\n"
               "  indirect\n"
               "    Do YUV conversion and scaling as separate passes. This will\n"
               "    first render the video into a video-sized RGB texture, and\n"
               "    draw the result on screen. The luma scaler is used to scale\n"
               "    the RGB image when rendering to screen. The chroma scaler\n"
               "    is used only on YUV conversion, and only if the video uses\n"
               "    chroma-subsampling.\n"
               "    This mechanism is disabled on RGB input.\n"
               "  force-gl2\n"
               "    Create a legacy GL context. This will randomly malfunction\n"
               "    if the proper extensions are not supported.\n"
               "  backend=<sys>\n"
               "    auto: auto-select (default)\n"
               "    cocoa: Cocoa/OSX\n"
               "    win: Win32/WGL\n"
               "    x11: X11/GLX\n"
               "\n");
        return -1;
    }

    if (p->use_srgb) {
        p->use_indirect = 1;
        p->use_gamma = 1;
    }

    if (p->use_scale_sep)
        p->use_indirect = 1;

    int backend = backend_arg ? mpgl_find_backend(backend_arg) : GLTYPE_AUTO;
    free(backend_arg);

    p->scalers[0].name = talloc_strdup(vo, lscale);
    p->scalers[1].name = talloc_strdup(vo, cscale);
    free(lscale);
    free(cscale);

    if (!handle_scaler_opt(vo, &p->scalers[0]))
        goto err_out;
    if (!handle_scaler_opt(vo, &p->scalers[1]))
        goto err_out;

    p->glctx = init_mpglcontext(backend, vo);
    if (!p->glctx)
        goto err_out;
    p->gl = p->glctx->gl;

    if (true) {
        if (create_window(vo, 320, 200, VOFLAG_HIDDEN) == SET_WINDOW_FAILED)
            goto err_out;
        if (!initGL(vo))
            goto err_out;
        // We created a window to test whether the GL context supports hardware
        // acceleration and so on. Destroy that window to make sure all state
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
        return query_format(vo, *(uint32_t *)data);
    case VOCTRL_GET_IMAGE:
        return get_image(vo, data);
    case VOCTRL_DRAW_IMAGE:
        return draw_image(vo, data);
    case VOCTRL_DRAW_EOSD:
        if (!data)
            return VO_FALSE;
        draw_eosd(vo, data);
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
        resize(vo);
        return VO_TRUE;
    case VOCTRL_BORDER:
        if (!p->glctx->border)
            break;
        p->glctx->border(vo);
        resize(vo);
        return VO_TRUE;
    case VOCTRL_GET_PANSCAN:
        return VO_TRUE;
    case VOCTRL_SET_PANSCAN:
        resize(vo);
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
        update_all_uniforms(vo);
        vo->want_redraw = true;
        return VO_TRUE;
    }
    case VOCTRL_SET_YUV_COLORSPACE: {
        if (p->is_yuv) {
            p->colorspace = *(struct mp_csp_details *)data;
            update_all_uniforms(vo);
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
            args->out_image = get_window_screenshot(vo);
        else
            args->out_image = get_screenshot(vo);
        return true;
    }
    case VOCTRL_REDRAW_FRAME:
        do_render(vo);
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
