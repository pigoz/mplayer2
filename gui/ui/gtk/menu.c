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
#include "help_mp.h"
#include "access_mpcontext.h"
#include "mixer.h"
#include "mpcommon.h"

#include "menu.h"
#include "gui/ui/widgets.h"
#include "gui/ui/gmplayer.h"
#include "gui/app.h"
#include "gui/interface.h"

#include "stream/stream.h"
#include "libmpdemux/demuxer.h"
#include "libmpdemux/stheader.h"
#include "libavutil/avstring.h"

#include "gui/ui/pixmaps/about.xpm"
#include "gui/ui/pixmaps/half.xpm"
#include "gui/ui/pixmaps/normal.xpm"
#include "gui/ui/pixmaps/double.xpm"
#include "gui/ui/pixmaps/full.xpm"
#include "gui/ui/pixmaps/exit.xpm"
#include "gui/ui/pixmaps/prefs.xpm"
#include "gui/ui/pixmaps/equalizer.xpm"
#include "gui/ui/pixmaps/playlist.xpm"
#include "gui/ui/pixmaps/skin.xpm"
#include "gui/ui/pixmaps/sound.xpm"
#include "gui/ui/pixmaps/open.xpm"
#include "gui/ui/pixmaps/play.xpm"
#include "gui/ui/pixmaps/stop.xpm"
#include "gui/ui/pixmaps/pause.xpm"
#include "gui/ui/pixmaps/prev.xpm"
#include "gui/ui/pixmaps/next.xpm"
#include "gui/ui/pixmaps/aspect.xpm"
#include "gui/ui/pixmaps/aspect11.xpm"
#include "gui/ui/pixmaps/aspect169.xpm"
#include "gui/ui/pixmaps/aspect235.xpm"
#include "gui/ui/pixmaps/aspect43.xpm"
#include "gui/ui/pixmaps/file2.xpm"
#include "gui/ui/pixmaps/url.xpm"
#include "gui/ui/pixmaps/sub.xpm"
#include "gui/ui/pixmaps/nosub.xpm"
#include "gui/ui/pixmaps/empty.xpm"
#include "gui/ui/pixmaps/loadeaf.xpm"
#include "gui/ui/pixmaps/title.xpm"
#ifdef CONFIG_CDDA
#include "gui/ui/pixmaps/cd.xpm"
#include "gui/ui/pixmaps/playcd.xpm"
#endif
#ifdef CONFIG_VCD
#include "gui/ui/pixmaps/vcd.xpm"
#include "gui/ui/pixmaps/playvcd.xpm"
#endif
#ifdef CONFIG_DVDREAD
#include "gui/ui/pixmaps/dvd.xpm"
#include "gui/ui/pixmaps/playdvd.xpm"
#include "gui/ui/pixmaps/chapter.xpm"
#include "gui/ui/pixmaps/dolby.xpm"
#include "gui/ui/pixmaps/audiolang.xpm"
#include "gui/ui/pixmaps/sublang.xpm"
#endif
#include "gui/ui/pixmaps/empty1px.xpm"

static void ActivateMenuItem( int Item )
{
// fprintf( stderr,"[menu] item: %d.%d\n",Item&0xffff,Item>>16 );
 gtkPopupMenu=Item & 0x0000ffff;
 gtkPopupMenuParam=Item >> 16;
 uiEventHandling( Item & 0x0000ffff,Item >> 16 );
}

static GtkWidget * AddMenuCheckItem(GtkWidget *window1, const char * immagine_xpm, GtkWidget* Menu,const char* label, gboolean state, int Number)
{
 GtkWidget * Label = NULL;
 GtkWidget * Pixmap = NULL;
 GtkWidget * hbox = NULL;
 GtkWidget * Item = NULL;

 GdkPixmap *PixmapIcon = NULL;
 GdkColor transparent;
 GdkBitmap *MaskIcon = NULL;

 PixmapIcon = gdk_pixmap_create_from_xpm_d (window1->window, &MaskIcon, &transparent,(gchar **)immagine_xpm );
 Pixmap = gtk_pixmap_new (PixmapIcon, MaskIcon);
 gdk_pixmap_unref (PixmapIcon);

 Item=gtk_check_menu_item_new();
 Label = gtk_label_new (label);

 hbox = gtk_hbox_new (FALSE, 8);
 gtk_box_pack_start (GTK_BOX (hbox), Pixmap, FALSE, FALSE, 0);
 gtk_box_pack_start (GTK_BOX (hbox), Label, FALSE, FALSE, 0);
 gtk_container_add (GTK_CONTAINER (Item), hbox);

 gtk_menu_append( GTK_MENU( Menu ),Item );

 gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(Item),state);
 gtk_signal_connect_object( GTK_OBJECT(Item),"activate",
   GTK_SIGNAL_FUNC(ActivateMenuItem),(gpointer)Number );
 gtk_menu_item_right_justify (GTK_MENU_ITEM (Item));
 gtk_widget_show_all(Item);

 return Item;
}
GtkWidget * AddMenuItem( GtkWidget *window1, const char * immagine_xpm,  GtkWidget * SubMenu,const char * label,int Number )
{
 GtkWidget * Label = NULL;
 GtkWidget * Pixmap = NULL;
 GtkWidget * hbox = NULL;
 GtkWidget * Item = NULL;
 GdkPixmap * PixmapIcon = NULL;
 GdkColor transparent;
 GdkBitmap * MaskIcon = NULL;

 PixmapIcon = gdk_pixmap_create_from_xpm_d (window1->window, &MaskIcon, &transparent,(gchar **)immagine_xpm );
 Pixmap = gtk_pixmap_new (PixmapIcon, MaskIcon);
 gdk_pixmap_unref (PixmapIcon);

 Item=gtk_menu_item_new();
 Label = gtk_label_new (label);

 hbox = gtk_hbox_new (FALSE, 8);
 gtk_box_pack_start (GTK_BOX (hbox), Pixmap, FALSE, FALSE, 0);
 gtk_box_pack_start (GTK_BOX (hbox), Label, FALSE, FALSE, 0);
 gtk_container_add (GTK_CONTAINER (Item), hbox);


 gtk_menu_append( GTK_MENU( SubMenu ),Item );
 gtk_signal_connect_object( GTK_OBJECT(Item),"activate",
   GTK_SIGNAL_FUNC(ActivateMenuItem),(gpointer)Number );

 gtk_menu_item_right_justify (GTK_MENU_ITEM (Item));
 gtk_widget_show_all(Item);
 return Item;
}


GtkWidget * AddSubMenu( GtkWidget *window1, const char * immagine_xpm, GtkWidget * Menu,const char * label )
{
 GtkWidget * Label = NULL;
 GtkWidget * Pixmap = NULL;
 GtkWidget * hbox = NULL;
 GtkWidget * Item = NULL;
 GtkWidget * SubItem = NULL;
 GdkPixmap * PixmapIcon = NULL;
 GdkColor transparent;
 GdkBitmap * MaskIcon = NULL;

 PixmapIcon = gdk_pixmap_create_from_xpm_d (window1->window, &MaskIcon, &transparent,(gchar **)immagine_xpm);
 Pixmap = gtk_pixmap_new (PixmapIcon, MaskIcon);
 gdk_pixmap_unref (PixmapIcon);

 SubItem=gtk_menu_item_new();
 Item=gtk_menu_new();
 Label = gtk_label_new (label);

 hbox = gtk_hbox_new (FALSE, 8);
 gtk_box_pack_start (GTK_BOX (hbox), Pixmap, FALSE, FALSE, 0);
 gtk_box_pack_start (GTK_BOX (hbox), Label, FALSE, FALSE, 0);
 gtk_container_add (GTK_CONTAINER (SubItem), hbox);

 gtk_menu_append( GTK_MENU( Menu ),SubItem );
 gtk_menu_item_set_submenu( GTK_MENU_ITEM( SubItem ),Item );

 gtk_widget_show_all( SubItem );
 return Item;
}

GtkWidget * AddSeparator( GtkWidget * Menu )
{
 GtkWidget * Item = NULL;

 Item=gtk_menu_item_new ();
 gtk_widget_show( Item );
 gtk_container_add( GTK_CONTAINER( Menu ),Item );
 gtk_widget_set_sensitive( Item,FALSE );

 return Item;
}

typedef struct
{
 int id;
 const char * name;
} Languages_t;

#define lng( a,b ) ( (int)(a) * 256 + b )
static Languages_t Languages[] =
         {
           { lng( 'a','b' ), "Abkhazian"                  },
           { lng( 'a','a' ), "Afar"                       },
           { lng( 'a','f' ), "Afrikaans"                  },
           { lng( 's','q' ), "Albanian"                   },
           { lng( 'a','m' ), "Amharic"                    },
           { lng( 'a','r' ), "Arabic"                     },
           { lng( 'h','y' ), "Armenian"                   },
           { lng( 'a','s' ), "Assamese"                   },
           { lng( 'a','e' ), "Avestan"                    },
           { lng( 'a','y' ), "Aymara"                     },
           { lng( 'a','z' ), "Azerbaijani"                },
           { lng( 'b','a' ), "Bashkir"                    },
           { lng( 'e','u' ), "Basque"                     },
           { lng( 'b','e' ), "Belarusian"                 },
           { lng( 'b','n' ), "Bengali"                    },
           { lng( 'b','h' ), "Bihari"                     },
           { lng( 'b','i' ), "Bislama"                    },
           { lng( 'b','s' ), "Bosnian"                    },
           { lng( 'b','r' ), "Breton"                     },
           { lng( 'b','g' ), "Bulgarian"                  },
           { lng( 'm','y' ), "Burmese"                    },
           { lng( 'c','a' ), "Catalan"                    },
           { lng( 'c','h' ), "Chamorro"                   },
           { lng( 'c','e' ), "Chechen"                    },
           { lng( 'n','y' ), "Chichewa;Nyanja"            },
           { lng( 'z','h' ), "Chinese"                    },
           { lng( 'c','u' ), "ChurchSlavic"               },
           { lng( 'c','v' ), "Chuvash"                    },
           { lng( 'k','w' ), "Cornish"                    },
           { lng( 'c','o' ), "Corsican"                   },
           { lng( 'h','r' ), "Croatian"                   },
           { lng( 'c','s' ), "Czech"                      },
           { lng( 'd','a' ), "Danish"                     },
           { lng( 'n','l' ), "Dutch"                      },
           { lng( 'd','z' ), "Dzongkha"                   },
           { lng( 'e','n' ), "English"                    },
           { lng( 'e','o' ), "Esperanto"                  },
           { lng( 'e','t' ), "Estonian"                   },
           { lng( 'f','o' ), "Faroese"                    },
           { lng( 'f','j' ), "Fijian"                     },
           { lng( 'f','i' ), "Finnish"                    },
           { lng( 'f','r' ), "French"                     },
           { lng( 'f','y' ), "Frisian"                    },
           { lng( 'g','d' ), "Gaelic(Scots"               },
           { lng( 'g','l' ), "Gallegan"                   },
           { lng( 'k','a' ), "Georgian"                   },
           { lng( 'd','e' ), "German"                     },
           { lng( 'e','l' ), "Greek"                      },
           { lng( 'g','n' ), "Guarani"                    },
           { lng( 'g','u' ), "Gujarati"                   },
           { lng( 'h','a' ), "Hausa"                      },
           { lng( 'h','e' ), "Hebrew"                     },
           { lng( 'i','w' ), "Hebrew"                     },
           { lng( 'h','z' ), "Herero"                     },
           { lng( 'h','i' ), "Hindi"                      },
           { lng( 'h','o' ), "HiriMotu"                   },
           { lng( 'h','u' ), "Hungarian"                  },
           { lng( 'i','s' ), "Icelandic"                  },
           { lng( 'i','d' ), "Indonesian"                 },
           { lng( 'i','n' ), "Indonesian"                 },
           { lng( 'i','a' ), "Interlingua"                },
           { lng( 'i','e' ), "Interlingue"                },
           { lng( 'i','u' ), "Inuktitut"                  },
           { lng( 'i','k' ), "Inupiaq"                    },
           { lng( 'g','a' ), "Irish"                      },
           { lng( 'i','t' ), "Italian"                    },
           { lng( 'j','a' ), "Japanese"                   },
           { lng( 'j','v' ), "Javanese"                   },
           { lng( 'j','w' ), "Javanese"                   },
           { lng( 'k','l' ), "Kalaallisut"                },
           { lng( 'k','n' ), "Kannada"                    },
           { lng( 'k','s' ), "Kashmiri"                   },
           { lng( 'k','k' ), "Kazakh"                     },
           { lng( 'k','m' ), "Khmer"                      },
           { lng( 'k','i' ), "Kikuyu"                     },
           { lng( 'r','w' ), "Kinyarwanda"                },
           { lng( 'k','y' ), "Kirghiz"                    },
           { lng( 'k','v' ), "Komi"                       },
           { lng( 'k','o' ), "Korean"                     },
           { lng( 'k','j' ), "Kuanyama"                   },
           { lng( 'k','u' ), "Kurdish"                    },
           { lng( 'l','o' ), "Lao"                        },
           { lng( 'l','a' ), "Latin"                      },
           { lng( 'l','v' ), "Latvian"                    },
           { lng( 'l','b' ), "Letzeburgesch"              },
           { lng( 'l','n' ), "Lingala"                    },
           { lng( 'l','t' ), "Lithuanian"                 },
           { lng( 'm','k' ), "Macedonian"                 },
           { lng( 'm','g' ), "Malagasy"                   },
           { lng( 'm','s' ), "Malay"                      },
           { lng( 'm','l' ), "Malayalam"                  },
           { lng( 'm','t' ), "Maltese"                    },
           { lng( 'g','v' ), "Manx"                       },
           { lng( 'm','i' ), "Maori"                      },
           { lng( 'm','r' ), "Marathi"                    },
           { lng( 'm','h' ), "Marshall"                   },
           { lng( 'm','o' ), "Moldavian"                  },
           { lng( 'm','n' ), "Mongolian"                  },
           { lng( 'n','a' ), "Nauru"                      },
           { lng( 'n','v' ), "Navajo"                     },
           { lng( 'n','d' ), "North Ndebele"              },
           { lng( 'n','r' ), "South Ndebele"              },
           { lng( 'n','g' ), "Ndonga"                     },
           { lng( 'n','e' ), "Nepali"                     },
           { lng( 's','e' ), "NorthernSami"               },
           { lng( 'n','o' ), "Norwegian"                  },
           { lng( 'n','b' ), "NorwegianBokmål"            },
           { lng( 'n','n' ), "NorwegianNynorsk"           },
           { lng( 'n','y' ), "Nyanja;Chichewa"            },
           { lng( 'o','c' ), "Occitan(post1500;Provençal" },
           { lng( 'o','r' ), "Oriya"                      },
           { lng( 'o','m' ), "Oromo"                      },
           { lng( 'o','s' ), "Ossetian;Ossetic"           },
           { lng( 'p','i' ), "Pali"                       },
           { lng( 'p','a' ), "Panjabi"                    },
           { lng( 'f','a' ), "Persian"                    },
           { lng( 'p','l' ), "Polish"                     },
           { lng( 'p','t' ), "Portuguese"                 },
           { lng( 'o','c' ), "Provençal;Occitan(post1500" },
           { lng( 'p','s' ), "Pushto"                     },
           { lng( 'q','u' ), "Quechua"                    },
           { lng( 'r','m' ), "Raeto-Romance"              },
           { lng( 'r','o' ), "Romanian"                   },
           { lng( 'r','n' ), "Rundi"                      },
           { lng( 'r','u' ), "Russian"                    },
           { lng( 's','m' ), "Samoan"                     },
           { lng( 's','g' ), "Sango"                      },
           { lng( 's','a' ), "Sanskrit"                   },
           { lng( 's','c' ), "Sardinian"                  },
           { lng( 's','r' ), "Serbian"                    },
           { lng( 's','n' ), "Shona"                      },
           { lng( 's','d' ), "Sindhi"                     },
           { lng( 's','i' ), "Sinhalese"                  },
           { lng( 's','k' ), "Slovak"                     },
           { lng( 's','l' ), "Slovenian"                  },
           { lng( 's','o' ), "Somali"                     },
           { lng( 's','t' ), "Sotho"                      },
           { lng( 'e','s' ), "Spanish"                    },
           { lng( 's','u' ), "Sundanese"                  },
           { lng( 's','w' ), "Swahili"                    },
           { lng( 's','s' ), "Swati"                      },
           { lng( 's','v' ), "Swedish"                    },
           { lng( 't','l' ), "Tagalog"                    },
           { lng( 't','y' ), "Tahitian"                   },
           { lng( 't','g' ), "Tajik"                      },
           { lng( 't','a' ), "Tamil"                      },
           { lng( 't','t' ), "Tatar"                      },
           { lng( 't','e' ), "Telugu"                     },
           { lng( 't','h' ), "Thai"                       },
           { lng( 'b','o' ), "Tibetan"                    },
           { lng( 't','i' ), "Tigrinya"                   },
           { lng( 't','o' ), "Tonga"                      },
           { lng( 't','s' ), "Tsonga"                     },
           { lng( 't','n' ), "Tswana"                     },
           { lng( 't','r' ), "Turkish"                    },
           { lng( 't','k' ), "Turkmen"                    },
           { lng( 't','w' ), "Twi"                        },
           { lng( 'u','g' ), "Uighur"                     },
           { lng( 'u','k' ), "Ukrainian"                  },
           { lng( 'u','r' ), "Urdu"                       },
           { lng( 'u','z' ), "Uzbek"                      },
           { lng( 'v','i' ), "Vietnamese"                 },
           { lng( 'v','o' ), "Volapük"                    },
           { lng( 'c','y' ), "Welsh"                      },
           { lng( 'w','o' ), "Wolof"                      },
           { lng( 'x','h' ), "Xhosa"                      },
           { lng( 'y','i' ), "Yiddish"                    },
           { lng( 'j','i' ), "Yiddish"                    },
           { lng( 'y','o' ), "Yoruba"                     },
           { lng( 'z','a' ), "Zhuang"                     },
           { lng( 'z','u' ), "Zulu"                       },
         };
#undef lng

#ifdef CONFIG_DVDREAD
static char * ChannelTypes[] =
	{ "Dolby Digital","","Mpeg1","Mpeg2","PCM","","Digital Theatre System" };
static char * ChannelNumbers[] =
	{ "","Stereo","","","","5.1" };
#endif

static const char * GetLanguage( int language )
{
 unsigned int i;
 for ( i=0;i<sizeof( Languages ) / sizeof( Languages_t );i++ )
  if ( Languages[i].id == language ) return Languages[i].name;
 return NULL;
}


GtkWidget * DVDSubMenu;
GtkWidget * DVDTitleMenu;
GtkWidget * DVDChapterMenu;
GtkWidget * DVDAudioLanguageMenu;
GtkWidget * DVDSubtitleLanguageMenu;
GtkWidget * AspectMenu;
GtkWidget * VCDSubMenu;
GtkWidget * VCDTitleMenu;
GtkWidget * CDSubMenu;
GtkWidget * CDTitleMenu;

GtkWidget * create_PopUpMenu( void )
{
 GtkWidget * window1;
 GtkWidget * Menu = NULL;
 GtkWidget * SubMenu = NULL;
 GtkWidget * MenuItem = NULL;
 GtkWidget * H, * N, * D, * F;
 demuxer_t *demuxer = mpctx_get_demuxer(guiInfo.mpcontext);
 mixer_t *mixer = mpctx_get_mixer(guiInfo.mpcontext);
 int global_sub_size = mpctx_get_global_sub_size(guiInfo.mpcontext);

 Menu=gtk_menu_new();
 gtk_widget_realize (Menu);
 window1 = gtk_widget_get_toplevel(Menu);


  AddMenuItem( window1, (const char*)about_xpm, Menu,MSGTR_MENU_AboutMPlayer"     ", evAbout );
  AddSeparator( Menu );
   SubMenu=AddSubMenu( window1, (const char*)open_xpm, Menu,MSGTR_MENU_Open );
    AddMenuItem( window1, (const char*)file2_xpm, SubMenu,MSGTR_MENU_PlayFile"    ", evLoadPlay );
#ifdef CONFIG_CDDA
    AddMenuItem( window1, (const char*)playcd_xpm, SubMenu,MSGTR_MENU_PlayCD, evPlayCD );
    CDSubMenu=AddSubMenu( window1, (const char*)cd_xpm, Menu,MSGTR_MENU_CD );
    AddMenuItem( window1, (const char*)playcd_xpm, CDSubMenu,MSGTR_MENU_PlayDisc,evPlayCD );
    AddSeparator( CDSubMenu );
    CDTitleMenu=AddSubMenu( window1, (const char*)title_xpm, CDSubMenu,MSGTR_MENU_Titles );
    if ( guiInfo.Tracks && ( guiInfo.StreamType == STREAMTYPE_CDDA ) )
     {
      char tmp[32]; int i;
      for ( i=1;i <= guiInfo.Tracks;i++ )
       {
        snprintf( tmp,32,MSGTR_MENU_Title,i );
    //AddMenuItem( CDTitleMenu,tmp,( i << 16 ) + ivSetCDTrack );
        AddMenuCheckItem(window1, (const char*)empty1px_xpm, CDTitleMenu,tmp, guiInfo.Track == i, ( i << 16 ) + ivSetCDTrack );
       }
     }
     else
      {
       MenuItem=AddMenuItem( window1, (const char*)empty1px_xpm, CDTitleMenu,MSGTR_MENU_None,evNone );
       gtk_widget_set_sensitive( MenuItem,FALSE );
      }
#endif
#ifdef CONFIG_VCD
    AddMenuItem( window1, (const char*)playvcd_xpm, SubMenu,MSGTR_MENU_PlayVCD, evPlayVCD );
    VCDSubMenu=AddSubMenu( window1, (const char*)vcd_xpm, Menu,MSGTR_MENU_VCD );
    AddMenuItem( window1, (const char*)playvcd_xpm, VCDSubMenu,MSGTR_MENU_PlayDisc,evPlayVCD );
    AddSeparator( VCDSubMenu );
    VCDTitleMenu=AddSubMenu( window1, (const char*)title_xpm, VCDSubMenu,MSGTR_MENU_Titles );
    if ( guiInfo.Tracks && ( guiInfo.StreamType == STREAMTYPE_VCD ) )
     {
      char tmp[32]; int i;
      for ( i=1;i < guiInfo.Tracks;i++ )
       {
        snprintf( tmp,32,MSGTR_MENU_Title,i );
    //AddMenuItem( VCDTitleMenu,tmp,( i << 16 ) + ivSetVCDTrack );
        AddMenuCheckItem(window1, (const char*)empty1px_xpm, VCDTitleMenu,tmp, guiInfo.Track == i + 1, ( ( i + 1 ) << 16 ) + ivSetVCDTrack );
       }
     }
     else
      {
       MenuItem=AddMenuItem( window1, (const char*)empty1px_xpm, VCDTitleMenu,MSGTR_MENU_None,evNone );
       gtk_widget_set_sensitive( MenuItem,FALSE );
      }
#endif
#ifdef CONFIG_DVDREAD
    AddMenuItem( window1, (const char*)playdvd_xpm, SubMenu,MSGTR_MENU_PlayDVD, evPlayDVD );
    DVDSubMenu=AddSubMenu( window1, (const char*)dvd_xpm, Menu,MSGTR_MENU_DVD );
    AddMenuItem( window1, (const char*)playdvd_xpm, DVDSubMenu,MSGTR_MENU_PlayDisc"    ", evPlayDVD );
//    AddMenuItem( DVDSubMenu,MSGTR_MENU_ShowDVDMenu, evNone );
    AddSeparator( DVDSubMenu );
    DVDTitleMenu=AddSubMenu( window1, (const char*)title_xpm, DVDSubMenu,MSGTR_MENU_Titles );
     if ( guiInfo.Tracks && ( guiInfo.StreamType == STREAMTYPE_DVD ) )
      {
       char tmp[32]; int i;
       for ( i=1 ; i<= guiInfo.Tracks;i++ )
        {
         snprintf( tmp,32,MSGTR_MENU_Title,i);
         AddMenuCheckItem( window1, (const char*)empty1px_xpm, DVDTitleMenu,tmp,
			   guiInfo.Track == i,
			   (i << 16) + ivSetDVDTitle );
        }
      }
      else
       {
        MenuItem=AddMenuItem( window1, (const char*)empty1px_xpm, DVDTitleMenu,MSGTR_MENU_None,evNone );
        gtk_widget_set_sensitive( MenuItem,FALSE );
       }
    DVDChapterMenu=AddSubMenu( window1, (const char*)chapter_xpm, DVDSubMenu,MSGTR_MENU_Chapters );
     if ( guiInfo.Chapters && ( guiInfo.StreamType == STREAMTYPE_DVD ) )
      {
       char tmp[32]; int i;
       for ( i=1;i <= guiInfo.Chapters;i++ )
        {
         snprintf( tmp,32,MSGTR_MENU_Chapter,i );
         AddMenuCheckItem( window1, (const char*)empty1px_xpm, DVDChapterMenu,tmp,guiInfo.Chapter == i,
			   ( i << 16 ) + ivSetDVDChapter );
        }
      }
      else
       {
        MenuItem=AddMenuItem( window1, (const char*)empty1px_xpm, DVDChapterMenu,MSGTR_MENU_None,evNone );
        gtk_widget_set_sensitive( MenuItem,FALSE );
       }
    DVDAudioLanguageMenu=AddSubMenu( window1, (const char*)audiolang_xpm, DVDSubMenu,MSGTR_MENU_AudioLanguages );
     if ( guiInfo.AudioStreams && ( guiInfo.StreamType == STREAMTYPE_DVD ) )
      {
       char tmp[64]; int i, id = demuxer ? demuxer->audio->id : audio_id;
       for ( i=0;i < guiInfo.AudioStreams;i++ )
        {
	 snprintf( tmp,64,"%s - %s %s",GetLanguage( guiInfo.AudioStream[i].language ),
	   ChannelTypes[ guiInfo.AudioStream[i].type ],
	   ChannelNumbers[ guiInfo.AudioStream[i].channels ] );
//	 if ( id == -1 ) id=audio_id; //guiInfo.AudioStream[i].id;
         AddMenuCheckItem( window1, (const char*)dolby_xpm, DVDAudioLanguageMenu,tmp,
			   id == guiInfo.AudioStream[i].id,
			   ( guiInfo.AudioStream[i].id << 16 ) + ivSetDVDAudio );
        }
      }
      else
       {
        MenuItem=AddMenuItem( window1, (const char*)empty1px_xpm, DVDAudioLanguageMenu,MSGTR_MENU_None,evNone );
        gtk_widget_set_sensitive( MenuItem,FALSE );
       }
    DVDSubtitleLanguageMenu=AddSubMenu( window1, (const char*)sublang_xpm, DVDSubMenu,MSGTR_MENU_SubtitleLanguages );
     if ( guiInfo.Subtitles && ( guiInfo.StreamType == STREAMTYPE_DVD ) )
      {
       char tmp[64]; int i;
       AddMenuItem( window1, (const char*)empty1px_xpm, DVDSubtitleLanguageMenu,MSGTR_MENU_None,( (unsigned short)-1 << 16 ) + ivSetDVDSubtitle );
       for ( i=0;i < guiInfo.Subtitles;i++ )
        {
         av_strlcpy( tmp,GetLanguage( guiInfo.Subtitle[i].language ),sizeof(tmp) );
         AddMenuCheckItem( window1, (const char*)empty1px_xpm, DVDSubtitleLanguageMenu,tmp,
			   dvdsub_id == guiInfo.Subtitle[i].id,
			   ( guiInfo.Subtitle[i].id << 16 ) + ivSetDVDSubtitle );
        }
      }
      else
       {
        MenuItem=AddMenuItem( window1, (const char*)empty1px_xpm, DVDSubtitleLanguageMenu,MSGTR_MENU_None,evNone );
        gtk_widget_set_sensitive( MenuItem,FALSE );
       }
#endif
    AddMenuItem( window1, (const char*)url_xpm, SubMenu,MSGTR_MENU_PlayURL, evLoadURL );
    AddMenuItem( window1, (const char*)sub_xpm, SubMenu,MSGTR_MENU_LoadSubtitle"   ", evLoadSubtitle );
    AddMenuItem( window1, (const char*)nosub_xpm, SubMenu,MSGTR_MENU_DropSubtitle,evDropSubtitle );
    AddMenuItem( window1, (const char*)loadeaf_xpm, SubMenu,MSGTR_MENU_LoadExternAudioFile, evLoadAudioFile );
   SubMenu=AddSubMenu(window1, (const char*)play_xpm, Menu,MSGTR_MENU_Playing );
    AddMenuItem( window1, (const char*)play_xpm, SubMenu,MSGTR_MENU_Play"        ", evPlay );
    AddMenuItem( window1, (const char*)pause_xpm, SubMenu,MSGTR_MENU_Pause, evPause );
    AddMenuItem( window1, (const char*)stop_xpm, SubMenu,MSGTR_MENU_Stop, evStop );
    AddMenuItem( window1, (const char*)next_xpm, SubMenu,MSGTR_MENU_NextStream, evNext );
    AddMenuItem( window1, (const char*)prev_xpm, SubMenu,MSGTR_MENU_PrevStream, evPrev );
//    AddSeparator( SubMenu );
//    AddMenuItem( SubMenu,"Back 10 sec", evBackward10sec );
//    AddMenuItem( SubMenu,"Fwd 10 sec", evForward10sec );
//    AddMenuItem( SubMenu,"Back 1 min", evBackward1min );
//    AddMenuItem( SubMenu,"Fwd 1 min", evForward1min );
//   SubMenu=AddSubMenu( Menu,MSGTR_MENU_Size );
//    AddMenuItem( SubMenu,MSGTR_MENU_NormalSize"      ", evNormalSize );
//    AddMenuItem( SubMenu,MSGTR_MENU_DoubleSize, evDoubleSize );
//    AddMenuItem( SubMenu,MSGTR_MENU_FullScreen, evFullScreen );

//  if ( guiInfo.Playing )
   {
    AspectMenu=AddSubMenu( window1, (const char*)aspect_xpm, Menu,MSGTR_MENU_AspectRatio );
    AddMenuItem( window1, (const char*)aspect11_xpm, AspectMenu,MSGTR_MENU_Original,( 1 << 16 ) + evSetAspect );
    AddMenuItem( window1, (const char*)aspect169_xpm, AspectMenu,"16:9",( 2 << 16 ) + evSetAspect );
    AddMenuItem( window1, (const char*)aspect43_xpm, AspectMenu,"4:3",( 3 << 16 ) + evSetAspect );
    AddMenuItem( window1, (const char*)aspect235_xpm, AspectMenu,"2.35",( 4 << 16 ) + evSetAspect );
   }

  if ( guiInfo.Playing && demuxer && guiInfo.StreamType != STREAMTYPE_DVD )
   {
    int i,c = 0;

    for ( i=0;i < MAX_A_STREAMS;i++ )
     if ( demuxer->a_streams[i] ) c++;

    if ( c > 1 )
     {
      SubMenu=AddSubMenu( window1, (const char*)empty_xpm, Menu,MSGTR_MENU_AudioTrack );
      for ( i=0;i < MAX_A_STREAMS;i++ )
       if ( demuxer->a_streams[i] )
        {
         int aid = ((sh_audio_t *)demuxer->a_streams[i])->aid;
         char tmp[32];
         snprintf( tmp,32,MSGTR_MENU_Track,aid );
         AddMenuItem( window1, (const char*)empty1px_xpm, SubMenu,tmp,( aid << 16 ) + ivSetAudio );
        }
     }

    for ( c=0,i=0;i < MAX_V_STREAMS;i++ )
     if ( demuxer->v_streams[i] ) c++;

    if ( c > 1 )
     {
      SubMenu=AddSubMenu( window1, (const char*)empty_xpm, Menu,MSGTR_MENU_VideoTrack );
      for ( i=0;i < MAX_V_STREAMS;i++ )
       if ( demuxer->v_streams[i] )
        {
         int vid = ((sh_video_t *)demuxer->v_streams[i])->vid;
         char tmp[32];
         snprintf( tmp,32,MSGTR_MENU_Track,vid );
         AddMenuItem( window1, (const char*)empty1px_xpm, SubMenu,tmp,( vid << 16 ) + ivSetVideo );
        }
     }
   }

  /* cheap subtitle switching for non-DVD streams */
  if ( global_sub_size && guiInfo.StreamType != STREAMTYPE_DVD )
   {
    int i;
    SubMenu=AddSubMenu( window1, (const char*)empty_xpm, Menu, MSGTR_MENU_Subtitles );
    for ( i=0;i < global_sub_size;i++ )
     {
      char tmp[32];
      snprintf( tmp, 32, MSGTR_MENU_Track, i );
      AddMenuItem( window1,(const char*)empty1px_xpm,SubMenu,tmp,( i << 16 ) + ivSetSubtitle );
     }
   }

  AddSeparator( Menu );
  MenuItem=AddMenuCheckItem( window1, (const char*)sound_xpm, Menu,MSGTR_MENU_Mute,mixer->muted,evMute );
  if ( !guiInfo.AudioChannels ) gtk_widget_set_sensitive( MenuItem,FALSE );
  AddMenuItem( window1, (const char*)playlist_xpm, Menu,MSGTR_MENU_PlayList, evPlaylist );
  AddMenuItem( window1, (const char*)skin_xpm, Menu,MSGTR_MENU_SkinBrowser, evSkinBrowser );
  AddMenuItem( window1, (const char*)prefs_xpm, Menu,MSGTR_MENU_Preferences, evPreferences );
  AddMenuItem( window1, (const char*)equalizer_xpm, Menu,MSGTR_Equalizer, evEqualizer );

  if ( guiInfo.VideoWindow )
   {
    int b1 = 0, b2 = 0, b_half = 0;
    AddSeparator( Menu );
    if ( !guiApp.subWindow.isFullScreen && guiInfo.Playing )
     {
      if ( ( guiApp.subWindow.Width == guiInfo.VideoWidth * 2 )&&
           ( guiApp.subWindow.Height == guiInfo.VideoHeight * 2 ) ) b2=1;
      else if ( ( guiApp.subWindow.Width == guiInfo.VideoWidth / 2 ) &&
                ( guiApp.subWindow.Height == guiInfo.VideoHeight / 2 ) ) b_half=1;
      else b1=1;
     } else b1=!guiApp.subWindow.isFullScreen;
    H=AddMenuCheckItem( window1, (const char*)half_xpm, Menu,MSGTR_MENU_HalfSize,b_half,evHalfSize );
    N=AddMenuCheckItem( window1, (const char*)normal_xpm, Menu,MSGTR_MENU_NormalSize"      ",b1,evNormalSize );
    D=AddMenuCheckItem( window1, (const char*)double_xpm, Menu,MSGTR_MENU_DoubleSize,b2,evDoubleSize );
    F=AddMenuCheckItem( window1, (const char*)full_xpm, Menu,MSGTR_MENU_FullScreen,guiApp.subWindow.isFullScreen,evFullScreen );
  if ( !guiInfo.Playing )
   {
    gtk_widget_set_sensitive( H,FALSE );
    gtk_widget_set_sensitive( N,FALSE );
    gtk_widget_set_sensitive( D,FALSE );
    gtk_widget_set_sensitive( F,FALSE );
   }
   }

  AddSeparator( Menu );
  AddMenuItem( window1, (const char*)exit_xpm, Menu,MSGTR_MENU_Exit, evExit );

 return Menu;
}
