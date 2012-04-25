.. _audio_filters:

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
