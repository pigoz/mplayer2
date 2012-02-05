#ifndef MPLAYER_COCOA_COMMON_H
#define MPLAYER_COCOA_COMMON_H

#include "video_out.h"
#include "mp_core.h" // MPContext definition

int vo_cocoa_init(struct vo *vo);
void vo_cocoa_uninit(struct vo *vo);

void vo_cocoa_update_xinerama_info(struct vo *vo);

int vo_cocoa_change_attributes(struct vo *vo);
int vo_cocoa_create_window(struct vo *vo, uint32_t d_width,
                           uint32_t d_height, uint32_t flags);

void vo_cocoa_swap_buffers(void);
int vo_cocoa_check_events(struct vo *vo);
void vo_cocoa_fullscreen(struct vo *vo);
void vo_cocoa_ontop(struct vo *vo);

// returns an int to conform to the gl extensions from other platforms
int vo_cocoa_swap_interval(int enabled);

void *vo_cocoa_cgl_context(void);
void *vo_cocoa_cgl_pixel_format(void);

void vo_cocoa_run_runloop(struct vo *vo);

void vo_cocoa_run_loop_schedule(
     void(*play_loop)(struct MPContext *),
     struct MPContext *context);

#endif /* MPLAYER_COCOA_COMMON_H */
