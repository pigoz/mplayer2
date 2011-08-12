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

#ifndef MPLAYER_GUI_INTERFACE_H
#define MPLAYER_GUI_INTERFACE_H

#include "config.h"
#include "libaf/af.h"
#include "libmpdemux/stheader.h"
#include "m_config.h"
#include "mp_core.h"
#include "playtree.h"
#include "stream/stream.h"

// These are in support of the non-GUI files that interact with
// the GUI and that only need to include interface.h for this.
// ------------------------------------------------------------
#include "cfg.h"

extern int use_gui;             // this is defined in mplayer.c
// ------------------------------------------------------------

#define GMPlayer "gmplayer"

// gui() instructions
enum {
    GUI_END_FILE,
    GUI_HANDLE_EVENTS,
    GUI_HANDLE_X_EVENT,
    GUI_PREPARE,
    GUI_REDRAW,
    GUI_RUN_COMMAND,
    GUI_SETUP_VIDEO_WINDOW,
    GUI_SET_AFILTER,
    GUI_SET_AUDIO,
    GUI_SET_CONTEXT,
    GUI_SET_FILE,
    GUI_SET_MIXER,
    GUI_SET_STATE,
    GUI_SET_STREAM,
    GUI_SET_VIDEO
};

// Playing states
#define GUI_STOP  0
#define GUI_PLAY  1
#define GUI_PAUSE 2

// NewPlay reasons
#define GUI_FILE_SAME 1
#define GUI_FILE_NEW  2

// mplayer() instructions
enum {
    MPLAYER_EXIT_GUI,
    MPLAYER_SET_AUTO_QUALITY,
    MPLAYER_SET_BRIGHTNESS,
    MPLAYER_SET_CONTRAST,
    MPLAYER_SET_EQUALIZER,
    MPLAYER_SET_EXTRA_STEREO,
    MPLAYER_SET_FONT_AUTOSCALE,
    MPLAYER_SET_FONT_BLUR,
    MPLAYER_SET_FONT_ENCODING,
    MPLAYER_SET_FONT_FACTOR,
    MPLAYER_SET_FONT_OSDSCALE,
    MPLAYER_SET_FONT_OUTLINE,
    MPLAYER_SET_FONT_TEXTSCALE,
    MPLAYER_SET_HUE,
    MPLAYER_SET_PANSCAN,
    MPLAYER_SET_SATURATION,
    MPLAYER_SET_SUB_ENCODING
};

typedef struct {
    int nr_of_audio_channels;
    stream_language_t audio_streams[32];
    int nr_of_subtitles;
    stream_language_t subtitles[32];
} guiDVDStruct;

typedef struct {
    MPContext *mpcontext;
    sh_video_t *sh_video;
    af_stream_t *afilter;

    int NewPlay;

#ifdef CONFIG_DVDREAD
    guiDVDStruct DVD;
    int Chapters;
    int Chapter;
    int Angles;
    int Angle;
#endif

#if defined(CONFIG_VCD) || defined(CONFIG_DVDREAD)
    int Tracks;
#endif

    int Playing;
    float Position;

    int MovieWidth;
    int MovieHeight;
    int MovieWindow;

    float Volume;
    float Balance;

    int Track;
    int AudioChannels;
    int StreamType;
    int TimeSec;
    int LengthInSec;

    char *Filename;

    char *Subtitlename;

    char *AudioFile;
} guiInterface_t;

extern guiInterface_t guiInfo;

/* MPlayer -> GUI */

int gui(int what, void *data);
void guiDone(void);
void guiInit(void);
int guiPlaylistAdd(play_tree_t *my_playtree, m_config_t *config);
int guiPlaylistInitialize(play_tree_t *my_playtree, m_config_t *config, int enqueue);

/* GUI -> MPlayer */

void mplayer(int what, float value, void *data);
void mplayerLoadFont(void);
void mplayerLoadSubtitle(const char *name);
void gmp_msg(int mod, int lev, const char *format, ...);

#endif /* MPLAYER_GUI_INTERFACE_H */
