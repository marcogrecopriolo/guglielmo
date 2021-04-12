#
#	Script to be used to create an appImage for guglielmo
#
sudo add-apt-repository ppa:beineri/opt-qt-5.12.2-xenial -y
sudo apt-get update -qq
    
sudo apt-get -y install qt5-qmake qt5-default libqt5opengl5-dev qtbase5-dev libqwt-qt5-dev libsndfile1-dev librtlsdr-dev librtlsdr0 libfftw3-dev portaudio19-dev libfaad-dev zlib1g-dev rtl-sdr libusb-1.0-0-dev mesa-common-dev libgl1-mesa-dev libsamplerate-dev 

sudo apt-get install git cmake

git clone https://github.com/marcogrecopriolo/guglielmo
cd guglielmo

mkdir build
cd build
cmake .. -DRTLSDR=ON  -DAIRSPY=ON -DSDRPLAY=ON -DLIMESDR=ON -DHACKRF=ON -DCMAKE_INSTALL_PREFIX=/usr
make -j4

ls -lh .
rm -rf *_autogen
mkdir -p appdir/usr/bin
cp guglielmo  appdir/usr/bin/guglielmo
 
mkdir -p appdir/usr/share/applications
cp guglielmo.desktop appdir/usr/share/applications
cp guglielmo.png appdir/guglielmo.png
touch appdir/guglielmo.png
mkdir -p ./appdir/usr/share/icons/hicolor/256x256/apps/
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
find ./appdir -executable -type f -exec ldd {} \; | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq
