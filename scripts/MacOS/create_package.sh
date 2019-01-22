#!/bin/bash

[ $# -lt 4 ] && { echo "Usage: $0 install_dir dcore_version app_identity install_identity"; exit 1; }

BASEDIR=$(dirname "$0")

rm -rf $BASEDIR/dist
mkdir -p $BASEDIR/dist/Applications
cp -R $1/bin/DECENT.app $BASEDIR/dist/Applications
cp $1/bin/decentd $BASEDIR/dist/Applications/DECENT.app/Contents/MacOS
cp $1/bin/cli_wallet $BASEDIR/dist/Applications/DECENT.app/Contents/MacOS

cp $BASEDIR/Resources/* $BASEDIR/dist/Applications/DECENT.app/Contents/Resources
sed s/VERSION/$2/ $BASEDIR/Info.plist > $BASEDIR/dist/Applications/DECENT.app/Contents/Info.plist
cd $BASEDIR/dist/Applications/DECENT.app/Contents

mkdir Plugins
cp -R /usr/local/opt/qt/plugins/iconengines Plugins
cp -R /usr/local/opt/qt/plugins/imageformats Plugins
cp -R /usr/local/opt/qt/plugins/platforms Plugins

mkdir Frameworks
cd Frameworks

cp /usr/local/opt/cryptopp/lib/libcryptopp.dylib .
cp /usr/local/opt/gmp/lib/libgmp.10.dylib .
cp /usr/local/opt/pbc/lib/libpbc.1.dylib .
cp /usr/local/opt/readline/lib/libreadline.8.dylib .
cp /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore .
cp /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui .
cp /usr/local/opt/qt/lib/QtPrintSupport.framework/Versions/5/QtPrintSupport .
cp /usr/local/opt/qt/lib/QtSvg.framework/Versions/5/QtSvg .
cp /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets .

cd ../MacOS
cp /usr/local/bin/ipfs .
install_name_tool -change /usr/local/opt/cryptopp/lib/libcryptopp.dylib @executable_path/../Frameworks/libcryptopp.dylib cli_wallet
install_name_tool -change /usr/local/opt/cryptopp/lib/libcryptopp.dylib @executable_path/../Frameworks/libcryptopp.dylib decentd
install_name_tool -change /usr/local/opt/cryptopp/lib/libcryptopp.dylib @executable_path/../Frameworks/libcryptopp.dylib DECENT
install_name_tool -change /usr/local/opt/gmp/lib/libgmp.10.dylib @executable_path/../Frameworks/libgmp.10.dylib cli_wallet
install_name_tool -change /usr/local/opt/gmp/lib/libgmp.10.dylib @executable_path/../Frameworks/libgmp.10.dylib decentd
install_name_tool -change /usr/local/opt/gmp/lib/libgmp.10.dylib @executable_path/../Frameworks/libgmp.10.dylib DECENT
install_name_tool -change /usr/local/opt/pbc/lib/libpbc.1.dylib @executable_path/../Frameworks/libpbc.1.dylib cli_wallet
install_name_tool -change /usr/local/opt/pbc/lib/libpbc.1.dylib @executable_path/../Frameworks/libpbc.1.dylib decentd
install_name_tool -change /usr/local/opt/pbc/lib/libpbc.1.dylib @executable_path/../Frameworks/libpbc.1.dylib DECENT
install_name_tool -change /usr/local/opt/readline/lib/libreadline.8.dylib @executable_path/../Frameworks/libreadline.8.dylib cli_wallet
install_name_tool -change /usr/local/opt/readline/lib/libreadline.8.dylib @executable_path/../Frameworks/libreadline.8.dylib decentd
install_name_tool -change /usr/local/opt/readline/lib/libreadline.8.dylib @executable_path/../Frameworks/libreadline.8.dylib DECENT
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore DECENT
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui DECENT
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets DECENT
install_name_tool -add_rpath @executable_path/../Frameworks/ DECENT

cd ../../..
codesign -s "$3" -v DECENT.app -f --deep -vv

cd ../..
pkgbuild --root dist --component-plist decent_pkg.plist DECENT.pkg --sign "$4"
productbuild --distribution decent_distribution.plist --resources dist/Applications/DECENT.app/Contents/Resources --package-path DECENT.pkg DECENT-distribution.$2.pkg --sign "$4"
