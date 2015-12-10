j4status
========

j4status, a small daemon to act on remote or local events


Website
-------

To get further information, please visit j4status’s website at:
http://j4status.j4tools.org/

You can also browse man pages online here:
http://j4status.j4tools.org/man/


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

And the result in i3bar:

![](http://j4status.j4tools.org/img/j4status-i3bar.png)


Build from Git
--------------

To build j4status from Git, you will need some additional dependencies:
- autoconf 2.65 (or newer)
- automake 1.11 (or newer)
- libtool
- pkg-config 0.25 (or newer) or pkgconf 0.2 (or newer)
