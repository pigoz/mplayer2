#ifndef MPLAYER_EOSD_RENDER
#define MPLAYER_EOSD_RENDER

#include <stddef.h>
#include <stdbool.h>

#include "sub/ass_mp.h"

void rgba_clear(unsigned char *image, size_t stride, int x, int y, int w,
                int h);
void eosd_render_rgba(unsigned char *image, size_t stride, int w, int h,
                      mp_eosd_images_t *imgs);
bool eosd_bounding_box(mp_eosd_images_t *imgs, int *x, int *y, int *w, int *h);

#endif
