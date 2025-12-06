version=`awk '/^objectVersion.*=/ { print $3 }' guglielmo.pro`
arch=`uname -m`
mkdir  build
cd build
cmake .. -DRTLSDR=ON -DAIRSPY=ON -DSDRPLAY=ON -DSDRPLAY_V3=ON -DLIMESDR=ON -DHACKRF=ON -DCMAKE_INSTALL_PREFIX=/usr
make -j4
cd ..
cp build/guglielmo  appdir/usr/bin/guglielmo
mkdir -p appdir/usr/lib

# look for local first, and quit after the first one
cp `find -L /usr/local /usr -name librtlsdr.so -print -quit 2>&-`  appdir/usr/lib
 
mkdir -p appdir/usr/share/applications
cp etc/guglielmo.desktop appdir/usr/share/applications
cp images/guglielmo.png appdir/guglielmo.png
touch appdir/guglielmo.png
mkdir -p appdir/usr/share/icons/hicolor/256x256/apps/
cp images/guglielmo.png appdir/usr/share/icons/hicolor/256x256/apps/
  
wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-${arch}.AppImage" 
chmod a+x linuxdeployqt*.AppImage
unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
./linuxdeployqt-continuous-${arch}.AppImage ./appdir/usr/share/applications/* -unsupported-allow-new-glibc -bundle-non-qt-libs -no-translations -extra-plugins=styles
./linuxdeployqt-continuous-${arch}.AppImage --appimage-extract
find appdir/usr/plugins/ -type f -exec squashfs-root/usr/bin/patchelf --set-rpath '$ORIGIN/../../lib' {} \;
chmod a+x etc/appimage/* ; rm appdir/AppRun ; cp etc/appimage/* appdir/
export PATH=squashfs-root/usr/bin/:$PATH
squashfs-root/usr/bin/appimagetool $(readlink -f ./appdir/)
if test "$version" != ""
then
    mv guglielmo-${arch}.AppImage guglielmo-${arch}-v${version}.AppImage
fi
