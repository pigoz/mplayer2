/*
 * Copyright (c) 2008 Georgi Petrov (gogothebee) <gogothebee@gmail.com>
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

#include <windows.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <d3d9.h>
#include "config.h"
#include "talloc.h"
#include "video_out.h"
#include "video_out_internal.h"
#include "fastmemcpy.h"
#include "mp_msg.h"
#include "aspect.h"
#include "w32_common.h"
#include "libavutil/common.h"
#include "sub/font_load.h"
#include "sub/sub.h"
#include "eosd_packer.h"

static const vo_info_t info =
{
    "Direct3D 9 Renderer",
    "direct3d",
    "Georgi Petrov (gogothebee) <gogothebee@gmail.com>",
    ""
};

// texture format for EOSD
// 0: use D3DFMT_A8L8
// 1: use D3DFMT_A8 (doesn't work with wine)
#define USE_A8 0

/*
 * Link essential libvo functions: preinit, config, control, draw_frame,
 * draw_slice, draw_osd, flip_page, check_events, uninit and
 * the structure info.
 */
const LIBVO_EXTERN(direct3d)

#define D3DFVF_OSD_VERTEX (D3DFVF_XYZ | D3DFVF_TEX1)

typedef struct {
    float x, y, z;      /* Position of vertex in 3D space */
    float tu, tv;       /* Texture coordinates */
} vertex_osd;

#define D3DFVF_EOSD_VERTEX (D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_DIFFUSE)

typedef struct {
    float x, y, z;
    D3DCOLOR color;
    float tu, tv;
} vertex_eosd;

/* Global variables "priv" structure. I try to keep their count low.
 */
static struct global_priv {
    int is_paused;              /**< 1 = Movie is paused,
                                0 = Movie is not paused */
    int is_clear_needed;        /**< 1 = Clear the backbuffer before StretchRect
                                0 = (default) Don't clear it */
    D3DLOCKED_RECT locked_rect; /**< The locked offscreen surface */
    RECT fs_movie_rect;         /**< Rect (upscaled) of the movie when displayed
                                in fullscreen */
    RECT fs_panscan_rect;       /**< PanScan source surface cropping in
                                fullscreen */
    int src_width;              /**< Source (movie) width */
    int src_height;             /**< Source (movie) heigth */
    int border_x;               /**< horizontal border value for OSD */
    int border_y;               /**< vertical border value for OSD */

    D3DFORMAT movie_src_fmt;        /**< Movie colorspace format (depends on
                                    the movie's codec) */
    D3DFORMAT desktop_fmt;          /**< Desktop (screen) colorspace format.
                                    Usually XRGB */

    HANDLE d3d9_dll;                /**< d3d9 Library HANDLE */
    IDirect3D9 * (WINAPI *pDirect3DCreate9)(UINT); /**< pointer to Direct3DCreate9 function */

    LPDIRECT3D9        d3d_handle;  /**< Direct3D Handle */
    LPDIRECT3DDEVICE9  d3d_device;  /**< The Direct3D Adapter */
    IDirect3DSurface9 *d3d_surface; /**< Offscreen Direct3D Surface. MPlayer
                                    renders inside it. Uses colorspace
                                    priv->movie_src_fmt */
    IDirect3DTexture9 *d3d_texture_osd; /**< Direct3D Texture. Uses RGBA */
    IDirect3DTexture9 *d3d_texture_system; /**< Direct3D Texture. System memory
                                    cannot lock a normal texture. Uses RGBA */
    IDirect3DSurface9 *d3d_backbuf; /**< Video card's back buffer (used to
                                    display next frame) */
    IDirect3DTexture9 *d3d_texture_eosd; /**< Direct3D Texture. Uses A8L8 */
    int cur_backbuf_width;          /**< Current backbuffer width */
    int cur_backbuf_height;         /**< Current backbuffer height */
    int is_osd_populated;           /**< 1 = OSD texture has something to display,
                                    0 = OSD texture is clear */
    int device_caps_power2_only;    /**< 1 = texture sizes have to be power 2
                                    0 = texture sizes can be anything */
    int device_caps_square_only;    /**< 1 = textures have to be square
                                    0 = textures do not have to be square */
    int device_texture_sys;         /**< 1 = device can texture from system memory
                                    0 = device requires shadow */
    int max_texture_width;          /**< from the device capabilities */
    int max_texture_height;         /**< from the device capabilities */
    int osd_width;                  /**< current width of the OSD */
    int osd_height;                 /**< current height of the OSD */
    int osd_texture_width;          /**< current width of the OSD texture */
    int osd_texture_height;         /**< current height of the OSD texture */
    int eosd_texture_width;
    int eosd_texture_height;

    struct eosd_packer *eosd;       /**< EOSD packer (image positions etc.) */
    vertex_eosd *eosd_vb;           /**< temporary memory for D3D when rendering EOSD */
} *priv;

typedef struct {
    const unsigned int  mplayer_fmt;   /**< Given by MPlayer */
    const D3DFORMAT     fourcc;        /**< Required by D3D's test function */
} struct_fmt_table;

/* Map table from reported MPlayer format to the required
   fourcc. This is needed to perform the format query. */

static const struct_fmt_table fmt_table[] = {
    {IMGFMT_YV12,  MAKEFOURCC('Y','V','1','2')},
    {IMGFMT_I420,  MAKEFOURCC('I','4','2','0')},
    {IMGFMT_IYUV,  MAKEFOURCC('I','Y','U','V')},
    {IMGFMT_YVU9,  MAKEFOURCC('Y','V','U','9')},
    {IMGFMT_YUY2,  D3DFMT_YUY2},
    {IMGFMT_UYVY,  D3DFMT_UYVY},
    {IMGFMT_BGR32, D3DFMT_X8R8G8B8},
    {IMGFMT_RGB32, D3DFMT_X8B8G8R8},
    {IMGFMT_BGR24, D3DFMT_R8G8B8}, //untested
    {IMGFMT_BGR16, D3DFMT_R5G6B5},
    {IMGFMT_BGR15, D3DFMT_X1R5G5B5},
    {IMGFMT_BGR8 , D3DFMT_R3G3B2}, //untested
};

#define DISPLAY_FORMAT_TABLE_ENTRIES (sizeof(fmt_table) / sizeof(fmt_table[0]))

typedef enum back_buffer_action {
    BACKBUFFER_CREATE,
    BACKBUFFER_RESET
} back_buffer_action_e;

static void generate_eosd(mp_eosd_images_t *);
static void draw_eosd(void);


/****************************************************************************
 *                                                                          *
 *                                                                          *
 *                                                                          *
 * Direct3D specific implementation functions                               *
 *                                                                          *
 *                                                                          *
 *                                                                          *
 ****************************************************************************/

/** @brief Calculate scaled fullscreen movie rectangle with
 *  preserved aspect ratio.
 */
static void calc_fs_rect(void)
{
    struct vo_rect src_rect;
    struct vo_rect dst_rect;
    struct vo_rect borders;
    calc_src_dst_rects(priv->src_width, priv->src_height, &src_rect, &dst_rect, &borders, NULL);

    priv->fs_movie_rect.left     = dst_rect.left;
    priv->fs_movie_rect.right    = dst_rect.right;
    priv->fs_movie_rect.top      = dst_rect.top;
    priv->fs_movie_rect.bottom   = dst_rect.bottom;
    priv->fs_panscan_rect.left   = src_rect.left;
    priv->fs_panscan_rect.right  = src_rect.right;
    priv->fs_panscan_rect.top    = src_rect.top;
    priv->fs_panscan_rect.bottom = src_rect.bottom;
    priv->border_x               = borders.left;
    priv->border_y               = borders.top;

    mp_msg(MSGT_VO, MSGL_V,
           "<vo_direct3d>Fullscreen movie rectangle: t: %ld, l: %ld, r: %ld, b:%ld\n",
           priv->fs_movie_rect.top,   priv->fs_movie_rect.left,
           priv->fs_movie_rect.right, priv->fs_movie_rect.bottom);

    /* The backbuffer should be cleared before next StretchRect. This is
     * necessary because our new draw area could be smaller than the
     * previous one used by StretchRect and without it, leftovers from the
     * previous frame will be left. */
    priv->is_clear_needed = 1;
}

/** @brief Destroy D3D Offscreen and Backbuffer surfaces.
 */
static void destroy_d3d_surfaces(void)
{
    mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>destroy_d3d_surfaces called.\n");
    /* Let's destroy the old (if any) D3D Surfaces */

    if (priv->locked_rect.pBits)
        IDirect3DSurface9_UnlockRect(priv->d3d_surface);
    priv->locked_rect.pBits = NULL;

    if (priv->d3d_surface)
        IDirect3DSurface9_Release(priv->d3d_surface);
    priv->d3d_surface = NULL;

    /* kill the OSD texture and its shadow copy */
    if (priv->d3d_texture_osd)
        IDirect3DTexture9_Release(priv->d3d_texture_osd);
    priv->d3d_texture_osd = NULL;

    if (priv->d3d_texture_system)
        IDirect3DTexture9_Release(priv->d3d_texture_system);
    priv->d3d_texture_system = NULL;

    if (priv->d3d_backbuf)
        IDirect3DSurface9_Release(priv->d3d_backbuf);
    priv->d3d_backbuf = NULL;

    if (priv->d3d_texture_eosd)
        IDirect3DSurface9_Release(priv->d3d_texture_eosd);
    priv->d3d_texture_eosd = NULL;
    priv->eosd_texture_width = priv->eosd_texture_height = 0;

    if (priv->eosd)
        eosd_packer_reinit(priv->eosd, 0, 0);
}

// Adjust the texture size *width/*height to fit the requirements of the D3D
// device. The texture size is only increased.
// xxx make clear what happens when exceeding max_texture_width/height,
//     see create_d3d_surfaces(), not sure why that does what it does
static void d3d_fix_texture_size(int *width, int *height)
{
    int tex_width = *width;
    int tex_height = *height;

    if (priv->device_caps_power2_only) {
        tex_width  = 1;
        tex_height = 1;
        while (tex_width  < *width) tex_width  <<= 1;
        while (tex_height < *height) tex_height <<= 1;
    }
    if (priv->device_caps_square_only)
        /* device only supports square textures */
        tex_width = tex_height = tex_width > tex_height ? tex_width : tex_height;
    // better round up to a multiple of 16
    // (xxx: why???)
    tex_width  = (tex_width  + 15) & ~15;
    tex_height = (tex_height + 15) & ~15;

    *width = tex_width;
    *height = tex_height;
}

/** @brief Create D3D Offscreen and Backbuffer surfaces. Each
 *         surface is created only if it's not already present.
 *  @return 1 on success, 0 on failure
 */
static int create_d3d_surfaces(void)
{
    int osd_width = vo_dwidth, osd_height = vo_dheight;
    int tex_width = osd_width, tex_height = osd_height;
    mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>create_d3d_surfaces called.\n");

    if (!priv->d3d_surface &&
        FAILED(IDirect3DDevice9_CreateOffscreenPlainSurface(
               priv->d3d_device, priv->src_width, priv->src_height,
               priv->movie_src_fmt, D3DPOOL_DEFAULT, &priv->d3d_surface, NULL))) {
        mp_msg(MSGT_VO, MSGL_ERR,
               "<vo_direct3d>Allocating offscreen surface failed.\n");
        return 0;
    }

    if (!priv->d3d_backbuf &&
        FAILED(IDirect3DDevice9_GetBackBuffer(priv->d3d_device, 0, 0,
                                              D3DBACKBUFFER_TYPE_MONO,
                                              &priv->d3d_backbuf))) {
        mp_msg(MSGT_VO, MSGL_ERR, "<vo_direct3d>Allocating backbuffer failed.\n");
        return 0;
    }

    d3d_fix_texture_size(&tex_width, &tex_height);

    // make sure we respect the size limits without breaking aspect or pow2-requirements
    while (tex_width > priv->max_texture_width || tex_height > priv->max_texture_height) {
        osd_width  >>= 1;
        osd_height >>= 1;
        tex_width  >>= 1;
        tex_height >>= 1;
    }

    priv->osd_width  = osd_width;
    priv->osd_height = osd_height;
    priv->osd_texture_width  = tex_width;
    priv->osd_texture_height = tex_height;

    mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>OSD texture size (%dx%d), requested (%dx%d).\n",
           vo_dwidth, vo_dheight, priv->osd_texture_width, priv->osd_texture_height);

    /* create OSD */
    if (!priv->d3d_texture_system &&
        FAILED(IDirect3DDevice9_CreateTexture(priv->d3d_device,
                                              priv->osd_texture_width,
                                              priv->osd_texture_height,
                                              1,
                                              D3DUSAGE_DYNAMIC,
                                              D3DFMT_A8L8,
                                              D3DPOOL_SYSTEMMEM,
                                              &priv->d3d_texture_system,
                                              NULL))) {
        mp_msg(MSGT_VO,MSGL_ERR,
               "<vo_direct3d>Allocating OSD texture in system RAM failed.\n");
        return 0;
    }

    if (!priv->device_texture_sys) {
        /* only create if we need a shadow version on the external device */
        if (!priv->d3d_texture_osd &&
            FAILED(IDirect3DDevice9_CreateTexture(priv->d3d_device,
                                                  priv->osd_texture_width,
                                                  priv->osd_texture_height,
                                                  1,
                                                  D3DUSAGE_DYNAMIC,
                                                  D3DFMT_A8L8,
                                                  D3DPOOL_DEFAULT,
                                                  &priv->d3d_texture_osd,
                                                  NULL))) {
            mp_msg(MSGT_VO,MSGL_ERR,
                   "<vo_direct3d>Allocating OSD texture in video RAM failed.\n");
            return 0;
        }
    }

    /* setup default renderstate */
    IDirect3DDevice9_SetRenderState(priv->d3d_device, D3DRS_SRCBLEND, D3DBLEND_ONE);
    IDirect3DDevice9_SetRenderState(priv->d3d_device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    IDirect3DDevice9_SetRenderState(priv->d3d_device, D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    IDirect3DDevice9_SetRenderState(priv->d3d_device, D3DRS_ALPHAREF, (DWORD)0x0);
    IDirect3DDevice9_SetRenderState(priv->d3d_device, D3DRS_LIGHTING, FALSE);
    IDirect3DDevice9_SetSamplerState(priv->d3d_device, 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    IDirect3DDevice9_SetSamplerState(priv->d3d_device, 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

    if (priv->eosd)
        eosd_packer_reinit(priv->eosd, priv->max_texture_width,
                           priv->max_texture_height);

    return 1;
}

/** @brief Fill D3D Presentation parameters
 */
static void fill_d3d_presentparams(D3DPRESENT_PARAMETERS *present_params)
{
    /* Prepare Direct3D initialization parameters. */
    memset(present_params, 0, sizeof(D3DPRESENT_PARAMETERS));
    present_params->Windowed               = TRUE;
    present_params->SwapEffect             = D3DSWAPEFFECT_COPY;
    present_params->Flags                  = D3DPRESENTFLAG_VIDEO;
    present_params->hDeviceWindow          = vo_w32_window; /* w32_common var */
    present_params->BackBufferWidth        = priv->cur_backbuf_width;
    present_params->BackBufferHeight       = priv->cur_backbuf_height;
    present_params->MultiSampleType        = D3DMULTISAMPLE_NONE;
    present_params->PresentationInterval   = D3DPRESENT_INTERVAL_ONE;
    present_params->BackBufferFormat       = priv->desktop_fmt;
    present_params->BackBufferCount        = 1;
    present_params->EnableAutoDepthStencil = FALSE;
}


/** @brief Create a new backbuffer. Create or Reset the D3D
 *         device.
 *  @return 1 on success, 0 on failure
 */
static int change_d3d_backbuffer(back_buffer_action_e action)
{
    D3DPRESENT_PARAMETERS present_params;

    destroy_d3d_surfaces();

    /* Grow the backbuffer in the required dimension. */
    if (vo_dwidth > priv->cur_backbuf_width)
        priv->cur_backbuf_width = vo_dwidth;

    if (vo_dheight > priv->cur_backbuf_height)
        priv->cur_backbuf_height = vo_dheight;

    /* The grown backbuffer dimensions are ready and fill_d3d_presentparams
     * will use them, so we can reset the device.
     */
    fill_d3d_presentparams(&present_params);

    /* vo_w32_window is w32_common variable. It's a handle to the window. */
    if (action == BACKBUFFER_CREATE &&
        FAILED(IDirect3D9_CreateDevice(priv->d3d_handle,
                                       D3DADAPTER_DEFAULT,
                                       D3DDEVTYPE_HAL, vo_w32_window,
                                       D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                       &present_params, &priv->d3d_device))) {
            mp_msg(MSGT_VO, MSGL_V,
                   "<vo_direct3d>Creating Direct3D device failed.\n");
        return 0;
    }

    if (action == BACKBUFFER_RESET &&
        FAILED(IDirect3DDevice9_Reset(priv->d3d_device, &present_params))) {
            mp_msg(MSGT_VO, MSGL_ERR,
                   "<vo_direct3d>Reseting Direct3D device failed.\n");
        return 0;
    }

    mp_msg(MSGT_VO, MSGL_V,
           "<vo_direct3d>New backbuffer (%dx%d), VO (%dx%d)\n",
           present_params.BackBufferWidth, present_params.BackBufferHeight,
           vo_dwidth, vo_dheight);

    return 1;
}

/** @brief Configure initial Direct3D context. The first
 *  function called to initialize the D3D context.
 *  @return 1 on success, 0 on failure
 */
static int configure_d3d(void)
{
    D3DDISPLAYMODE disp_mode;
    D3DVIEWPORT9 vp = {0, 0, vo_dwidth, vo_dheight, 0, 1};

    mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>configure_d3d called.\n");

    destroy_d3d_surfaces();

    /* Get the current desktop display mode, so we can set up a back buffer
     * of the same format. */
    if (FAILED(IDirect3D9_GetAdapterDisplayMode(priv->d3d_handle,
                                                D3DADAPTER_DEFAULT,
                                                &disp_mode))) {
        mp_msg(MSGT_VO, MSGL_ERR,
               "<vo_direct3d>Reading adapter display mode failed.\n");
        return 0;
    }

    /* Write current Desktop's colorspace format in the global storage. */
    priv->desktop_fmt = disp_mode.Format;

    if (!change_d3d_backbuffer(BACKBUFFER_CREATE))
        return 0;

    if (!create_d3d_surfaces())
        return 0;

    if (FAILED(IDirect3DDevice9_SetViewport(priv->d3d_device,
                                            &vp))) {
        mp_msg(MSGT_VO, MSGL_ERR, "<vo_direct3d>Setting viewport failed.\n");
        return 0;
    }

    calc_fs_rect();

    return 1;
}

/** @brief Reconfigure the whole Direct3D. Called only
 *  when the video adapter becomes uncooperative.
 *  @return 1 on success, 0 on failure
 */
static int reconfigure_d3d(void)
{
    mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>reconfigure_d3d called.\n");

    /* Destroy the offscreen, OSD and backbuffer surfaces */
    destroy_d3d_surfaces();

    /* Destroy the D3D Device */
    if (priv->d3d_device)
        IDirect3DDevice9_Release(priv->d3d_device);
    priv->d3d_device = NULL;

    /* Stop the whole Direct3D */
    IDirect3D9_Release(priv->d3d_handle);

    /* Initialize Direct3D from the beginning */
    priv->d3d_handle = priv->pDirect3DCreate9(D3D_SDK_VERSION);
    if (!priv->d3d_handle) {
        mp_msg(MSGT_VO, MSGL_ERR, "<vo_direct3d>Initializing Direct3D failed.\n");
        return 0;
    }

    /* Configure Direct3D */
    if (!configure_d3d())
        return 0;

    return 1;
}

/** @brief Resize Direct3D context on window resize.
 *  @return 1 on success, 0 on failure
 */
static int resize_d3d(void)
{
    D3DVIEWPORT9 vp = {0, 0, vo_dwidth, vo_dheight, 0, 1};

    mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>resize_d3d called.\n");

    /* Make sure that backbuffer is large enough to accomodate the new
       viewport dimensions. Grow it if necessary. */

    if (vo_dwidth > priv->cur_backbuf_width ||
        vo_dheight > priv->cur_backbuf_height) {
        if (!change_d3d_backbuffer(BACKBUFFER_RESET))
            return 0;
    }

    /* Destroy the OSD textures. They should always match the new dimensions
     * of the onscreen window, so on each resize we need new OSD dimensions.
     */

    if (priv->d3d_texture_osd)
        IDirect3DTexture9_Release(priv->d3d_texture_osd);
    priv->d3d_texture_osd = NULL;

    if (priv->d3d_texture_system)
        IDirect3DTexture9_Release(priv->d3d_texture_system);
    priv->d3d_texture_system = NULL;


    /* Recreate the OSD. The function will observe that the offscreen plain
     * surface and the backbuffer are not destroyed and will skip their creation,
     * effectively recreating only the OSD.
     */

    if (!create_d3d_surfaces())
        return 0;

    if (FAILED(IDirect3DDevice9_SetViewport(priv->d3d_device,
                                            &vp))) {
        mp_msg(MSGT_VO, MSGL_ERR, "<vo_direct3d>Setting viewport failed.\n");
        return 0;
    }

    calc_fs_rect();

#ifdef CONFIG_FREETYPE
    // font needs to be adjusted
    force_load_font = 1;
#endif
    // OSD needs to be drawn fresh for new size
    vo_osd_changed(OSDTYPE_OSD);

    return 1;
}

/** @brief Uninitialize Direct3D and close the window.
 */
static void uninit_d3d(void)
{
    mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>uninit_d3d called.\n");

    destroy_d3d_surfaces();

    /* Destroy the D3D Device */
    if (priv->d3d_device)
        IDirect3DDevice9_Release(priv->d3d_device);
    priv->d3d_device = NULL;

    /* Stop the whole D3D. */
    if (priv->d3d_handle) {
        mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>Stopping Direct3D.\n");
        IDirect3D9_Release(priv->d3d_handle);
    }
    priv->d3d_handle = NULL;
}

static uint32_t d3d_draw_frame(void);

/** @brief Render a frame on the screen.
 *  @param mpi mpi structure with the decoded frame inside
 *  @return VO_TRUE on success, VO_ERROR on failure
 */
static uint32_t d3d_upload_and_render_frame(mp_image_t *mpi)
{
    /* Uncomment when direct rendering is implemented.
     * if (mpi->flags & MP_IMGFLAG_DIRECT) ...
     */

    /* If the D3D device is uncooperative (not initialized), return success.
       The device will be probed for reinitialization in the next flip_page() */
    if (!priv->d3d_device)
        return VO_TRUE;

    if (mpi->flags & MP_IMGFLAG_DRAW_CALLBACK)
        goto skip_upload;

    if (mpi->flags & MP_IMGFLAG_PLANAR) { /* Copy a planar frame. */
        draw_slice(mpi->planes, mpi->stride, mpi->w, mpi->h, 0, 0);
        goto skip_upload;
    }

    /* If we're here, then we should lock the rect and copy a packed frame */
    if (!priv->locked_rect.pBits) {
        if (FAILED(IDirect3DSurface9_LockRect(priv->d3d_surface,
                                              &priv->locked_rect, NULL, 0))) {
            mp_msg(MSGT_VO, MSGL_ERR, "<vo_direct3d>Surface lock failed.\n");
            return VO_ERROR;
        }
    }

    memcpy_pic(priv->locked_rect.pBits, mpi->planes[0], mpi->stride[0],
               mpi->height, priv->locked_rect.Pitch, mpi->stride[0]);

skip_upload:
    /* This unlock is used for both slice_draw path and DRAW_IMAGE path. */
    if (FAILED(IDirect3DSurface9_UnlockRect(priv->d3d_surface))) {
        mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>Surface unlock failed.\n");
        return VO_ERROR;
    }
    priv->locked_rect.pBits = NULL;

    return d3d_draw_frame();
}

static uint32_t d3d_draw_frame(void)
{
    if (FAILED(IDirect3DDevice9_BeginScene(priv->d3d_device))) {
        mp_msg(MSGT_VO, MSGL_ERR, "<vo_direct3d>BeginScene failed.\n");
        return VO_ERROR;
    }

    if (priv->is_clear_needed) {
        IDirect3DDevice9_Clear(priv->d3d_device, 0, NULL,
                               D3DCLEAR_TARGET, 0, 0, 0);
        priv->is_clear_needed = 0;
    }

    if (FAILED(IDirect3DDevice9_StretchRect(priv->d3d_device,
                                            priv->d3d_surface,
                                            &priv->fs_panscan_rect,
                                            priv->d3d_backbuf,
                                            &priv->fs_movie_rect,
                                            D3DTEXF_LINEAR))) {
        mp_msg(MSGT_VO, MSGL_ERR,
               "<vo_direct3d>Copying frame to the backbuffer failed.\n");
        return VO_ERROR;
    }

    if (FAILED(IDirect3DDevice9_EndScene(priv->d3d_device))) {
        mp_msg(MSGT_VO, MSGL_ERR, "<vo_direct3d>EndScene failed.\n");
        return VO_ERROR;
    }

    return VO_TRUE;
}

static const struct_fmt_table *check_format(uint32_t movie_fmt)
{
    int i;
    for (i = 0; i < DISPLAY_FORMAT_TABLE_ENTRIES; i++) {
        if (fmt_table[i].mplayer_fmt == movie_fmt) {
            /* Test conversion from Movie colorspace to
             * display's target colorspace. */
            if (FAILED(IDirect3D9_CheckDeviceFormatConversion(priv->d3d_handle,
                                                              D3DADAPTER_DEFAULT,
                                                              D3DDEVTYPE_HAL,
                                                              fmt_table[i].fourcc,
                                                              priv->desktop_fmt))) {
                mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>Rejected image format: %s\n",
                       vo_format_name(fmt_table[i].mplayer_fmt));
                return NULL;
            }

            mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>Accepted image format: %s\n",
                   vo_format_name(fmt_table[i].mplayer_fmt));

            return &fmt_table[i];
        }
    }

    return 0;
}


/** @brief Query if movie colorspace is supported by the HW.
 *  @return 0 on failure, device capabilities (not probed
 *          currently) on success.
 */
static int query_format(uint32_t movie_fmt)
{
    if (!check_format(movie_fmt))
        return 0;

    int eosd_caps = VFCAP_CSP_SUPPORTED | VFCAP_CSP_SUPPORTED_BY_HW
                    | VFCAP_OSD | VFCAP_HWSCALE_UP | VFCAP_HWSCALE_DOWN;
    if (priv->eosd)
        eosd_caps |= VFCAP_EOSD | VFCAP_EOSD_UNSCALED;
    return eosd_caps;
}

/****************************************************************************
 *                                                                          *
 *                                                                          *
 *                                                                          *
 * libvo Control / Callback functions                                       *
 *                                                                          *
 *                                                                          *
 *                                                                          *
 ****************************************************************************/




/** @brief libvo Callback: Preinitialize the video card.
 *  Preinit the hardware just enough to be queried about
 *  supported formats.
 *
 *  @return 0 on success, -1 on failure
 */

static int preinit(const char *arg)
{
    D3DDISPLAYMODE disp_mode;
    D3DCAPS9 disp_caps;
    DWORD texture_caps;
    DWORD dev_caps;

    /* Set to zero all global variables. */
    priv = talloc_zero(NULL, struct global_priv);

    //xxx make it possible to disable eosd by command line
    bool enable_eosd = true;

    if (enable_eosd)
        priv->eosd = eosd_packer_create(priv);

    priv->d3d9_dll = LoadLibraryA("d3d9.dll");
    if (!priv->d3d9_dll) {
        mp_msg(MSGT_VO, MSGL_ERR, "<vo_direct3d>Unable to dynamically load d3d9.dll\n");
        goto err_out;
    }

    priv->pDirect3DCreate9 = (void *)GetProcAddress(priv->d3d9_dll, "Direct3DCreate9");
    if (!priv->pDirect3DCreate9) {
        mp_msg(MSGT_VO, MSGL_ERR, "<vo_direct3d>Unable to find entry point of Direct3DCreate9\n");
        goto err_out;
    }

    priv->d3d_handle = priv->pDirect3DCreate9(D3D_SDK_VERSION);
    if (!priv->d3d_handle) {
        mp_msg(MSGT_VO, MSGL_ERR, "<vo_direct3d>Initializing Direct3D failed.\n");
        goto err_out;
    }

    if (FAILED(IDirect3D9_GetAdapterDisplayMode(priv->d3d_handle,
                                                D3DADAPTER_DEFAULT,
                                                &disp_mode))) {
        mp_msg(MSGT_VO, MSGL_ERR, "<vo_direct3d>Reading display mode failed.\n");
        goto err_out;
    }

    /* Store in priv->desktop_fmt the user desktop's colorspace. Usually XRGB. */
    priv->desktop_fmt = disp_mode.Format;
    priv->cur_backbuf_width = disp_mode.Width;
    priv->cur_backbuf_height = disp_mode.Height;

    mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>Setting backbuffer dimensions to (%dx%d).\n",
           disp_mode.Width, disp_mode.Height);

    if (FAILED(IDirect3D9_GetDeviceCaps(priv->d3d_handle,
                                        D3DADAPTER_DEFAULT,
                                        D3DDEVTYPE_HAL,
                                        &disp_caps))) {
        mp_msg(MSGT_VO, MSGL_ERR, "<vo_direct3d>Reading display capabilities failed.\n");
        goto err_out;
    }

    /* Store relevant information reguarding caps of device */
    texture_caps                  = disp_caps.TextureCaps;
    dev_caps                      = disp_caps.DevCaps;
    priv->device_caps_power2_only =  (texture_caps & D3DPTEXTURECAPS_POW2) &&
                                    !(texture_caps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL);
    priv->device_caps_square_only = texture_caps & D3DPTEXTURECAPS_SQUAREONLY;
    priv->device_texture_sys      = dev_caps & D3DDEVCAPS_TEXTURESYSTEMMEMORY;
    priv->max_texture_width       = disp_caps.MaxTextureWidth;
    priv->max_texture_height      = disp_caps.MaxTextureHeight;

    mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>device_caps_power2_only %d, device_caps_square_only %d\n"
                            "<vo_direct3d>device_texture_sys %d\n"
                            "<vo_direct3d>max_texture_width %d, max_texture_height %d\n",
           priv->device_caps_power2_only, priv->device_caps_square_only,
           priv->device_texture_sys, priv->max_texture_width,
           priv->max_texture_height);

    /* w32_common framework call. Configures window on the screen, gets
     * fullscreen dimensions and does other useful stuff.
     */
    if (!vo_w32_init()) {
        mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>Configuring onscreen window failed.\n");
        goto err_out;
    }

    return 0;

err_out:
    uninit();
    return -1;
}

static void full_redraw(void)
{
    priv->is_clear_needed = 1;
    d3d_draw_frame();
    draw_osd();
    draw_eosd();
    flip_page();
}

/** @brief libvo Callback: Handle control requests.
 *  @return VO_TRUE on success, VO_NOTIMPL when not implemented
 */
static int control(uint32_t request, void *data)
{
    switch (request) {
    case VOCTRL_QUERY_FORMAT:
        return query_format(*(uint32_t*) data);
    case VOCTRL_GET_IMAGE: /* Direct Rendering. Not implemented yet. */
        mp_msg(MSGT_VO, MSGL_V,
               "<vo_direct3d>Direct Rendering request. Not implemented yet.\n");
        return VO_NOTIMPL;
    case VOCTRL_DRAW_IMAGE:
        return d3d_upload_and_render_frame(data);
    case VOCTRL_FULLSCREEN:
        vo_w32_fullscreen();
        resize_d3d();
        full_redraw();
        return VO_TRUE;
    case VOCTRL_RESET:
        return VO_NOTIMPL;
    case VOCTRL_PAUSE:
        priv->is_paused = 1;
        return VO_TRUE;
    case VOCTRL_RESUME:
        priv->is_paused = 0;
        return VO_TRUE;
    case VOCTRL_REDRAW_FRAME:
        priv->is_clear_needed = 1;
        d3d_draw_frame();
        return VO_TRUE;
    case VOCTRL_SET_EQUALIZER:
        return VO_NOTIMPL;
    case VOCTRL_GET_EQUALIZER:
        return VO_NOTIMPL;
    case VOCTRL_ONTOP:
        vo_w32_ontop();
        return VO_TRUE;
    case VOCTRL_BORDER:
        vo_w32_border();
        resize_d3d();
        return VO_TRUE;
    case VOCTRL_UPDATE_SCREENINFO:
        w32_update_xinerama_info();
        return VO_TRUE;
    case VOCTRL_SET_PANSCAN:
        calc_fs_rect();
        return VO_TRUE;
    case VOCTRL_GET_PANSCAN:
        return VO_TRUE;
    case VOCTRL_DRAW_EOSD:
        if (!data)
            return VO_FALSE;
        assert(priv->eosd);
        generate_eosd(data);
        draw_eosd();
        return VO_TRUE;
    case VOCTRL_GET_EOSD_RES: {
        assert(priv->eosd);
        struct mp_eosd_res *r = data;
        r->w = vo_dwidth;
        r->h = vo_dheight;
        r->ml = r->mr = priv->border_x;
        r->mt = r->mb = priv->border_y;
        return VO_TRUE;
    }
    }
    return VO_FALSE;
}

/** @brief libvo Callback: Configre the Direct3D adapter.
 *  @param width    Movie source width
 *  @param height   Movie source height
 *  @param d_width  Screen (destination) width
 *  @param d_height Screen (destination) height
 *  @param options  Options bitmap
 *  @param title    Window title
 *  @param format   Movie colorspace format (using MPlayer's
 *                  defines, e.g. IMGFMT_YUY2)
 *  @return 0 on success, VO_ERROR on failure
 */
static int config(uint32_t width, uint32_t height, uint32_t d_width,
                  uint32_t d_height, uint32_t options, char *title,
                  uint32_t format)
{

    priv->src_width  = width;
    priv->src_height = height;

    const struct_fmt_table *fmt_entry = check_format(format);
    if (!fmt_entry)
        return VO_ERROR;

    priv->movie_src_fmt = fmt_entry->fourcc;

    /* w32_common framework call. Creates window on the screen with
     * the given coordinates.
     */
    if (!vo_w32_config(d_width, d_height, options)) {
        mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>Creating onscreen window failed.\n");
        return VO_ERROR;
    }

    /* "config" may be called several times, so if this is not the first
     * call, we should destroy Direct3D adapter and surfaces before
     * calling configure_d3d, which will create them again.
     */
    destroy_d3d_surfaces();

    /* Destroy the D3D Device */
    if (priv->d3d_device)
        IDirect3DDevice9_Release(priv->d3d_device);
    priv->d3d_device = NULL;

    if (!configure_d3d())
        return VO_ERROR;

    return 0; /* Success */
}

/** @brief libvo Callback: Flip next already drawn frame on the
 *         screen.
 */
static void flip_page(void)
{
    RECT rect = {0, 0, vo_dwidth, vo_dheight};
    if (!priv->d3d_device ||
        FAILED(IDirect3DDevice9_Present(priv->d3d_device, &rect, 0, 0, 0))) {
        mp_msg(MSGT_VO, MSGL_V,
               "<vo_direct3d>Trying to reinitialize uncooperative video adapter.\n");
        if (!reconfigure_d3d()) {
            mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>Reinitialization failed.\n");
            return;
        }
        else
            mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>Video adapter reinitialized.\n");
    }
}

/** @brief libvo Callback: Uninitializes all pointers and closes
 *         all D3D related stuff,
 */
static void uninit(void)
{
    mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>uninit called.\n");

    uninit_d3d();
    vo_w32_uninit(); /* w32_common framework call */
    if (priv->d3d9_dll)
        FreeLibrary(priv->d3d9_dll);
    priv->d3d9_dll = NULL;
    talloc_free(priv);
    priv = NULL;
}

/** @brief libvo Callback: Handles video window events.
 */
static void check_events(void)
{
    int flags;
    /* w32_common framework call. Handles video window events.
     * Updates global libvo's vo_dwidth/vo_dheight upon resize
     * with the new window width/height.
     */
    flags = vo_w32_check_events();
    if (flags & VO_EVENT_RESIZE)
        resize_d3d();

    if ((flags & VO_EVENT_EXPOSE) && priv->is_paused)
        flip_page();
}

/** @brief libvo Callback: Draw slice
 *  @return 0 on success
 */
static int draw_slice(uint8_t *src[], int stride[], int w,int h,int x,int y )
{
    char *my_src;   /**< Pointer to the source image */
    char *dst;      /**< Pointer to the destination image */
    int  uv_stride; /**< Stride of the U/V planes */

    /* If the D3D device is uncooperative (not initialized), return success.
       The device will be probed for reinitialization in the next flip_page() */
    if (!priv->d3d_device)
        return 0;

    /* Lock the offscreen surface if it's not already locked. */
    if (!priv->locked_rect.pBits) {
        if (FAILED(IDirect3DSurface9_LockRect(priv->d3d_surface,
                                              &priv->locked_rect, NULL, 0))) {
            mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>Surface lock failure.\n");
            return VO_FALSE;
        }
    }

    uv_stride = priv->locked_rect.Pitch / 2;

    /* Copy Y */
    dst = priv->locked_rect.pBits;
    dst = dst + priv->locked_rect.Pitch * y + x;
    my_src = src[0];
    memcpy_pic(dst, my_src, w, h, priv->locked_rect.Pitch, stride[0]);

    w /= 2;
    h /= 2;
    x /= 2;
    y /= 2;

    /* Copy U */
    dst = priv->locked_rect.pBits;
    dst = dst + priv->locked_rect.Pitch * priv->src_height
          + uv_stride * y + x;
    if (priv->movie_src_fmt == MAKEFOURCC('Y','V','1','2'))
        my_src = src[2];
    else
        my_src = src[1];

    memcpy_pic(dst, my_src, w, h, uv_stride, stride[1]);

    /* Copy V */
    dst = priv->locked_rect.pBits;
    dst = dst + priv->locked_rect.Pitch * priv->src_height
          + uv_stride * (priv->src_height / 2) + uv_stride * y + x;
    if (priv->movie_src_fmt == MAKEFOURCC('Y','V','1','2'))
        my_src=src[1];
    else
        my_src=src[2];

    memcpy_pic(dst, my_src, w, h, uv_stride, stride[2]);

    return 0; /* Success */
}

/** @brief libvo Callback: Unused function
 */
static int draw_frame(uint8_t *src[])
{
    mp_msg(MSGT_VO, MSGL_V, "<vo_direct3d>draw_frame called.\n");
    return VO_FALSE;
}

/** @brief Maps MPlayer alpha to D3D
 *         0x0 -> transparent and discarded by alpha test
 *         0x1 -> 0xFF to become opaque
 *         other alpha values are inverted +1 (2 = -2)
 *         These values are then inverted again with
           the texture filter D3DBLEND_INVSRCALPHA
 */
static void vo_draw_alpha_l8a8(int w, int h, unsigned char* src,
                               unsigned char *srca, int srcstride,
                               unsigned char* dstbase, int dststride)
{
    int y;
    for (y = 0; y < h; y++) {
        unsigned short *dst = (unsigned short*)dstbase;
        int x;
        for (x = 0; x < w; x++) {
            dst[x] = (-srca[x] << 8) | src[x];
        }
        src     += srcstride;
        srca    += srcstride;
        dstbase += dststride;
    }
}

/** @brief Callback function to render the OSD to the texture
 */
static void draw_alpha(int x0, int y0, int w, int h, unsigned char *src,
                       unsigned char *srca, int stride)
{
    D3DLOCKED_RECT  locked_rect;   /**< Offscreen surface we lock in order
                                   to copy MPlayer's frame inside it.*/

    if (FAILED(IDirect3DTexture9_LockRect(priv->d3d_texture_system, 0,
                                          &locked_rect, NULL, 0))) {
        mp_msg(MSGT_VO,MSGL_ERR,"<vo_direct3d>OSD texture lock failed.\n");
        return;
    }

    vo_draw_alpha_l8a8(w, h, src, srca, stride,
        (unsigned char *)locked_rect.pBits + locked_rect.Pitch*y0 + 2*x0, locked_rect.Pitch);

    /* this unlock is used for both slice_draw path and D3DRenderFrame path */
    if (FAILED(IDirect3DTexture9_UnlockRect(priv->d3d_texture_system, 0))) {
        mp_msg(MSGT_VO,MSGL_ERR,"<vo_direct3d>OSD texture unlock failed.\n");
        return;
    }

    priv->is_osd_populated = 1;
}

/** @brief libvo Callback: Draw OSD/Subtitles,
 */
static void draw_osd(void)
{
    // we can not render OSD if we lost the device e.g. because it was uncooperative
    if (!priv->d3d_device)
        return;

    if (vo_osd_changed(0)) {
        D3DLOCKED_RECT  locked_rect;   /**< Offscreen surface we lock in order
                                         to copy MPlayer's frame inside it.*/

        /* clear the OSD */
        if (FAILED(IDirect3DTexture9_LockRect(priv->d3d_texture_system, 0,
                                              &locked_rect, NULL, 0))) {
            mp_msg(MSGT_VO,MSGL_ERR, "<vo_direct3d>OSD texture lock failed.\n");
            return;
        }

        /* clear the whole texture to avoid issues due to interpolation */
        memset(locked_rect.pBits, 0, locked_rect.Pitch * priv->osd_texture_height);

        /* this unlock is used for both slice_draw path and D3DRenderFrame path */
        if (FAILED(IDirect3DTexture9_UnlockRect(priv->d3d_texture_system, 0))) {
            mp_msg(MSGT_VO,MSGL_ERR, "<vo_direct3d>OSD texture unlock failed.\n");
            return;
        }

        priv->is_osd_populated = 0;
        /* required for if subs are in the boarder region */
        priv->is_clear_needed = 1;

        vo_draw_text_ext(priv->osd_width, priv->osd_height, priv->border_x, priv->border_y,
                         priv->border_x, priv->border_y, priv->src_width, priv->src_height, draw_alpha);

        if (!priv->device_texture_sys)
        {
            /* only DMA to the shadow if its required */
            if (FAILED(IDirect3DDevice9_UpdateTexture(priv->d3d_device,
                                                      (IDirect3DBaseTexture9 *)priv->d3d_texture_system,
                                                      (IDirect3DBaseTexture9 *)priv->d3d_texture_osd))) {
                mp_msg(MSGT_VO,MSGL_ERR, "<vo_direct3d>OSD texture transfer failed.\n");
                return;
            }
        }
    }

    /* update OSD */

    if (priv->is_osd_populated) {

        vertex_osd osd_quad_vb[] = {
            {-1.0f, 1.0f, 0.0f,  0, 0 },
            { 1.0f, 1.0f, 0.0f,  1, 0 },
            {-1.0f,-1.0f, 0.0f,  0, 1 },
            { 1.0f,-1.0f, 0.0f,  1, 1 }
        };

        /* calculate the texture coordinates */
        osd_quad_vb[1].tu =
            osd_quad_vb[3].tu = (float)priv->osd_width  / priv->osd_texture_width;
        osd_quad_vb[2].tv =
            osd_quad_vb[3].tv = (float)priv->osd_height / priv->osd_texture_height;

        if (FAILED(IDirect3DDevice9_BeginScene(priv->d3d_device))) {
            mp_msg(MSGT_VO,MSGL_ERR,"<vo_direct3d>BeginScene failed.\n");
            return;
        }

        /* turn on alpha test */
        IDirect3DDevice9_SetRenderState(priv->d3d_device, D3DRS_ALPHABLENDENABLE, TRUE);
        IDirect3DDevice9_SetRenderState(priv->d3d_device, D3DRS_ALPHATESTENABLE, TRUE);

        /* need to use a texture here (done here as we may be able to texture from system memory) */
        IDirect3DDevice9_SetTexture(priv->d3d_device, 0,
            (IDirect3DBaseTexture9 *)(priv->device_texture_sys
            ? priv->d3d_texture_system : priv->d3d_texture_osd));

        IDirect3DDevice9_SetFVF(priv->d3d_device, D3DFVF_OSD_VERTEX);
        IDirect3DDevice9_DrawPrimitiveUP(priv->d3d_device, D3DPT_TRIANGLESTRIP, 2, osd_quad_vb, sizeof(vertex_osd));

        /* turn off alpha test */
        IDirect3DDevice9_SetRenderState(priv->d3d_device, D3DRS_ALPHATESTENABLE, FALSE);
        IDirect3DDevice9_SetRenderState(priv->d3d_device, D3DRS_ALPHABLENDENABLE, FALSE);

        if (FAILED(IDirect3DDevice9_EndScene(priv->d3d_device))) {
            mp_msg(MSGT_VO,MSGL_ERR,"<vo_direct3d>EndScene failed.\n");
            return;
        }
    }
}

static void d3d_realloc_eosd_texture(void)
{
    int new_w = priv->eosd->surface.w;
    int new_h = priv->eosd->surface.h;

    d3d_fix_texture_size(&new_w, &new_h);

    if (new_w == priv->eosd_texture_width && new_h == priv->eosd_texture_height)
        return;

    // fortunately, we don't need to keep the old image data
    // we can always free it
    if (priv->d3d_texture_eosd)
        IDirect3DTexture9_Release(priv->d3d_texture_eosd);
    priv->d3d_texture_eosd = NULL;

    priv->eosd_texture_width = new_w;
    priv->eosd_texture_height = new_h;

    mp_msg(MSGT_VO, MSGL_DBG2, "<vo_direct3d>reallocate EOSD surface.\n");

    if (FAILED(IDirect3DDevice9_CreateTexture(priv->d3d_device,
                                              priv->eosd_texture_width,
                                              priv->eosd_texture_height,
                                              1,
                                              D3DUSAGE_DYNAMIC,
#if USE_A8
                                              D3DFMT_A8,
#else
                                              D3DFMT_A8L8,
#endif
                                              D3DPOOL_DEFAULT,
                                              &priv->d3d_texture_eosd,
                                              NULL))) {
        mp_msg(MSGT_VO,MSGL_ERR,
               "<vo_direct3d>Allocating EOSD texture failed.\n");
        priv->eosd_texture_width = 0;
        priv->eosd_texture_height = 0;
        return;
    }
}

static D3DCOLOR ass_to_d3d_color(uint32_t color)
{
    uint32_t r = (color >> 24) & 0xff;
    uint32_t g = (color >> 16) & 0xff;
    uint32_t b = (color >> 8) & 0xff;
    uint32_t a = 0xff - (color & 0xff);
    return D3DCOLOR_ARGB(a, r, g, b);
}

static void generate_eosd(mp_eosd_images_t *imgs)
{
    bool need_upload, need_resize;
    eosd_packer_generate(priv->eosd, imgs, &need_upload, &need_resize);
    if (!need_upload)
        return;
    // even if the texture size is unchanged, the texture might have been free'd
    d3d_realloc_eosd_texture();
    if (!priv->d3d_texture_eosd)
        return; // failed to allocate

    // reupload all EOSD images

    // we need 2 primitives per quad which makes 6 vertices (we could reduce the
    // number of vertices by using an indexed vertex array, but it's probably
    // not worth doing)
    priv->eosd_vb = talloc_realloc_size(priv->eosd, priv->eosd_vb,
                                        priv->eosd->targets_count
                                            * sizeof(vertex_eosd) * 6);

    struct eosd_rect rc;
    eosd_packer_calculate_source_bb(priv->eosd, &rc);
    RECT dirty_rc = { rc.x0, rc.y0, rc.x1, rc.y1 };

    D3DLOCKED_RECT  locked_rect;

    if (FAILED(IDirect3DTexture9_LockRect(priv->d3d_texture_eosd, 0,
                                          &locked_rect, &dirty_rc, 0)))
    {
        mp_msg(MSGT_VO,MSGL_ERR, "<vo_direct3d>EOSD texture lock failed.\n");
        return;
    }

    //memset(locked_rect.pBits, 0, locked_rect.Pitch * priv->eosd_texture_height);

    float eosd_w = priv->eosd_texture_width;
    float eosd_h = priv->eosd_texture_height;

    for (int i = 0; i < priv->eosd->targets_count; i++) {
        struct eosd_target *target = &priv->eosd->targets[i];
        ASS_Image *img = target->ass_img;
        char *src = img->bitmap;
#if USE_A8
        char *dst = (char*)locked_rect.pBits + target->source.x0
                    + locked_rect.Pitch * target->source.y0;
#else
        char *dst = (char*)locked_rect.pBits + target->source.x0*2
                    + locked_rect.Pitch * target->source.y0;
#endif
        for (int y = 0; y < img->h; y++) {
#if USE_A8
            memcpy(dst, src, img->w);
#else
            for (int x = 0; x < img->w; x++) {
                dst[x*2+0] = 255;
                dst[x*2+1] = src[x];
            }
#endif
            src += img->stride;
            dst += locked_rect.Pitch;
        }

        D3DCOLOR color = ass_to_d3d_color(img->color);

        float x0 = target->dest.x0;
        float y0 = target->dest.y0;
        float x1 = target->dest.x1;
        float y1 = target->dest.y1;
        float tx0 = target->source.x0 / eosd_w;
        float ty0 = target->source.y0 / eosd_h;
        float tx1 = target->source.x1 / eosd_w;
        float ty1 = target->source.y1 / eosd_h;

        vertex_eosd *v = &priv->eosd_vb[i*6];
        v[0] = (vertex_eosd) { x0, y0, 0, color, tx0, ty0 };
        v[1] = (vertex_eosd) { x1, y0, 0, color, tx1, ty0 };
        v[2] = (vertex_eosd) { x0, y1, 0, color, tx0, ty1 };
        v[3] = (vertex_eosd) { x1, y1, 0, color, tx1, ty1 };
        v[4] = v[2];
        v[5] = v[1];
    }

    if (FAILED(IDirect3DTexture9_UnlockRect(priv->d3d_texture_eosd, 0))) {
        mp_msg(MSGT_VO,MSGL_ERR, "<vo_direct3d>EOSD texture unlock failed.\n");
        return;
    }
}

// unfortunately we can't use the D3DX library

static void d3d_matrix_identity(D3DMATRIX *m)
{
    memset(m, 0, sizeof(D3DMATRIX));
    m->_11 = m->_22 = m->_33 = m->_44 = 1.0f;
}

static void d3d_matrix_ortho(D3DMATRIX *m, float left, float right,
                             float bottom, float top)
{
    d3d_matrix_identity(m);
    m->_11 = 2.0f / (right - left);
    m->_22 = 2.0f / (top - bottom);
    m->_33 = 1.0f;
    m->_41 = -(right + left) / (right - left);
    m->_42 = -(top + bottom) / (top - bottom);
    m->_43 = 0;
    m->_44 = 1.0f;
}

static void draw_eosd(void)
{
    // we can not render OSD if we lost the device e.g. because it was uncooperative
    if (!priv->d3d_device)
        return;

    if (!priv->eosd->targets_count)
        return;

    //xxx need to set up a transform for EOSD rendering when drawing it
    if (FAILED(IDirect3DDevice9_BeginScene(priv->d3d_device))) {
        mp_msg(MSGT_VO,MSGL_ERR,"<vo_direct3d>BeginScene failed (EOSD).\n");
        return;
    }

    IDirect3DDevice9_SetRenderState(priv->d3d_device, D3DRS_ALPHABLENDENABLE, TRUE);
    //IDirect3DDevice9_SetRenderState(priv->d3d_device, D3DRS_ALPHATESTENABLE, TRUE);

    D3DMATRIX m;
    // so that screen coordinates map to D3D ones
    D3DVIEWPORT9 p;
    IDirect3DDevice9_GetViewport(priv->d3d_device, &p);
    d3d_matrix_ortho(&m, 0.5f, p.Width + 0.5f, p.Height + 0.5f, 0.5f);
    IDirect3DDevice9_SetTransform(priv->d3d_device, D3DTS_VIEW, &m);

    IDirect3DDevice9_SetTexture(priv->d3d_device, 0,
                                (IDirect3DBaseTexture9*)priv->d3d_texture_eosd);

    IDirect3DDevice9_SetRenderState(priv->d3d_device,
                                    D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    IDirect3DDevice9_SetRenderState(priv->d3d_device,
                                    D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

#if USE_A8
    // do not use the color value from the A8 texture, because that is black
    // we need either white, or no blending with the texture color at all
    // use the value in D3DTSS_CONSTANT instead (0xffffffff=white by default)
    // xxx wine doesn't like this (fails to compile the generated GL shader)
    //     and D3DTA_SPECULAR leaves the images black
    IDirect3DDevice9_SetTextureStageState(priv->d3d_device, 0,
                                          D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    IDirect3DDevice9_SetTextureStageState(priv->d3d_device,0,
                                          D3DTSS_COLORARG1, D3DTA_CONSTANT);
#endif

    IDirect3DDevice9_SetTextureStageState(priv->d3d_device, 0,
                                          D3DTSS_ALPHAOP, D3DTOP_MODULATE);

    IDirect3DDevice9_SetFVF(priv->d3d_device, D3DFVF_EOSD_VERTEX);
    IDirect3DDevice9_DrawPrimitiveUP(priv->d3d_device, D3DPT_TRIANGLELIST,
                                     priv->eosd->targets_count * 2,
                                     priv->eosd_vb, sizeof(vertex_eosd));

    d3d_matrix_identity(&m);
    IDirect3DDevice9_SetTransform(priv->d3d_device, D3DTS_VIEW, &m);

    IDirect3DDevice9_SetTextureStageState(priv->d3d_device, 0,
                                          D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);

    IDirect3DDevice9_SetRenderState(priv->d3d_device, D3DRS_ALPHABLENDENABLE, FALSE);
    //IDirect3DDevice9_SetRenderState(priv->d3d_device, D3DRS_ALPHATESTENABLE, FALSE);

    if (FAILED(IDirect3DDevice9_EndScene(priv->d3d_device))) {
        mp_msg(MSGT_VO,MSGL_ERR,"<vo_direct3d>EndScene failed (EOSD).\n");
        return;
    }

}
