#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include <libavutil/common.h>

#include "libvo/eosd_render.h"

static void eosd_draw_alpha_rgba(unsigned char *src,
                                 int src_w, int src_h, int src_stride,
                                 unsigned char *dst,
                                 size_t dst_stride,
                                 int dst_x, int dst_y,
                                 uint32_t color)
{
    const unsigned int r = (color >> 24) & 0xff;
    const unsigned int g = (color >> 16) & 0xff;
    const unsigned int b = (color >>  8) & 0xff;
    const unsigned int a = 0xff - (color & 0xff);

    dst += dst_y * dst_stride + dst_x * 4;

    for (int y = 0; y < src_h; y++, dst += dst_stride, src += src_stride) {
        for (int x = 0; x < src_w; x++) {
            const unsigned int v = src[x];
            dst[4*x + 0] = (r * v + dst[4*x + 0] * (0xff - v)) / 255;
            dst[4*x + 1] = (g * v + dst[4*x + 1] * (0xff - v)) / 255;
            dst[4*x + 2] = (b * v + dst[4*x + 2] * (0xff - v)) / 255;
            dst[4*x + 3] = (a * v + dst[4*x + 3] * (0xff - v)) / 255;
        }
    }
}

void rgba_clear(unsigned char *image, size_t stride, int x, int y, int w, int h)
{
    image += x * 4 + y * stride;
    while (h--) {
        memset(image, 0, w * 4);
        image += stride;
    }
}

bool eosd_bounding_box(mp_eosd_images_t *imgs, int *x, int *y, int *w, int *h)
{
    int x0 = INT_MAX, y0 = INT_MAX;
    int x1 = 0, y1 = 0;

    ASS_Image *img = imgs->imgs;
    for (ASS_Image *p = img; p; p = p->next) {
        x0 = FFMIN(x0, p->dst_x);
        y0 = FFMIN(y0, p->dst_y);
        x1 = FFMAX(x1, p->dst_x + p->w);
        y1 = FFMAX(y1, p->dst_y + p->h);
    }

    // avoid degenerate bounding box if empty
    *x = FFMIN(x0, x1);
    *y = FFMIN(y0, y1);
    *w = FFMAX(x1 - x0, 0);
    *h = FFMAX(y1 - y0, 0);

    return x1 > x0 && y1 > y0;
}

void eosd_render_rgba(unsigned char *image, size_t stride, int w, int h,
                      mp_eosd_images_t *imgs)
{
    ASS_Image *img = imgs->imgs;
    for (ASS_Image *p = img; p; p = p->next) {
        if (p->w <= 0 || p->h <= 0)
            continue;
        eosd_draw_alpha_rgba(p->bitmap, p->w, p->h, p->stride,
                             image, stride,
                             p->dst_x, p->dst_y, p->color);
    }
}
