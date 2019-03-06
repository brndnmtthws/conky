[![Build Status](https://travis-ci.org/brndnmtthws/conky.svg?branch=master)](https://travis-ci.org/brndnmtthws/conky) [![pipeline status](https://gitlab.com/brndnmtthws-oss/conky/badges/master/pipeline.svg)](https://gitlab.com/brndnmtthws-oss/conky/commits/master) [![coverage report](https://gitlab.com/brndnmtthws-oss/conky/badges/master/coverage.svg)](https://gitlab.com/brndnmtthws-oss/conky/commits/master)

<p align="center"><img width="300" src="logo/conky-logotype-horizontal-violet.png"></p>

**Conky** is a free, light-weight system monitor for X, that displays
any kind of information on your desktop.

👉 Grab the [latest release from GitHub](https://github.com/brndnmtthws/conky/releases/latest).

[📹 An introduction to Conky (YouTube)](https://www.youtube.com/watch?v=bHtpLEoRKmg&t=19s).

## Features

Conky can display more than 300 built-in objects, including support for:

- A plethora of OS stats (uname, uptime, **CPU usage**, **mem
  usage**, disk usage, **"top"** like process stats, and **network
  monitoring**, just to name a few).
- Built-in **IMAP** and **POP3** support.
- Built-in support for many popular music players ([MPD][],
  [XMMS2][], [Audacious][]).
- Can be extended using built-in [**Lua**](lua) support, or any of your
  own scripts and programs ([more](https://github.com/brndnmtthws/conky/wiki#tutorial)).
- Built-in [**Imlib2**][imlib2] and [**Cairo**][cairo] bindings for arbitrary drawing
  with Lua ([more](https://github.com/brndnmtthws/conky/wiki/Lua)).
- Runs on Linux, FreeBSD, OpenBSD, DragonFlyBSD, NetBSD, Solaris, Haiku OS, and macOS!

... and much much more.

Conky can display information either as text, or using simple progress
bars and graph widgets, with different fonts and colours.

## Screenshots

[![screenshot](https://github.com/brndnmtthws/conky/wiki/configs/brenden/screenshot-thumb.png)](https://raw.github.com/wiki/brndnmtthws/conky/configs/brenden/screenshot.png)
[![screenshot](https://github.com/brndnmtthws/conky/wiki/configs/ke49/screenshot-thumb.png)](https://raw.github.com/wiki/brndnmtthws/conky/configs/ke49/screenshot.png)
[![screenshot](https://github.com/brndnmtthws/conky/wiki/configs/jc/screenshot-thumb.png)](https://raw.github.com/wiki/brndnmtthws/conky/configs/jc/screenshot.png)

See the User Configs below for more screenshots and associated config files.

## Quickstart

Conky comes bundled with many package managers. However, if you'd like to try the latest release of Conky, you can try the AppImage build. If you have `jq` and `curl` installed, run the following command to fetch the latest AppImage:

```ShellSession
$ curl -sL -o conky-x86_64.AppImage \
    $(curl -sL https://api.github.com/repos/brndnmtthws/conky/releases/latest | \
    jq --raw-output '.assets[0] | .browser_download_url')
$ ls
conky-x86_64.AppImage
```

If you don't have `jq` and `curl` installed, go to
https://github.com/brndnmtthws/conky/releases/latest and fetch the latest
AppImage. Then:

```ShellSession
$ chmod +x ./conky-x86_64.AppImage
$ ./conky-x86_64.AppImage -C > ~/.conkyrc
$ ./conky-x86_64.AppImage
```

And that's it! [Check out the Wiki](https://github.com/brndnmtthws/conky/wiki) for more details on configuring Conky.

_Note_: To use the AppImage, you may need to install additional runtime libraries.

## Documentation

The [GitHub Wiki](https://github.com/brndnmtthws/conky/wiki) serves as a central hub for all of
Conky's documentation.

- [Installation](https://github.com/brndnmtthws/conky/wiki/Installation)
- [Configuration Settings](https://github.com/brndnmtthws/conky/wiki/Configurations)
- [User Configs](https://github.com/brndnmtthws/conky/wiki/Configs)
- [Frequently Asked Questions](https://github.com/brndnmtthws/conky/wiki/FAQ)

## License

Conky is licensed under the terms of the [GPLv3](LICENSE) license.

## Contributing

Contributions are welcome from anyone.

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on contributing to Conky.

[mpd]: https://musicpd.org/
[xmms2]: https://github.com/xmms2/wiki/wiki
[audacious]: https://audacious-media-player.org/
[luawiki]: https://en.wikipedia.org/wiki/Lua_%28programming_language%29
[imlib2]: https://docs.enlightenment.org/api/imlib2/html/
[cairo]: https://www.cairographics.org/
