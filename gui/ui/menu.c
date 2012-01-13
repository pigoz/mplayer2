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
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "help_mp.h"
#include "mp_msg.h"
#include "gui/app.h"
#include "gmplayer.h"

#include "widgets.h"

unsigned char * menuDrawBuffer = NULL;
int             menuRender = 1;
int             menuItem = -1;
int             oldMenuItem = -1;
int             menuX,menuY;
static int      menuIsInitialized = 0;

static void uiMenuDraw( void )
{
 uint32_t * buf = NULL;
 uint32_t * drw = NULL;
 int             x,y,tmp;

 if ( !guiApp.menuIsPresent || !guiApp.menu.Bitmap.Image ) return;
 if ( !menuRender && !guiApp.menuWindow.Visible ) return;

 if ( menuRender || menuItem != oldMenuItem )
  {
   memcpy( menuDrawBuffer,guiApp.menu.Bitmap.Image,guiApp.menu.Bitmap.ImageSize );
// ---
   if ( menuItem != -1 )
    {
     buf=(uint32_t *)menuDrawBuffer;
     drw=(uint32_t *)guiApp.menuSelected.Bitmap.Image;
     for ( y=guiApp.menuItems[ menuItem ].y; y < guiApp.menuItems[ menuItem ].y + guiApp.menuItems[ menuItem ].height; y++ )
       for ( x=guiApp.menuItems[ menuItem ].x; x < guiApp.menuItems[ menuItem ].x + guiApp.menuItems[ menuItem ].width; x++ )
         {
          tmp=drw[ y * guiApp.menuSelected.width + x ];
          if ( !IS_TRANSPARENT ( tmp ) ) buf[ y * guiApp.menu.width + x ]=tmp;
         }
    }
   oldMenuItem=menuItem;
// ---
   wsConvert( &guiApp.menuWindow,menuDrawBuffer );
   menuRender=0;
  }
 wsPutImage( &guiApp.menuWindow );
}

void uiMenuMouseHandle( int RX,int RY )
{
 int x,y,i;

 if ( !guiApp.menu.Bitmap.Image ) return;

 menuItem=-1;
 x=RX - guiApp.menuWindow.X;
 y=RY - guiApp.menuWindow.Y;
 if ( ( x < 0 ) || ( y < 0  ) || ( x > guiApp.menu.width ) || ( y > guiApp.menu.height ) )
  {
   wsPostRedisplay( &guiApp.menuWindow );
   return;
  }

 for( i=0;i<=guiApp.IndexOfMenuItems;i++ )
  {
   if ( wgIsRect( x,y,
         guiApp.menuItems[i].x,guiApp.menuItems[i].y,
         guiApp.menuItems[i].x+guiApp.menuItems[i].width,guiApp.menuItems[i].y+guiApp.menuItems[i].height ) ) { menuItem=i; break; }
  }
 wsPostRedisplay( &guiApp.menuWindow );
}

void uiShowMenu( int mx,int my )
{
 int x,y;

 if ( !guiApp.menuIsPresent || !guiApp.menu.Bitmap.Image ) return;

 x=mx;
 if ( x + guiApp.menuWindow.Width > wsMaxX ) x=wsMaxX - guiApp.menuWindow.Width - 1 + wsOrgX;
 y=my;
 if ( y + guiApp.menuWindow.Height > wsMaxY ) y=wsMaxY - guiApp.menuWindow.Height - 1 + wsOrgY;

 menuX=x; menuY=y;

 menuItem = 0;

 wsMoveWindow( &guiApp.menuWindow,True,x,y );
 wsRaiseWindowTop( wsDisplay,guiApp.menuWindow.WindowID );
 wsSetLayer( wsDisplay,guiApp.menuWindow.WindowID,1 );
 menuRender=1;
 wsVisibleWindow( &guiApp.menuWindow,wsShowWindow );
 wsPostRedisplay( &guiApp.menuWindow );
}

void uiHideMenu( int mx,int my,int w )
{
 int x,y,i=menuItem;

 if ( !guiApp.menuIsPresent || !guiApp.menu.Bitmap.Image ) return;

 x=mx-menuX;
 y=my-menuY;
// x=RX - guiApp.menuWindow.X;
// y=RY - guiApp.menuWindow.Y;

 wsVisibleWindow( &guiApp.menuWindow,wsHideWindow );

 if ( ( x < 0 ) || ( y < 0 ) ) return;

// printf( "---------> %d %d,%d\n",i,x,y );
// printf( "--------> mi: %d,%d %dx%d\n",guiApp.menuItems[i].x,guiApp.menuItems[i].y,guiApp.menuItems[i].width,guiApp.menuItems[i].height );
 if ( wgIsRect( x,y,
        guiApp.menuItems[i].x,guiApp.menuItems[i].y,
        guiApp.menuItems[i].x+guiApp.menuItems[i].width,
        guiApp.menuItems[i].y+guiApp.menuItems[i].height ) )
   {
    uiEventHandling( guiApp.menuItems[i].message,(float)w );
   }
}

void uiMenuInit( void )
{

 if ( menuIsInitialized || !guiApp.menuIsPresent || !guiApp.menu.Bitmap.Image ) return;

 guiApp.menu.x=0;
 guiApp.menu.y=0;

 if ( ( menuDrawBuffer = calloc( 1,guiApp.menu.Bitmap.ImageSize ) ) == NULL )
  {
    mp_msg( MSGT_GPLAYER,MSGL_DBG2,MSGTR_NEMFMR );
   gtkMessageBox( GTK_MB_FATAL,MSGTR_NEMFMR );
   return;
  }

 wsCreateWindow( &guiApp.menuWindow,
 guiApp.menu.x,guiApp.menu.y,guiApp.menu.width,guiApp.menu.height,
 wsNoBorder,wsShowMouseCursor|wsHandleMouseButton|wsHandleMouseMove,wsOverredirect|wsHideFrame|wsMaxSize|wsMinSize|wsHideWindow,"MPlayer menu" );

 wsSetShape( &guiApp.menuWindow,guiApp.menu.Mask.Image );

 mp_msg( MSGT_GPLAYER,MSGL_DBG2,"[menu] menuWindow ID: 0x%x\n",(int)guiApp.menuWindow.WindowID );

 menuIsInitialized=1;
 guiApp.menuWindow.ReDraw=uiMenuDraw;
// guiApp.menuWindow.MouseHandler=uiMenuMouseHandle;
// guiApp.menuWindow.KeyHandler=uiMainKeyHandle;
 menuRender=1; wsPostRedisplay( &guiApp.menuWindow );
}
