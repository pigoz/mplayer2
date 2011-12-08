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

#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <gdk/gdkprivate.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "config.h"
#include "help_mp.h"
#include "mp_msg.h"
#include "libavutil/intreadwrite.h"
#include "libvo/x11_common.h"

#include "widgets.h"
#include "gui/app.h"
#include "gui/interface.h"
#include "gui/wm/ws.h"

#include "actions.h"
#include "gtk/fileselect.h"

GtkWidget *PopUpMenu = NULL;

GtkWidget *WarningPixmap;
GtkWidget *ErrorPixmap;

int gtkPopupMenu      = 0;
int gtkPopupMenuParam = 0;
int gtkInitialized    = 0;

#include "gtk/skinbrowser.h"
#include "gtk/playlist.h"
#include "gtk/msgbox.h"
#include "gtk/about.h"
#include "gtk/preferences.h"
#include "gtk/menu.h"
#include "gtk/url.h"
#include "gtk/equalizer.h"

#include "pixmaps/mplayer.xpm"

#define THRESHOLD 128   // transparency values equal to or above this will become
                        // opaque, all values below this will become transparent

// --- init & close gtk

guiIcon_t guiIcon;

void gtkInit(void)
{
    int argc = 0, i;
    char *arg[3], **argv = arg;
    GdkPixbuf *pixbuf;
    GdkPixmap *gdkIcon;
    GdkBitmap *gdkIconMask;
    guchar *data;

    mp_msg(MSGT_GPLAYER, MSGL_V, "GTK init.\n");

    arg[argc++] = GMPlayer;

    if (mDisplayName) {            // MPlayer option '-display' was given
        arg[argc++] = "--display"; // Pass corresponding command line arguments to GTK,
        arg[argc++] = mDisplayName; // to open the requested display for the GUI, too.
    }

#ifdef CONFIG_GTK2
    gtk_disable_setlocale();
#endif

    gtk_init(&argc, &argv);

    pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)mplayer_xpm);

    gdk_pixbuf_render_pixmap_and_mask_for_colormap(pixbuf, gdk_colormap_get_system(), &gdkIcon, &gdkIconMask, THRESHOLD);

    if (gdk_pixbuf_get_colorspace(pixbuf) == GDK_COLORSPACE_RGB &&
        gdk_pixbuf_get_n_channels(pixbuf) == 4 &&
        gdk_pixbuf_get_bits_per_sample(pixbuf) == 8) {
        guiIcon.collection_size = 2 + gdk_pixbuf_get_width(pixbuf) * gdk_pixbuf_get_height(pixbuf);

        guiIcon.collection = malloc(guiIcon.collection_size * sizeof(*guiIcon.collection));

        if (guiIcon.collection) {
            guiIcon.collection[0] = gdk_pixbuf_get_width(pixbuf);
            guiIcon.collection[1] = gdk_pixbuf_get_height(pixbuf);

            data = gdk_pixbuf_get_pixels(pixbuf);

            for (i = 2; i < guiIcon.collection_size; data += 4, i++)
                guiIcon.collection[i] = (data[3] << 24) | AV_RB24(data);  // RGBA -> ARGB
        }
    } else
        mp_msg(MSGT_GPLAYER, MSGL_WARN, MSGTR_ICONERROR, "mplayer");

    // start up GTK which realizes the pixmaps
    gtk_main_iteration_do(FALSE);

    guiIcon.normal      = GDK_PIXMAP_XID(gdkIcon);
    guiIcon.normal_mask = GDK_PIXMAP_XID(gdkIconMask);

    gtkInitialized = 1;
}

void gtkAddIcon(GtkWidget *window)
{
    wsSetIcon(gdk_display, GDK_WINDOW_XWINDOW(window->window), &guiIcon);
}

void gtkClearList(GtkWidget *list)
{
    gtk_clist_clear(GTK_CLIST(list));
}

int gtkFindCList(GtkWidget *list, char *item)
{
    gint j;
    gchar *tmpstr;

    for (j = 0; j < GTK_CLIST(list)->rows; j++) {
        gtk_clist_get_text(GTK_CLIST(list), j, 0, &tmpstr);

        if (!strcmp(tmpstr, item))
            return j;
    }

    return -1;
}

void gtkSetDefaultToCList(GtkWidget *list, char *item)
{
    gint i;

    if ((i = gtkFindCList(list, item)) > -1)
        gtk_clist_select_row(GTK_CLIST(list), i, 0);
}

void gtkEventHandling(void)
{
    int i;

    for (i = 0; i < 25; i++)
        gtk_main_iteration_do(0);
}

// --- funcs

void gtkMessageBox(int type, const gchar *str)
{
    if (!gtkInitialized)
        return;

    ShowMessageBox(str);
    gtk_label_set_text(GTK_LABEL(gtkMessageBoxText), str);

    /* enable linewrapping by alex */
// GTK_LABEL(gtkMessageBoxText)->max_width = 80;
    if (strlen(str) > 80)
        gtk_label_set_line_wrap(GTK_LABEL(gtkMessageBoxText), TRUE);
    else
        gtk_label_set_line_wrap(GTK_LABEL(gtkMessageBoxText), FALSE);

    switch (type) {
    case GTK_MB_FATAL:
        gtk_window_set_title(GTK_WINDOW(MessageBox), MSGTR_MSGBOX_LABEL_FatalError);
        gtk_widget_hide(WarningPixmap);
        gtk_widget_show(ErrorPixmap);
        break;

    case GTK_MB_ERROR:
        gtk_window_set_title(GTK_WINDOW(MessageBox), MSGTR_MSGBOX_LABEL_Error);
        gtk_widget_hide(WarningPixmap);
        gtk_widget_show(ErrorPixmap);
        break;

    case GTK_MB_WARNING:
        gtk_window_set_title(GTK_WINDOW(MessageBox), MSGTR_MSGBOX_LABEL_Warning);
        gtk_widget_show(WarningPixmap);
        gtk_widget_hide(ErrorPixmap);
        break;
    }

    gtk_widget_show(MessageBox);
    gtkSetLayer(MessageBox);

    if (type == GTK_MB_FATAL)
        while (MessageBox)
            gtk_main_iteration_do(0);
}

void gtkSetLayer(GtkWidget *wdg)
{
    wsSetLayer(gdk_display, GDK_WINDOW_XWINDOW(wdg->window), guiApp.subWindow.isFullScreen);
    gtkActive(wdg);
}

void gtkActive(GtkWidget *wdg)
{
    wsRaiseWindowTop(gdk_display, GDK_WINDOW_XWINDOW(wdg->window));
}

void gtkShow(int type, char *param)
{
    switch (type) {
    case evEqualizer:
        ShowEqualizer();
        gtkSetLayer(Equalizer);
        break;

    case evSkinBrowser:
        ShowSkinBrowser();

//        gtkClearList( SkinList );
        if (gtkFillSkinList(sbMPlayerPrefixDir) &&
            gtkFillSkinList(sbMPlayerDirInHome)) {
            gtkSetDefaultToCList(SkinList, param);
            gtk_clist_sort(GTK_CLIST(SkinList));
            gtk_widget_show(SkinBrowser);
            gtkSetLayer(SkinBrowser);
        } else {
            gtk_widget_destroy(SkinBrowser);
            gtkMessageBox(GTK_MB_ERROR, "Skin dirs not found ... Please install skins.");
        }

        break;

    case evPreferences:
        ShowPreferences();
        break;

    case evPlaylist:
        ShowPlayList();
        gtkSetLayer(PlayList);
        break;

    case evLoad:
        ShowFileSelect(fsVideoSelector, 0);
        gtkSetLayer(fsFileSelect);
        break;

    case evFirstLoad:
        ShowFileSelect(fsVideoSelector, 0);
        gtkSetLayer(fsFileSelect);
        break;

    case evLoadSubtitle:
        ShowFileSelect(fsSubtitleSelector, 0);
        gtkSetLayer(fsFileSelect);
        break;

    case evLoadAudioFile:
        ShowFileSelect(fsAudioSelector, 0);
        gtkSetLayer(fsFileSelect);
        break;

    case evAbout:
        ShowAboutBox();
        gtkSetLayer(About);
        break;

    case evShowPopUpMenu:
        gtkPopupMenu      = evNone;
        gtkPopupMenuParam = 0;

        if (PopUpMenu) {
            gtk_widget_hide(PopUpMenu);
            gtk_widget_destroy(PopUpMenu);
        }

        PopUpMenu = create_PopUpMenu();
        gtk_menu_popup(GTK_MENU(PopUpMenu), NULL, NULL, NULL, NULL, 0, 0);
        break;

    case evHidePopUpMenu:

        if (PopUpMenu) {
            gtk_widget_hide(PopUpMenu);
            gtk_widget_destroy(PopUpMenu);
            PopUpMenu = NULL;
        }

        break;

    case evPlayNetwork:
        ShowURLDialogBox();
        gtkSetLayer(URL);
        break;
    }
}
