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

#include <stdlib.h>
#include "talloc.h"
#include "osdep/unicode.h"

#ifndef __MINGW32__
// Unix

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int mp_open(const char *pathname, int flags, int mode)
{
    return open(pathname, flags, mode);
}

void mp_get_converted_argv(int *argc, char ***argv)
{
}

#else  /* #ifndef __MINGW32__ */
// Windows (mingw)

#include <windows.h>
#include <io.h>

//copied and modified from libav
//http://git.libav.org/?p=libav.git;a=blob;f=libavformat/os_support.c;h=a0fcd6c9ba2be4b0dbcc476f6c53587345cc1152;hb=HEAD#l30
//http://git.libav.org/?p=libav.git;a=blob;f=cmdutils.c;h=ade3f10ce2fc030e32e375a85fbd06c26d43a433#l161

static wchar_t *to_utf16(void *talloc_ctx, const char *s)
{
    int count = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    if (count <= 0)
        abort();
    wchar_t *ret = talloc_array(talloc_ctx, wchar_t, count);
    MultiByteToWideChar(CP_UTF8, 0, s, -1, ret, count);
    return ret;
}

static char *to_utf8(void *talloc_ctx, const wchar_t *s)
{
    int count = WideCharToMultiByte(CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL);
    if (count <= 0)
        abort();
    char *ret = talloc_array(talloc_ctx, char, count);
    WideCharToMultiByte(CP_UTF8, 0, s, -1, ret, count, NULL, NULL);
    return ret;
}

int mp_open(const char *pathname, int flags, int mode)
{
    wchar_t *wpath = to_utf16(NULL, pathname);
    int res = _wopen(wpath, flags, mode);
    talloc_free(wpath);
    return res;
}

static char** win32_argv_utf8;
static int win32_argc;

void mp_get_converted_argv(int *argc, char ***argv)
{
    if (!win32_argv_utf8) {
        win32_argc = 0;
        wchar_t **argv_w = CommandLineToArgvW(GetCommandLineW(), &win32_argc);
        if (win32_argc <= 0 || !argv_w)
            return;

        win32_argv_utf8 = talloc_zero_array(NULL, char*, win32_argc + 1);

        for (int i = 0; i < win32_argc; i++) {
            win32_argv_utf8[i] = to_utf8(NULL, argv_w[i]);
        }

        LocalFree(argv_w);
    }

    *argc = win32_argc;
    *argv = win32_argv_utf8;
}

#endif
