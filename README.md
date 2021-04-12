Guglielmo implements a simple FM and DAB receiver based on Qt and the Qt-dab and sdr-j-fm packages.

Usage
-----

Should be quite simple:

On the top left, find the stereo, signal strength and signal quality indicators.
Green is good, red is bad, and blank, no station is playing.
Below, find the DAB or FM station name, and whatever text the station is currently choosing to display.

On the bottom you have the volume and squelch knobs, the DAB and FM buttons, and the play/stop and
record/stop recording buttons.

The squelch is a control that silences the sound when the signal is poor.
The higher the value, the better the signal quality has to be for any sound to be played.
It's only active in FM mode.

On the right, the widgets change between DAB and FM modes.

In DAB mode, top to bottom, there's the channel name, a list of all the services (AKA stations) in
the current channel, a channel selector, and the previous and next buttons.
Previous and next switch to the previous or next service, if there's a previous or next service to
select, or the previous or next channel if already at either end of the service list.
Or you can switch service or channel using the appropriate selectors.

FM mode sports a frequency display, and a large, old style, frequency knob.
Turn it either way to select the frequency.
The knob turns several times, much like in the analog days of old.
Below there's a scan down, scan up and stop scan buttons, which can be used to find the previous or
next FM station.

The preset controls are at the bottom.
M+ adds the current service or frequency to the presets.
M- removes the current station from the presets, if it had been previously added.

The burger menu button at the top right activates the about and settings windows.

Building
--------

The executable is created either with qmake or cmake.
Using qmake, modify guglielmo.pro and comment out the devices you don't wish to include in the build.
Run qmake (or qmake-qt5, depending on the qt5 installation), then make, and you'll find the executable in
the bin directory appropriate for your port (eg linux-bin).
For cmake:
	mkdir build
	cd build
	cmake .. -DXXX=ON -DYYY=ON ... -DZZZ=ON
	make

where XXX, YYY and ZZZ are the mnemonics for the devices to include.

At this moment in time, the only device tested is the SDR RTL stick, and the only build linux.

TODO
----

- recording
- FM scanning
- integrate Qt audio with pulseaudio
- RDS is very signal sensitive at moment
- debug support
- MPRIS support
- cleanup the source, remove objects that are not needed, consolidate buffer usage
- make loading settings more resilient to duff values

Acknowledgements
----------------

The backends have wholesale lifted from Qt-dab and Sdr-j-fm by Jan van Katwijk and
various other contributors.
I have merely integrated the two processors and adapted the devices to work against
the guglielmo interface.

The few icons not included in the Qt default style have been taken from freesvg.org.
My understanding is that they are in the public domain.
The guglielmo icon has been created by myself starting from public domain Guglielmo
Marconi facing left portrait dated 1908.
As with the rest of this package, you are free to use it within the limits of the GPL v2
license - just make sure that you acknowledge my original work wherever you use it.

Marco Greco
marco greco priolo at gmail dot com
