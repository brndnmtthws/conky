[![Build Status](https://travis-ci.org/brndnmtthws/conky.svg?branch=master)](https://travis-ci.org/brndnmtthws/conky) [![Quality Gate](https://sonarcloud.io/api/project_badges/measure?project=conky&metric=alert_status)](https://sonarcloud.io/dashboard/index/conky) [![Maintainability](https://sonarcloud.io/api/project_badges/measure?project=conky&metric=sqale_rating)](https://sonarcloud.io/dashboard/index/conky) [![Code Coverage](https://sonarcloud.io/api/project_badges/measure?project=conky&metric=coverage)](https://sonarcloud.io/dashboard/index/conky) [![Sonarcloud Bugs](https://sonarcloud.io/api/project_badges/measure?project=conky&metric=bugs)](https://sonarcloud.io/dashboard/index/conky)
<p align="center"><img width="300" src="logo/logotype-horizontal-violet.png"></p>

**Conky** is a free, light-weight system monitor for X, that displays
any kind of information on your desktop.

👉 Grab the [latest release from GitHub](https://github.com/brndnmtthws/conky/releases/latest).

[📹 An introduction to Conky (YouTube)](https://www.youtube.com/watch?v=bHtpLEoRKmg&t=19s).

### Features

Conky can display more than 300 built-in objects, including support for:

 * A plethora of OS stats (uname, uptime, **CPU usage**, **mem
   usage**, disk usage, **"top"** like process stats, and **network
   monitoring**, just to name a few).
 * Built-in **IMAP** and **POP3** support.
 * Built-in support for many popular music players ([MPD][],
   [XMMS2][], [BMPx][], [Audacious][]).
 * Can be extended using built-in [**Lua**](lua) support, or any of your
   own scripts and programs ([more](https://github.com/brndnmtthws/conky/wiki#tutorial)).
 * Built-in [**Imlib2**][Imlib2] and [**Cairo**][cairo] bindings for arbitrary drawing
   with Lua ([more](https://github.com/brndnmtthws/conky/wiki/Lua-API)).
 * Runs on Linux, FreeBSD, OpenBSD, DragonFlyBSD, NetBSD, Solaris, Haiku OS, and macOS!

... and much much more.

Conky can display information either as text, or using simple progress
bars and graph widgets, with different fonts and colours.

### Screenshots

[![screenshot](https://github.com/brndnmtthws/conky/wiki/configs/brenden/screenshot-thumb.png)](https://raw.github.com/wiki/brndnmtthws/conky/configs/brenden/screenshot.png)
[![screenshot](https://github.com/brndnmtthws/conky/wiki/configs/ke49/screenshot-thumb.png)](https://raw.github.com/wiki/brndnmtthws/conky/configs/ke49/screenshot.png)
[![screenshot](https://github.com/brndnmtthws/conky/wiki/configs/jc/screenshot-thumb.png)](https://raw.github.com/wiki/brndnmtthws/conky/configs/jc/screenshot.png)

See the User Configs below for more screenshots and associated config files.

### Documentation

The [GitHub Wiki](https://github.com/brndnmtthws/conky/wiki) serves as a central hub for all of
Conky's documentation.

* [Installation](https://github.com/brndnmtthws/conky/wiki/Installation)
* [Configuration Settings](https://github.com/brndnmtthws/conky/wiki/Configurations)
* [User Configs](https://github.com/brndnmtthws/conky/wiki/Configs)
* [Frequently Asked Questions](https://github.com/brndnmtthws/conky/wiki/FAQ)

### License

Conky is licensed under the terms of the [GPLv3](LICENSE) and
[BSD](LICENSE.BSD) licenses.

### Contributing

To report bugs or issues, open [new issues](https://github.com/brndnmtthws/conky/issues/new).
To submit code changes, open [new pull requests](https://github.com/brndnmtthws/conky/compare).

Patches submitted in issues, email, or elsewhere will likely be ignored. When submitting PRs, please:

 * Describe the changes, why they were necessary, etc
 * Describe how the changes affect existing behaviour
 * Describe how you tested and validated your changes
 * Include any relevant screenshots/evidence demonstrating that the changes work and have been tested
 * Any new source files should include a GPLv3 license header
 * Any contributed code must be GPLv3 licensed

[MPD]: https://musicpd.org/
[XMMS2]: https://github.com/xmms2/wiki/wiki
[BMPx]: https://www.beep-media-player.org/
[Audacious]: https://audacious-media-player.org/
[luawiki]: https://en.wikipedia.org/wiki/Lua_%28programming_language%29
[Imlib2]: https://docs.enlightenment.org/api/imlib2/html/
[cairo]: https://www.cairographics.org/
