![guglielmo](/images/guglielmo.png)
# Guglielmo

Guglielmo implements a simple FM and DAB receiver based on Qt and the Qt-dab and sdr-j-fm packages.

The primary reason it is being developed is there is a lack of media centre quality Open Source
Software Defined Radios: most of the packages out there focus more on hobbyist features,
such as signal and content monitoring, leaving out media features like a volume slider or MPRIS control.

Yes, I have blown the ribbon tweeter fuses on my maggies because my previous go to SDR DAB receiver
started at full blast, and I run my media centre headless: I don't really want to scramble for a _VNC_
session when I want to stop the music, when I could simply use _KDE connect_ on my phone.

There is also a distinct lack of FM SDR receivers, which is disappointing, since, at least in the UK,
for reasons of cost, most stations transmit at a fairly poor bitRate, if not downright in mono, and
FM stations seem to still be a better proposition in terms of sound quality.

## Usage

The main window is divided in two: the right side changes depending on the mode, DAB or FM, the left
side being common to both.

### DAB

![DAB screen](/images/dab.png)

In DAB mode, top to bottom, there's the channel name, a list of all the services (AKA stations) in
the current channel, a channel selector, and the previous and next buttons.

Previous and next switch to the previous or next service, if there's a previous or next service to
select, or the previous or next channel if already at either end of the service list.

Or you can switch service or channel using the appropriate selectors.

The central display can switch between the services list for the current channel and the slide show
for the current service using the "stations" and "slides" menus accessible from the burger menu.

### FM

![FM screen](/images/fm.png)

FM mode sports a frequency display, and a large, old style, frequency knob.

Turn it either way to select the frequency.

The knob turns several times, much like in the analog days of old.

Below there's a scan down, scan up and stop scan buttons, which can be used to find the previous or
next FM station.

### Presets

![presets](/images/presets.png)

The preset controls are at the bottom of the right widget.

The central selector is used to choose the next preset.

Provided that there's a DAB service selected, or in FM mode, M+ adds the current service or frequency
to the presets, whileM- removes the current station from the presets, if it had been previously added.

### Left widget

On the top left, find the stereo, signal strength and signal quality indicators.

Green is good, red is bad, and blank, no station is playing.

Below, find the DAB or FM station name, and whatever text the station is currently choosing to display.

On the bottom you have the volume and squelch knobs, the DAB and FM buttons, and the play/stop and
record/stop recording buttons.

The squelch is a control that silences the sound when the signal is poor.

The higher the value, the better the signal quality has to be for any sound to be played.
It's only active in FM mode.

### Settings

The burger menu button at the top right activates the about and settings windows.

![settings screen](/images/settings.png)

Currently there are 6 tabs, controlling

- presets
- the UI interface 
- remote control settings
- the sound output
- FM settings
- device settings

There's no tweakable DAB settings on offer.

#### Presets

The presets tab sports a presets editor (rearrange, delete or sort presets) and a station scan facility.

Once a DAB or FM scan has been completed, even partially, stations can be dragged directly to the preset list, or moved there by selecting them and using the move (<) button.

Scan lists can be added to with a new scan, are preserved across settings usages, and can be cleared.

They are not preserved across runs.

#### UI

The actual themes depend on the platform and the build.

#### Remote control

This tab determines if the next / previous track signal sent by MPRIS controllers
switches presets or stations.
This tab is disabled for build that do not support MPRIS.

#### Sound

Choose the sound plugin (Qt or Portaudio), and in case of PortAudio, the output port.

#### FM

This controls some of the FM settings, most notably

- the FM decoder
- the de-emphasis filter (use 50µs unless you are in the US)
- the parameters for the low pass filter

#### Device

This tab offers a combo box selecting any of the available devices, and for each of the devices

- Automatic Gain Control
- Device gain
- LNA gain (if the device is equipped with a low noise amplifier)

AGC has up to four supported modes: Off, On, Software and Combined.
The supported modes are device dependent.

Off and On are self explanatory, while Software and Combined turn on software based AGC.
For devices that support it, Combined uses both Hardware and Software AGC at the same time.

The gains range depends on the device, and may either be in percentiles (with the actual device
gains remapped) or the actual device gains.
The defaults are mid range.

Note that when AGC is selected, depending on the device, any change to the IF gain may only take
place after AGC is turned off.

## MPRIS control

Guglielmo can send slides to MPRIS controllers and accept volume changes, play, pause, stop signals, as
well as skip to next and skip to previous.

Mpris-qt5 does not implement playlists, and even if it did, I haven't found a single MPRIS controller that
handles them.

For this reason I haven't currently implemented my plan of having playlists for presets and skip previous
and next for stations, and unless matters change, I am unlikely to implement it ever.

For now, you can change the skip track buttons mode of operation between presets and stations.

## Debugging

Two options you can use to produce diagnostic output: -v and -d <component mask>.

The first set the verbosity level, and can be repeated to increase it, the second specifies what
components should produce diagnostic output.

The available components are listed in include/logging.h, to have all possible logging just use -d -1.

There is no need to specify both options: using -d on its own implies -v, and using -v implies -d -1.

## Running

Whether you are using an AppImage or your own build, you are expected to have installed the package(s)
supporting the device you intend to use.

## Building

The executable is created either with qmake or cmake.

Using qmake, modify guglielmo.pro and comment out the devices you don't wish to include in the build.

Run qmake (or qmake-qt5, depending on the qt5 installation), then make, and you'll find the executable in
the bin directory appropriate for your port (eg linux-bin).

For cmake:

	mkdir build
	cd build
	cmake ..
	make

If you want to build supporting only specific devices, use

	cmake .. -DXXX=ON [-DYYY=ON ...]
	
where XXX, YYY, etc are the mnemonics for the devices to include.

Currently supported devices are

- AIRSPY
- SDRPLAY
- SDRPLAY V3
- RTLSDR
- HACKRF
- LIMESDR
- PLUTO

The binary supplied with each version supports all available devices except for PLUTO.

At this moment in time, the only devices tested are the RTL SDR stick (both V3 and V4), the SDRplay RSP1a, 
and the AirSpy Mini and R2..

The RTL SDR stick has been successfully tested on Linux (OpenSuse Leap 15.2 and 15.5, Ubutu 22.04, macOS High
Sierra x86_64, Windows 11), the RSP1a with Linux and Windows, and the AirSpys so far on Linux only.

Windows does build with Visual Studio, and Mingw plus Msys or Msys2, but the process needs a fair
amount of manual intervention.

## TODO

- integrate Qt audio with pulseaudio
- debug verbosity (and logging in general)
- cleanup the source, remove objects that are not needed, consolidate buffer usage
- make loading settings more resilient to duff values

## Acknowledgements, copyright, etc...

The backends have wholesale lifted from Qt-dab and Sdr-j-fm by Jan van Katwijk and
various other contributors.

I have written the interface, integrated the two processors, rewritten the devices to work
against the guglielmo interface and fixed several bugs and improved several things, most
notably the FM and RDS subsystem.

Carl Laufer at rtl-sdr.com, Jon Hudson at sdrplay.com and Youssef Touil at AirSpy deserve
a special mention for having provided hardware that I have used to develop the drivers.

The few icons not included in the Qt default style have been taken from freesvg.org.

My understanding is that they are in the public domain.

The guglielmo icon has been created by myself starting from public domain Guglielmo
Marconi facing left portrait dated 1908.

As with the rest of this package, you are free to use it within the limits of the GPL v2
license - just make sure that you acknowledge my original work wherever you use it.
