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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "m_option.h"

#include "stream/stream.h"
#include "demuxer.h"
#include "stheader.h"

#include "libaf/af_format.h"

static int channels = 2;
static int samplerate = 44100;
static int samplesize = 2;
static int bitrate = 0;
static int format = 0x1; // Raw PCM

static demuxer_t* demux_rawaudio_open(demuxer_t* demuxer) {
  sh_audio_t* sh_audio;
  WAVEFORMATEX* w;

  sh_audio = new_sh_audio(demuxer,0);
  sh_audio->wf = w = malloc(sizeof(*w));
  w->wFormatTag = sh_audio->format = mmioFOURCC('t', 'w', 'o', 's');
  w->nChannels = sh_audio->channels = channels;
  w->nSamplesPerSec = sh_audio->samplerate = samplerate;
  if (bitrate > 999)
    w->nAvgBytesPerSec = bitrate/8;
  else if (bitrate > 0)
    w->nAvgBytesPerSec = bitrate*125;
  else
    w->nAvgBytesPerSec = samplerate*samplesize*channels;
  w->nBlockAlign = channels*samplesize;
  sh_audio->samplesize = samplesize;
  w->wBitsPerSample = 8*samplesize;
  w->cbSize = 0;

  demuxer->movi_start = demuxer->stream->start_pos;
  demuxer->movi_end = demuxer->stream->end_pos;

  demuxer->audio->id = 0;
  demuxer->audio->sh = sh_audio;
  sh_audio->ds = demuxer->audio;
  sh_audio->needs_parsing = 1;

  return demuxer;
}

static int demux_rawaudio_fill_buffer(demuxer_t* demuxer, demux_stream_t *ds) {
  sh_audio_t* sh_audio = demuxer->audio->sh;
  int bytes_per_sec = 44100 * 2 * 2;
  int l = bytes_per_sec;
  off_t spos = stream_tell(demuxer->stream);
  demux_packet_t*  dp;

  if(demuxer->stream->eof)
    return 0;

  dp = new_demux_packet(l);
  dp->pts = (spos - demuxer->movi_start)  / (float)(bytes_per_sec);
  dp->pos = (spos - demuxer->movi_start);

  l = stream_read(demuxer->stream,dp->buffer,l);
  resize_demux_packet(dp, l);
  ds_add_packet(ds,dp);

  return 1;
}

static void demux_rawaudio_seek(demuxer_t *demuxer,float rel_seek_secs,float audio_delay,int flags){
  stream_t* s = demuxer->stream;
  sh_audio_t* sh_audio = demuxer->audio->sh;
  off_t base,pos;

  base = (flags & SEEK_ABSOLUTE) ? demuxer->movi_start : stream_tell(s);
  if(flags & SEEK_FACTOR)
    pos = base + ((demuxer->movi_end - demuxer->movi_start)*rel_seek_secs);
  else
    pos = base + (rel_seek_secs*sh_audio->i_bps);

  pos -= (pos % (sh_audio->channels * sh_audio->samplesize) );
  stream_seek(s,pos);
//  printf("demux_rawaudio: streamtell=%d\n",(int)stream_tell(demuxer->stream));
}

static int demux_bin_control(demuxer_t *demuxer,int cmd, void *arg) {
    switch(cmd) {
        // Need this to get length in tl_cue.c - it's basically duplicated
        // from get_time_length(), which we can't use directly.
        // Also, sh_audio->i_bps is not initialized yet, or something.
        case DEMUXER_CTRL_GET_TIME_LENGTH: ;
            int bytes_per_sec = 44100 * 2 * 2;
            *((double *)arg) = (double) (demuxer->movi_end - demuxer->movi_start) /
               bytes_per_sec;
            return DEMUXER_CTRL_OK;

        default:
            return DEMUXER_CTRL_NOTIMPL;
    }
}

const demuxer_desc_t demuxer_desc_bin = {
  "BIN audio demuxer (crappy hack)",
  "bin",
  "bin",
  "?",
  "",
  DEMUXER_TYPE_BIN,
  0, // no autodetect
  NULL,
  demux_rawaudio_fill_buffer,
  demux_rawaudio_open,
  NULL,
  demux_rawaudio_seek,
  demux_bin_control,
};
