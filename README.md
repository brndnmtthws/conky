## Conky for macOS

**Conky** is a free, light-weight system monitor for X, that displays
any kind of information on your desktop.

**AND** this is an attempt to bringing **Conky** on macOS! ***PLEASE NOTE THAT THIS MAY NOT BE DOABLE*** or may require complete redesign of some parts of conky... For original project checkout this: [Original Project](https://github.com/brndnmtthws/conky)

### Version

This is a fork of the latest version of conky ( at the time I'm writing this )
Also, if this fork is 3166 commits behind the master it means it is up-to-date.  If you see a different number probably the original conky had updates...

### Features

**NOW CONKY LOADS ON MACOS CONFIG FILES BUT SOME FEATURES MAY NOT WORK BECAUSE IT IS AN EARLY STAGE.**
**For more information check this is the output of the command ```conky --version```**

```
conky 1.10.7_pre compiled Sat
Jul 22 13:07:42 EEST 2017 for Darwin 16.7.0 x86_64

Compiled in features:

System config file: /etc/conky/conky.conf
Package library path: /usr/local/lib/conky


 General:
  * math
  * builtin default configuration
  * old configuration syntax
  * apcupsd
  * iostats
  * ncurses
  * Internationalization support
 X11:
  * Xdamage extension
  * Xinerama extension (virtual display)
  * Xft
  * ARGB visual
  * Own window

 Music detection:
  * MPD
  * MOC

 Default values:
  * Netdevice: eth0
  * Local configfile: $HOME/.conkyrc
  * Localedir: /usr/local/share/locale
  * Maximum netdevices: 64
  * Maximum text size: 16384
  * Size text buffer: 256
```

BUT THESE ARE THE FEATURES THE REAL CONKY SUPPORTS AND I WOULD LIKE TO SEE ON MACOS, TOO:

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

download/clone project, then cd into project's directory, 
and... Choose if you want to use Unix Makefiles or Xcode build system.

**If you want to use Unix Makefiles then type the**

```
mkdir build
cd build
ccmake ..
```

Here you will have to press 'c', then when the configuring finishes, type 'e'.
You will see a list with all availiable build options.  Go down and enable BUILD_ICONV.

Then press 'c' again, when it finishes 'e', then 'c' again, 'e' and finally 'g'...
Thats it...

Type:

```
make
make install ( if you want to install it )
```

If you hit make install just type 'conky' on terminal to run it, otherwise just go into the build directory you made and find
the conky executable

**USE XCODE FOR BUILDING / DEVELOPING**

It is possible to use Xcode to build the project...

download/clone project, then cd into project's directory, 
and... 

```
mkdir build
cd build
cmake .. -G Xcode
```

this should make an xcode project inside the build directory you just created.
Head into it and find the conky.xcodeproj file and open it...

This should launch Xcode! Once that is ready, you should see your project called "conky" on the left.
Select that and basically look at the project options in the middle of your screen where it says: "Resource Tags", "Build Settings" etc.  Also, on top of "Prefetched" it says "ALL_BUILD".  Click that and select "conky" instead.  Select "Build Settings" !!

```
Under Linking->Runpath Search Paths-> add /usr/local/opt/gettext/lib
Under Linking->Other Linker Flags-> add /usr/local/opt/gettext/lib/libintl.dylib
Under Linking->Other Linker Flags-> remove the -lintl parameter.
Under Search Paths->Header Search Paths-> add /usr/local/opt/gettext/include
```
You now need to make sure your build target is "BUILD_ALL".  To do this just click the button next to the one saying "My Mac" on the upper-left and then choose "BUILD_ALL" from the options shown. ( this is important because it builds some files for conky to compile. )

Then hit build.  Once it has finished, change the build target to "conky" and hit "run"... Thats it! You can now work on conky using Xcode!

You can now hit build to build all targets but this will not run conky.  Select on the upper left corner the target to be conky and build/run again..

**These Xcode configuration steps probably wont be needed in future versions!**

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
