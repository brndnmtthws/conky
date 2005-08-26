#!/bin/sh

rm -rf ../conky-1.3.0* ../conky_*
make distclean
sh autogen.sh
./configure
make dist
cp -f conky-1.3.0.tar.gz ../conky_1.3.0.orig.tar.gz
cd ..
tar xvzf conky_1.3.0.orig.tar.gz
cd conky-1.3.0
#dh_make -s
cp -r ../conky/debian .
rm -rf debian/CVS
debuild
#dpkg-buildpackage -rfakeroot -S -k<private key id>
