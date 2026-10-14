[![Linux build](https://github.com/brndnmtthws/conky/actions/workflows/build-and-test-linux.yaml/badge.svg)](https://github.com/brndnmtthws/conky/actions/workflows/build-and-test-linux.yaml) [![macOS build](https://github.com/brndnmtthws/conky/actions/workflows/build-and-test-macos.yaml/badge.svg)](https://github.com/brndnmtthws/conky/actions/workflows/build-and-test-macos.yaml) [![Docker build](https://github.com/brndnmtthws/conky/actions/workflows/docker.yaml/badge.svg)](https://github.com/brndnmtthws/conky/actions/workflows/docker.yaml) [![AppImage build](https://github.com/brndnmtthws/conky/actions/workflows/publish-appimage.yml/badge.svg)](https://github.com/brndnmtthws/conky/actions/workflows/publish-appimage.yml)

[📕 Documentation](https://conky.cc/)

[💬 Join the Matrix chat](https://matrix.to/#/#conky:frens.io)

<p align="center"><img width="300" src="data/logo/conky-logotype-horizontal-violet.png"></p>

**Conky** is a free, lightweight system monitor for X that displays any kind of
information on your desktop. It can also run on Wayland (with caveats) and
macOS, and output to your console, a file, or even HTTP (oh my!).

👉 Grab the [latest release from GitHub](https://github.com/brndnmtthws/conky/releases/latest).

[📹 An introduction to Conky (YouTube)](https://www.youtube.com/watch?v=bHtpLEoRKmg&t=19s)

## Features

Conky can display more than 300 built-in objects, including support for:

- A plethora of OS stats (uname, uptime, **CPU usage**, **memory usage**, disk
  usage, **"top"**-like process stats, and **network monitoring**, just to name
  a few).
- Built-in **IMAP** and **POP3** support.
- Built-in support for many popular music players ([MPD][], [XMMS2][],
  [Audacious][]).
- Extensible using built-in [**Lua**][luawiki] support, or any of your own
  scripts and programs ([more](https://github.com/brndnmtthws/conky/wiki#tutorial)).
- Built-in [**Imlib2**][imlib2] and [**Cairo**][cairo] bindings for arbitrary
  drawing with Lua ([more](https://github.com/brndnmtthws/conky/wiki/Lua)).
- Runs on Linux, FreeBSD, OpenBSD, DragonFlyBSD, NetBSD, Solaris, Haiku, and macOS!
- [Docker image](https://hub.docker.com/r/brndnmtthws/conky/) available for amd64, armv7, and aarch64 (aka armv8).

... and much more.

Conky can display information either as text or using simple progress bars and
graph widgets with different fonts and colours, and handle [mouse events](https://github.com/brndnmtthws/conky/wiki/Mouse-Events).

## Screenshots

[![screenshot](https://github.com/brndnmtthws/conky/wiki/configs/brenden/screenshot-thumb.png)](https://raw.github.com/wiki/brndnmtthws/conky/configs/brenden/screenshot.png)
[![screenshot](https://github.com/brndnmtthws/conky/wiki/configs/ke49/screenshot-thumb.png)](https://raw.github.com/wiki/brndnmtthws/conky/configs/ke49/screenshot.png)
[![screenshot](https://github.com/brndnmtthws/conky/wiki/configs/jc/screenshot-thumb.png)](https://raw.github.com/wiki/brndnmtthws/conky/configs/jc/screenshot.png)

See [Community Configs](https://github.com/brndnmtthws/conky/wiki/Community-Configurations) on the Wiki for more screenshots and config files.

## Installing Conky

Many package managers already include Conky. To try the latest version, use the
AppImage or the Nix flake.

### AppImage

Download the latest AppImage from
<https://github.com/brndnmtthws/conky/releases/latest>. Make it executable,
create a default config, and run it:

```ShellSession
chmod +x ./conky-*.AppImage        # make it executable
./conky-*.AppImage -C > ~/.conkyrc # create a default config
./conky-*.AppImage                 # run
```

And that's it! [Check out the Wiki](https://github.com/brndnmtthws/conky/wiki) for more details on configuring Conky.

_Note_: To use the AppImage, you may need to install additional runtime libraries.

### Nix Flake

As of Conky v1.17.0, a Nix flake is available in addition to the upstream Nix
package. Add it to your flake inputs:

```nix
{
  inputs = {
    conky.url = "github:brndnmtthws/conky";
  };
}
```

Then use `inputs.conky.packages.${system}.default` (or
`inputs.conky.packages.${system}.conky` for versions <=v1.19.8) to include the
package. To pin a specific version, add the Git tag to the URL, such as
`github:brndnmtthws/conky?ref=refs/tags/v1.19.8`.

A Conky package is also available in
[nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/os-specific/linux/conky/default.nix).
Although it may lag behind the latest release, it offers more configuration options.

## Documentation

&rarr; [**Reference documentation**](https://conky.cc/) &larr;

The [Conky Wiki](https://github.com/brndnmtthws/conky/wiki) also serves as a
central hub. Key pages include:

- [Installation](https://github.com/brndnmtthws/conky/wiki/Installation)
- [Configuration Settings](https://github.com/brndnmtthws/conky/wiki/Configurations)
- [Community Configs](https://github.com/brndnmtthws/conky/wiki/Community-Configurations)
- [Frequently Asked Questions](https://github.com/brndnmtthws/conky/wiki/FAQ)

## Development Setup

Conky uses [mise](https://mise.jdx.dev/) to pin developer tools such as CMake,
Ninja, Bun, uv, and lefthook. After installing mise, bootstrap the toolchain
from the repository root:

```sh
mise install
mise run doctor
```

`mise run doctor` checks for native libraries provided by your system package
manager, such as X11, Cairo, Lua, Imlib2, librsvg, ncurses, and libxml2. Once it
passes, use the mise tasks for the standard development workflow:

```sh
mise run configure
mise run build
mise run test
```

If your shell has `mise activate` enabled, entering the repository runs
`mise i -q` automatically. Tool downloads are locked by `mise.lock`; update the
lockfile with `MISE_LOCKED=0 mise lock` after changing tool versions.

## License

Conky is licensed under the terms of the [GPLv3](LICENSE).

## Contributing

Contributions are welcome from anyone. Please read the
[Contributing](https://github.com/brndnmtthws/conky/wiki/Contributing) page on
the Wiki for guidelines.

## Supporting This Project

Conky exists thanks to the hard work of volunteers. Please consider sponsoring
the project's developers if you get value out of Conky.

[mpd]: https://musicpd.org/
[xmms2]: https://github.com/xmms2/wiki/wiki
[audacious]: https://audacious-media-player.org/
[luawiki]: https://en.wikipedia.org/wiki/Lua_%28programming_language%29
[imlib2]: https://docs.enlightenment.org/api/imlib2/html/
[cairo]: https://www.cairographics.org/
