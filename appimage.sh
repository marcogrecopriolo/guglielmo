mkdir build
cd build
cmake .. -DRTLSDR=ON -DAIRSPY=ON -DSDRPLAY=ON -DLIMESDR=ON -DHACKRF=ON -DCMAKE_INSTALL_PREFIX=/usr
make -j4
mkdir -p appdir/usr/bin
cd ..
cp build/guglielmo  appdir/usr/bin/guglielmo
 
mkdir -p appdir/usr/share/applications
cp guglielmo.desktop appdir/usr/share/applications
cp guglielmo.png appdir/guglielmo.png
touch appdir/guglielmo.png
mkdir -p appdir/usr/share/icons/hicolor/256x256/apps/
cp guglielmo.png appdir/usr/share/icons/hicolor/256x256/apps/
  
wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage" 
chmod a+x linuxdeployqt*.AppImage
unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
./linuxdeployqt*.AppImage ./appdir/usr/share/applications/* -bundle-non-qt-libs -no-translations
./linuxdeployqt*.AppImage --appimage-extract
find appdir/usr/plugins/ -type f -exec squashfs-root/usr/bin/patchelf --set-rpath '$ORIGIN/../../lib' {} \;
chmod a+x appimage/* ; rm appdir/AppRun ; cp appimage/* appdir/
export PATH=squashfs-root/usr/bin/:$PATH
squashfs-root/usr/bin/appimagetool $(readlink -f ./appdir/)
# find ./appdir -executable -type f -exec ldd {} \; | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq
