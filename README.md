## Conky for macOS

[![Build Status](https://travis-ci.org/brndnmtthws/conky.png)](https://travis-ci.org/brndnmtthws/conky)

**Conky** is a free, light-weight system monitor for X, that displays
any kind of information on your desktop.

### About this fork

This is a fork an almost up to date version of conky with patches for macOS support.

You can check out how it operates in a video: [conky on macOS](https://www.youtube.com/watch?v=l3tIiDdnC68)

**Not all features mentioned below work! But with your help we can make this a complete port! :)**

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
   with Lua ([more](wiki/Lua-API)).

... and much much more.

Conky can display information either as text, or using simple progress
bars and graph widgets, with different fonts and colours.

### Screenshots

[![screenshot](https://github.com/brndnmtthws/conky/wiki/configs/brenden/screenshot-thumb.png)](https://raw.github.com/wiki/brndnmtthws/conky/configs/brenden/screenshot.png)
[![screenshot](https://github.com/brndnmtthws/conky/wiki/configs/ke49/screenshot-thumb.png)](https://raw.github.com/wiki/brndnmtthws/conky/configs/ke49/screenshot.png)
[![screenshot](https://github.com/brndnmtthws/conky/wiki/configs/jc/screenshot-thumb.png)](https://raw.github.com/wiki/brndnmtthws/conky/configs/jc/screenshot.png)

See the [User Configs](https://github.com/brndnmtthws/conky/wiki/User-Configs) section of the wiki for more
screenshots and their associated Conky config files.

### Build on macOS

#### What you need

* Xcode command-line tools
* Homebrew
* Xquartz
* freetype, gettext, lua

Once you get these installed please run the following commands as you see them!
**The last command MUST be run if you want gettext support!**

```
brew install cmake freetype gettext lua
brew link --force gettext
```

#### Actual build process

download/clone project and `cd` into project's directory,
You can now choose if you want to use **Unix Makefiles** or **Xcode build system**.

**Using Unix Makefiles**

type the following:

```
mkdir build
cd build
ccmake ..
```

Press 'c', again 'c', then 'e' and finally 'g'...
Continue by typing:

```
make
make install  # optional
```

( If you hit `make install` you can run conky by typing `conky` on terminal )

**USE XCODE FOR BUILDING / DEVELOPING**

type the following to setup an Xcode project file inside folder called `build`:

```
mkdir build
cd build
cmake .. -G Xcode
```

Head into `build`, find the `conky.xcodeproj` file and open it...

This should launch Xcode! Once that is ready, you should see your project called "conky" on the left.
Make sure the scheme "ALL_BUILD" is selected on the upper left (scheme or target, however you like...)
and click the `conky` xcode project icon to alter the `Build Settings`.  Make the following changes:

```
Under Search Paths->Library Search Paths-> add /usr/local/lib
Under Search Paths->Header Search Paths-> add /usr/local/include
```
With the build target being "BUILD_ALL" press the Xcode build button.
If it is successful go on and change build target to `conky`

**( This order was important, please follow exactly! )**

You are now ready to compile/work on conky as you wish using Xcode IDE!
**Configuration is complete!**

### Documentation

The [GitHub Wiki](https://github.com/brndnmtthws/conky/wiki) serves as a central hub for all of
Conky's documentation. Quick links:

* [Install](https://github.com/brndnmtthws/conky/wiki/Installation)
* [Configure](https://github.com/brndnmtthws/conky/wiki/Configuration-Settings)
* [User Configs](https://github.com/brndnmtthws/conky/wiki/User-Configs)
* [Frequently Asked Questions](https://github.com/brndnmtthws/conky/wiki/FAQ)

### License

Conky is licensed under the terms of the [GPLv3](LICENSE.GPL) and
[BSD](LICENSE.BSD) licenses.

### Contributing

To submit code changes, please open pull requests against [the GitHub repository](https://github.com/brndnmtthws/conky/edit/master/README.md). Patches submitted in issues, email, or elsewhere will likely be ignored. Here's some general guidelines when submitting PRs:

 * In your pull request, please:
   * Describe the changes, why they were necessary, etc
   * Describe how the changes affect existing behaviour
   * Describe how you tested and validated your changes
   * Include any relevant screenshots/evidence demonstrating that the changes work and have been tested
 * Any new source files should include a GPLv3 license header
 * Any contributed code must be GPLv3 licensed

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
