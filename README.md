[![Linux build](https://github.com/brndnmtthws/conky/actions/workflows/build-and-test-linux.yaml/badge.svg)](https://github.com/brndnmtthws/conky/actions/workflows/build-and-test-linux.yaml) [![macOS build](https://github.com/brndnmtthws/conky/actions/workflows/build-and-test-macos.yaml/badge.svg)](https://github.com/brndnmtthws/conky/actions/workflows/build-and-test-macos.yaml) [![Docker build](https://github.com/brndnmtthws/conky/actions/workflows/docker.yaml/badge.svg)](https://github.com/brndnmtthws/conky/actions/workflows/docker.yaml) [![AppImage build](https://github.com/brndnmtthws/conky/actions/workflows/publish-appimage.yml/badge.svg)](https://github.com/brndnmtthws/conky/actions/workflows/publish-appimage.yml)

<p align="center"><img width="300" src="data/logo/conky-logotype-horizontal-violet.png"></p>

**Conky** is a free, lightweight system monitor for X, Wayland (with caveats), and macOS that renders information on your desktop or outputs to the console, a file, or HTTP.

[📕 Documentation](https://conky.cc/) · [📖 Wiki](https://github.com/brndnmtthws/conky/wiki) · [📦 Latest Release](https://github.com/brndnmtthws/conky/releases/latest) · [💬 Matrix Chat](https://matrix.to/#/#conky:frens.io) · [📹 Intro Video (YouTube)](https://www.youtube.com/watch?v=bHtpLEoRKmg&t=19s)

## Install & Run

Conky is available in most package managers (see [Installation](https://github.com/brndnmtthws/conky/wiki/Installation)). For the latest release, use the AppImage or Nix flake.

### AppImage

Download from [GitHub Releases](https://github.com/brndnmtthws/conky/releases/latest) (host runtime libraries may be required):

```ShellSession
chmod +x ./conky-*.AppImage        # make it executable
./conky-*.AppImage -C > ~/.conkyrc # create a default config
./conky-*.AppImage                 # run
```

### Nix Flake

Available since v1.17.0 alongside the [nixpkgs package](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/os-specific/linux/conky/default.nix):

```nix
{
  inputs = {
    conky.url = "github:brndnmtthws/conky";
  };
}
```

Use `inputs.conky.packages.${system}.default` (`inputs.conky.packages.${system}.conky` for `<=v1.19.8`), or pin a tag via `github:brndnmtthws/conky?ref=refs/tags/v1.19.8`.

## Features

- **300+ built-in objects**: OS stats (uname, uptime, CPU, memory, disk, `top`-like process stats, network), **IMAP/POP3** mail, and music players ([MPD][], [XMMS2][], [Audacious][]).
- **Widgets & interactivity**: Text, progress bars, and graph widgets with custom fonts, colors, and [mouse events](https://github.com/brndnmtthws/conky/wiki/Mouse-Events).
- **Extensibility**: Custom scripts and [**Lua**][luawiki] ([tutorial](https://github.com/brndnmtthws/conky/wiki#tutorial)) with [**Imlib2**][imlib2] and [**Cairo**][cairo] drawing bindings ([Lua docs](https://github.com/brndnmtthws/conky/wiki/Lua)).
- **Cross-platform**: Linux, FreeBSD, OpenBSD, DragonFlyBSD, NetBSD, Solaris, Haiku, and macOS, plus [Docker images](https://hub.docker.com/r/brndnmtthws/conky/) (`amd64`, `armv7`, `aarch64`/`armv8`).

[![screenshot](https://github.com/brndnmtthws/conky/wiki/configs/brenden/screenshot-thumb.png)](https://raw.github.com/wiki/brndnmtthws/conky/configs/brenden/screenshot.png)
[![screenshot](https://github.com/brndnmtthws/conky/wiki/configs/ke49/screenshot-thumb.png)](https://raw.github.com/wiki/brndnmtthws/conky/configs/ke49/screenshot.png)
[![screenshot](https://github.com/brndnmtthws/conky/wiki/configs/jc/screenshot-thumb.png)](https://raw.github.com/wiki/brndnmtthws/conky/configs/jc/screenshot.png)

## Documentation

- [**Reference Documentation**](https://conky.cc/) — config settings, objects, and Lua API
- [**Conky Wiki**](https://github.com/brndnmtthws/conky/wiki) — [Installation](https://github.com/brndnmtthws/conky/wiki/Installation) · [Configuration Settings](https://github.com/brndnmtthws/conky/wiki/Configurations) · [Community Configs](https://github.com/brndnmtthws/conky/wiki/Community-Configurations) · [FAQ](https://github.com/brndnmtthws/conky/wiki/FAQ)

## Development

Bootstrap the toolchain with [mise](https://mise.jdx.dev/) (see [Contributing](https://github.com/brndnmtthws/conky/wiki/Contributing) for guidelines):

```sh
mise install && mise run doctor
mise run configure && mise run build && mise run test
```

## License & Support

Licensed under the [GPLv3](LICENSE). Contributions are welcome ([Contributing guide](https://github.com/brndnmtthws/conky/wiki/Contributing)); consider sponsoring Conky's volunteer developers if you find it useful.

[mpd]: https://musicpd.org/
[xmms2]: https://github.com/xmms2/wiki/wiki
[audacious]: https://audacious-media-player.org/
[luawiki]: https://en.wikipedia.org/wiki/Lua_%28programming_language%29
[imlib2]: https://docs.enlightenment.org/api/imlib2/html/
[cairo]: https://www.cairographics.org/
