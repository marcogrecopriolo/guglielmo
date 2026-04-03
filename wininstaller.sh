version=`awk '/^objectVersion.*=/ { print $3 }' guglielmo.pro`
arch=`uname -m`
sed -i "s|<Version>[^<]*</Version>|<Version>${version}</Version>|" installer/windows/config/config.xml
make
cp windows-bin/guglielmo.exe installer/windows/packages/org.sqsl.guglielmo/data
cd installer/windows
binarycreator -v -c config/config.xml -p packages -f guglielmo-installer-${arch}-v${version}.exe
binarycreator -v -c config/config.xml -p packages -p packages-extra -f guglielmo-installer-with-drivers-${arch}-v${version}.exe
