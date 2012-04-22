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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#if HAVE_MALLOC_H
#include <malloc.h>
#endif

#include "stream/stream.h"
#include "stream/stream_dvdnav.h"
#define OSD_NAV_BOX_ALPHA 0x7f

#include "libmpcodecs/dec_teletext.h"
#include "osdep/timer.h"

#include "talloc.h"
#include "mplayer.h"
#include "mp_msg.h"
#include "libvo/video_out.h"
#include "sub.h"
#include "spudec.h"
#include "libavutil/common.h"


char * const sub_osd_names[]={
    _("Seekbar"),
    _("Play"),
    _("Pause"),
    _("Stop"),
    _("Rewind"),
    _("Forward"),
    _("Clock"),
    _("Contrast"),
    _("Saturation"),
    _("Volume"),
    _("Brightness"),
    _("Hue"),
    _("Balance")
};
char * const sub_osd_names_short[] ={ "", "|>", "||", "[]", "<<" , ">>", "", "", "", "", "", "", "" };

void* vo_osd_teletext_page=NULL;
int vo_osd_teletext_half = 0;
int vo_osd_teletext_mode=0;
int vo_osd_teletext_format=0;
int sub_unicode=0;
int sub_utf8=0;
int sub_pos=100;
int sub_width_p=100;
int sub_alignment=2; /* 0=top, 1=center, 2=bottom */
int sub_visibility=1;
int sub_bg_color=0; /* subtitles background color */
int sub_bg_alpha=0;
int sub_justify=0;
#ifdef CONFIG_DVDNAV
static nav_highlight_t nav_hl;
#endif

int vo_osd_progbar_type=-1;
int vo_osd_progbar_value=100;   // 0..256
subtitle* vo_sub=NULL;
char *subtitle_font_encoding = NULL;
float text_font_scale_factor = 3.5;
float osd_font_scale_factor = 4.0;
float subtitle_font_radius = 2.0;
float subtitle_font_thickness = 2.0;
// 0 = no autoscale
// 1 = video height
// 2 = video width
// 3 = diagonal
int subtitle_autoscale = 3;

char *font_name = NULL;
char *sub_font_name = NULL;
float font_factor = 0.75;
float sub_delay = 0;
float sub_fps = 0;

// renders char to a big per-object buffer where alpha and bitmap are separated
void draw_alpha_buf(mp_osd_obj_t* obj, int x0,int y0, int w,int h, unsigned char* src, unsigned char *srca, int stride)
{
    int dststride = obj->stride;
    int dstskip = obj->stride-w;
    int srcskip = stride-w;
    int i, j;
    unsigned char *b = obj->bitmap_buffer + (y0-obj->bbox.y1)*dststride + (x0-obj->bbox.x1);
    unsigned char *a = obj->alpha_buffer  + (y0-obj->bbox.y1)*dststride + (x0-obj->bbox.x1);
    unsigned char *bs = src;
    unsigned char *as = srca;

    if (x0 < obj->bbox.x1 || x0+w > obj->bbox.x2 || y0 < obj->bbox.y1 || y0+h > obj->bbox.y2) {
	fprintf(stderr, "osd text out of range: bbox [%d %d %d %d], txt [%d %d %d %d]\n",
		obj->bbox.x1, obj->bbox.x2, obj->bbox.y1, obj->bbox.y2,
		x0, x0+w, y0, y0+h);
	return;
    }

    for (i = 0; i < h; i++) {
	for (j = 0; j < w; j++, b++, a++, bs++, as++) {
	    if (*b < *bs) *b = *bs;
	    if (*as) {
		if (*a == 0 || *a > *as) *a = *as;
	    }
	}
	b+= dstskip;
	a+= dstskip;
	bs+= srcskip;
	as+= srcskip;
    }
}

// allocates/enlarges the alpha/bitmap buffer
void osd_alloc_buf(mp_osd_obj_t* obj)
{
    int len;
    if (obj->bbox.x2 < obj->bbox.x1) obj->bbox.x2 = obj->bbox.x1;
    if (obj->bbox.y2 < obj->bbox.y1) obj->bbox.y2 = obj->bbox.y1;
    obj->stride = ((obj->bbox.x2-obj->bbox.x1)+7)&(~7);
    len = obj->stride*(obj->bbox.y2-obj->bbox.y1);
    if (obj->allocated<len) {
	obj->allocated = len;
	free(obj->bitmap_buffer);
	free(obj->alpha_buffer);
	obj->bitmap_buffer = memalign(16, len);
	obj->alpha_buffer  = memalign(16, len);
    }
    memset(obj->bitmap_buffer, sub_bg_color, len);
    memset(obj->alpha_buffer, sub_bg_alpha, len);
}

// renders the buffer
void vo_draw_text_from_buffer(mp_osd_obj_t* obj,void (*draw_alpha)(void *ctx, int x0,int y0, int w,int h, unsigned char* src, unsigned char *srca, int stride), void *ctx)
{
    if (obj->allocated > 0) {
	draw_alpha(ctx,
		   obj->bbox.x1,obj->bbox.y1,
		   obj->bbox.x2-obj->bbox.x1,
		   obj->bbox.y2-obj->bbox.y1,
		   obj->bitmap_buffer,
		   obj->alpha_buffer,
		   obj->stride);
    }
}

unsigned utf8_get_char(const char **str) {
  const uint8_t *strp = (const uint8_t *)*str;
  unsigned c;
  GET_UTF8(c, *strp++, goto no_utf8;);
  *str = (const char *)strp;
  return c;

no_utf8:
  strp = (const uint8_t *)*str;
  c = *strp++;
  *str = (const char *)strp;
  return c;
}

#ifdef CONFIG_DVDNAV
void osd_set_nav_box (uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey) {
  nav_hl.sx = sx;
  nav_hl.sy = sy;
  nav_hl.ex = ex;
  nav_hl.ey = ey;
}

inline static void vo_update_nav (mp_osd_obj_t *obj, int dxs, int dys, int left_border, int top_border,
                      int right_border, int bottom_border, int orig_w, int orig_h) {
  int len;
  int sx = nav_hl.sx, sy = nav_hl.sy;
  int ex = nav_hl.ex, ey = nav_hl.ey;
  int scaled_w = dxs - left_border - right_border;
  int scaled_h = dys - top_border - bottom_border;
  if (scaled_w != orig_w) {
    sx = sx * scaled_w / orig_w;
    ex = ex * scaled_w / orig_w;
  }
  if (scaled_h != orig_h) {
    sy = sy * scaled_h / orig_h;
    ey = ey * scaled_h / orig_h;
  }
  sx += left_border; ex += left_border;
  sy += top_border;  ey += top_border;
  sx = FFMIN(FFMAX(sx, 0), dxs);
  ex = FFMIN(FFMAX(ex, 0), dxs);
  sy = FFMIN(FFMAX(sy, 0), dys);
  ey = FFMIN(FFMAX(ey, 0), dys);

  obj->bbox.x1 = obj->x = sx;
  obj->bbox.y1 = obj->y = sy;
  obj->bbox.x2 = ex;
  obj->bbox.y2 = ey;

  osd_alloc_buf (obj);
  len = obj->stride * (obj->bbox.y2 - obj->bbox.y1);
  memset (obj->bitmap_buffer, OSD_NAV_BOX_ALPHA, len);
  memset (obj->alpha_buffer, OSD_NAV_BOX_ALPHA, len);
  obj->flags |= OSDFLAG_BBOX | OSDFLAG_CHANGED;
  if (obj->bbox.y2 > obj->bbox.y1 && obj->bbox.x2 > obj->bbox.x1)
    obj->flags |= OSDFLAG_VISIBLE;
}
#endif


inline static void vo_update_spudec_sub(struct osd_state *osd, mp_osd_obj_t* obj)
{
  unsigned int bbox[4];
  spudec_calc_bbox(vo_spudec, osd->w, osd->h, bbox);
  obj->bbox.x1 = bbox[0];
  obj->bbox.x2 = bbox[1];
  obj->bbox.y1 = bbox[2];
  obj->bbox.y2 = bbox[3];
  obj->flags |= OSDFLAG_BBOX;
}

inline static void vo_draw_spudec_sub(mp_osd_obj_t* obj, void (*draw_alpha)(void *ctx, int x0, int y0, int w, int h, unsigned char* src, unsigned char* srca, int stride), void *ctx)
{
    spudec_draw_scaled(vo_spudec, obj->dxs, obj->dys, draw_alpha, ctx);
}

void *vo_spudec=NULL;
void *vo_vobsub=NULL;

static int draw_alpha_init_flag=0;

void vo_draw_alpha_init(void);

       mp_osd_obj_t* vo_osd_list=NULL;

static mp_osd_obj_t* new_osd_obj(int type){
    mp_osd_obj_t* osd=malloc(sizeof(mp_osd_obj_t));
    memset(osd,0,sizeof(mp_osd_obj_t));
    osd->next=vo_osd_list;
    vo_osd_list=osd;
    osd->type=type;
    osd->alpha_buffer = NULL;
    osd->bitmap_buffer = NULL;
    osd->allocated = -1;
    return osd;
}

void osd_free(struct osd_state *osd)
{
    osd_destroy_backend(osd);
    mp_osd_obj_t* obj=vo_osd_list;
    while(obj){
	mp_osd_obj_t* next=obj->next;
	free(obj->alpha_buffer);
	free(obj->bitmap_buffer);
	free(obj);
	obj=next;
    }
    vo_osd_list=NULL;
    talloc_free(osd);
}

static int osd_update_ext(struct osd_state *osd, int dxs, int dys,
                          int left_border, int top_border, int right_border,
                          int bottom_border, int orig_w, int orig_h)
{
    mp_osd_obj_t* obj=vo_osd_list;
    int chg=0;

    osd->w = dxs;
    osd->h = dys;

    osd_font_load(osd);

    while(obj){
      if(dxs!=obj->dxs || dys!=obj->dys || obj->flags&OSDFLAG_FORCE_UPDATE){
        int vis=obj->flags&OSDFLAG_VISIBLE;
	obj->flags&=~OSDFLAG_BBOX;
	switch(obj->type){
#ifdef CONFIG_DVDNAV
        case OSDTYPE_DVDNAV:
           vo_update_nav(obj,dxs,dys, left_border, top_border, right_border, bottom_border, orig_w, orig_h);
           break;
#endif
	case OSDTYPE_SUBTITLE:
	    vo_update_text_sub(osd, obj);
	    break;
	case OSDTYPE_TELETEXT:
	    vo_update_text_teletext(osd, obj);
	    break;
	case OSDTYPE_PROGBAR:
	    vo_update_text_progbar(osd, obj);
	    break;
	case OSDTYPE_SPU:
	    if(sub_visibility && vo_spudec && spudec_visible(vo_spudec)){
	        vo_update_spudec_sub(osd, obj);
		obj->flags|=OSDFLAG_VISIBLE|OSDFLAG_CHANGED;
	    }
	    else
		obj->flags&=~OSDFLAG_VISIBLE;
	    break;
	case OSDTYPE_OSD:
	    if(osd->osd_text[0]){
		vo_update_text_osd(osd, obj);
		obj->flags|=OSDFLAG_VISIBLE|OSDFLAG_CHANGED;
	    } else
		obj->flags&=~OSDFLAG_VISIBLE;
	    break;
	}
	// check bbox:
	if(!(obj->flags&OSDFLAG_BBOX)){
	    // we don't know, so assume the whole screen changed :(
	    obj->bbox.x1=obj->bbox.y1=0;
	    obj->bbox.x2=dxs;
	    obj->bbox.y2=dys;
	    obj->flags|=OSDFLAG_BBOX;
	} else {
	    // check bbox, reduce it if it's out of bounds (corners):
	    if(obj->bbox.x1<0) obj->bbox.x1=0;
	    if(obj->bbox.y1<0) obj->bbox.y1=0;
	    if(obj->bbox.x2>dxs) obj->bbox.x2=dxs;
	    if(obj->bbox.y2>dys) obj->bbox.y2=dys;
	    if(obj->flags&OSDFLAG_VISIBLE)
	    // debug:
	    mp_msg(MSGT_OSD,MSGL_DBG2,"OSD update: %d;%d %dx%d  \n",
		obj->bbox.x1,obj->bbox.y1,obj->bbox.x2-obj->bbox.x1,
		obj->bbox.y2-obj->bbox.y1);
	}
	// check if visibility changed:
	if(vis != (obj->flags&OSDFLAG_VISIBLE) ) obj->flags|=OSDFLAG_CHANGED;
	// remove the cause of automatic update:
	obj->dxs=dxs; obj->dys=dys;
	obj->flags&=~OSDFLAG_FORCE_UPDATE;
      }
      if(obj->flags&OSDFLAG_CHANGED){
        chg|=1<<obj->type;
	mp_msg(MSGT_OSD,MSGL_DBG2,"OSD chg: %d  V: %s  pb:%d  \n",obj->type,(obj->flags&OSDFLAG_VISIBLE)?"yes":"no",vo_osd_progbar_type);
      }
      obj=obj->next;
    }
    return chg;
}

int osd_update(struct osd_state *osd, int dxs, int dys)
{
    return osd_update_ext(osd, dxs, dys, 0, 0, 0, 0, dxs, dys);
}

struct osd_state *osd_create(struct MPOpts *opts, struct ass_library *asslib)
{
    struct osd_state *osd = talloc_zero(NULL, struct osd_state);
    *osd = (struct osd_state){
        .opts = opts,
        .ass_library = asslib,
    };
    if(!draw_alpha_init_flag){
	draw_alpha_init_flag=1;
	vo_draw_alpha_init();
    }
    // temp hack, should be moved to mplayer later
    new_osd_obj(OSDTYPE_OSD);
    new_osd_obj(OSDTYPE_SUBTITLE);
    new_osd_obj(OSDTYPE_PROGBAR);
    new_osd_obj(OSDTYPE_SPU);
#ifdef CONFIG_DVDNAV
    new_osd_obj(OSDTYPE_DVDNAV);
#endif
    new_osd_obj(OSDTYPE_TELETEXT);
    osd_font_invalidate();
    osd_set_text(osd, NULL);
    osd_init_backend(osd);
    return osd;
}

void osd_set_text(struct osd_state *osd, const char *text) {
    talloc_free(osd->osd_text);
    //osd->text must never be NULL
    if (!text)
        text = "";
    osd->osd_text = talloc_strdup(osd, text);
}

int vo_osd_changed_flag=0;

void osd_remove_text(struct osd_state *osd, int dxs, int dys,
                    void (*remove)(int x0, int y0, int w, int h))
{
    mp_osd_obj_t* obj=vo_osd_list;
    osd_update(osd, dxs, dys);
    while(obj){
      if(((obj->flags&OSDFLAG_CHANGED) || (obj->flags&OSDFLAG_VISIBLE)) &&
         (obj->flags&OSDFLAG_OLD_BBOX)){
          int w=obj->old_bbox.x2-obj->old_bbox.x1;
	  int h=obj->old_bbox.y2-obj->old_bbox.y1;
	  if(w>0 && h>0){
	      vo_osd_changed_flag=obj->flags&OSDFLAG_CHANGED;	// temp hack
              remove(obj->old_bbox.x1,obj->old_bbox.y1,w,h);
	  }
//	  obj->flags&=~OSDFLAG_OLD_BBOX;
      }
      obj=obj->next;
    }
}

void osd_draw_text_ext(struct osd_state *osd, int dxs, int dys,
                       int left_border, int top_border, int right_border,
                       int bottom_border, int orig_w, int orig_h,
                       void (*draw_alpha)(void *ctx, int x0, int y0, int w,
                                          int h, unsigned char* src,
                                          unsigned char *srca,
                                          int stride),
                   void *ctx)
{
    mp_osd_obj_t* obj=vo_osd_list;
    osd_update_ext(osd, dxs, dys, left_border, top_border, right_border,
                   bottom_border, orig_w, orig_h);
    while(obj){
      if(obj->flags&OSDFLAG_VISIBLE){
	vo_osd_changed_flag=obj->flags&OSDFLAG_CHANGED;	// temp hack
	switch(obj->type){
	case OSDTYPE_SPU:
	    vo_draw_spudec_sub(obj, draw_alpha, ctx); // FIXME
	    break;
#ifdef CONFIG_DVDNAV
        case OSDTYPE_DVDNAV:
#endif
	case OSDTYPE_TELETEXT:
	case OSDTYPE_OSD:
	case OSDTYPE_SUBTITLE:
	case OSDTYPE_PROGBAR:
	    vo_draw_text_from_buffer(obj, draw_alpha, ctx);
	    break;
	}
	obj->old_bbox=obj->bbox;
	obj->flags|=OSDFLAG_OLD_BBOX;
      }
      obj->flags&=~OSDFLAG_CHANGED;
      obj=obj->next;
    }
}

void osd_draw_text(struct osd_state *osd, int dxs, int dys,
                   void (*draw_alpha)(void *ctx, int x0, int y0, int w, int h,
                                      unsigned char* src, unsigned char *srca,
                                      int stride),
                   void *ctx)
{
    osd_draw_text_ext(osd, dxs, dys, 0, 0, 0, 0, dxs, dys, draw_alpha, ctx);
}

static int vo_osd_changed_status = 0;

int vo_osd_changed(int new_value)
{
    mp_osd_obj_t* obj=vo_osd_list;
    int ret = vo_osd_changed_status;
    vo_osd_changed_status = new_value;

    while(obj){
	if(obj->type==new_value) obj->flags|=OSDFLAG_FORCE_UPDATE;
	obj=obj->next;
    }

    return ret;
}

void vo_osd_resized()
{
    // font needs to be adjusted
    osd_font_invalidate();
    // OSD needs to be drawn fresh for new size
    vo_osd_changed(OSDTYPE_OSD);
    vo_osd_changed(OSDTYPE_SUBTITLE);
}

// return TRUE if we have osd in the specified rectangular area:
int vo_osd_check_range_update(int x1,int y1,int x2,int y2){
    mp_osd_obj_t* obj=vo_osd_list;
    while(obj){
	if(obj->flags&OSDFLAG_VISIBLE){
	    if(	(obj->bbox.x1<=x2 && obj->bbox.x2>=x1) &&
		(obj->bbox.y1<=y2 && obj->bbox.y2>=y1) &&
		obj->bbox.y2 > obj->bbox.y1 && obj->bbox.x2 > obj->bbox.x1
		) return 1;
	}
	obj=obj->next;
    }
    return 0;
}
