/*
 * unicode/utf-8 for OS specific I/O
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
#ifndef MPLAYER_UNICODE_H
#define MPLAYER_UNICODE_H

#include <stdio.h>

#include "config.h"

typedef struct MP_DIR MP_DIR;

int mp_open(const char *pathname, int flags, int mode);
FILE *mp_fopen(const char *path, const char *mode);
MP_DIR *mp_opendir(const char *name);
// Like readdir(). Return dirent->d_name as a talloc'ed null terminated string
// (allocated under talloc_ctx). If the end of the directory stream is reached
// or an error happened, return NULL.
// Note that the other fields of dirent are useless if you take portability in
// account.
char *mp_readdir(MP_DIR *dirp, void *talloc_ctx);
int mp_closedir(MP_DIR *dirp);
// The mode parameter is ignored on Windows.
int mp_mkdir(const char *pathname, int mode);
// Return 1 if the file or directory exists, 0 otherwise.
int mp_file_exists(const char *pathname);

void mp_get_converted_argv(int *argc, char ***argv);

#endif /* MPLAYER_UNICODE_H */
