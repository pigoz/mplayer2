.. vim: set et sts=4 sw=4:
.. mplayer2 © 2000-2011 mplayer2 Team
.. This man page was/is done by Gabucino, Diego Biurrun, Jonas Jermann.

========
mplayer2
========

------------
movie player
------------

:Manual section: 1


SYNOPSIS
========

| **mplayer** [options] [file|URL|playlist|-]
| **mplayer** [options] file1 [specific options] [file2] [specific options]
| **mplayer** [options] {group of files and options} [group-specific options]
| **mplayer** [br]://[title][/device] [options]
| **mplayer** [dvd|dvdnav]://[title|[start\_title]-end\_title][/device] [options]
| **mplayer** \vcd://track[/device]
| **mplayer** \tv://[channel][/input_id] [options]
| **mplayer** radio://[channel|frequency][/capture] [options]
| **mplayer** \pvr:// [options]
| **mplayer** \dvb://[card\_number@]channel [options]
| **mplayer** \mf://[filemask|\@listfile] [-mf options] [options]
| **mplayer** [cdda|cddb]://track[-endtrack][:speed][/device] [options]
| **mplayer** \cue://file[:track] [options]
| **mplayer** [file|mms[t]|http|http\_proxy|rt[s]p|ftp|udp|unsv|icyx|noicyx|smb]:// [user:pass\@]URL[:port] [options]
| **mplayer** \sdp://file [options]
| **mplayer** \mpst://host[:port]/URL [options]
| **mplayer** \tivo://host/[list|llist|fsid] [options]


DESCRIPTION
===========

**mplayer** is a movie player for Linux (runs on many other platforms and CPU
architectures, see the documentation). It supports a wide variety of video
file formats, audio and video codecs, and subtitle types. Special input URL
types are available to read input from a variety of sources other than disk
files. Depending on platform, a variety of different video and audio output
methods are supported.

Usage examples to get you started quickly can be found at the end of this man
page.


INTERACTIVE CONTROL
===================

MPlayer has a fully configurable, command-driven control layer which allows you
to control MPlayer using keyboard, mouse, joystick or remote control (with
LIRC). See the ``--input`` option for ways to customize it.

keyboard control
----------------

LEFT and RIGHT
    Seek backward/forward 10 seconds. Shift+arrow does a 1 second exact seek
    (see ``--hr-seek``; currently modifier keys like shift only work if used in
    an X output window).

UP and DOWN
    Seek forward/backward 1 minute. Shift+arrow does a 5 second exact seek (see
    ``--hr-seek``; currently modifier keys like shift only work if used in an X
    output window).

PGUP and PGDWN
    Seek forward/backward 10 minutes.

[ and ]
    Decrease/increase current playback speed by 10%.

{ and }
    Halve/double current playback speed.

BACKSPACE
    Reset playback speed to normal.

< and >
    Go backward/forward in the playlist.

ENTER
    Go forward in the playlist, even over the end.

HOME and END
    next/previous playtree entry in the parent list

INS and DEL (ASX playlist only)
    next/previous alternative source.

p / SPACE
    Pause (pressing again unpauses).

.
    Step forward. Pressing once will pause movie, every consecutive press will
    play one frame and then go into pause mode again.

q / ESC
    Stop playing and quit.

U
    Stop playing (and quit if ``--idle`` is not used).

\+ and -
    Adjust audio delay by +/- 0.1 seconds.

/ and *
    Decrease/increase volume.

9 and 0
    Decrease/increase volume.

( and )
    Adjust audio balance in favor of left/right channel.

m
    Mute sound.

\_ (MPEG-TS, AVI and libavformat only)
    Cycle through the available video tracks.

\# (DVD, Blu-ray, MPEG, Matroska, AVI and libavformat only)
    Cycle through the available audio tracks.

TAB (MPEG-TS and libavformat only)
    Cycle through the available programs.

f
    Toggle fullscreen (see also ``--fs``).

T
    Toggle stay-on-top (see also ``--ontop``).

w and e
    Decrease/increase pan-and-scan range.

o
    Toggle OSD states: none / seek / seek + timer / seek + timer + total time.

d
    Toggle frame dropping states: none / skip display / skip decoding (see
    ``--framedrop`` and ``--hardframedrop``).

v
    Toggle subtitle visibility.

j and J
    Cycle through the available subtitles.

y and g
    Step forward/backward in the subtitle list.

F
    Toggle displaying "forced subtitles".

a
    Toggle subtitle alignment: top / middle / bottom.

x and z
    Adjust subtitle delay by +/- 0.1 seconds.

V
    Toggle subtitle VSFilter aspect compatibility mode. See
    ``--ass-vsfilter-aspect-compat`` for more info.

C (``--capture`` only)
    Start/stop capturing the primary stream.

r and t
    Move subtitles up/down.

i (``--edlout`` mode only)
    Set start or end of an EDL skip and write it out to the given file.

s (``--vf`` screenshot only)
    Take a screenshot.

S (``--vf`` screenshot only)
    Start/stop taking screenshots.

I
    Show filename on the OSD.

P
    Show progression bar, elapsed time and total duration on the OSD.

! and @
    Seek to the beginning of the previous/next chapter.

D (``--vo=vdpau``, ``--vf=yadif``, ``--vf=kerndeint`` only)
    Activate/deactivate deinterlacer.

A
    Cycle through the available DVD angles.

c (currently ``--vo=vdpau`` and ``--vo=xv`` only)
    Change YUV colorspace.

(The following keys are valid only when using a video output that supports the
corresponding adjustment, the software equalizer (``--vf=eq`` or ``--vf=eq2``)
or hue filter (``--vf=hue``).)

1 and 2
    Adjust contrast.

3 and 4
    Adjust brightness.

5 and 6
    Adjust hue.

7 and 8
    Adjust saturation.

(The following keys are valid only when using the quartz or corevideo video
output driver.)

command + 0
    Resize movie window to half its original size.

command + 1
    Resize movie window to its original size.

command + 2
    Resize movie window to double its original size.

command + f
    Toggle fullscreen (see also ``--fs``).

command + [ and command + ]
    Set movie window alpha.

(The following keys are valid only when using the sdl video output driver.)

c
    Cycle through available fullscreen modes.

n
    Restore original mode.

(The following keys are valid if you have a keyboard with multimedia keys.)

PAUSE
    Pause.

STOP
    Stop playing and quit.

PREVIOUS and NEXT
    Seek backward/forward 1 minute.

(The following keys are only valid if you compiled with TV or DVB input
support and will take precedence over the keys defined above.)

h and k
    Select previous/next channel.

n
    Change norm.

u
    Change channel list.

(The following keys are only valid if you compiled with dvdnav support: They
are used to navigate the menus.)

keypad 8
    Select button up.

keypad 2
    Select button down.

keypad 4
    Select button left.

keypad 6
    Select button right.

keypad 5
    Return to main menu.

keypad 7
    Return to nearest menu (the order of preference is: chapter->title->root).

keypad ENTER
    Confirm choice.

(The following keys are used for controlling TV teletext. The data may come
from either an analog TV source or an MPEG transport stream.)

X
    Switch teletext on/off.

Q and W
    Go to next/prev teletext page.

mouse control
-------------

button 3 and button 4
    Seek backward/forward 1 minute.

button 5 and button 6
    Decrease/increase volume.

joystick control
----------------

left and right
    Seek backward/forward 10 seconds.

up and down
    Seek forward/backward 1 minute.

button 1
    Pause.

button 2
    Toggle OSD states: none / seek / seek + timer / seek + timer + total time.

button 3 and button 4
    Decrease/increase volume.


USAGE
=====

Every *flag* option has a *no-flag* counterpart, e.g. the opposite of the
``--fs`` option is ``--no-fs``. ``--fs=yes`` is same as ``--fs``, ``--fs=no``
is the same as ``--no-fs``.

If an option is marked as *(XXX only)*, it will only work in combination with
the *XXX* option or if *XXX* is compiled in.

| *NOTE*: The suboption parser (used for example for ``--ao=pcm`` suboptions)
  supports a special kind of string-escaping intended for use with external
  GUIs.
| It has the following format:
| %n%string\_of\_length\_n
| *EXAMPLES*:
| `mplayer --ao pcm:file=%10%C:test.wav test.avi`
| Or in a script:
| `mplayer --ao pcm:file=%\`expr length "$NAME"\`%"$NAME" test.avi`


CONFIGURATION FILES
===================

You can put all of the options in configuration files which will be read every
time MPlayer is run. The system-wide configuration file 'mplayer.conf' is in
your configuration directory (e.g. ``/etc/mplayer`` or
``/usr/local/etc/mplayer``), the user specific one is ``~/.mplayer/config``.
User specific options override system-wide options and options given on the
command line override either. The syntax of the configuration files is
``option=<value>``, everything after a *#* is considered a comment. Options
that work without values can be enabled by setting them to *yes* or *1* or
*true* and disabled by setting them to *no* or *0* or *false*. Even suboptions
can be specified in this way.

You can also write file-specific configuration files. If you wish to have a
configuration file for a file called 'movie.avi', create a file named
'movie.avi.conf' with the file-specific options in it and put it in
``~/.mplayer/``. You can also put the configuration file in the same directory
as the file to be played, as long as you give the ``--use-filedir-conf``
option (either on the command line or in your global config file). If a
file-specific configuration file is found in the same directory, no
file-specific configuration is loaded from ``~/.mplayer``. In addition, the
``--use-filedir-conf`` option enables directory-specific configuration files.
For this, MPlayer first tries to load a mplayer.conf from the same directory
as the file played and then tries to load any file-specific configuration.

*EXAMPLE MPLAYER CONFIGURATION FILE:*

| # Use Matrox driver by default.
| vo=xmga
| # I love practicing handstands while watching videos.
| flip=yes
| # Decode multiple files from PNG,
| # start with mf://filemask
| mf=type=png:fps=25
| # Eerie negative images are cool.
| vf=eq2=1.0:-0.8


PROFILES
========

To ease working with different configurations profiles can be defined in the
configuration files. A profile starts with its name between square brackets,
e.g. *[my-profile]*. All following options will be part of the profile. A
description (shown by ``--profile=help``) can be defined with the profile-desc
option. To end the profile, start another one or use the profile name
*default* to continue with normal options.

*EXAMPLE MPLAYER PROFILE:*

| [protocol.dvd]
| profile-desc="profile for dvd:// streams"
| vf=pp=hb/vb/dr/al/fd
| alang=en
|
| [protocol.dvdnav]
| profile-desc="profile for dvdnav:// streams"
| profile=protocol.dvd
| mouse-movements=yes
| nocache=yes
|
| [extension.flv]
| profile-desc="profile for .flv files"
| flip=yes
|
| [vo.pnm]
| outdir=/tmp
|
| [ao.alsa]
| device=spdif


GENERAL OPTIONS
===============



PLAYER OPTIONS
==============



DEMUXER/STREAM OPTIONS
======================



OSD/SUBTITLE OPTIONS
====================

*NOTE*: See also ``--vf=expand``.



AUDIO OUTPUT OPTIONS
====================

AUDIO OUTPUT DRIVERS
====================

Audio output drivers are interfaces to different audio output facilities. The
syntax is:

--ao=<driver1[:suboption1[=value]:...],driver2,...[,]>
    Specify a priority list of audio output drivers to be used.

If the list has a trailing ',' MPlayer will fall back on drivers not contained
in the list. Suboptions are optional and can mostly be omitted.

*NOTE*: See ``--ao=help`` for a list of compiled-in audio output drivers.

*EXAMPLE*:

    - ``--ao=alsa,oss,`` Try the ALSA driver, then the OSS driver, then others.
    - ``--ao=alsa:noblock:device=hw=0.3`` Sets noblock-mode and the device-name
      as first card, fourth device.

Available audio output drivers are:

alsa
    ALSA 0.9/1.x audio output driver

    noblock
        Sets noblock-mode.
    device=<device>
        Sets the device name. Replace any ',' with '.' and any ':' with '=' in
        the ALSA device name. For hwac3 output via S/PDIF, use an "iec958" or
        "spdif" device, unless you really know how to set it correctly.

alsa5
    ALSA 0.5 audio output driver

oss
    OSS audio output driver

    <dsp-device>
        Sets the audio output device (default: ``/dev/dsp``).
    <mixer-device>
        Sets the audio mixer device (default: ``/dev/mixer``).
    <mixer-channel>
        Sets the audio mixer channel (default: pcm).

sdl (SDL only)
    highly platform independent SDL (Simple Directmedia Layer) library audio
    output driver

    <driver>
        Explicitly choose the SDL audio driver to use (default: let SDL
        choose).

arts
    audio output through the aRts daemon

esd
    audio output through the ESD daemon

    <server>
        Explicitly choose the ESD server to use (default: localhost).

jack
    audio output through JACK (Jack Audio Connection Kit)

    port=<name>
        Connects to the ports with the given name (default: physical ports).
    name=<client
        Client name that is passed to JACK (default: MPlayer [<PID>]). Useful
        if you want to have certain connections established automatically.
    (no-)estimate
        Estimate the audio delay, supposed to make the video playback smoother
        (default: enabled).
    (no-)autostart
        Automatically start jackd if necessary (default: disabled). Note that
        this seems unreliable and will spam stdout with server messages.

nas
    audio output through NAS

coreaudio (Mac OS X only)
    native Mac OS X audio output driver

    device_id=<id>
        ID of output device to use (0 = default device)
    help
        List all available output devices with their IDs.

openal
    Experimental OpenAL audio output driver

pulse
    PulseAudio audio output driver

    [<host>][:<output sink>]
        Specify the host and optionally output sink to use. An empty <host>
        string uses a local connection, "localhost" uses network transfer
        (most likely not what you want).

sgi (SGI only)
    native SGI audio output driver

    <output device name>
        Explicitly choose the output device/interface to use (default:
        system-wide default). For example, 'Analog Out' or 'Digital Out'.

sun (Sun only)
    native Sun audio output driver

    <device>
        Explicitly choose the audio device to use (default: ``/dev/audio``).

win32 (Windows only)
    native Windows waveout audio output driver

dsound (Windows only)
    DirectX DirectSound audio output driver

    device=<devicenum>
        Sets the device number to use. Playing a file with ``-v`` will show a
        list of available devices.

kai (OS/2 only)
    OS/2 KAI audio output driver

    uniaud
        Force UNIAUD mode.
    dart
        Force DART mode.
    (no-)share
        Open audio in shareable or exclusive mode.
    bufsize=<size>
        Set buffer size to <size> in samples (default: 2048).

dart (OS/2 only)
    OS/2 DART audio output driver

    (no-)share
        Open DART in shareable or exclusive mode.
    bufsize=<size>
        Set buffer size to <size> in samples (default: 2048).

ivtv (IVTV only)
    IVTV specific MPEG audio output driver. Works with ``--ac=hwmpa`` only.

v4l2 (requires Linux 2.6.22+ kernel)
    Audio output driver for V4L2 cards with hardware MPEG decoder.

mpegpes (DVB only)
    Audio output driver for DVB cards that writes the output to an MPEG-PES
    file if no DVB card is installed.

    card=<1-4>
        DVB card to use if more than one card is present. If not specified
        MPlayer will search the first usable card.
    file=<filename>
        output filename

null
    Produces no audio output but maintains video playback speed. Use
    ``--nosound`` for benchmarking.

pcm
    raw PCM/wave file writer audio output

    (no-)waveheader
        Include or do not include the wave header (default: included). When
        not included, raw PCM will be generated.
    file=<filename>
        Write the sound to <filename> instead of the default
        ``audiodump.wav``. If nowaveheader is specified, the default is
        ``audiodump.pcm``.

rsound
    audio output to an RSound daemon

    host=<name/path>
        Set the address of the server (default: localhost).  Can be either a
        network hostname for TCP connections or a Unix domain socket path
        starting with '/'.
    port=<number>
        Set the TCP port used for connecting to the server (default: 12345).
        Not used if connecting to a Unix domain socket.

plugin
    plugin audio output driver


VIDEO OUTPUT OPTIONS
====================



VIDEO OUTPUT DRIVERS
====================

Video output drivers are interfaces to different video output facilities. The
syntax is:

--vo=<driver1[:suboption1[=value]:...],driver2,...[,]>
    Specify a priority list of video output drivers to be used.

If the list has a trailing ',' MPlayer will fall back on drivers not contained
in the list. Suboptions are optional and can mostly be omitted.

*NOTE*: See ``--vo=help`` for a list of compiled-in video output drivers.

*EXAMPLE*:

    ``--vo=xmga,xv,``
        Try the Matrox X11 driver, then the Xv driver, then others.
    ``--vo=directx:noaccel``
        Uses the DirectX driver with acceleration features turned off.

Available video output drivers are:

xv (X11 only)
    Uses the XVideo extension to enable hardware accelerated playback. If you
    cannot use a hardware specific driver, this is probably the best option.
    For information about what colorkey is used and how it is drawn run
    MPlayer with ``-v`` option and look out for the lines tagged with ``[xv
    common]`` at the beginning.

    adaptor=<number>
        Select a specific XVideo adaptor (check xvinfo results).
    port=<number>
        Select a specific XVideo port.
    ck=<cur|use|set>
        Select the source from which the colorkey is taken (default: cur).

        cur
          The default takes the colorkey currently set in Xv.
        use
          Use but do not set the colorkey from MPlayer (use the ``--colorkey``
          option to change it).
        set
          Same as use but also sets the supplied colorkey.

    ck-method=<man|bg|auto>
        Sets the colorkey drawing method (default: man).

        man
          Draw the colorkey manually (reduces flicker in some cases).
        bg
          Set the colorkey as window background.
        auto
          Let Xv draw the colorkey.

x11 (X11 only)
    Shared memory video output driver without hardware acceleration that works
    whenever X11 is present.

xover (X11 only)
    Adds X11 support to all overlay based video output drivers. Currently only
    supported by tdfx_vid.

    <vo_driver>
        Select the driver to use as source to overlay on top of X11.

vdpau (X11 only)
    Uses the VDPAU interface to display and optionally also decode video.
    Hardware decoding is used with ``--vc=ffmpeg12vdpau``,
    ``--vc=ffwmv3vdpau``, ``--vc=ffvc1vdpau``, ``--vc=ffh264vdpau`` or
    ``--vc=ffodivxvdpau``.

    sharpen=<-1-1>
        For positive values, apply a sharpening algorithm to the video, for
        negative values a blurring algorithm (default: 0).
    denoise=<0-1>
        Apply a noise reduction algorithm to the video (default: 0, no noise
        reduction).
    deint=<-4-4>
        Select deinterlacing mode (default: -3). Positive values choose mode
        and enable deinterlacing. Corresponding negative values select the
        same deinterlacing mode, but do not enable deinterlacing on startup
        (useful in configuration files to specify what mode will be enabled by
        the "D" key). All modes respect ``--field-dominance``.

        0
            same as -3
        1
            Show only first field, similar to ``--vf=field``.
        2
            Bob deinterlacing, similar to ``--vf=tfields=1``.
        3
            motion adaptive temporal deinterlacing. May lead to A/V desync
            with slow video hardware and/or high resolution.
        4
            motion adaptive temporal deinterlacing with edge-guided spatial
            interpolation. Needs fast video hardware.
    chroma-deint
        Makes temporal deinterlacers operate both on luma and chroma (default).
        Use nochroma-deint to solely use luma and speed up advanced
        deinterlacing. Useful with slow video memory.
    pullup
        Try to apply inverse telecine, needs motion adaptive temporal
        deinterlacing.
    hqscaling=<0-9>
        0
            Use default VDPAU scaling (default).
        1-9
            Apply high quality VDPAU scaling (needs capable hardware).
    fps=<number>
        Override autodetected display refresh rate value (the value is needed
        for framedrop to allow video playback rates higher than display
        refresh rate, and for vsync-aware frame timing adjustments). Default 0
        means use autodetected value. A positive value is interpreted as a
        refresh rate in Hz and overrides the autodetected value. A negative
        value disables all timing adjustment and framedrop logic.
    queuetime_windowed=<number> and queuetime_fs=<number>
        Use VDPAU's presentation queue functionality to queue future video
        frame changes at most this many milliseconds in advance (default: 50).
        See below for additional information.
    output_surfaces=<2-15>
        Allocate this many output surfaces to display video frames (default:
        3). See below for additional information.

    Using the VDPAU frame queueing functionality controlled by the queuetime
    options makes MPlayer's frame flip timing less sensitive to system CPU
    load and allows MPlayer to start decoding the next frame(s) slightly
    earlier which can reduce jitter caused by individual slow-to-decode
    frames. However the NVIDIA graphics drivers can make other window behavior
    such as window moves choppy if VDPAU is using the blit queue (mainly
    happens if you have the composite extension enabled) and this feature is
    active. If this happens on your system and it bothers you then you can set
    the queuetime value to 0 to disable this feature. The settings to use in
    windowed and fullscreen mode are separate because there should be less
    reason to disable this for fullscreen mode (as the driver issue shouldn't
    affect the video itself).

    You can queue more frames ahead by increasing the queuetime values and the
    output_surfaces count (to ensure enough surfaces to buffer video for a
    certain time ahead you need at least as many surfaces as the video has
    frames during that time, plus two). This could help make video smoother in
    some cases. The main downsides are increased video RAM requirements for
    the surfaces and laggier display response to user commands (display
    changes only become visible some time after they're queued). The graphics
    driver implementation may also have limits on the length of maximum
    queuing time or number of queued surfaces that work well or at all.

dga (X11 only)
    Play video through the XFree86 Direct Graphics Access extension.
    Considered obsolete.

sdl (SDL only, buggy/outdated)
    Highly platform independent SDL (Simple Directmedia Layer) library video
    output driver. Since SDL uses its own X11 layer, MPlayer X11 options do
    not have any effect on SDL. Note that it has several minor bugs
    (``--vm``/``--no-vm`` is mostly ignored, ``--fs`` behaves like ``--no-vm``
    should, window is in top-left corner when returning from fullscreen,
    panscan is not supported, ...).

    driver=<driver>
        Explicitly choose the SDL driver to use.
    (no-)forcexv
        Use XVideo through the sdl video output driver (default: forcexv).
    (no-)hwaccel
        Use hardware accelerated scaler (default: hwaccel).

direct3d (Windows only) (BETA CODE!)
    Video output driver that uses the Direct3D interface (useful for Vista).

directx (Windows only)
    Video output driver that uses the DirectX interface.

    noaccel
        Turns off hardware acceleration. Try this option if you have display
        problems.

kva (OS/2 only)
    Video output driver that uses the libkva interface.

    snap
        Force SNAP mode.
    wo
        Force WarpOverlay! mode.
    dive
        Force DIVE mode.
    (no-)t23
        Enable or disable workaround for T23 laptop (default: disabled). Try
        to enable this option if your video card supports upscaling only.

quartz (Mac OS X only)
    Mac OS X Quartz video output driver. Under some circumstances, it might be
    more efficient to force a packed YUV output format, with e.g.
    ``--vf=format=yuy2``.

    device_id=<number>
        Choose the display device to use in fullscreen.
    fs_res=<width>:<height>
        Specify the fullscreen resolution (useful on slow systems).

corevideo (Mac OS X 10.4 or 10.3.9 with QuickTime 7)
    Mac OS X CoreVideo video output driver

    device_id=<number>
        Choose the display device to use for fullscreen or set it to -1 to
        always use the same screen the video window is on (default: -1 -
        auto).
    shared_buffer
        Write output to a shared memory buffer instead of displaying it and
        try to open an existing NSConnection for communication with a GUI.
    buffer_name=<name>
        Name of the shared buffer created with shm_open as well as the name of
        the NSConnection MPlayer will try to open (default: "mplayerosx").
        Setting buffer_name implicitly enables shared_buffer.

fbdev (Linux only)
    Uses the kernel framebuffer to play video.

    <device>
        Explicitly choose the fbdev device name to use (e.g. ``/dev/fb0``).

fbdev2 (Linux only)
    Uses the kernel framebuffer to play video, alternative implementation.

    <device>
        Explicitly choose the fbdev device name to use (default: ``/dev/fb0``).

vesa
    Very general video output driver that should work on any VESA VBE 2.0
    compatible card.

    (no-)dga
        Turns DGA mode on or off (default: on).
    neotv_pal
        Activate the NeoMagic TV out and set it to PAL norm.
    neotv_ntsc
        Activate the NeoMagic TV out and set it to NTSC norm.
    lvo
        Activate the Linux Video Overlay on top of VESA mode.

svga
    Play video using the SVGA library.

    <video mode>
        Specify video mode to use. The mode can be given in a
        <width>x<height>x<colors> format, e.g. 640x480x16M or be a graphics
        mode number, e.g. 84.
    bbosd
        Draw OSD into black bands below the movie (slower).
    native
        Use only native drawing functions. This avoids direct rendering, OSD
        and hardware acceleration.
    retrace
        Force frame switch on vertical retrace. Usable only with ``--double``.
        It has the same effect as the ``--vsync`` option.
    sq
        Try to select a video mode with square pixels.

gl
    OpenGL video output driver, simple version. Video size must be smaller
    than the maximum texture size of your OpenGL implementation. Intended to
    work even with the most basic OpenGL implementations, but also makes use
    of newer extensions, which allow support for more colorspaces and direct
    rendering. For optimal speed try adding the options ``--dr=-noslices``

    The code performs very few checks, so if a feature does not work, this
    might be because it is not supported by your card/OpenGL implementation
    even if you do not get any error message. Use ``glxinfo`` or a similar
    tool to display the supported OpenGL extensions.

    (no-)ati-hack
        ATI drivers may give a corrupted image when PBOs are used (when using
        ``--dr`` or `force-pbo`). This option fixes this, at the expense of
        using a bit more memory.
    (no-)force-pbo
        Always uses PBOs to transfer textures even if this involves an extra
        copy. Currently this gives a little extra speed with NVidia drivers
        and a lot more speed with ATI drivers. May need ``--no-slices`` and
        the ati-hack suboption to work correctly.
    (no-)scaled-osd
        Changes the way the OSD behaves when the size of the window changes
        (default: disabled). When enabled behaves more like the other video
        output drivers, which is better for fixed-size fonts. Disabled looks
        much better with FreeType fonts and uses the borders in fullscreen
        mode. Does not work correctly with ass subtitles (see ``--ass``), you
        can instead render them without OpenGL support via ``--vf=ass``.
    osdcolor=<0xAARRGGBB>
        Color for OSD (default: 0x00ffffff, corresponds to non-transparent
        white).
    rectangle=<0,1,2>
        Select usage of rectangular textures which saves video RAM, but often
        is slower (default: 0).

        0
            Use power-of-two textures (default).
        1
            Use the ``GL_ARB_texture_rectangle`` extension.
        2
            Use the ``GL_ARB_texture_non_power_of_two`` extension. In some
            cases only supported in software and thus very slow.

    swapinterval=<n>
        Minimum interval between two buffer swaps, counted in displayed frames
        (default: 1). 1 is equivalent to enabling VSYNC, 0 to disabling VSYNC.
        Values below 0 will leave it at the system default. This limits the
        framerate to (horizontal refresh rate / n). Requires
        ``GLX_SGI_swap_control`` support to work. With some (most/all?)
        implementations this only works in fullscreen mode.
    ycbcr
        Use the ``GL_MESA_ycbcr_texture`` extension to convert YUV to RGB. In
        most cases this is probably slower than doing software conversion to
        RGB.
    yuv=<n>
        Select the type of YUV to RGB conversion. The default is
        auto-detection deciding between values 0 and 2.

        0
            Use software conversion. Compatible with all OpenGL versions.
            Provides brightness, contrast and saturation control.
        1
            Same as 2. This used to use nVidia-specific extensions, which
            didn't provide any advantages over using fragment programs, except
            possibly on very ancient graphic cards. It produced a gray-ish
            output, which is why it has been removed.
        2
            Use a fragment program. Needs the ``GL_ARB_fragment_program``
            extension and at least three texture units. Provides brightness,
            contrast, saturation and hue control.
        3
            Use a fragment program using the POW instruction. Needs the
            ``GL_ARB_fragment_program`` extension and at least three texture
            units. Provides brightness, contrast, saturation, hue and gamma
            control. Gamma can also be set independently for red, green and
            blue. Method 4 is usually faster.
        4
            Use a fragment program with additional lookup. Needs the
            ``GL_ARB_fragment_program`` extension and at least four texture
            units. Provides brightness, contrast, saturation, hue and gamma
            control. Gamma can also be set independently for red, green and
            blue.
        5
            Use ATI-specific method (for older cards). This uses an
            ATI-specific extension (``GL_ATI_fragment_shader`` - not
            ``GL_ARB_fragment_shader``!). At least three texture units are
            needed. Provides saturation and hue control. This method is fast
            but inexact.
        6
            Use a 3D texture to do conversion via lookup. Needs the
            ``GL_ARB_fragment_program extension`` and at least four texture
            units. Extremely slow (software emulation) on some (all?) ATI
            cards since it uses a texture with border pixels. Provides
            brightness, contrast, saturation, hue and gamma control. Gamma can
            also be set independently for red, green and blue. Speed depends
            more on GPU memory bandwidth than other methods.

    lscale=<n>
        Select the scaling function to use for luminance scaling. Only valid
        for yuv modes 2, 3, 4 and 6.

        0
            Use simple linear filtering (default).
        1
            Use bicubic B-spline filtering (better quality). Needs one
            additional texture unit. Older cards will not be able to handle
            this for chroma at least in fullscreen mode.
        2
            Use cubic filtering in horizontal, linear filtering in vertical
            direction. Works on a few more cards than method 1.
        3
            Same as 1 but does not use a lookup texture. Might be faster on
            some cards.
        4
            Use experimental unsharp masking with 3x3 support and a default
            strength of 0.5 (see `filter-strength`).
        5
            Use experimental unsharp masking with 5x5 support and a default
            strength of 0.5 (see `filter-strength`).

    cscale=<n>
        Select the scaling function to use for chrominance scaling. For
        details see `lscale`.
    filter-strength=<value>
        Set the effect strength for the `lscale`/`cscale` filters that support
        it.
    stereo=<value>
        Select a method for stereo display. You may have to use ``--aspect`` to
        fix the aspect value. Experimental, do not expect too much from it.

        0
            Normal 2D display
        1
            Convert side by side input to full-color red-cyan stereo.
        2
            Convert side by side input to full-color green-magenta stereo.
        3
            Convert side by side input to quadbuffered stereo. Only supported
            by very few OpenGL cards.

    The following options are only useful if writing your own fragment
    programs.

    customprog=<filename>
        Load a custom fragment program from <filename>. See
        ``TOOLS/edgedect.fp`` for an example.
    customtex=<filename>
        Load a custom "gamma ramp" texture from <filename>. This can be used
        in combination with yuv=4 or with the customprog option.
    (no-)customtlin
        If enabled (default) use ``GL_LINEAR`` interpolation, otherwise use
        ``GL_NEAREST`` for customtex texture.
    (no-)customtrect
        If enabled, use texture_rectangle for customtex texture. Default is
        disabled.
    (no-)mipmapgen
        If enabled, mipmaps for the video are automatically generated. This
        should be useful together with the customprog and the TXB instruction
        to implement blur filters with a large radius. For most OpenGL
        implementations this is very slow for any non-RGB formats. Default is
        disabled.

    Normally there is no reason to use the following options, they mostly
    exist for testing purposes.

    (no-)glfinish
        Call ``glFinish()`` before swapping buffers. Slower but in some cases
        more correct output (default: disabled).
    (no-)manyfmts
        Enables support for more (RGB and BGR) color formats (default:
        enabled). Needs OpenGL version >= 1.2.
    slice-height=<0-...>
        Number of lines copied to texture in one piece (default: 0). 0 for
        whole image.

        *NOTE*: If YUV colorspace is used (see `yuv` suboption), special rules
        apply: If the decoder uses slice rendering (see ``--no-slices``), this
        setting has no effect, the size of the slices as provided by the
        decoder is used. If the decoder does not use slice rendering, the
        default is 16.
    (no-)osd
        Enable or disable support for OSD rendering via OpenGL (default:
        enabled). This option is for testing; to disable the OSD use
        ``--osdlevel=0`` instead.

null
    Produces no video output. Useful for benchmarking.

aa
    ASCII art video output driver that works on a text console. You can get a
    list and an explanation of available suboptions by executing ``mplayer
    --vo=aa:help``.

    *NOTE*: The driver does not handle ``--aspect`` correctly.

    *HINT*: You probably have to specify ``--monitorpixelaspect``. Try
    ``mplayer --vo=aa --monitorpixelaspect=0.5``.

caca
    Color ASCII art video output driver that works on a text console.

bl
    Video playback using the Blinkenlights UDP protocol. This driver is highly
    hardware specific.

    <subdevice>
        Explicitly choose the Blinkenlights subdevice driver to use. It is
        something like ``arcade:host=localhost:2323`` or
        ``hdl:file=name1,file=name2``. You must specify a subdevice.

ggi
    GGI graphics system video output driver

    <driver>
        Explicitly choose the GGI driver to use. Replace any ',' that would
        appear in the driver string by a '.'.

directfb
    Play video using the DirectFB library.

    (no-)input
        Use the DirectFB instead of the MPlayer keyboard code (default:
        enabled).
    buffermode=single|double|triple
        Double and triple buffering give best results if you want to avoid
        tearing issues. Triple buffering is more efficient than double
        buffering as it does not block MPlayer while waiting for the vertical
        retrace. Single buffering should be avoided (default: single).
    fieldparity=top|bottom
        Control the output order for interlaced frames (default: disabled).
        Valid values are top = top fields first, bottom = bottom fields first.
        This option does not have any effect on progressive film material like
        most MPEG movies are. You need to enable this option if you have
        tearing issues or unsmooth motions watching interlaced film material.
    layer=N
        Will force layer with ID N for playback (default: -1 - auto).
    dfbopts=<list>
        Specify a parameter list for DirectFB.

dfbmga
    Matrox G400/G450/G550 specific video output driver that uses the DirectFB
    library to make use of special hardware features. Enables CRTC2 (second
    head), displaying video independently of the first head.

    (no-)input
        same as directfb (default: disabled)
    buffermode=single|double|triple
        same as directfb (default: triple)
    fieldparity=top|bottom
        same as directfb
    (no-)bes
        Enable the use of the Matrox BES (backend scaler) (default: disabled).
        Gives very good results concerning speed and output quality as
        interpolated picture processing is done in hardware. Works only on the
        primary head.
    (no-)spic
        Make use of the Matrox sub picture layer to display the OSD (default:
        enabled).
    (no-)crtc2
        Turn on TV-out on the second head (default: enabled). The output
        quality is amazing as it is a full interlaced picture with proper sync
        to every odd/even field.
    tvnorm=pal|ntsc|auto
        Will set the TV norm of the Matrox card without the need for modifying
        ``/etc/directfbrc`` (default: disabled). Valid norms are pal = PAL,
        ntsc = NTSC. Special norm is auto (auto-adjust using PAL/NTSC) because
        it decides which norm to use by looking at the framerate of the movie.

mga (Linux only)
    Matrox specific video output driver that makes use of the YUV back end
    scaler on Gxxx cards through a kernel module. If you have a Matrox card,
    this is the fastest option.

    <device>
        Explicitly choose the Matrox device name to use (default:
        ``/dev/mga_vid``).

xmga (Linux, X11 only)
    The mga video output driver, running in an X11 window.

    <device>
        Explicitly choose the Matrox device name to use (default:
        ``/dev/mga_vid``).

s3fb (Linux only) (see also ``--dr``)
    S3 Virge specific video output driver. This driver supports the card's YUV
    conversion and scaling, double buffering and direct rendering features.
    Use ``--vf=format=yuy2`` to get hardware-accelerated YUY2 rendering, which
    is much faster than YV12 on this card.

    <device>
        Explicitly choose the fbdev device name to use (default: ``/dev/fb0``).

wii (Linux only)
    Nintendo Wii/GameCube specific video output driver.

3dfx (Linux only)
    3dfx-specific video output driver that directly uses the hardware on top
    of X11. Only 16 bpp are supported.

tdfxfb (Linux only)
    This driver employs the tdfxfb framebuffer driver to play movies with YUV
    acceleration on 3dfx cards.

    <device>
        Explicitly choose the fbdev device name to use (default: ``/dev/fb0``).

tdfx_vid (Linux only)
    3dfx-specific video output driver that works in combination with the
    tdfx_vid kernel module.

    <device>
        Explicitly choose the device name to use (default: ``/dev/tdfx_vid``).

dxr3 (DXR3 only)
    Sigma Designs em8300 MPEG decoder chip (Creative DXR3, Sigma Designs
    Hollywood Plus) specific video output driver. See also the lavc video
    filter.

    overlay
        Activates the overlay instead of TV-out.
    prebuf
        Turns on prebuffering.
    sync
        Will turn on the new sync-engine.
    norm=<norm>
        Specifies the TV norm.

        :0: Does not change current norm (default).
        :1: Auto-adjust using PAL/NTSC.
        :2: Auto-adjust using PAL/PAL-60.
        :3: PAL
        :4: PAL-60
        :5: NTSC

    <0-3>
        Specifies the device number to use if you have more than one em8300
        card.

ivtv (IVTV only)
    Conexant CX23415 (iCompression iTVC15) or Conexant CX23416 (iCompression
    iTVC16) MPEG decoder chip (Hauppauge WinTV PVR-150/250/350/500) specific
    video output driver for TV-out. See also the lavc video filter.

    <device>
        Explicitly choose the MPEG decoder device name to use (default:
        ``/dev/video16``).
    <output>
        Explicitly choose the TV-out output to be used for the video signal.

v4l2 (requires Linux 2.6.22+ kernel)
    Video output driver for V4L2 compliant cards with built-in hardware MPEG
    decoder. See also the lavc video filter.

    <device>
        Explicitly choose the MPEG decoder device name to use (default:
        ``/dev/video16``).
    <output>
        Explicitly choose the TV-out output to be used for the video signal.

mpegpes (DVB only)
    Video output driver for DVB cards that writes the output to an MPEG-PES
    file if no DVB card is installed.

    card=<1-4>
        Specifies the device number to use if you have more than one DVB
        output card (V3 API only, such as 1.x.y series drivers). If not
        specified MPlayer will search the first usable card.
    <filename>
        output filename (default: ``./grab.mpg``)

md5sum
    Calculate MD5 sums of each frame and write them to a file. Supports RGB24
    and YV12 colorspaces. Useful for debugging.

    outfile=<value>
        Specify the output filename (default: ``./md5sums``).

yuv4mpeg
    Transforms the video stream into a sequence of uncompressed YUV 4:2:0
    images and stores it in a file (default: ``./stream.yuv``). The format is
    the same as the one employed by mjpegtools, so this is useful if you want
    to process the video with the mjpegtools suite. It supports the YV12
    format. If your source file has a different format and is interlaced, make
    sure to use ``--vf=scale=::1`` to ensure the conversion uses interlaced
    mode. You can combine it with the ``--fixed-vo`` option to concatenate
    files with the same dimensions and fps value.

    interlaced
        Write the output as interlaced frames, top field first.
    interlaced_bf
        Write the output as interlaced frames, bottom field first.
    file=<filename>
        Write the output to <filename> instead of the default ``stream.yuv``.

    *NOTE*: If you do not specify any option the output is progressive (i.e.
    not interlaced).

gif89a
    Output each frame into a single animated GIF file in the current
    directory. It supports only RGB format with 24 bpp and the output is
    converted to 256 colors.

    <fps>
        Float value to specify framerate (default: 5.0).
    <output>
        Specify the output filename (default: ``./out.gif``).

    *NOTE*: You must specify the framerate before the filename or the
    framerate will be part of the filename.

    *EXAMPLE*: ``mplayer video.nut --vo=gif89a:fps=15:output=test.gif``

jpeg
    Output each frame into a JPEG file in the current directory. Each file
    takes the frame number padded with leading zeros as name.

    [no]progressive
        Specify standard or progressive JPEG (default: noprogressive).
    [no]baseline
        Specify use of baseline or not (default: baseline).
    optimize=<0-100>
        optimization factor (default: 100)
    smooth=<0-100>
        smooth factor (default: 0)
    quality=<0-100>
        quality factor (default: 75)
    outdir=<dirname>
        Specify the directory to save the JPEG files to (default: ``./``).
    subdirs=<prefix>
        Create numbered subdirectories with the specified prefix to save the
        files in instead of the current directory.
    maxfiles=<value> (subdirs only)
        Maximum number of files to be saved per subdirectory. Must be equal to
        or larger than 1 (default: 1000).

pnm
    Output each frame into a PNM file in the current directory. Each file
    takes the frame number padded with leading zeros as name. It supports PPM,
    PGM and PGMYUV files in both raw and ASCII mode. See also ``pnm(5)``,
    ``ppm(5)`` and ``pgm(5)``.

    ppm
        Write PPM files (default).
    pgm
        Write PGM files.
    pgmyuv
        Write PGMYUV files. PGMYUV is like PGM, but it also contains the U and
        V plane, appended at the bottom of the picture.
    raw
        Write PNM files in raw mode (default).
    ascii
        Write PNM files in ASCII mode.
    outdir=<dirname>
        Specify the directory to save the PNM files to (default: ``./``).
    subdirs=<prefix>
        Create numbered subdirectories with the specified prefix to save the
        files in instead of the current directory.
    maxfiles=<value> (subdirs only)
        Maximum number of files to be saved per subdirectory. Must be equal to
        or larger than 1 (default: 1000).

png
    Output each frame into a PNG file in the current directory. Each file
    takes the frame number padded with leading zeros as name. 24bpp RGB and
    BGR formats are supported.

    z=<0-9>
        Specifies the compression level. 0 is no compression, 9 is maximum
        compression.
    alpha
        Create PNG files with an alpha channel. Note that MPlayer in general
        does not support alpha, so this will only be useful in some rare
        cases.

tga
    Output each frame into a Targa file in the current directory. Each file
    takes the frame number padded with leading zeros as name. The purpose of
    this video output driver is to have a simple lossless image writer to use
    without any external library. It supports the BGR[A] color format, with
    15, 24 and 32 bpp. You can force a particular format with the format video
    filter.

    *EXAMPLE*: ``mplayer video.nut --vf=format=bgr15 --vo=tga``


DECODING/FILTERING OPTIONS
==========================



AUDIO FILTERS
=============

Audio filters allow you to modify the audio stream and its properties. The
syntax is:

--af=<filter1[=parameter1:parameter2:...],filter2,...>
    Setup a chain of audio filters.

*NOTE*: To get a full list of available audio filters, see ``--af=help``.

Audio filters are managed in lists. There are a few commands to manage the
filter list.

--af-add=<filter1[,filter2,...]>
    Appends the filters given as arguments to the filter list.

--af-pre=<filter1[,filter2,...]>
    Prepends the filters given as arguments to the filter list.

--af-del=<index1[,index2,...]>
    Deletes the filters at the given indexes. Index numbers start at 0,
    negative numbers address the end of the list (-1 is the last).

--af-clr
    Completely empties the filter list.

Available filters are:

resample[=srate[:sloppy[:type]]]
    Changes the sample rate of the audio stream. Can be used if you have a
    fixed frequency sound card or if you are stuck with an old sound card that
    is only capable of max 44.1kHz. This filter is automatically enabled if
    necessary. It only supports 16-bit integer and float in native-endian
    format as input.

    <srate>
        output sample frequency in Hz. The valid range for this parameter is
        8000 to 192000. If the input and output sample frequency are the same
        or if this parameter is omitted the filter is automatically unloaded.
        A high sample frequency normally improves the audio quality,
        especially when used in combination with other filters.
    <sloppy>
        Allow (1) or disallow (0) the output frequency to differ slightly from
        the frequency given by <srate> (default: 1). Can be used if the
        startup of the playback is extremely slow.
    <type>
        Select which resampling method to use.

        :0: linear interpolation (fast, poor quality especially when
            upsampling)
        :1: polyphase filterbank and integer processing
        :2: polyphase filterbank and floating point processing
            (slow, best quality)

    *EXAMPLE*:

    ``mplayer --af=resample=44100:0:0``
        would set the output frequency of the resample filter to 44100Hz using
        exact output frequency scaling and linear interpolation.

lavcresample[=srate[:length[:linear[:count[:cutoff]]]]]
    Changes the sample rate of the audio stream to an integer <srate> in Hz.
    It only supports the 16-bit native-endian format.

    <srate>
        the output sample rate
    <length>
        length of the filter with respect to the lower sampling rate (default:
        16)
    <linear>
        if 1 then filters will be linearly interpolated between polyphase
        entries
    <count>
        log2 of the number of polyphase entries (..., 10->1024, 11->2048,
        12->4096, ...) (default: 10->1024)
    <cutoff>
        cutoff frequency (0.0-1.0), default set depending upon filter length

lavcac3enc[=tospdif[:bitrate[:minchn]]]
    Encode multi-channel audio to AC-3 at runtime using libavcodec. Supports
    16-bit native-endian input format, maximum 6 channels. The output is
    big-endian when outputting a raw AC-3 stream, native-endian when
    outputting to S/PDIF. The output sample rate of this filter is same with
    the input sample rate. When input sample rate is 48kHz, 44.1kHz, or 32kHz,
    this filter directly use it. Otherwise a resampling filter is
    auto-inserted before this filter to make the input and output sample rate
    be 48kHz. You need to specify ``--channels=N`` to make the decoder decode
    audio into N-channel, then the filter can encode the N-channel input to
    AC-3.

    <tospdif>
        Output raw AC-3 stream if zero or not set, output to S/PDIF for
        passthrough when <tospdif> is set non-zero.
    <bitrate>
        The bitrate to encode the AC-3 stream. Set it to either 384 or 384000
        to get 384kbits.

        Valid values: 32, 40, 48, 56, 64, 80, 96, 112, 128,
        160, 192, 224, 256, 320, 384, 448, 512, 576, 640.

        Default bitrate is based on the input channel number:

        :1ch: 96
        :2ch: 192
        :3ch: 224
        :4ch: 384
        :5ch: 448
        :6ch: 448

    <minchn>
        If the input channel number is less than <minchn>, the filter will
        detach itself (default: 5).

sweep[=speed]
    Produces a sine sweep.

    <0.0-1.0>
        Sine function delta, use very low values to hear the sweep.

sinesuppress[=freq:decay]
    Remove a sine at the specified frequency. Useful to get rid of the 50/60Hz
    noise on low quality audio equipment. It probably only works on mono input.

    <freq>
        The frequency of the sine which should be removed (in Hz) (default:
        50)
    <decay>
        Controls the adaptivity (a larger value will make the filter adapt to
        amplitude and phase changes quicker, a smaller value will make the
        adaptation slower) (default: 0.0001). Reasonable values are around
        0.001.

bs2b[=option1:option2:...]
    Bauer stereophonic to binaural transformation using ``libbs2b``. Improves
    the headphone listening experience by making the sound similar to that
    from loudspeakers, allowing each ear to hear both channels and taking into
    account the distance difference and the head shadowing effect. It is
    applicable only to 2 channel audio.

    fcut=<300-1000>
        Set cut frequency in Hz.
    feed=<10-150>
        Set feed level for low frequencies in 0.1*dB.
    profile=<value>
        Several profiles are available for convenience:

        :default: will be used if nothing else was specified (fcut=700,
                  feed=45)
        :cmoy:    Chu Moy circuit implementation (fcut=700, feed=60)
        :jmeier:  Jan Meier circuit implementation (fcut=650, feed=95)

    If fcut or feed options are specified together with a profile, they will
    be applied on top of the selected profile.

hrtf[=flag]
    Head-related transfer function: Converts multichannel audio to 2 channel
    output for headphones, preserving the spatiality of the sound.

    ==== ===================================
    Flag Meaning
    ==== ===================================
    m    matrix decoding of the rear channel
    s    2-channel matrix decoding
    0    no matrix decoding (default)
    ==== ===================================

equalizer=[g1:g2:g3:...:g10]
    10 octave band graphic equalizer, implemented using 10 IIR band pass
    filters. This means that it works regardless of what type of audio is
    being played back. The center frequencies for the 10 bands are:

    === ==========
    No. frequency
    === ==========
    0    31.25  Hz
    1    62.50  Hz
    2   125.00  Hz
    3   250.00  Hz
    4   500.00  Hz
    5     1.00 kHz
    6     2.00 kHz
    7     4.00 kHz
    8     8.00 kHz
    9    16.00 kHz
    === ==========

    If the sample rate of the sound being played is lower than the center
    frequency for a frequency band, then that band will be disabled. A known
    bug with this filter is that the characteristics for the uppermost band
    are not completely symmetric if the sample rate is close to the center
    frequency of that band. This problem can be worked around by upsampling
    the sound using the resample filter before it reaches this filter.

    <g1>:<g2>:<g3>:...:<g10>
        floating point numbers representing the gain in dB for each frequency
        band (-12-12)

    *EXAMPLE*:

    ``mplayer --af=equalizer=11:11:10:5:0:-12:0:5:12:12 media.avi``
        Would amplify the sound in the upper and lower frequency region while
        canceling it almost completely around 1kHz.

channels=nch[:nr:from1:to1:from2:to2:from3:to3:...]
    Can be used for adding, removing, routing and copying audio channels. If
    only <nch> is given the default routing is used, it works as follows: If
    the number of output channels is bigger than the number of input channels
    empty channels are inserted (except mixing from mono to stereo, then the
    mono channel is repeated in both of the output channels). If the number of
    output channels is smaller than the number of input channels the exceeding
    channels are truncated.

    <nch>
        number of output channels (1-8)
    <nr>
        number of routes (1-8)
    <from1:to1:from2:to2:from3:to3:...>
        Pairs of numbers between 0 and 7 that define where to route each
        channel.

    *EXAMPLE*:

    ``mplayer --af=channels=4:4:0:1:1:0:2:2:3:3 media.avi``
        Would change the number of channels to 4 and set up 4 routes that swap
        channel 0 and channel 1 and leave channel 2 and 3 intact. Observe that
        if media containing two channels was played back, channels 2 and 3
        would contain silence but 0 and 1 would still be swapped.

    ``mplayer --af=channels=6:4:0:0:0:1:0:2:0:3 media.avi``
        Would change the number of channels to 6 and set up 4 routes that copy
        channel 0 to channels 0 to 3. Channel 4 and 5 will contain silence.

format[=format]
    Convert between different sample formats. Automatically enabled when
    needed by the sound card or another filter. See also ``--format``.

    <format>
        Sets the desired format. The general form is 'sbe', where 's' denotes
        the sign (either 's' for signed or 'u' for unsigned), 'b' denotes the
        number of bits per sample (16, 24 or 32) and 'e' denotes the
        endianness ('le' means little-endian, 'be' big-endian and 'ne' the
        endianness of the computer MPlayer is running on). Valid values
        (amongst others) are: 's16le', 'u32be' and 'u24ne'. Exceptions to this
        rule that are also valid format specifiers: u8, s8, floatle, floatbe,
        floatne, mulaw, alaw, mpeg2, ac3 and imaadpcm.

volume[=v[:sc]]
    Implements software volume control. Use this filter with caution since it
    can reduce the signal to noise ratio of the sound. In most cases it is
    best to set the level for the PCM sound to max, leave this filter out and
    control the output level to your speakers with the master volume control
    of the mixer. In case your sound card has a digital PCM mixer instead of
    an analog one, and you hear distortion, use the MASTER mixer instead. If
    there is an external amplifier connected to the computer (this is almost
    always the case), the noise level can be minimized by adjusting the master
    level and the volume knob on the amplifier until the hissing noise in the
    background is gone.

    This filter has a second feature: It measures the overall maximum sound
    level and prints out that level when MPlayer exits. This feature currently
    only works with floating-point data, use e.g. ``--af-adv=force=5``, or use
    ``--af=stats``.

    *NOTE*: This filter is not reentrant and can therefore only be enabled
    once for every audio stream.

    <v>
        Sets the desired gain in dB for all channels in the stream from -200dB
        to +60dB, where -200dB mutes the sound completely and +60dB equals a
        gain of 1000 (default: 0).
    <sc>
        Turns soft clipping on (1) or off (0). Soft-clipping can make the
        sound more smooth if very high volume levels are used. Enable this
        option if the dynamic range of the loudspeakers is very low.

        *WARNING*: This feature creates distortion and should be considered a
        last resort.

    *EXAMPLE*:

    ``mplayer --af=volume=10.1:0 media.avi``
        Would amplify the sound by 10.1dB and hard-clip if the sound level is
        too high.

pan=n[:L00:L01:L02:...L10:L11:L12:...Ln0:Ln1:Ln2:...]
    Mixes channels arbitrarily. Basically a combination of the volume and the
    channels filter that can be used to down-mix many channels to only a few,
    e.g. stereo to mono or vary the "width" of the center speaker in a
    surround sound system. This filter is hard to use, and will require some
    tinkering before the desired result is obtained. The number of options for
    this filter depends on the number of output channels. An example how to
    downmix a six-channel file to two channels with this filter can be found
    in the examples section near the end.

    <n>
        number of output channels (1-8)
    <Lij>
        How much of input channel i is mixed into output channel j (0-1). So
        in principle you first have n numbers saying what to do with the first
        input channel, then n numbers that act on the second input channel
        etc. If you do not specify any numbers for some input channels, 0 is
        assumed.

    *EXAMPLE*:

    ``mplayer --af=pan=1:0.5:0.5 media.avi``
        Would down-mix from stereo to mono.

    ``mplayer --af=pan=3:1:0:0.5:0:1:0.5 media.avi``
        Would give 3 channel output leaving channels 0 and 1 intact, and mix
        channels 0 and 1 into output channel 2 (which could be sent to a
        subwoofer for example).

sub[=fc:ch]
    Adds a subwoofer channel to the audio stream. The audio data used for
    creating the subwoofer channel is an average of the sound in channel 0 and
    channel 1. The resulting sound is then low-pass filtered by a 4th order
    Butterworth filter with a default cutoff frequency of 60Hz and added to a
    separate channel in the audio stream.

    *Warning*: Disable this filter when you are playing DVDs with Dolby
    Digital 5.1 sound, otherwise this filter will disrupt the sound to the
    subwoofer.

    <fc>
        cutoff frequency in Hz for the low-pass filter (20Hz to 300Hz)
        (default: 60Hz) For the best result try setting the cutoff frequency
        as low as possible. This will improve the stereo or surround sound
        experience.
    <ch>
        Determines the channel number in which to insert the sub-channel
        audio. Channel number can be between 0 and 7 (default: 5). Observe
        that the number of channels will automatically be increased to <ch> if
        necessary.

    *EXAMPLE*:

    ``mplayer --af=sub=100:4 --channels=5 media.avi``
        Would add a sub-woofer channel with a cutoff frequency of 100Hz to
        output channel 4.

center
    Creates a center channel from the front channels. May currently be low
    quality as it does not implement a high-pass filter for proper extraction
    yet, but averages and halves the channels instead.

    <ch>
        Determines the channel number in which to insert the center channel.
        Channel number can be between 0 and 7 (default: 5). Observe that the
        number of channels will automatically be increased to <ch> if
        necessary.

surround[=delay]
    Decoder for matrix encoded surround sound like Dolby Surround. Many files
    with 2 channel audio actually contain matrixed surround sound. Requires a
    sound card supporting at least 4 channels.

    <delay>
        delay time in ms for the rear speakers (0 to 1000) (default: 20) This
        delay should be set as follows: If d1 is the distance from the
        listening position to the front speakers and d2 is the distance from
        the listening position to the rear speakers, then the delay should be
        set to 15ms if d1 <= d2 and to 15 + 5*(d1-d2) if d1 > d2.

    *EXAMPLE*:

    ``mplayer --af=surround=15 --channels=4 media.avi``
        Would add surround sound decoding with 15ms delay for the sound to the
        rear speakers.

delay[=ch1:ch2:...]
    Delays the sound to the loudspeakers such that the sound from the
    different channels arrives at the listening position simultaneously. It is
    only useful if you have more than 2 loudspeakers.

    ch1,ch2,...
        The delay in ms that should be imposed on each channel (floating point
        number between 0 and 1000).

    To calculate the required delay for the different channels do as follows:

    1. Measure the distance to the loudspeakers in meters in relation to your
       listening position, giving you the distances s1 to s5 (for a 5.1
       system). There is no point in compensating for the subwoofer (you will
       not hear the difference anyway).

    2. Subtract the distances s1 to s5 from the maximum distance, i.e.
       ``s[i] = max(s) - s[i]; i = 1...5``.

    3. Calculate the required delays in ms as ``d[i] = 1000*s[i]/342; i =
       1...5``.

    *EXAMPLE*:

    ``mplayer --af=delay=10.5:10.5:0:0:7:0 media.avi``
        Would delay front left and right by 10.5ms, the two rear channels and
        the sub by 0ms and the center channel by 7ms.

export[=mmapped_file[:nsamples]]
    Exports the incoming signal to other processes using memory mapping
    (``mmap()``). Memory mapped areas contain a header:

    | int nch                      /\* number of channels \*/
    | int size                     /\* buffer size \*/
    | unsigned long long counter   /\* Used to keep sync, updated every time new data is exported. \*/

    The rest is payload (non-interleaved) 16 bit data.

    <mmapped_file>
        file to map data to (default: ``~/.mplayer/mplayer-af_export``)
    <nsamples>
        number of samples per channel (default: 512)

    *EXAMPLE*:

    ``mplayer --af=export=/tmp/mplayer-af_export:1024 media.avi``
        Would export 1024 samples per channel to ``/tmp/mplayer-af_export``.

extrastereo[=mul]
    (Linearly) increases the difference between left and right channels which
    adds some sort of "live" effect to playback.

    <mul>
        Sets the difference coefficient (default: 2.5). 0.0 means mono sound
        (average of both channels), with 1.0 sound will be unchanged, with
        -1.0 left and right channels will be swapped.

volnorm[=method:target]
    Maximizes the volume without distorting the sound.

    <method>
        Sets the used method.

        1
            Use a single sample to smooth the variations via the standard
            weighted mean over past samples (default).
        2
            Use several samples to smooth the variations via the standard
            weighted mean over past samples.

    <target>
        Sets the target amplitude as a fraction of the maximum for the sample
        type (default: 0.25).

ladspa=file:label[:controls...]
    Load a LADSPA (Linux Audio Developer's Simple Plugin API) plugin. This
    filter is reentrant, so multiple LADSPA plugins can be used at once.

    <file>
        Specifies the LADSPA plugin library file. If ``LADSPA_PATH`` is set,
        it searches for the specified file. If it is not set, you must supply
        a fully specified pathname.
    <label>
        Specifies the filter within the library. Some libraries contain only
        one filter, but others contain many of them. Entering 'help' here,
        will list all available filters within the specified library, which
        eliminates the use of 'listplugins' from the LADSPA SDK.
    <controls>
        Controls are zero or more floating point values that determine the
        behavior of the loaded plugin (for example delay, threshold or gain).
        In verbose mode (add ``-v`` to the MPlayer command line), all
        available controls and their valid ranges are printed. This eliminates
        the use of 'analyseplugin' from the LADSPA SDK.

comp
    Compressor/expander filter usable for microphone input. Prevents artifacts
    on very loud sound and raises the volume on very low sound. This filter is
    untested, maybe even unusable.

gate
    Noise gate filter similar to the comp audio filter. This filter is
    untested, maybe even unusable.

karaoke
    Simple voice removal filter exploiting the fact that voice is usually
    recorded with mono gear and later 'center' mixed onto the final audio
    stream. Beware that this filter will turn your signal into mono. Works
    well for 2 channel tracks; do not bother trying it on anything but 2
    channel stereo.

scaletempo[=option1:option2:...]
    Scales audio tempo without altering pitch, optionally synced to playback
    speed (default).

    This works by playing 'stride' ms of audio at normal speed then consuming
    'stride*scale' ms of input audio. It pieces the strides together by
    blending 'overlap'% of stride with audio following the previous stride. It
    optionally performs a short statistical analysis on the next 'search' ms
    of audio to determine the best overlap position.

    scale=<amount>
        Nominal amount to scale tempo. Scales this amount in addition to
        speed. (default: 1.0)
    stride=<amount>
        Length in milliseconds to output each stride. Too high of value will
        cause noticable skips at high scale amounts and an echo at low scale
        amounts. Very low values will alter pitch. Increasing improves
        performance. (default: 60)
    overlap=<percent>
        Percentage of stride to overlap. Decreasing improves performance.
        (default: .20)
    search=<amount>
        Length in milliseconds to search for best overlap position. Decreasing
        improves performance greatly. On slow systems, you will probably want
        to set this very low. (default: 14)
    speed=<tempo|pitch|both|none>
        Set response to speed change.

        tempo
             Scale tempo in sync with speed (default).
        pitch
             Reverses effect of filter. Scales pitch without altering tempo.
             Add ``[ speed_mult 0.9438743126816935`` and ``] speed_mult
             1.059463094352953`` to your ``input.conf`` to step by musical
             semi-tones.

             *WARNING*: Loses sync with video.
        both
            Scale both tempo and pitch.
        none
            Ignore speed changes.

    *EXAMPLE*:

    ``mplayer --af=scaletempo --speed=1.2 media.ogg``
        Would playback media at 1.2x normal speed, with audio at normal pitch.
        Changing playback speed, would change audio tempo to match.

    ``mplayer --af=scaletempo=scale=1.2:speed=none --speed=1.2 media.ogg``
        Would playback media at 1.2x normal speed, with audio at normal pitch,
        but changing playback speed has no effect on audio tempo.

    ``mplayer --af=scaletempo=stride=30:overlap=.50:search=10 media.ogg``
        Would tweak the quality and performace parameters.

    ``mplayer --af=format=floatne,scaletempo media.ogg``
        Would make scaletempo use float code. Maybe faster on some platforms.

    ``mplayer --af=scaletempo=scale=1.2:speed=pitch audio.ogg``
        Would playback audio file at 1.2x normal speed, with audio at normal
        pitch. Changing playback speed, would change pitch, leaving audio
        tempo at 1.2x.

stats
    Collects and prints statistics about the audio stream, especially the
    volume. These statistics are especially intended to help adjusting the
    volume while avoiding clipping. The volumes are printed in dB and
    compatible with the volume audio filter.


VIDEO FILTERS
=============

Video filters allow you to modify the video stream and its properties. The
syntax is:

--vf=<filter1[=parameter1:parameter2:...],filter2,...>
    Setup a chain of video filters.

Many parameters are optional and set to default values if omitted. To
explicitly use a default value set a parameter to '-1'. Parameters w:h means
width x height in pixels, x:y means x;y position counted from the upper left
corner of the bigger image.

*NOTE*: To get a full list of available video filters, see ``--vf=help``.

Video filters are managed in lists. There are a few commands to manage the
filter list.

--vf-add=<filter1[,filter2,...]>
    Appends the filters given as arguments to the filter list.

--vf-pre=<filter1[,filter2,...]>
    Prepends the filters given as arguments to the filter list.

--vf-del=<index1[,index2,...]>
    Deletes the filters at the given indexes. Index numbers start at 0,
    negative numbers address the end of the list (-1 is the last).

--vf-clr
    Completely empties the filter list.

With filters that support it, you can access parameters by their name.

--vf=<filter>=help
    Prints the parameter names and parameter value ranges for a particular
    filter.

--vf=<filter=named_parameter1=value1[:named_parameter2=value2:...]>
    Sets a named parameter to the given value. Use on and off or yes and no to
    set flag parameters.

Available filters are:

crop[=w:h:x:y]
    Crops the given part of the image and discards the rest. Useful to remove
    black bands from widescreen movies.

    <w>,<h>
        Cropped width and height, defaults to original width and height.
    <x>,<y>
        Position of the cropped picture, defaults to center.

cropdetect[=limit:round[:reset]]
    Calculates necessary cropping parameters and prints the recommended
    parameters to stdout.

    <limit>
        Threshold, which can be optionally specified from nothing (0) to
        everything (255) (default: 24).
    <round>
        Value which the width/height should be divisible by (default: 16). The
        offset is automatically adjusted to center the video. Use 2 to get
        only even dimensions (needed for 4:2:2 video). 16 is best when
        encoding to most video codecs.
    <reset>
        Counter that determines after how many frames cropdetect will reset
        the previously detected largest video area and start over to detect
        the current optimal crop area (default: 0). This can be useful when
        channel logos distort the video area. 0 indicates never reset and
        return the largest area encountered during playback.

rectangle[=w:h:x:y]
    Draws a rectangle of the requested width and height at the specified
    coordinates over the image and prints current rectangle parameters to the
    console. This can be used to find optimal cropping parameters. If you bind
    the ``input.conf`` directive 'change_rectangle' to keystrokes, you can
    move and resize the rectangle on the fly.

    <w>,<h>
        width and height (default: -1, maximum possible width where boundaries
        are still visible.)
    <x>,<y>
        top left corner position (default: -1, uppermost leftmost)

expand[=w:h:x:y:osd:aspect:round]
    Expands (not scales) movie resolution to the given value and places the
    unscaled original at coordinates x, y. Can be used for placing
    subtitles/OSD in the resulting black bands.

    <w>,<h>
        Expanded width,height (default: original width,height). Negative
        values for w and h are treated as offsets to the original size.

        *EXAMPLE*:

        `expand=0:-50:0:0`
            Adds a 50 pixel border to the bottom of the picture.

    <x>,<y>
        position of original image on the expanded image (default: center)

    <osd>
        OSD/subtitle rendering

        :0: disable (default)
        :1: enable

    <aspect>
        Expands to fit an aspect instead of a resolution (default: 0).

        *EXAMPLE*:

        `expand=800:::::4/3`
            Expands to 800x600, unless the source is higher resolution, in
            which case it expands to fill a 4/3 aspect.

    <round>
        Rounds up to make both width and height divisible by <r> (default: 1).

flip
    Flips the image upside down. See also ``--flip``.

mirror
    Mirrors the image on the Y axis.

rotate[=<0-7>]
    Rotates the image by 90 degrees and optionally flips it. For values
    between 4-7 rotation is only done if the movie geometry is portrait and
    not landscape.

    :0: Rotate by 90 degrees clockwise and flip (default).
    :1: Rotate by 90 degrees clockwise.
    :2: Rotate by 90 degrees counterclockwise.
    :3: Rotate by 90 degrees counterclockwise and flip.

scale[=w:h[:interlaced[:chr_drop[:par[:par2[:presize[:noup[:arnd]]]]]]]]
    Scales the image with the software scaler (slow) and performs a YUV<->RGB
    colorspace conversion (see also ``--sws``).

    <w>,<h>
        scaled width/height (default: original width/height)

        *NOTE*: If ``--zoom`` is used, and underlying filters (including
        libvo) are incapable of scaling, it defaults to d_width/d_height!

        :0:      scaled d_width/d_height
        :-1:     original width/height
        :-2:     Calculate w/h using the other dimension and the prescaled
                 aspect ratio.
        :-3:     Calculate w/h using the other dimension and the original
                 aspect ratio.
        :-(n+8): Like -n above, but rounding the dimension to the closest
                 multiple of 16.

    <interlaced>
        Toggle interlaced scaling.

        :0: off (default)
        :1: on

    <chr_drop>
        chroma skipping

        :0: Use all available input lines for chroma.
        :1: Use only every 2. input line for chroma.
        :2: Use only every 4. input line for chroma.
        :3: Use only every 8. input line for chroma.

    <par>[:<par2>] (see also ``--sws``)
        Set some scaling parameters depending on the type of scaler selected
        with ``--sws``.

        | --sws=2 (bicubic):  B (blurring) and C (ringing)
        |     0.00:0.60 default
        |     0.00:0.75 VirtualDub's "precise bicubic"
        |     0.00:0.50 Catmull-Rom spline
        |     0.33:0.33 Mitchell-Netravali spline
        |     1.00:0.00 cubic B-spline

        --sws=7 (gaussian): sharpness (0 (soft) - 100 (sharp))

        --sws=9 (lanczos):  filter length (1-10)

    <presize>
        Scale to preset sizes.

        :qntsc: 352x240 (NTSC quarter screen)
        :qpal:  352x288 (PAL quarter screen)
        :ntsc:  720x480 (standard NTSC)
        :pal:   720x576 (standard PAL)
        :sntsc: 640x480 (square pixel NTSC)
        :spal:  768x576 (square pixel PAL)

    <noup>
        Disallow upscaling past the original dimensions.

        :0: Allow upscaling (default).
        :1: Disallow upscaling if one dimension exceeds its original value.
        :2: Disallow upscaling if both dimensions exceed their original values.

    <arnd>
        Accurate rounding for the vertical scaler, which may be faster or
        slower than the default rounding.

        :0: Disable accurate rounding (default).
        :1: Enable accurate rounding.

dsize[=aspect|w:h:aspect-method:r]
    Changes the intended display size/aspect at an arbitrary point in the
    filter chain. Aspect can be given as a fraction (4/3) or floating point
    number (1.33). Alternatively, you may specify the exact display width and
    height desired. Note that this filter does *not* do any scaling itself; it
    just affects what later scalers (software or hardware) will do when
    auto-scaling to correct aspect.

    <w>,<h>
        New display width and height.

        Can also be these special values:

        :0:  original display width and height
        :-1: original video width and height (default)
        :-2: Calculate w/h using the other dimension and the original display
             aspect ratio.
        :-3: Calculate w/h using the other dimension and the original video
             aspect ratio.

        *EXAMPLE*:

        ``dsize=800:-2``
            Specifies a display resolution of 800x600 for a 4/3 aspect video,
            or 800x450 for a 16/9 aspect video.

    <aspect-method>
        Modifies width and height according to original aspect ratios.

        :-1: Ignore original aspect ratio (default).
        :0:  Keep display aspect ratio by using <w> and <h> as maximum
             resolution.
        :1:  Keep display aspect ratio by using <w> and <h> as minimum
             resolution.
        :2:  Keep video aspect ratio by using <w> and <h> as maximum
             resolution.
        :3:  Keep video aspect ratio by using <w> and <h> as minimum
             resolution.

        *EXAMPLE*:

        ``dsize=800:600:0``
            Specifies a display resolution of at most 800x600, or smaller, in
            order to keep aspect.

    <r>
        Rounds up to make both width and height divisible by <r> (default: 1).

yvu9
    Forces software YVU9 to YV12 colorspace conversion. Deprecated in favor of
    the software scaler.

yuvcsp
    Clamps YUV color values to the CCIR 601 range without doing real
    conversion.

palette
    RGB/BGR 8 -> 15/16/24/32bpp colorspace conversion using palette.

format[=fourcc[:outfourcc]]
    Restricts the colorspace for the next filter without doing any conversion.
    Use together with the scale filter for a real conversion.

    *NOTE*: For a list of available formats see ``format=fmt=help``.

    <fourcc>
        format name like rgb15, bgr24, yv12, etc (default: yuy2)
    <outfourcc>
        Format name that should be substituted for the output. If this is not
        100% compatible with the <fourcc> value it will crash.

        *EXAMPLE*

        ====================== =====================
        Valid                  Invalid (will crash)
        ====================== =====================
        ``format=rgb24:bgr24`` ``format=rgb24:yv12``
        ``format=yuyv:yuy2``
        ====================== =====================

noformat[=fourcc]
    Restricts the colorspace for the next filter without doing any conversion.
    Unlike the format filter, this will allow any colorspace except the one
    you specify.

    *NOTE*: For a list of available formats see ``noformat=fmt=help``.

    <fourcc>
        format name like rgb15, bgr24, yv12, etc (default: yv12)

pp[=filter1[:option1[:option2...]]/[-]filter2...]
    Enables the specified chain of postprocessing subfilters. Subfilters must
    be separated by '/' and can be disabled by prepending a '-'. Each
    subfilter and some options have a short and a long name that can be used
    interchangeably, i.e. dr/dering are the same. All subfilters share common
    options to determine their scope:

    a/autoq
        Automatically switch the subfilter off if the CPU is too slow.
    c/chrom
        Do chrominance filtering, too (default).
    y/nochrom
        Do luminance filtering only (no chrominance).
    n/noluma
        Do chrominance filtering only (no luminance).

    *NOTE*: ``--pphelp`` shows a list of available subfilters.

    Available subfilters are:

    hb/hdeblock[:difference[:flatness]]
        horizontal deblocking filter

        :<difference>: Difference factor where higher values mean more
                       deblocking (default: 32).
        :<flatness>:   Flatness threshold where lower values mean more
                       deblocking (default: 39).

    vb/vdeblock[:difference[:flatness]]
        vertical deblocking filter

        :<difference>: Difference factor where higher values mean more
                       deblocking (default: 32).
        :<flatness>:   Flatness threshold where lower values mean more
                       deblocking (default: 39).

    ha/hadeblock[:difference[:flatness]]
        accurate horizontal deblocking filter

        :<difference>: Difference factor where higher values mean more
                       deblocking (default: 32).
        :<flatness>:   Flatness threshold where lower values mean more
                       deblocking (default: 39).

    va/vadeblock[:difference[:flatness]]
        accurate vertical deblocking filter

        :<difference>: Difference factor where higher values mean more
                       deblocking (default: 32).
        :<flatness>:   Flatness threshold where lower values mean more
                       deblocking (default: 39).

    The horizontal and vertical deblocking filters share the difference and
    flatness values so you cannot set different horizontal and vertical
    thresholds.

    h1/x1hdeblock
        experimental horizontal deblocking filter

    v1/x1vdeblock
        experimental vertical deblocking filter

    dr/dering
        deringing filter

    tn/tmpnoise[:threshold1[:threshold2[:threshold3]]]
        temporal noise reducer

        :<threshold1>: larger -> stronger filtering
        :<threshold2>: larger -> stronger filtering
        :<threshold3>: larger -> stronger filtering

    al/autolevels[:f/fullyrange]
        automatic brightness / contrast correction

        :f/fullyrange: Stretch luminance to (0-255).

    lb/linblenddeint
        Linear blend deinterlacing filter that deinterlaces the given block by
        filtering all lines with a (1 2 1) filter.

    li/linipoldeint
        Linear interpolating deinterlacing filter that deinterlaces the given
        block by linearly interpolating every second line.

    ci/cubicipoldeint
        Cubic interpolating deinterlacing filter deinterlaces the given block
        by cubically interpolating every second line.

    md/mediandeint
        Median deinterlacing filter that deinterlaces the given block by
        applying a median filter to every second line.

    fd/ffmpegdeint
        FFmpeg deinterlacing filter that deinterlaces the given block by
        filtering every second line with a (-1 4 2 4 -1) filter.

    l5/lowpass5
        Vertically applied FIR lowpass deinterlacing filter that deinterlaces
        the given block by filtering all lines with a (-1 2 6 2 -1) filter.

    fq/forceQuant[:quantizer]
        Overrides the quantizer table from the input with the constant
        quantizer you specify.

        :<quantizer>: quantizer to use

    de/default
        default pp filter combination (hb:a,vb:a,dr:a)

    fa/fast
        fast pp filter combination (h1:a,v1:a,dr:a)

    ac
        high quality pp filter combination (ha:a:128:7,va:a,dr:a)

    *EXAMPLE*:

    ``--vf=pp=hb/vb/dr/al``
        horizontal and vertical deblocking, deringing and automatic
        brightness/contrast

    ``--vf=pp=de/-al``
        default filters without brightness/contrast correction

    ``--vf=pp=default/tmpnoise:1:2:3``
        Enable default filters & temporal denoiser.

    ``--vf=pp=hb:y/vb:a``
        Horizontal deblocking on luminance only, and switch vertical
        deblocking on or off automatically depending on available CPU time.

spp[=quality[:qp[:mode]]]
    Simple postprocessing filter that compresses and decompresses the image at
    several (or - in the case of quality level 6 - all) shifts and averages
    the results.

    <quality>
        0-6 (default: 3)

    <qp>
        Force quantization parameter (default: 0, use QP from video).

    <mode>

        :0: hard thresholding (default)
        :1: soft thresholding (better deringing, but blurrier)
        :4: like 0, but also use B-frames' QP (may cause flicker)
        :5: like 1, but also use B-frames' QP (may cause flicker)

uspp[=quality[:qp]]
    Ultra simple & slow postprocessing filter that compresses and decompresses
    the image at several (or - in the case of quality level 8 - all) shifts
    and averages the results.

    The way this differs from the behavior of spp is that uspp actually
    encodes & decodes each case with libavcodec Snow, whereas spp uses a
    simplified intra only 8x8 DCT similar to MJPEG.

    <quality>
        0-8 (default: 3)

    <qp>
        Force quantization parameter (default: 0, use QP from video).

fspp[=quality[:qp[:strength[:bframes]]]]
    faster version of the simple postprocessing filter

    <quality>
        4-5 (equivalent to spp; default: 4)

    <qp>
        Force quantization parameter (default: 0, use QP from video).

    <-15-32>
        Filter strength, lower values mean more details but also more
        artifacts, while higher values make the image smoother but also
        blurrier (default: 0 - PSNR optimal).

    <bframes>
        0: do not use QP from B-frames (default)
        1: use QP from B-frames too (may cause flicker)

pp7[=qp[:mode]]
    Variant of the spp filter, similar to spp=6 with 7 point DCT where only
    the center sample is used after IDCT.

    <qp>
        Force quantization parameter (default: 0, use QP from video).

    <mode>
        :0: hard thresholding
        :1: soft thresholding (better deringing, but blurrier)
        :2: medium thresholding (default, good results)

qp=equation
    quantization parameter (QP) change filter

    <equation>
        some equation like ``2+2*sin(PI*qp)``

geq=equation
    generic equation change filter

    <equation>
        Some equation, e.g. ``p(W-X\,Y)`` to flip the image horizontally. You
        can use whitespace to make the equation more readable. There are a
        couple of constants that can be used in the equation:

        :PI:      the number pi
        :E:       the number e
        :X / Y:   the coordinates of the current sample
        :W / H:   width and height of the image
        :SW / SH: width/height scale depending on the currently filtered plane,
                  e.g. 1,1 and 0.5,0.5 for YUV 4:2:0.
        :p(x,y):  returns the value of the pixel at location x/y of the current
                  plane.

test
    Generate various test patterns.

rgbtest[=width:height]
    Generate an RGB test pattern useful for detecting RGB vs BGR issues. You
    should see a red, green and blue stripe from top to bottom.

    <width>
        Desired width of generated image (default: 0). 0 means width of input
        image.

    <height>
        Desired height of generated image (default: 0). 0 means height of
        input image.

lavc[=quality:fps]
    Fast software YV12 to MPEG-1 conversion with libavcodec for use with
    DVB/DXR3/IVTV/V4L2.

    <quality>
        :1-31: fixed qscale
        :32-:  fixed bitrate in kbits

    <fps>
        force output fps (float value) (default: 0, autodetect based on height)

dvbscale[=aspect]
    Set up optimal scaling for DVB cards, scaling the x axis in hardware and
    calculating the y axis scaling in software to keep aspect. Only useful
    together with expand and scale.

    <aspect>
        Control aspect ratio, calculate as ``DVB_HEIGHT*ASPECTRATIO`` (default:
        ``576*4/3=768``), set it to ``576*(16/9)=1024`` for a 16:9 TV.

    *EXAMPLE*:

    ``--vf=dvbscale,scale=-1:0,expand=-1:576:-1:-1:1,lavc``
        FIXME: Explain what this does.

noise[=luma[u][t|a][h][p]:chroma[u][t|a][h][p]]
    Adds noise.

    :<0-100>: luma noise
    :<0-100>: chroma noise
    :u:       uniform noise (gaussian otherwise)
    :t:       temporal noise (noise pattern changes between frames)
    :a:       averaged temporal noise (smoother, but a lot slower)
    :h:       high quality (slightly better looking, slightly slower)
    :p:       mix random noise with a (semi)regular pattern

denoise3d[=luma_spatial:chroma_spatial:luma_tmp:chroma_tmp]
    This filter aims to reduce image noise producing smooth images and making
    still images really still (This should enhance compressibility.).

    <luma_spatial>
        spatial luma strength (default: 4)
    <chroma_spatial>
        spatial chroma strength (default: 3)
    <luma_tmp>
        luma temporal strength (default: 6)
    <chroma_tmp>
        chroma temporal strength (default:
        ``luma_tmp*chroma_spatial/luma_spatial``)

hqdn3d[=luma_spatial:chroma_spatial:luma_tmp:chroma_tmp]
    High precision/quality version of the denoise3d filter. Parameters and
    usage are the same.

ow[=depth[:luma_strength[:chroma_strength]]]
    Overcomplete Wavelet denoiser.

    <depth>
        Larger depth values will denoise lower frequency components more, but
        slow down filtering (default: 8).
    <luma_strength>
        luma strength (default: 1.0)
    <chroma_strength>
        chroma strength (default: 1.0)

eq[=brightness:contrast] (OBSOLETE)
    Software equalizer with interactive controls just like the hardware
    equalizer, for cards/drivers that do not support brightness and contrast
    controls in hardware.

    <-100-100>
        initial brightness
    <-100-100>
        initial contrast

eq2[=gamma:contrast:brightness:saturation:rg:gg:bg:weight]
    Alternative software equalizer that uses lookup tables (very slow),
    allowing gamma correction in addition to simple brightness and contrast
    adjustment. Note that it uses the same MMX optimized code as ``--vf=eq``
    if all gamma values are 1.0. The parameters are given as floating point
    values.

    <0.1-10>
        initial gamma value (default: 1.0)
    <-2-2>
        initial contrast, where negative values result in a negative image
        (default: 1.0)
    <-1-1>
        initial brightness (default: 0.0)
    <0-3>
        initial saturation (default: 1.0)
    <0.1-10>
        gamma value for the red component (default: 1.0)
    <0.1-10>
        gamma value for the green component (default: 1.0)
    <0.1-10>
        gamma value for the blue component (default: 1.0)
    <0-1>
        The weight parameter can be used to reduce the effect of a high gamma
        value on bright image areas, e.g. keep them from getting overamplified
        and just plain white. A value of 0.0 turns the gamma correction all
        the way down while 1.0 leaves it at its full strength (default: 1.0).

hue[=hue:saturation]
    Software equalizer with interactive controls just like the hardware
    equalizer, for cards/drivers that do not support hue and saturation
    controls in hardware.

    <-180-180>
        initial hue (default: 0.0)
    <-100-100>
        initial saturation, where negative values result in a negative chroma
        (default: 1.0)

halfpack[=f]
    Convert planar YUV 4:2:0 to half-height packed 4:2:2, downsampling luma
    but keeping all chroma samples. Useful for output to low-resolution
    display devices when hardware downscaling is poor quality or is not
    available. Can also be used as a primitive luma-only deinterlacer with
    very low CPU usage.

    <f>
        By default, halfpack averages pairs of lines when downsampling. Any
        value different from 0 or 1 gives the default (averaging) behavior.

        :0: Only use even lines when downsampling.
        :1: Only use odd lines when downsampling.

ilpack[=mode]
    When interlaced video is stored in YUV 4:2:0 formats, chroma interlacing
    does not line up properly due to vertical downsampling of the chroma
    channels. This filter packs the planar 4:2:0 data into YUY2 (4:2:2) format
    with the chroma lines in their proper locations, so that in any given
    scanline, the luma and chroma data both come from the same field.

    <mode>
        Select the sampling mode.

        :0: nearest-neighbor sampling, fast but incorrect
        :1: linear interpolation (default)

decimate[=max:hi:lo:frac]
    Drops frames that do not differ greatly from the previous frame in order
    to reduce framerate. The main use of this filter is for very-low- bitrate
    encoding (e.g. streaming over dialup modem), but it could in theory be
    used for fixing movies that were inverse-telecined incorrectly.

    <max>
        Sets the maximum number of consecutive frames which can be dropped (if
        positive), or the minimum interval between dropped frames (if
        negative).
    <hi>,<lo>,<frac>
        A frame is a candidate for dropping if no 8x8 region differs by more
        than a threshold of <hi>, and if not more than <frac> portion (1
        meaning the whole image) differs by more than a threshold of <lo>.
        Values of <hi> and <lo> are for 8x8 pixel blocks and represent actual
        pixel value differences, so a threshold of 64 corresponds to 1 unit of
        difference for each pixel, or the same spread out differently over the
        block.

dint[=sense:level]
    The drop-deinterlace (dint) filter detects and drops the first from a set
    of interlaced video frames.

    <0.0-1.0>
        relative difference between neighboring pixels (default: 0.1)
    <0.0-1.0>
        What part of the image has to be detected as interlaced to drop the
        frame (default: 0.15).

lavcdeint (OBSOLETE)
    FFmpeg deinterlacing filter, same as ``--vf=pp=fd``

kerndeint[=thresh[:map[:order[:sharp[:twoway]]]]]
    Donald Graft's adaptive kernel deinterlacer. Deinterlaces parts of a video
    if a configurable threshold is exceeded.

    <0-255>
        threshold (default: 10)
    <map>
        :0: Ignore pixels exceeding the threshold (default).
        :1: Paint pixels exceeding the threshold white.

    <order>
        :0: Leave fields alone (default).
        :1: Swap fields.

    <sharp>
        :0: Disable additional sharpening (default).
        :1: Enable additional sharpening.

    <twoway>
        :0: Disable twoway sharpening (default).
        :1: Enable twoway sharpening.

unsharp[=l|cWxH:amount[:l|cWxH:amount]]
    unsharp mask / gaussian blur

    l
        Apply effect on luma component.

    c
        Apply effect on chroma components.

    <width>x<height>
        width and height of the matrix, odd sized in both directions (min =
        3x3, max = 13x11 or 11x13, usually something between 3x3 and 7x7)

    amount
        Relative amount of sharpness/blur to add to the image (a sane range
        should be -1.5-1.5).

        :<0: blur
        :>0: sharpen

swapuv
    Swap U & V plane.

il[=d|i][s][:[d|i][s]]
    (De)interleaves lines. The goal of this filter is to add the ability to
    process interlaced images pre-field without deinterlacing them. You can
    filter your interlaced DVD and play it on a TV without breaking the
    interlacing. While deinterlacing (with the postprocessing filter) removes
    interlacing permanently (by smoothing, averaging, etc) deinterleaving
    splits the frame into 2 fields (so called half pictures), so you can
    process (filter) them independently and then re-interleave them.

    :d: deinterleave (placing one above the other)
    :i: interleave
    :s: swap fields (exchange even & odd lines)

fil[=i|d]
    (De)interleaves lines. This filter is very similar to the il filter but
    much faster, the main disadvantage is that it does not always work.
    Especially if combined with other filters it may produce randomly messed
    up images, so be happy if it works but do not complain if it does not for
    your combination of filters.

    :d: Deinterleave fields, placing them side by side.
    :i: Interleave fields again (reversing the effect of fil=d).

field[=n]
    Extracts a single field from an interlaced image using stride arithmetic
    to avoid wasting CPU time. The optional argument n specifies whether to
    extract the even or the odd field (depending on whether n is even or odd).

detc[=var1=value1:var2=value2:...]
    Attempts to reverse the 'telecine' process to recover a clean,
    non-interlaced stream at film framerate. This was the first and most
    primitive inverse telecine filter to be added to MPlayer. It works by
    latching onto the telecine 3:2 pattern and following it as long as
    possible. This makes it suitable for perfectly-telecined material, even in
    the presence of a fair degree of noise, but it will fail in the presence
    of complex post-telecine edits. Development on this filter is no longer
    taking place, as ivtc, pullup, and filmdint are better for most
    applications. The following arguments (see syntax above) may be used to
    control detc's behavior:

    <dr>
        Set the frame dropping mode.

        :0: Do not drop frames to maintain fixed output framerate (default).
        :1: Always drop a frame when there have been no drops or telecine
            merges in the past 5 frames.
        :2: Always maintain exact 5:4 input to output frame ratio.

    <am>
        Analysis mode.

        :0: Fixed pattern with initial frame number specified by <fr>.
        :1: aggressive search for telecine pattern (default)

    <fr>
        Set initial frame number in sequence. 0-2 are the three clean
        progressive frames; 3 and 4 are the two interlaced frames. The
        default, -1, means 'not in telecine sequence'. The number specified
        here is the type for the imaginary previous frame before the movie
        starts.

    <t0>, <t1>, <t2>, <t3>
        Threshold values to be used in certain modes.

ivtc[=1]
    Experimental 'stateless' inverse telecine filter. Rather than trying to
    lock on to a pattern like the detc filter does, ivtc makes its decisions
    independently for each frame. This will give much better results for
    material that has undergone heavy editing after telecine was applied, but
    as a result it is not as forgiving of noisy input, for example TV capture.
    The optional parameter (ivtc=1) corresponds to the dr=1 option for the
    detc filter, and should not be used with MPlayer. Further development on
    ivtc has stopped, as the pullup and filmdint filters appear to be much
    more accurate.

pullup[=jl:jr:jt:jb:sb:mp]
    Third-generation pulldown reversal (inverse telecine) filter, capable of
    handling mixed hard-telecine, 24000/1001 fps progressive, and 30000/1001
    fps progressive content. The pullup filter is designed to be much more
    robust than detc or ivtc, by taking advantage of future context in making
    its decisions. Like ivtc, pullup is stateless in the sense that it does
    not lock onto a pattern to follow, but it instead looks forward to the
    following fields in order to identify matches and rebuild progressive
    frames. It is still under development, but believed to be quite accurate.

    jl, jr, jt, and jb
        These options set the amount of "junk" to ignore at the left, right,
        top, and bottom of the image, respectively. Left/right are in units of
        8 pixels, while top/bottom are in units of 2 lines. The default is 8
        pixels on each side.

    sb (strict breaks)
        Setting this option to 1 will reduce the chances of pullup generating
        an occasional mismatched frame, but it may also cause an excessive
        number of frames to be dropped during high motion sequences.
        Conversely, setting it to -1 will make pullup match fields more
        easily. This may help processing of video where there is slight
        blurring between the fields, but may also cause there to be interlaced
        frames in the output.

    mp (metric plane)
        This option may be set to 1 or 2 to use a chroma plane instead of the
        luma plane for doing pullup's computations. This may improve accuracy
        on very clean source material, but more likely will decrease accuracy,
        especially if there is chroma noise (rainbow effect) or any grayscale
        video. The main purpose of setting mp to a chroma plane is to reduce
        CPU load and make pullup usable in realtime on slow machines.

filmdint[=options]
    Inverse telecine filter, similar to the pullup filter above. It is
    designed to handle any pulldown pattern, including mixed soft and hard
    telecine and limited support for movies that are slowed down or sped up
    from their original framerate for TV. Only the luma plane is used to find
    the frame breaks. If a field has no match, it is deinterlaced with simple
    linear approximation. If the source is MPEG-2, this must be the first
    filter to allow access to the field-flags set by the MPEG-2 decoder.
    Depending on the source MPEG, you may be fine ignoring this advice, as
    long as you do not see lots of "Bottom-first field" warnings. With no
    options it does normal inverse telecine. When this filter is used with
    MPlayer, it will result in an uneven framerate during playback, but it is
    still generally better than using pp=lb or no deinterlacing at all.
    Multiple options can be specified separated by /.

    crop=<w>:<h>:<x>:<y>
        Just like the crop filter, but faster, and works on mixed hard and
        soft telecined content as well as when y is not a multiple of 4. If x
        or y would require cropping fractional pixels from the chroma planes,
        the crop area is extended. This usually means that x and y must be
        even.

    io=<ifps>:<ofps>
        For each ifps input frames the filter will output ofps frames. This
        could be used to filter movies that are broadcast on TV at a frame
        rate different from their original framerate.

    luma_only=<n>
        If n is nonzero, the chroma plane is copied unchanged. This is useful
        for YV12 sampled TV, which discards one of the chroma fields.

    mmx2=<n>
        On x86, if n=1, use MMX2 optimized functions, if n=2, use 3DNow!
        optimized functions, otherwise, use plain C. If this option is not
        specified, MMX2 and 3DNow! are auto-detected, use this option to
        override auto-detection.

    fast=<n>
        The larger n will speed up the filter at the expense of accuracy. The
        default value is n=3. If n is odd, a frame immediately following a
        frame marked with the REPEAT_FIRST_FIELD MPEG flag is assumed to be
        progressive, thus filter will not spend any time on soft-telecined
        MPEG-2 content. This is the only effect of this flag if MMX2 or 3DNow!
        is available. Without MMX2 and 3DNow, if n=0 or 1, the same
        calculations will be used as with n=2 or 3. If n=2 or 3, the number of
        luma levels used to find the frame breaks is reduced from 256 to 128,
        which results in a faster filter without losing much accuracy. If n=4
        or 5, a faster, but much less accurate metric will be used to find the
        frame breaks, which is more likely to misdetect high vertical detail
        as interlaced content.

    verbose=<n>
        If n is nonzero, print the detailed metrics for each frame. Useful for
        debugging.

    dint_thres=<n>
        Deinterlace threshold. Used during de-interlacing of unmatched frames.
        Larger value means less deinterlacing, use n=256 to completely turn
        off deinterlacing. Default is n=8.

    comb_thres=<n>
        Threshold for comparing a top and bottom fields. Defaults to 128.

    diff_thres=<n>
        Threshold to detect temporal change of a field. Default is 128.

    sad_thres=<n>
        Sum of Absolute Difference threshold, default is 64.

divtc[=options]
    Inverse telecine for deinterlaced video. If 3:2-pulldown telecined video
    has lost one of the fields or is deinterlaced using a method that keeps
    one field and interpolates the other, the result is a juddering video that
    has every fourth frame duplicated. This filter is intended to find and
    drop those duplicates and restore the original film framerate. Two
    different modes are available: One pass mode is the default and is
    straightforward to use, but has the disadvantage that any changes in the
    telecine phase (lost frames or bad edits) cause momentary judder until the
    filter can resync again. Two pass mode avoids this by analyzing the whole
    video beforehand so it will have forward knowledge about the phase changes
    and can resync at the exact spot. These passes do *not* correspond to pass
    one and two of the encoding process. You must run an extra pass using
    divtc pass one before the actual encoding throwing the resulting video
    away. Use ``--nosound --ovc=raw -o /dev/null`` to avoid wasting CPU power
    for this pass. You may add something like ``crop=2:2:0:0`` after divtc to
    speed things up even more. Then use divtc pass two for the actual
    encoding. If you use multiple encoder passes, use divtc pass two for all
    of them. The options are:

    pass=1|2
        Use two pass mode.

    file=<filename>
        Set the two pass log filename (default: ``framediff.log``).

    threshold=<value>
        Set the minimum strength the telecine pattern must have for the filter
        to believe in it (default: 0.5). This is used to avoid recognizing
        false pattern from the parts of the video that are very dark or very
        still.

    window=<numframes>
        Set the number of past frames to look at when searching for pattern
        (default: 30). Longer window improves the reliability of the pattern
        search, but shorter window improves the reaction time to the changes
        in the telecine phase. This only affects the one pass mode. The two
        pass mode currently uses fixed window that extends to both future and
        past.

    phase=0|1|2|3|4
        Sets the initial telecine phase for one pass mode (default: 0). The
        two pass mode can see the future, so it is able to use the correct
        phase from the beginning, but one pass mode can only guess. It catches
        the correct phase when it finds it, but this option can be used to fix
        the possible juddering at the beginning. The first pass of the two
        pass mode also uses this, so if you save the output from the first
        pass, you get constant phase result.

    deghost=<value>
        Set the deghosting threshold (0-255 for one pass mode, -255-255 for
        two pass mode, default 0). If nonzero, deghosting mode is used. This
        is for video that has been deinterlaced by blending the fields
        together instead of dropping one of the fields. Deghosting amplifies
        any compression artifacts in the blended frames, so the parameter
        value is used as a threshold to exclude those pixels from deghosting
        that differ from the previous frame less than specified value. If two
        pass mode is used, then negative value can be used to make the filter
        analyze the whole video in the beginning of pass-2 to determine
        whether it needs deghosting or not and then select either zero or the
        absolute value of the parameter. Specify this option for pass-2, it
        makes no difference on pass-1.

phase[=t|b|p|a|u|T|B|A|U][:v]
    Delay interlaced video by one field time so that the field order changes.
    The intended use is to fix PAL movies that have been captured with the
    opposite field order to the film-to-video transfer. The options are:

    t
        Capture field order top-first, transfer bottom-first. Filter will
        delay the bottom field.

    b
        Capture bottom-first, transfer top-first. Filter will delay the top
        field.

    p
        Capture and transfer with the same field order. This mode only exists
        for the documentation of the other options to refer to, but if you
        actually select it, the filter will faithfully do nothing ;-)

    a
        Capture field order determined automatically by field flags, transfer
        opposite. Filter selects among t and b modes on a frame by frame basis
        using field flags. If no field information is available, then this
        works just like u.

    u
        Capture unknown or varying, transfer opposite. Filter selects among t
        and b on a frame by frame basis by analyzing the images and selecting
        the alternative that produces best match between the fields.

    T
        Capture top-first, transfer unknown or varying. Filter selects among t
        and p using image analysis.

    B
        Capture bottom-first, transfer unknown or varying. Filter selects
        among b and p using image analysis.

    A
        Capture determined by field flags, transfer unknown or varying. Filter
        selects among t, b and p using field flags and image analysis. If no
        field information is available, then this works just like U. This is
        the default mode.

    U
        Both capture and transfer unknown or varying. Filter selects among t,
        b and p using image analysis only.

    v
        Verbose operation. Prints the selected mode for each frame and the
        average squared difference between fields for t, b, and p
        alternatives.

telecine[=start]
    Apply 3:2 'telecine' process to increase framerate by 20%. This most
    likely will not work correctly with MPlayer. The optional start parameter
    tells the filter where in the telecine pattern to start (0-3).

tinterlace[=mode]
    Temporal field interlacing - merge pairs of frames into an interlaced
    frame, halving the framerate. Even frames are moved into the upper field,
    odd frames to the lower field. This can be used to fully reverse the
    effect of the tfields filter (in mode 0). Available modes are:

    :0: Move odd frames into the upper field, even into the lower field,
        generating a full-height frame at half framerate.
    :1: Only output odd frames, even frames are dropped; height unchanged.
    :2: Only output even frames, odd frames are dropped; height unchanged.
    :3: Expand each frame to full height, but pad alternate lines with black;
        framerate unchanged.
    :4: Interleave even lines from even frames with odd lines from odd frames.
        Height unchanged at half framerate.

tfields[=mode[:field_dominance]]
    Temporal field separation - split fields into frames, doubling the output
    framerate.

    <mode>
        :0: Leave fields unchanged (will jump/flicker).
        :1: Interpolate missing lines. (The algorithm used might not be so
            good.)
        :2: Translate fields by 1/4 pixel with linear interpolation (no jump).
        :4: Translate fields by 1/4 pixel with 4tap filter (higher quality)
            (default).

    <field_dominance> (DEPRECATED)
        :-1: auto (default) Only works if the decoder exports the appropriate
             information and no other filters which discard that information
             come before tfields in the filter chain, otherwise it falls back
             to 0 (top field first).
        :0:  top field first
        :1:  bottom field first

        *NOTE*: This option will possibly be removed in a future version. Use
        ``--field-dominance`` instead.

yadif=[mode[:field_dominance]]
    Yet another deinterlacing filter

    <mode>
        :0: Output 1 frame for each frame.
        :1: Output 1 frame for each field.
        :2: Like 0 but skips spatial interlacing check.
        :3: Like 1 but skips spatial interlacing check.

    <field_dominance> (DEPRECATED)
        Operates like tfields.

        *NOTE*: This option will possibly be removed in a future version. Use
        ``--field-dominance`` instead.

mcdeint=[mode[:parity[:qp]]]
    Motion compensating deinterlacer. It needs one field per frame as input
    and must thus be used together with tfields=1 or yadif=1/3 or equivalent.

    <mode>
        :0: fast
        :1: medium
        :2: slow, iterative motion estimation
        :3: extra slow, like 2 plus multiple reference frames

    <parity>
        0 or 1 selects which field to use (note: no autodetection yet!).

    <qp>
        Higher values should result in a smoother motion vector field but less
        optimal individual vectors.

boxblur=radius:power[:radius:power]
    box blur

    <radius>
        blur filter strength
    <power>
        number of filter applications

sab=radius:pf:colorDiff[:radius:pf:colorDiff]
    shape adaptive blur

    <radius>
        blur filter strength (~0.1-4.0) (slower if larger)
    <pf>
        prefilter strength (~0.1-2.0)
    <colorDiff>
        maximum difference between pixels to still be considered (~0.1-100.0)

smartblur=radius:strength:threshold[:radius:strength:threshold]
    smart blur

    <radius>
        blur filter strength (~0.1-5.0) (slower if larger)
    <strength>
        blur (0.0-1.0) or sharpen (-1.0-0.0)
    <threshold>
        filter all (0), filter flat areas (0-30) or filter edges (-30-0)

perspective=x0:y0:x1:y1:x2:y2:x3:y3:t
    Correct the perspective of movies not filmed perpendicular to the screen.

    <x0>,<y0>,...
        coordinates of the top left, top right, bottom left, bottom right
        corners
    <t>
        linear (0) or cubic resampling (1)

2xsai
    Scale and smooth the image with the 2x scale and interpolate algorithm.

1bpp
    1bpp bitmap to YUV/BGR 8/15/16/32 conversion

down3dright[=lines]
    Reposition and resize stereoscopic images. Extracts both stereo fields and
    places them side by side, resizing them to maintain the original movie
    aspect.

    <lines>
        number of lines to select from the middle of the image (default: 12)

bmovl=hidden:opaque:fifo
    The bitmap overlay filter reads bitmaps from a FIFO and displays them on
    top of the movie, allowing some transformations on the image. See also
    ``TOOLS/bmovl-test.c`` for a small bmovl test program.

    <hidden>
        Set the default value of the 'hidden' flag (0=visible, 1=hidden).
    <opaque>
        Set the default value of the 'opaque' flag (0=transparent, 1=opaque).
    <fifo>
        path/filename for the FIFO (named pipe connecting ``mplayer
        --vf=bmovl`` to the controlling application)

    FIFO commands are:

    RGBA32 width height xpos ypos alpha clear
        followed by width*height*4 Bytes of raw RGBA32 data.
    ABGR32 width height xpos ypos alpha clear
        followed by width*height*4 Bytes of raw ABGR32 data.
    RGB24 width height xpos ypos alpha clear
        followed by width*height*3 Bytes of raw RGB24 data.
    BGR24 width height xpos ypos alpha clear
        followed by width*height*3 Bytes of raw BGR24 data.
    ALPHA width height xpos ypos alpha
        Change alpha transparency of the specified area.
    CLEAR width height xpos ypos
        Clear area.
    OPAQUE
        Disable all alpha transparency. Send "ALPHA 0 0 0 0 0" to enable it
        again.
    HIDE
        Hide bitmap.
    SHOW
        Show bitmap.

    Arguments are:

    <width>, <height>
        image/area size
    <xpos>, <ypos>
        Start blitting at position x/y.
    <alpha>
        Set alpha difference. If you set this to -255 you can then send a
        sequence of ALPHA-commands to set the area to -225, -200, -175 etc for
        a nice fade-in-effect! ;)

        :0:    same as original
        :255:  Make everything opaque.
        :-255: Make everything transparent.

    <clear>
        Clear the framebuffer before blitting.

        :0: The image will just be blitted on top of the old one, so you do
            not need to send 1.8MB of RGBA32 data every time a small part of
            the screen is updated.
        :1: clear

framestep=I|[i]step
    Renders only every nth frame or every intra frame (keyframe).

    If you call the filter with I (uppercase) as the parameter, then *only*
    keyframes are rendered. For DVDs it generally means one in every 15/12
    frames (IBBPBBPBBPBBPBB), for AVI it means every scene change or every
    keyint value.

    When a keyframe is found, an 'I!' string followed by a newline character
    is printed, leaving the current line of MPlayer output on the screen,
    because it contains the time (in seconds) and frame number of the keyframe
    (You can use this information to split the AVI.).

    If you call the filter with a numeric parameter 'step' then only one in
    every 'step' frames is rendered.

    If you put an 'i' (lowercase) before the number then an 'I!' is printed
    (like the I parameter).

    If you give only the i then nothing is done to the frames, only I! is
    printed.

tile=xtiles:ytiles:output:start:delta
    Tile a series of images into a single, bigger image. If you omit a
    parameter or use a value less than 0, then the default value is used. You
    can also stop when you are satisfied (``... --vf=tile=10:5 ...``). It is
    probably a good idea to put the scale filter before the tile :-)

    The parameters are:

    <xtiles>
        number of tiles on the x axis (default: 5)
    <ytiles>
        number of tiles on the y axis (default: 5)
    <output>
        Render the tile when 'output' number of frames are reached, where
        'output' should be a number less than xtile * ytile. Missing tiles are
        left blank. You could, for example, write an 8 * 7 tile every 50
        frames to have one image every 2 seconds @ 25 fps.
    <start>
        outer border thickness in pixels (default: 2)
    <delta>
        inner border thickness in pixels (default: 4)

delogo[=x:y:w:h:t]
    Suppresses a TV station logo by a simple interpolation of the surrounding
    pixels. Just set a rectangle covering the logo and watch it disappear (and
    sometimes something even uglier appear - your mileage may vary).

    <x>,<y>
        top left corner of the logo
    <w>,<h>
        width and height of the cleared rectangle
    <t>
        Thickness of the fuzzy edge of the rectangle (added to w and h). When
        set to -1, a green rectangle is drawn on the screen to simplify
        finding the right x,y,w,h parameters.
    file=<file>
        You can specify a text file to load the coordinates from.  Each line
        must have a timestamp (in seconds, and in ascending order) and the
        "x:y:w:h:t" coordinates (*t* can be omitted).

remove-logo=/path/to/logo_bitmap_file_name.pgm
    Suppresses a TV station logo, using a PGM or PPM image file to determine
    which pixels comprise the logo. The width and height of the image file
    must match those of the video stream being processed. Uses the filter
    image and a circular blur algorithm to remove the logo.

    ``/path/to/logo_bitmap_file_name.pgm``
        [path] + filename of the filter image.

screenshot
    Allows acquiring screenshots of the movie using slave mode commands that
    can be bound to keypresses. See the slave mode documentation and the
    ``INTERACTIVE CONTROL`` section for details. Files named ``shotNNNN.png``
    will be saved in the working directory, using the first available number -
    no files will be overwritten. The filter has no overhead when not used. It
    does however break playback in some cases, especially VDPAU hardware
    decoding is incompatible with the filter. Thus it is not completely safe to
    add the filter to default configuration "just in case you might want to
    take screenshots". Make sure that the screenshot filter is added after all
    other filters whose effect you want to record on the saved image. E.g. it
    should be the last filter if you want to have an exact screenshot of what
    you see on the monitor.

ass
    Moves SSA/ASS subtitle rendering to an arbitrary point in the filter
    chain. See the ``--ass`` option.

    *EXAMPLE*:

    ``--vf=ass,screenshot``
        Moves SSA/ASS rendering before the screenshot filter. Screenshots
        taken this way will contain subtitles.

blackframe[=amount:threshold]
    Detect frames that are (almost) completely black. Can be useful to detect
    chapter transitions or commercials. Output lines consist of the frame
    number of the detected frame, the percentage of blackness, the frame type
    and the frame number of the last encountered keyframe.

    <amount>
        Percentage of the pixels that have to be below the threshold (default:
        98).

    <threshold>
        Threshold below which a pixel value is considered black (default: 32).

stereo3d[=in:out]
    Stereo3d converts between different stereoscopic image formats.

    <in>
        Stereoscopic image format of input. Possible values:

        sbsl or side_by_side_left_first
            side by side parallel (left eye left, right eye right)
        sbsr or side_by_side_right_first
            side by side crosseye (right eye left, left eye right)
        abl or above_below_left_first
            above-below (left eye above, right eye below)
        abl or above_below_right_first
            above-below (right eye above, left eye below)
        ab2l or above_below_half_height_left_first
            above-below with half height resolution (left eye above, right eye
            below)
        ab2r or above_below_half_height_right_first
            above-below with half height resolution (right eye above, left eye
            below)

    <out>
        Stereoscopic image format of output. Possible values are all the input
        formats as well as:

        arcg or anaglyph_red_cyan_gray
            anaglyph red/cyan gray (red filter on left eye, cyan filter on
            right eye)
        arch or anaglyph_red_cyan_half_color
            anaglyph red/cyan half colored (red filter on left eye, cyan filter
            on right eye)
        arcc or anaglyph_red_cyan_color
            anaglyph red/cyan color (red filter on left eye, cyan filter on
            right eye)
        arcd or anaglyph_red_cyan_dubois
            anaglyph red/cyan color optimized with the least squares
            projection of dubois (red filter on left eye, cyan filter on right
            eye)
        agmg or anaglyph_green_magenta_gray
            anaglyph green/magenta gray (green filter on left eye, magenta
            filter on right eye)
        agmh or anaglyph_green_magenta_half_color
            anaglyph green/magenta half colored (green filter on left eye,
            magenta filter on right eye)
        agmc or anaglyph_green_magenta_color
            anaglyph green/magenta colored (green filter on left eye, magenta
            filter on right eye)
        aybg or anaglyph_yellow_blue_gray
            anaglyph yellow/blue gray (yellow filter on left eye, blue filter
            on right eye)
        aybh or anaglyph_yellow_blue_half_color
            anaglyph yellow/blue half colored (yellow filter on left eye, blue
            filter on right eye)
        aybc or anaglyph_yellow_blue_color
            anaglyph yellow/blue colored (yellow filter on left eye, blue
            filter on right eye)
        irl or interleave_rows_left_first
            Interleaved rows (left eye has top row, right eye starts on next
            row)
        irr or interleave_rows_right_first
            Interleaved rows (right eye has top row, left eye starts on next
            row)
        ml or mono_left
            mono output (left eye only)
        mr or mono_right
            mono output (right eye only)

gradfun[=strength[:radius]]
    Fix the banding artifacts that are sometimes introduced into nearly flat
    regions by truncation to 8bit colordepth. Interpolates the gradients that
    should go where the bands are, and dithers them.

    This filter is designed for playback only. Do not use it prior to lossy
    compression, because compression tends to lose the dither and bring back
    the bands.

    <strength>
        Maximum amount by which the filter will change any one pixel. Also the
        threshold for detecting nearly flat regions (default: 1.2).

    <radius>
        Neighborhood to fit the gradient to. Larger radius makes for smoother
        gradients, but also prevents the filter from modifying pixels near
        detailed regions (default: 16).

fixpts[=options]
    Fixes the presentation timestamps (PTS) of the frames. By default, the PTS
    passed to the next filter is dropped, but the following options can change
    that:

    print
        Print the incoming PTS.

    fps=<fps>
        Specify a frame per second value.

    start=<pts>
        Specify an initial value for the PTS.

    autostart=<n>
        Uses the *n*\th incoming PTS as the initial PTS. All previous PTS are
        kept, so setting a huge value or -1 keeps the PTS intact.

    autofps=<n>
        Uses the *n*\th incoming PTS after the end of autostart to determine
        the framerate.

    *EXAMPLE*:

    ``--vf=fixpts=fps=24000/1001,ass,fixpts``
        Generates a new sequence of PTS, uses it for ASS subtitles, then drops
        it. Generating a new sequence is useful when the timestamps are reset
        during the program; this is frequent on DVDs. Dropping it may be
        necessary to avoid confusing encoders.

    *NOTE*: Using this filter together with any sort of seeking (including
    ``--ss``) may make demons fly out of your nose.


ENVIRONMENT VARIABLES
=====================

There are a number of environment variables that can be used to control the
behavior of MPlayer.

``MPLAYER_CHARSET`` (see also ``--msgcharset``)
    Convert console messages to the specified charset (default: autodetect). A
    value of "noconv" means no conversion.

``MPLAYER_HOME``
    Directory where MPlayer looks for user settings.

``MPLAYER_LOCALEDIR``
    Directory where MPlayer looks for gettext translation files (if enabled).

``MPLAYER_VERBOSE`` (see also ``-v`` and ``--msglevel``)
    Set the initial verbosity level across all message modules (default: 0).
    The resulting verbosity corresponds to that of ``--msglevel=5`` plus the
    value of ``MPLAYER_VERBOSE``.

libaf:
    ``LADSPA_PATH``
        If ``LADSPA_PATH`` is set, it searches for the specified file. If it
        is not set, you must supply a fully specified pathname.

        FIXME: This is also mentioned in the ladspa section.

libdvdcss:
    ``DVDCSS_CACHE``
        Specify a directory in which to store title key values. This will
        speed up descrambling of DVDs which are in the cache. The
        ``DVDCSS_CACHE`` directory is created if it does not exist, and a
        subdirectory is created named after the DVD's title or manufacturing
        date. If ``DVDCSS_CACHE`` is not set or is empty, libdvdcss will use
        the default value which is ``${HOME}/.dvdcss/`` under Unix and
        ``C:\Documents and Settings\$USER\Application Data\dvdcss\`` under
        Win32. The special value "off" disables caching.

    ``DVDCSS_METHOD``
        Sets the authentication and decryption method that libdvdcss will use
        to read scrambled discs. Can be one of title, key or disc.

        key
           is the default method. libdvdcss will use a set of calculated
           player keys to try and get the disc key. This can fail if the drive
           does not recognize any of the player keys.

        disc
           is a fallback method when key has failed. Instead of using player
           keys, libdvdcss will crack the disc key using a brute force
           algorithm. This process is CPU intensive and requires 64 MB of
           memory to store temporary data.

        title
           is the fallback when all other methods have failed. It does not
           rely on a key exchange with the DVD drive, but rather uses a crypto
           attack to guess the title key. On rare cases this may fail because
           there is not enough encrypted data on the disc to perform a
           statistical attack, but in the other hand it is the only way to
           decrypt a DVD stored on a hard disc, or a DVD with the wrong region
           on an RPC2 drive.

    ``DVDCSS_RAW_DEVICE``
        Specify the raw device to use. Exact usage will depend on your
        operating system, the Linux utility to set up raw devices is raw(8)
        for instance. Please note that on most operating systems, using a raw
        device requires highly aligned buffers: Linux requires a 2048 bytes
        alignment (which is the size of a DVD sector).

    ``DVDCSS_VERBOSE``
        Sets the libdvdcss verbosity level.

        :0: Outputs no messages at all.
        :1: Outputs error messages to stderr.
        :2: Outputs error messages and debug messages to stderr.

    ``DVDREAD_NOKEYS``
        Skip retrieving all keys on startup. Currently disabled.

    ``HOME``
        FIXME: Document this.

libao2:
    ``AO_SUN_DISABLE_SAMPLE_TIMING``
        FIXME: Document this.

    ``AUDIODEV``
        FIXME: Document this.

    ``AUDIOSERVER``
        Specifies the Network Audio System server to which the nas audio
        output driver should connect and the transport that should be used. If
        unset DISPLAY is used instead. The transport can be one of tcp and
        unix. Syntax is ``tcp/<somehost>:<someport>``,
        ``<somehost>:<instancenumber>`` or ``[unix]:<instancenumber>``. The
        NAS base port is 8000 and <instancenumber> is added to that.

        *EXAMPLES*:

        ``AUDIOSERVER=somehost:0``
             Connect to NAS server on somehost using default port and
             transport.
        ``AUDIOSERVER=tcp/somehost:8000``
             Connect to NAS server on somehost listening on TCP port 8000.
        ``AUDIOSERVER=(unix)?:0``
             Connect to NAS server instance 0 on localhost using unix domain
             sockets.

    ``DISPLAY``
        FIXME: Document this.

osdep:
    ``TERM``
        FIXME: Document this.

libvo:
    ``DISPLAY``
        FIXME: Document this.

    ``FRAMEBUFFER``
        FIXME: Document this.

    ``HOME``
        FIXME: Document this.

libmpdemux:

    ``HOME``
        FIXME: Document this.

    ``HOMEPATH``
        FIXME: Document this.

    ``http_proxy``
        FIXME: Document this.

    ``LOGNAME``
        FIXME: Document this.

    ``USERPROFILE``
        FIXME: Document this.

libavformat:

    ``AUDIO_FLIP_LEFT``
        FIXME: Document this.

    ``BKTR_DEV``
        FIXME: Document this.

    ``BKTR_FORMAT``
        FIXME: Document this.

    ``BKTR_FREQUENCY``
        FIXME: Document this.

    ``http_proxy``
        FIXME: Document this.

    ``no_proxy``
        FIXME: Document this.


FILES
=====

``/usr/local/etc/mplayer/mplayer.conf``
    MPlayer system-wide settings

``~/.mplayer/config``
    MPlayer user settings

``~/.mplayer/input.conf``
    input bindings (see ``--input=keylist`` for the full list)

``~/.mplayer/font/``
    font directory (There must be a ``font.desc`` file and files with ``.RAW``
    extension.)

``~/.mplayer/DVDkeys/``
    cached CSS keys


EXAMPLES OF MPLAYER USAGE
=========================

Quickstart Blu-ray playing:
    - ``mplayer br:////path/to/disc``
    - ``mplayer br:// --bluray-device=/path/to/disc``

Quickstart DVD playing:
    ``mplayer dvd://1``

Play in Japanese with English subtitles:
    ``mplayer dvd://1 --alang=ja --slang=en``

Play only chapters 5, 6, 7:
    ``mplayer dvd://1 --chapter=5-7``

Play only titles 5, 6, 7:
    ``mplayer dvd://5-7``

Play a multiangle DVD:
    ``mplayer dvd://1 --dvdangle=2``

Play from a different DVD device:
    ``mplayer dvd://1 --dvd-device=/dev/dvd2``

Play DVD video from a directory with VOB files:
    ``mplayer dvd://1 --dvd-device=/path/to/directory/``

Copy a DVD title to hard disk, saving to file title1.vob :
    ``mplayer dvd://1 --dumpstream --dumpfile=title1.vob``

Play a DVD with dvdnav from path /dev/sr1:
    ``mplayer dvdnav:////dev/sr1``

Stream from HTTP:
    ``mplayer http://mplayer.hq/example.avi``

Stream using RTSP:
    ``mplayer rtsp://server.example.com/streamName``

Convert subtitles to MPsub format:
    ``mplayer dummy.avi --sub=source.sub --dumpmpsub``

Convert subtitles to MPsub format without watching the movie:
    ``mplayer /dev/zero --rawvideo=pal:fps=xx --demuxer=rawvideo --vc=null --vo=null --noframedrop --benchmark --sub=source.sub --dumpmpsub``

input from standard V4L:
    ``mplayer tv:// --tv=driver=v4l:width=640:height=480:outfmt=i420 --vc=rawi420 --vo=xv``

Play DTS-CD with passthrough:
    ``mplayer --ac=hwdts --rawaudio=format=0x2001 --cdrom-device=/dev/cdrom cdda://``

    You can also use ``--afm=hwac3`` instead of ``--ac=hwdts``. Adjust
    ``/dev/cdrom`` to match the CD-ROM device on your system. If your external
    receiver supports decoding raw DTS streams, you can directly play it via
    ``cdda://`` without setting format, hwac3 or hwdts.

Play a 6-channel AAC file with only two speakers:
    ``mplayer --rawaudio=format=0xff --demuxer=rawaudio --af=pan=2:.32:.32:.39:.06:.06:.39:.17:-.17:-.17:.17:.33:.33 adts_he-aac160_51.aac``

    You might want to play a bit with the pan values (e.g multiply with a
    value) to increase volume or avoid clipping.

checkerboard invert with geq filter:
    ``mplayer --vf=geq='128+(p(X\,Y)-128)*(0.5-gt(mod(X/SW\,128)\,64))*(0.5-gt(mod(Y/SH\,128)\,64))*4'``


AUTHORS
=======

MPlayer was initially written by Arpad Gereoffy. See the ``AUTHORS`` file for
a list of some of the many other contributors.

MPlayer is (C) 2000-2011 The MPlayer Team

This man page was written mainly by Gabucino, Jonas Jermann and Diego Biurrun.
