version=`awk '/^objectVersion.*=/ { print $3 }' guglielmo.pro`
mkdir  build
cd build
cmake .. -DRTLSDR=ON -DAIRSPY=ON -DSDRPLAY=ON -DSDRPLAY_V3=ON -DLIMESDR=ON -DHACKRF=ON -DCMAKE_INSTALL_PREFIX=/usr
make -j4
cd ..
mkdir -p appdir/usr/bin
cp build/guglielmo  appdir/usr/bin/guglielmo
mkdir -p appdir/usr/lib
cp /usr/lib64/librtlsdr.so  appdir/usr/lib
 
mkdir -p appdir/usr/share/applications
cp etc/guglielmo.desktop appdir/usr/share/applications
cp images/guglielmo.png appdir/guglielmo.png
touch appdir/guglielmo.png
mkdir -p appdir/usr/share/icons/hicolor/256x256/apps/
cp images/guglielmo.png appdir/usr/share/icons/hicolor/256x256/apps/
  
wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage" 
chmod a+x linuxdeployqt*.AppImage
unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
./linuxdeployqt*.AppImage ./appdir/usr/share/applications/* -bundle-non-qt-libs -no-translations -extra-plugins=styles
./linuxdeployqt*.AppImage --appimage-extract
find appdir/usr/plugins/ -type f -exec squashfs-root/usr/bin/patchelf --set-rpath '$ORIGIN/../../lib' {} \;
chmod a+x etc/appimage/* ; rm appdir/AppRun ; cp etc/appimage/* appdir/
export PATH=squashfs-root/usr/bin/:$PATH
squashfs-root/usr/bin/appimagetool $(readlink -f ./appdir/)
if test "$version" != ""
then
    mv guglielmo-x86_64.AppImage guglielmo-x86_64-v$version.AppImage
fi
