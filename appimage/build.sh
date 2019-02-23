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
cmake -DRELEASE=ON -DCMAKE_INSTALL_PREFIX=/usr "$REPO_ROOT"

# build project and install files into AppDir
make -j4
make install DESTDIR=AppDir

wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage

# make them executable
chmod +x linuxdeploy-x86_64.AppImage

./linuxdeploy-x86_64.AppImage \
    --appdir AppDir \
    -e AppDir/usr/bin/conky \
    -i AppDir/usr/share/icons/hicolor/scalable/apps/conky-logomark-violet.svg \
    -d AppDir/usr/share/applications/conky.desktop \
    --output appimage

wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage

chmod +x appimagetool-x86_64.AppImage

./appimagetool-x86_64.AppImage conky*.AppImage --sign --sign-key E3034071

mv conky*.AppImage "$OLD_CWD"
