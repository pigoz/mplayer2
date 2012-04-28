/*
 * Cocoa OpenGL Backend
 *
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

#ifndef MPLAYER_COCOA_COMMON_H
#define MPLAYER_COCOA_COMMON_H

#include "video_out.h"

bool vo_cocoa_gui_running(void);
void *vo_cocoa_glgetaddr(const char *s);

int vo_cocoa_init(struct vo *vo);
void vo_cocoa_uninit(struct vo *vo);

void vo_cocoa_update_xinerama_info(struct vo *vo);

int vo_cocoa_change_attributes(struct vo *vo);
int vo_cocoa_create_window(struct vo *vo, uint32_t d_width,
                           uint32_t d_height, uint32_t flags,
                           int gl3profile);

void vo_cocoa_swap_buffers(void);
int vo_cocoa_check_events(struct vo *vo);
void vo_cocoa_fullscreen(struct vo *vo);
void vo_cocoa_ontop(struct vo *vo);

// returns an int to conform to the gl extensions from other platforms
int vo_cocoa_swap_interval(int enabled);

void *vo_cocoa_cgl_context(void);
void *vo_cocoa_cgl_pixel_format(void);

#endif /* MPLAYER_COCOA_COMMON_H */
