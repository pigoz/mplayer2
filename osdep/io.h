/*
 * unicode/utf-8 I/O helpers and wrappers for Windows
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
 * with mplayer2; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef MPLAYER_OSDEP_IO
#define MPLAYER_OSDEP_IO

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>

#ifdef _WIN32
#include <wchar.h>
wchar_t *mp_from_utf8(void *talloc_ctx, const char *s);
char *mp_to_utf8(void *talloc_ctx, const wchar_t *s);
#endif

#ifdef __MINGW32__

// Windows' MAX_PATH/PATH_MAX/FILENAME_MAX is fixed to 260, but this limit
// applies to unicode paths encoded with wchar_t (2 bytes on Windows). The UTF-8
// version could end up bigger in memory. In the worst case each wchar_t is
// encoded to 3 bytes in UTF-8, so in the worst case we have:
//      wcslen(wpath) <= strlen(utf8path) * 3
// Thus we need MP_PATH_MAX as the UTF-8/char version of PATH_MAX.
#define MP_PATH_MAX (FILENAME_MAX * 3)

void mp_get_converted_argv(int *argc, char ***argv);

int mp_stat(const char *path, struct stat *buf);
int mp_fprintf(FILE *stream, const char *format, ...);
int mp_open(const char *Filename, int OpenFlag, ...);
int mp_creat(const char *Filename, int PermissionMode);
FILE *mp_fopen(const char * __restrict__ Filename,
               const char * __restrict Mode);
DIR* mp_opendir(const char *path);
struct dirent* mp_readdir(DIR *dir);
int mp_closedir(DIR *dir);
int mp_mkdir(const char *path, int mode);

// NOTE: stat is not overridden with mp_stat, because MinGW-w64 defines it as
//       macro, and "struct stat" still has to work.

#define fprintf mp_fprintf
#define open mp_open
#define creat mp_creat
#define fopen mp_fopen
#define opendir mp_opendir
#define readdir mp_readdir
#define closedir mp_closedir
#define mkdir mp_mkdir

#else /* __MINGW32__ */

#include <unistd.h>

#define MP_PATH_MAX PATH_MAX

#define mp_stat stat

#endif /* __MINGW32__ */

#endif
