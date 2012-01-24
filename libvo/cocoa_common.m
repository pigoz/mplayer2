#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreServices/CoreServices.h> // for CGDisplayHideCursor
#include "cocoa_common.h"

#include "options.h"
#include "video_out.h"
#include "aspect.h"

#include "mp_fifo.h"
#include "talloc.h"

#include "input/input.h"
#include "input/keycodes.h"
#include "osx_common.h"
#include "mp_msg.h"

#define NSLeftAlternateKeyMask (0x000020 | NSAlternateKeyMask)
#define NSRightAlternateKeyMask (0x000040 | NSAlternateKeyMask)

@interface GLMPlayerWindow : NSWindow <NSWindowDelegate>
- (BOOL) canBecomeKeyWindow;
- (BOOL) canBecomeMainWindow;
- (void) fullscreen;
- (void) mouseEvent:(NSEvent *)theEvent;
- (void) mulSize:(float)multiplier;
- (void) setContentSize:(NSSize)newSize keepCentered:(BOOL)keepCentered;
@end

@interface GLMPlayerOpenGLView : NSOpenGLView
@end

struct create_window_args {
    uint32_t d_width;
    uint32_t d_height;
    uint32_t flags;
};

@interface MPlayerApplication : NSObject
- (void) createWindow:(NSData *) args;
- (void) destroyWindow;
@end

struct vo_cocoa_state {
    NSAutoreleasePool *pool;
    MPlayerApplication *app;
    GLMPlayerWindow *window;
    GLMPlayerOpenGLView *glView;
    NSOpenGLContext *glContext;

    NSSize current_video_size;
    NSSize previous_video_size;

    NSRect screen_frame;
    NSScreen *screen_handle;
    NSArray *screen_array;

    NSInteger windowed_mask;
    NSInteger fullscreen_mask;

    NSRect windowed_frame;

    NSString *window_title;

    NSInteger windowed_window_level;
    NSInteger fullscreen_window_level;

    int last_screensaver_update;

    int display_cursor;
    int cursor_timer;
    int cursor_autohide_delay;

    bool did_resize;
    bool out_fs_resize;

    int last_ret_val;
};

struct vo_cocoa_state *s = nil;

struct vo *l_vo;

// local function definitions
struct vo_cocoa_state *vo_cocoa_init_state(void);
void vo_set_level(int ontop);
void update_screen_info(void);
void resize_window(struct vo *vo);
void vo_cocoa_display_cursor(int requested_state);
void create_menu(void);

struct vo_cocoa_state *vo_cocoa_init_state(void)
{
    struct vo_cocoa_state *s = talloc_ptrtype(NULL, s);
    *s = (struct vo_cocoa_state){
        .did_resize = NO,
        .current_video_size = {0,0},
        .previous_video_size = {0,0},
        .windowed_mask = NSTitledWindowMask|NSClosableWindowMask|NSMiniaturizableWindowMask|NSResizableWindowMask,
        .fullscreen_mask = NSBorderlessWindowMask,
        .fullscreen_window_level = NSNormalWindowLevel + 1,
        .windowed_frame = {{0,0},{0,0}},
        .out_fs_resize = NO,
        .display_cursor = 1,
        .pool = nil,
        .window = nil,
        .glView = nil,
        .glContext = nil,
        .app = nil,
        .last_ret_val = 0
    };
    return s;
}

@implementation MPlayerApplication
- (void) createWindow:(NSData *) argsAsData
{
    struct create_window_args *args = (struct create_window_args *)[argsAsData bytes];
    struct MPOpts *opts = l_vo->opts;
    if (s->current_video_size.width > 0 || s->current_video_size.height > 0)
        s->previous_video_size = s->current_video_size;
    s->current_video_size = NSMakeSize(args->d_width, args->d_height);

    if (!(s->window || s->glContext)) { // keep using the same window
        s->window = [[GLMPlayerWindow alloc] initWithContentRect:NSMakeRect(0, 0, args->d_width, args->d_height)
                                             styleMask:s->windowed_mask
                                             backing:NSBackingStoreBuffered defer:NO];

        s->glView = [[GLMPlayerOpenGLView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];

        [s->window setContentView:s->glView];
        [s->glView release];
        [s->window setAcceptsMouseMovedEvents:YES];

        [NSApp setDelegate:s->window];
        [s->window setDelegate:s->window];
        [s->window setContentSize:s->current_video_size];
        [s->window setContentAspectRatio:s->current_video_size];
        [s->window center];

        if (args->flags & VOFLAG_HIDDEN) {
            [s->window orderOut:nil];
        } else {
            [s->window makeKeyAndOrderFront:nil];
        }

        if (args->flags & VOFLAG_FULLSCREEN)
            vo_cocoa_fullscreen(l_vo);

        vo_set_level(opts->vo_ontop);
    } else {
        if (s->current_video_size.width  != s->previous_video_size.width ||
            s->current_video_size.height != s->previous_video_size.height) {
            if (vo_fs) {
                // we will resize as soon as we get out of fullscreen
                s->out_fs_resize = YES;
            } else {
                // only if we are not in fullscreen and the video size did change
                // we actually resize the window and set a new aspect ratio
                [s->window setContentSize:s->current_video_size keepCentered:YES];
                [s->window setContentAspectRatio:s->current_video_size];
            }
        }
    }

    resize_window(l_vo);

    if (s->window_title)
        [s->window_title release];

    s->window_title = [[NSString alloc] initWithUTF8String:vo_get_window_title(l_vo)];
    [s->window setTitle: s->window_title];

    s->last_ret_val = 0;
    return;
}

- (void) destroyWindow
{
    CGDisplayShowCursor(kCGDirectMainDisplay);
    [s->window release];
    s->window = nil;
    [s->glContext release];
    s->glContext = nil;

    return;
}

@end

void start_cocoa_app(void)
{
    [[NSAutoreleasePool alloc] init];
    NSApplicationLoad();
    NSApp = [NSApplication sharedApplication];
    [NSApp setActivationPolicy: NSApplicationActivationPolicyRegular];
    create_menu();
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];
}

int vo_cocoa_init(struct vo *vo)
{
    l_vo = vo;
    s = vo_cocoa_init_state();
    s->pool = [[NSAutoreleasePool alloc] init];
    s->cursor_autohide_delay = l_vo->opts->cursor_autohide_delay;

    if (!s->app)
        s->app = [[MPlayerApplication alloc] init];

    return 1;
}

void vo_cocoa_uninit(struct vo *vo)
{
    [s->app performSelectorOnMainThread:@selector(destroyWindow)
            withObject:nil waitUntilDone:YES];
    [s->app release];
    s->app = nil;
    [s->pool release];
    s->pool = nil;

    talloc_free(s);
    s = nil;
}

void update_screen_info(void)
{
    s->screen_array = [NSScreen screens];
    if (xinerama_screen >= (int)[s->screen_array count]) {
        mp_msg(MSGT_VO, MSGL_INFO, "[cocoa] Device ID %d does not exist, falling back to main device\n", xinerama_screen);
        xinerama_screen = -1;
    }

    if (xinerama_screen < 0) { // default behaviour
        if (! (s->screen_handle = [s->window screen]) )
            s->screen_handle = [s->screen_array objectAtIndex:0];
    } else {
        s->screen_handle = [s->screen_array objectAtIndex:(xinerama_screen)];
    }

    s->screen_frame = [s->screen_handle frame];
}

void vo_cocoa_update_xinerama_info(struct vo *vo)
{
    update_screen_info();
    aspect_save_screenres(vo, s->screen_frame.size.width, s->screen_frame.size.height);
}

int vo_cocoa_change_attributes(struct vo *vo)
{
    return 0;
}

void resize_window(struct vo *vo)
{
    vo->dwidth = [[s->window contentView] frame].size.width;
    vo->dheight = [[s->window contentView] frame].size.height;
    [s->glContext update];
}

void vo_set_level(int ontop)
{
    if (ontop) {
        s->windowed_window_level = NSNormalWindowLevel + 1;
    } else {
        s->windowed_window_level = NSNormalWindowLevel;
    }

    if (!vo_fs)
        [s->window setLevel:s->windowed_window_level];
}

void vo_cocoa_ontop(struct vo *vo)
{
    struct MPOpts *opts = vo->opts;
    opts->vo_ontop = !opts->vo_ontop;
    vo_set_level(opts->vo_ontop);
}

int vo_cocoa_create_window(struct vo *vo, uint32_t d_width,
                           uint32_t d_height, uint32_t flags)
{
    [[NSAutoreleasePool alloc] init];
    l_vo = vo;

    // build NSData with arguments
    struct create_window_args args = { d_width, d_height, flags };
    [s->app performSelectorOnMainThread:@selector(createWindow:)
            withObject:[NSData dataWithBytes: &args length: sizeof(struct create_window_args)]
            waitUntilDone:YES];

    NSOpenGLPixelFormatAttribute attrs[] = {
        NSOpenGLPFADoubleBuffer, // double buffered
        NSOpenGLPFADepthSize, (NSOpenGLPixelFormatAttribute)16, // 16 bit depth buffer
        (NSOpenGLPixelFormatAttribute)0
    };

    NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    s->glContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];

    [s->glView setOpenGLContext:s->glContext];
    [s->glContext setView:s->glView];
    [s->glContext makeCurrentContext];

    return s->last_ret_val;
}

void vo_cocoa_swap_buffers()
{
    [s->glContext flushBuffer];
}

void vo_cocoa_display_cursor(int requested_state)
{
    if (requested_state) {
        if (!vo_fs || s->cursor_autohide_delay > -2) {
            s->display_cursor = requested_state;
            CGDisplayShowCursor(kCGDirectMainDisplay);
        }
    } else {
        if (s->cursor_autohide_delay != -1) {
            s->display_cursor = requested_state;
            CGDisplayHideCursor(kCGDirectMainDisplay);
        }
    }
}

int vo_cocoa_check_events(struct vo *vo)
{
    if (s->did_resize) {
        s->did_resize = NO;
        resize_window(vo);
        return VO_EVENT_RESIZE;
    }

    float curTime = TickCount()/60;
    int msCurTime = (int) (curTime * 1000);

    // automatically hide mouse cursor
    if (vo_fs && s->display_cursor &&
        (msCurTime - s->cursor_timer >= s->cursor_autohide_delay)) {
        vo_cocoa_display_cursor(0);
        s->cursor_timer = msCurTime;
    }

    //update activity every 30 seconds to prevent
    //screensaver from starting up.
    if ((int)curTime - s->last_screensaver_update >= 30 || s->last_screensaver_update == 0)
    {
        UpdateSystemActivity(UsrActivity);
        s->last_screensaver_update = (int)curTime;
    }
    return 0;
}

void vo_cocoa_fullscreen(struct vo *vo)
{
    // this must happen in the main thread since it modifies the NSWindow flags
    [s->window performSelectorOnMainThread:@selector(fullscreen)
               withObject:nil waitUntilDone:YES];
    resize_window(vo);
}

int vo_cocoa_swap_interval(int enabled)
{
    [s->glContext setValues:&enabled forParameter:NSOpenGLCPSwapInterval];
    return 0;
}

void create_menu()
{
    NSMenu *menu;
    NSMenuItem *menuItem;

    menu = [[NSMenu new] autorelease];
    menuItem = [[NSMenuItem new] autorelease];
    [menu addItem: menuItem];
    [NSApp setMainMenu: menu];

    menu = [[NSMenu alloc] initWithTitle:@"Movie"];
    menuItem = [[NSMenuItem alloc] initWithTitle:@"Half Size" action:@selector(halfSize) keyEquivalent:@"0"]; [menu addItem:menuItem];
    menuItem = [[NSMenuItem alloc] initWithTitle:@"Normal Size" action:@selector(normalSize) keyEquivalent:@"1"]; [menu addItem:menuItem];
    menuItem = [[NSMenuItem alloc] initWithTitle:@"Double Size" action:@selector(doubleSize) keyEquivalent:@"2"]; [menu addItem:menuItem];

    menuItem = [[NSMenuItem alloc] initWithTitle:@"Movie" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:menu];
    [[NSApp mainMenu] addItem:menuItem];

    [menu release];
    [menuItem release];
}

@implementation GLMPlayerWindow
- (void) fullscreen
{
    if (!vo_fs) {
        [NSApp setPresentationOptions:NSApplicationPresentationHideDock|NSApplicationPresentationHideMenuBar];
        s->windowed_frame = [self frame];
        [self setHasShadow:NO];
        [self setStyleMask:s->fullscreen_mask];
        [self setFrame:s->screen_frame display:YES animate:NO];
        [self setLevel:s->fullscreen_window_level];
        vo_fs = VO_TRUE;
        vo_cocoa_display_cursor(0);
        [self setMovableByWindowBackground: NO];
    } else {
        [NSApp setPresentationOptions:NSApplicationPresentationDefault];
        [self setHasShadow:YES];
        [self setStyleMask:s->windowed_mask];
        [self setTitle:s->window_title];
        [self setFrame:s->windowed_frame display:YES animate:NO];
        if (s->out_fs_resize) {
            [self setContentSize:s->current_video_size keepCentered:YES];
            s->out_fs_resize = NO;
        }
        [self setContentAspectRatio:s->current_video_size];
        [self setLevel:s->windowed_window_level];
        vo_fs = VO_FALSE;
        vo_cocoa_display_cursor(1);
        [self setMovableByWindowBackground: YES];
    }
}

- (BOOL) canBecomeMainWindow { return YES; }
- (BOOL) canBecomeKeyWindow { return YES; }
- (BOOL) acceptsFirstResponder { return YES; }
- (BOOL) becomeFirstResponder { return YES; }
- (BOOL) resignFirstResponder { return YES; }

- (BOOL) isMovableByWindowBackground
{
    // this is only valid as a starting value. it will be rewritten in the -fullscreen method.
    return !vo_fs;
}

- (void) keyDown:(NSEvent *)theEvent
{
    unsigned char charcode;
    if (([theEvent modifierFlags] & NSRightAlternateKeyMask) == NSRightAlternateKeyMask)
        charcode = *[[theEvent characters] UTF8String];
    else
        charcode = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];

    int key = convert_key([theEvent keyCode], charcode);

    if (key > -1) {
        if ([theEvent modifierFlags] & NSShiftKeyMask)
            key |= KEY_MODIFIER_SHIFT;
        if ([theEvent modifierFlags] & NSControlKeyMask)
            key |= KEY_MODIFIER_CTRL;
        if (([theEvent modifierFlags] & NSLeftAlternateKeyMask) == NSLeftAlternateKeyMask)
            key |= KEY_MODIFIER_ALT;
        if ([theEvent modifierFlags] & NSCommandKeyMask)
            key |= KEY_MODIFIER_META;
        mplayer_put_key(l_vo->key_fifo, key);
    }
}

- (void) mouseMoved: (NSEvent *) theEvent
{
    if (vo_fs)
        vo_cocoa_display_cursor(1);
}

- (void) mouseDragged:(NSEvent *)theEvent
{
    [self mouseEvent: theEvent];
}

- (void) mouseDown:(NSEvent *)theEvent
{
    [self mouseEvent: theEvent];
}

- (void) mouseUp:(NSEvent *)theEvent
{
    [self mouseEvent: theEvent];
}

- (void) rightMouseDown:(NSEvent *)theEvent
{
    [self mouseEvent: theEvent];
}

- (void) rightMouseUp:(NSEvent *)theEvent
{
    [self mouseEvent: theEvent];
}

- (void) otherMouseDown:(NSEvent *)theEvent
{
    [self mouseEvent: theEvent];
}

- (void) otherMouseUp:(NSEvent *)theEvent
{
    [self mouseEvent: theEvent];
}

- (void) scrollWheel:(NSEvent *)theEvent
{
    if ([theEvent deltaY] > 0)
        mplayer_put_key(l_vo->key_fifo, MOUSE_BTN3);
    else
        mplayer_put_key(l_vo->key_fifo, MOUSE_BTN4);
}

- (void) mouseEvent:(NSEvent *)theEvent
{
    if ( [theEvent buttonNumber] >= 0 && [theEvent buttonNumber] <= 9 )
    {
        int buttonNumber = [theEvent buttonNumber];
        // Fix to mplayer defined button order: left, middle, right
        if (buttonNumber == 1)
            buttonNumber = 2;
        else if (buttonNumber == 2)
            buttonNumber = 1;
        switch ([theEvent type]) {
            case NSLeftMouseDown:
                if ([theEvent clickCount] == 2)
                    vo_cocoa_fullscreen(l_vo);
                break;
            case NSRightMouseDown:
            case NSOtherMouseDown:
                mplayer_put_key(l_vo->key_fifo, (MOUSE_BTN0 + buttonNumber) | MP_KEY_DOWN);
                break;
            case NSLeftMouseUp:
                break;
            case NSRightMouseUp:
            case NSOtherMouseUp:
                mplayer_put_key(l_vo->key_fifo, MOUSE_BTN0 + buttonNumber);
                break;
        }
    }
}

- (void) applicationWillBecomeActive:(NSNotification *)aNotification
{
    if (vo_fs) {
        [s->window setLevel:s->fullscreen_window_level];
        [NSApp setPresentationOptions:NSApplicationPresentationHideDock|NSApplicationPresentationHideMenuBar];
        [s->window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps: YES];
    }
}

- (void) applicationWillResignActive:(NSNotification *)aNotification
{
    if (vo_fs) {
        [s->window setLevel:s->windowed_window_level];
        [NSApp setPresentationOptions:NSApplicationPresentationDefault];
    }
}

- (void) normalSize
{
    if (!vo_fs)
      [self setContentSize:s->current_video_size keepCentered:YES];
}

- (void) halfSize { [self mulSize:0.5f];}

- (void) doubleSize { [self mulSize:2.0f];}

- (void) mulSize:(float)multiplier
{
    if (!vo_fs) {
        NSSize size = [[self contentView] frame].size;
        size.width  = s->current_video_size.width  * (multiplier);
        size.height = s->current_video_size.height * (multiplier);
        [self setContentSize:size keepCentered:YES];
    }
}

- (void) setContentSize:(NSSize)ns keepCentered:(BOOL)keepCentered
{
    if (keepCentered) {
        NSRect nf = [self frame];
        NSRect vf = [[self screen] visibleFrame];
        int title_height = nf.size.height - [[self contentView] bounds].size.height;
        double ratio = (double)ns.width / (double)ns.height;

        // clip the new size to the visibleFrame's size if needed
        if (ns.width > vf.size.width || ns.height + title_height > vf.size.height) {
            ns = vf.size;
            ns.height -= title_height; // make space for the title bar

            if (ns.width > ns.height) {
                ns.height = ((double)ns.width * 1/ratio + 0.5);
            } else {
                ns.width = ((double)ns.height * ratio + 0.5);
            }
        }

        int dw = nf.size.width - ns.width;
        int dh = nf.size.height - ns.height - title_height;

        nf.origin.x += dw / 2;
        nf.origin.y += dh / 2;

        [self setFrame: NSMakeRect(nf.origin.x, nf.origin.y, ns.width, ns.height + title_height) display:YES animate:NO];
    } else {
        [self setContentSize:ns];
    }
}
@end

@implementation GLMPlayerOpenGLView
- (void) drawRect: (NSRect)rect
{
    [[NSColor clearColor] set];
    NSRectFill([self bounds]);
    s->did_resize = YES;
}
@end
