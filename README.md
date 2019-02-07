j4status
========

j4status, a small daemon to act on remote or local events


Website
-------

To get further information, please visit j4status’s website at:
https://sardemff7.github.io/j4status/


My config
---------

Here is my configuration:

    [Plugins]
    Output = i3bar
    Input = mpd;pulseaudio;nl;sensors;upower;time;

    [Time]
    Zones = Europe/Paris;UTC;
    Formats = %F %a %T;%T;

    [Sensors]
    Sensors = coretemp-isa-0000;

    [MPD]
    Actions=mouse:1 toggle;mouse:4 previous;mouse:5 next;

    [PulseAudio]
    Actions=mouse:1 mute toggle;mouse:4 raise;mouse:5 lower;

    [Override sensors:coretemp-isa-0000/temp1]
    Disable=true

    [Netlink]
    Interfaces=eth;wlan;


Build from Git
--------------

To build j4status from Git, you will need some additional dependencies:
- Meson 0.47.0 (or newer)
- pkg-config 0.25 (or newer) or pkgconf 0.2 (or newer)

Make sure to clone the repository with submodules: `git clone --recursive`
Alternatively, you can clone them as a second step: `git submodule update --init`
