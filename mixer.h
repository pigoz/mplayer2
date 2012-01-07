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

#ifndef MPLAYER_MIXER_H
#define MPLAYER_MIXER_H

#include <stdbool.h>

#include "libaf/af.h"
#include "libao2/audio_out.h"

extern char * mixer_device;
extern char * mixer_channel;
extern int soft_vol;
extern float soft_vol_max;

typedef struct mixer_s {
    struct ao *ao;
    af_stream_t *afilter;
    int volstep;
    bool muted;
    float last_l, last_r;
    float restore_vol_l, restore_vol_r;
    bool restore_volume;
    float balance;
    bool restore_balance;
} mixer_t;

void mixer_reinit(mixer_t *mixer);
void mixer_uninit(mixer_t *mixer);
void mixer_getvolume(mixer_t *mixer, float *l, float *r);
void mixer_setvolume(mixer_t *mixer, float l, float r);
void mixer_incvolume(mixer_t *mixer);
void mixer_decvolume(mixer_t *mixer);
void mixer_getbothvolume(mixer_t *mixer, float *b);
void mixer_mute(mixer_t *mixer);
bool mixer_getmuted(mixer_t *mixer);
void mixer_setmuted(mixer_t *mixer, bool mute);
void mixer_getbalance(mixer_t *mixer, float *bal);
void mixer_setbalance(mixer_t *mixer, float bal);

#endif /* MPLAYER_MIXER_H */
