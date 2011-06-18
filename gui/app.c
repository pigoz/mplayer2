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

#include "app.h"

#include "interface.h"
#include "skin/font.h"

#include "libavutil/common.h"

guiItems guiApp = {
    .IndexOfMainItems    = -1,
    .IndexOfPlaybarItems = -1,
    .IndexOfMenuItems    = -1
};

static const evName evNames[] = {
    { evNone,              "evNone"              },
    { evPlay,              "evPlay"              },
    { evStop,              "evStop"              },
    { evPause,             "evPause"             },
    { evPrev,              "evPrev"              },
    { evNext,              "evNext"              },
    { evLoad,              "evLoad"              },
    { evEqualizer,         "evEqualizer"         },
    { evPlayList,          "evPlaylist"          },
    { evExit,              "evExit"              },
    { evIconify,           "evIconify"           },
    { evIncBalance,        "evIncBalance"        }, // NOTE TO MYSELF: not all of these events
    { evDecBalance,        "evDecBalance"        }, // are actually implemented, and update doc
    { evFullScreen,        "evFullScreen"        },
    { evFName,             "evFName"             },
    { evMovieTime,         "evMovieTime"         },
    { evAbout,             "evAbout"             },
    { evLoadPlay,          "evLoadPlay"          },
    { evPreferences,       "evPreferences"       },
    { evSkinBrowser,       "evSkinBrowser"       },
    { evBackward10sec,     "evBackward10sec"     },
    { evForward10sec,      "evForward10sec"      },
    { evBackward1min,      "evBackward1min"      },
    { evForward1min,       "evForward1min"       },
    { evBackward10min,     "evBackward10min"     },
    { evForward10min,      "evForward10min"      },
    { evIncVolume,         "evIncVolume"         },
    { evDecVolume,         "evDecVolume"         },
    { evMute,              "evMute"              },
    { evIncAudioBufDelay,  "evIncAudioBufDelay"  },
    { evDecAudioBufDelay,  "evDecAudioBufDelay"  },
    { evPlaySwitchToPause, "evPlaySwitchToPause" },
    { evPauseSwitchToPlay, "evPauseSwitchToPlay" },
    { evNormalSize,        "evHalfSize"          },
    { evNormalSize,        "evNormalSize"        },
    { evDoubleSize,        "evDoubleSize"        },
    { evSetMoviePosition,  "evSetMoviePosition"  },
    { evSetVolume,         "evSetVolume"         },
    { evSetBalance,        "evSetBalance"        },
    { evHelp,              "evHelp"              },
    { evLoadSubtitle,      "evLoadSubtitle"      },
    { evPlayDVD,           "evPlayDVD"           },
    { evPlayVCD,           "evPlayVCD"           },
    { evSetURL,            "evSetURL"            },
    { evLoadAudioFile,     "evLoadAudioFile"     },
    { evDropSubtitle,      "evDropSubtitle"      },
    { evSetAspect,         "evSetAspect"         }
};

static void appClearItem(wItem *item)
{
    bpFree(&item->Bitmap);
    bpFree(&item->Mask);
    free(item->label);
    free(item->text);
    memset(item, 0, sizeof(*item));
}

void appFreeStruct(void)
{
    int i;

    appClearItem(&guiApp.main);
    guiApp.mainDecoration = 0;

    appClearItem(&guiApp.sub);

    appClearItem(&guiApp.playbar);
    guiApp.playbarIsPresent = 0;

    appClearItem(&guiApp.menu);
    appClearItem(&guiApp.menuSelected);
    guiApp.menuIsPresent = 0;

    for (i = 0; i <= guiApp.IndexOfMainItems; i++)
        appClearItem(&guiApp.mainItems[i]);
    for (i = 0; i <= guiApp.IndexOfPlaybarItems; i++)
        appClearItem(&guiApp.playbarItems[i]);
    for (i = 0; i <= guiApp.IndexOfMenuItems; i++)
        appClearItem(&guiApp.menuItems[i]);

    guiApp.IndexOfMainItems    = -1;
    guiApp.IndexOfPlaybarItems = -1;
    guiApp.IndexOfMenuItems    = -1;

    fntFreeFont();
}

int appFindMessage(unsigned char *str)
{
    unsigned int i;

    for (i = 0; i < FF_ARRAY_ELEMS(evNames); i++)
        if (!strcmp(evNames[i].name, str))
            return evNames[i].message;

    return -1;
}

void btnModify(int event, float state)
{
    int i;

    for (i = 0; i <= guiApp.IndexOfMainItems; i++) {
        if (guiApp.mainItems[i].message == event) {
            switch (guiApp.mainItems[i].type) {
            case itButton:
                guiApp.mainItems[i].pressed = (int)state;
                break;

            case itPotmeter:
            case itVPotmeter:
            case itHPotmeter:
                if (state < 0.0f)
                    state = 0.0f;
                if (state > 100.0f)
                    state = 100.0f;
                guiApp.mainItems[i].value = state;
                break;
            }
        }
    }

    for (i = 0; i <= guiApp.IndexOfPlaybarItems; i++) {
        if (guiApp.playbarItems[i].message == event) {
            switch (guiApp.playbarItems[i].type) {
            case itButton:
                guiApp.playbarItems[i].pressed = (int)state;
                break;

            case itPotmeter:
            case itVPotmeter:
            case itHPotmeter:
                if (state < 0.0f)
                    state = 0.0f;
                if (state > 100.0f)
                    state = 100.0f;
                guiApp.playbarItems[i].value = state;
                break;
            }
        }
    }
}

void btnSet(int event, int set)
{
    int i;

    for (i = 0; i <= guiApp.IndexOfMainItems; i++)
        if (guiApp.mainItems[i].message == event)
            guiApp.mainItems[i].pressed = set;

    for (i = 0; i <= guiApp.IndexOfPlaybarItems; i++)
        if (guiApp.playbarItems[i].message == event)
            guiApp.playbarItems[i].pressed = set;
}
