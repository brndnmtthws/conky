## Conky

[![Build Status](https://travis-ci.org/brndnmtthws/conky.png)](https://travis-ci.org/brndnmtthws/conky)

**Conky** is a free, light-weight system monitor for X, that displays
any kind of information on your desktop.

- Stable version: [**1.9.0**][stable-src]
- Development version: [**master**][devel-src]


### Features

Conky can display more than 300 built-in objects, including support for:

 * A plethora of OS stats (uname, uptime, **CPU usage**, **mem
   usage**, disk usage, **"top"** like process stats, and **network
   monitoring**, just to name a few).
 * Built-in **IMAP** and **POP3** support.
 * Built-in support for many popular music players ([MPD][],
   [XMMS2][], [BMPx][], [Audacious][]).
 * Can be extended using built-in [**Lua**](lua) support, or any of your
   own scripts and programs ([more](Lua Interface)).
 * Built-in [**Imlib2**][Imlib2] and [**Cairo**][cairo] bindings for arbitrary drawing
   with Lua ([more](wiki/Lua-Interface)).

... and much much more.

Conky can display information either as text, or using simple progress
bars and graph widgets, with different fonts and colours.


### Screenshots

[![screenshot](conky/wiki/configs/brenden/screenshot-thumb.png)](conky/wiki/configs/brenden/screenshot.png)
[![screenshot](conky/wiki/configs/ke49/screenshot-thumb.png)](conky/wiki/configs/ke49/screenshot.png)
[![screenshot](conky/wiki/configs/jc/screenshot-thumb.png)](conky/wiki/configs/jc/screenshot.png)

See the [User Configs](conky/wiki/User-Configs) section of the wiki for more
screenshots and their associated conky config files.


### Documentation

The [Github Wiki](conky/wiki) serves as a central hub for all of
Conky's documentation. Quick links:

* [Install](conky/wiki/Installation)
* [Configure](conky/wiki/Configure)
* [User Configs](conky/wiki/User-Configs)
* [Frequently Asked Questions](conky/wiki/Frequently-Asked-Questions)


### License

Conky is licensed under the terms of the [GPLv3](conky/blob/master/LICENSE.GPL) and
[BSD](conky/blob/master/LICENSE.BSD) licenses.


[MPD]: http://musicpd.org/
[XMMS2]: http://wiki.xmms2.xmms.se/index.php/Main_Page
[BMPx]: http://bmpx.backtrace.info/site/BMPx_Homepage
[Audacious]: http://audacious-media-player.org/
[luawiki]: http://en.wikipedia.org/wiki/Lua_%28programming_language%29
[stable-src]: https://github.com/brndnmtthws/conky/archive/1.9.0.tar.gz
[devel-src]: https://github.com/brndnmtthws/conky/archive/master.tar.gz
[wiki]: https://github.com/brndnmtthws/conky/wiki
[lists]: http://sourceforge.net/mail/?group_id=143975
[ircconky]: irc://irc.freenode.net/conky
[Imlib2]: http://docs.enlightenment.org/api/imlib2/html/
[cairo]: http://www.cairographics.org/

