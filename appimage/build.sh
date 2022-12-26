#!/bin/bash

set -e
set -x

# building in temporary directory to keep system clean
# use RAM disk if possible (as in: not building on CI system like Travis, and RAM disk is available)
if [ "$CI" == "" ] && [ -d /dev/shm ]; then
    TEMP_BASE=/dev/shm
else
    TEMP_BASE=/tmp
fi

BUILD_DIR=$(mktemp -d -p "$TEMP_BASE" AppImageLauncher-build-XXXXXX)

# make sure to clean up build dir, even if errors occur
cleanup () {
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
}
trap cleanup EXIT

# store repo root as variable
REPO_ROOT=$(readlink -f $(dirname $(dirname $0)))
OLD_CWD=$(readlink -f .)

# switch to build dir
pushd "$BUILD_DIR"

# configure build files with cmake
# we need to explicitly set the install prefix, as CMake's default is /usr/local for some reason...
cmake -G Ninja                         \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo    \
  -DRELEASE=ON                         \
  -DBUILD_AUDACIOUS=ON                 \
  -DBUILD_DOCS=ON                      \
  -DBUILD_HTTP=ON                      \
  -DBUILD_ICAL=ON                      \
  -DBUILD_ICONV=ON                     \
  -DBUILD_IRC=ON                       \
  -DBUILD_IRC=ON                       \
  -DBUILD_JOURNAL=ON                   \
  -DBUILD_LUA_CAIRO=ON                 \
  -DBUILD_LUA_IMLIB2=ON                \
  -DBUILD_LUA_RSVG=ON                  \
  -DBUILD_MYSQL=ON                     \
  -DBUILD_NVIDIA=ON                    \
  -DBUILD_PULSEAUDIO=ON                \
  -DBUILD_RSS=ON                       \
  -DBUILD_WAYLAND=OFF                  \
  -DBUILD_WLAN=ON                      \
  -DBUILD_X11=ON                       \
  -DBUILD_XMMS2=ON                     \
  -DCMAKE_INSTALL_PREFIX=./AppDir/usr  \
  "$REPO_ROOT"

# build project and install files into AppDir
cmake --build .
cmake --install .

wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage

# make them executable
chmod +x linuxdeploy-x86_64.AppImage

./linuxdeploy-x86_64.AppImage \
  --appdir AppDir \
  -e AppDir/usr/bin/conky \
  -i AppDir/usr/share/icons/hicolor/scalable/apps/conky-logomark-violet.svg \
  -d AppDir/usr/share/applications/conky.desktop

wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage

chmod +x appimagetool-x86_64.AppImage

./appimagetool-x86_64.AppImage AppDir --sign --sign-key E3034071

for f in conky*.AppImage
do
  sha256sum $f > $f.sha256
done

mv conky*.AppImage* "$OLD_CWD"

# gzip & copy the man page, which will be attached to releases
gzip doc/conky.1
mv doc/conky.1.gz "$OLD_CWD"
