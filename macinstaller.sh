make
mkdir -p macx-bin/guglielmo.app/Contents/Frameworks
cp /usr/local/lib/librtlsdr.dylib macx-bin/guglielmo.app/Contents/Frameworks
macdeployqt macx-bin/guglielmo.app
rm -rf installer/macx/packages/org.sqsl.guglielmo/data/guglielmo.app
mkdir -p installer/macx/packages/org.sqsl.guglielmo/data
cp -R macx-bin/guglielmo.app installer/macx/packages/org.sqsl.guglielmo/data
cd installer/macx
/Users/administrator/Qt/Tools/QtInstallerFramework/4.5/bin/binarycreator -v -c config/config.xml -p packages -f guglielmo-installer.dmg
