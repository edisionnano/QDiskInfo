#!/bin/sh

set -eu

export ARCH="$(uname -m)"
export APPIMAGE_EXTRACT_AND_RUN=1
export QT_STYLE_OVERRIDE=Breeze # Make sure the breeze theme is dlopened

APP=QDiskInfo
APPIMAGETOOL="https://github.com/pkgforge-dev/appimagetool-uruntime/releases/download/continuous/appimagetool-$ARCH.AppImage"
UPINFO="gh-releases-zsync|$(echo "$GITHUB_REPOSITORY" | tr '/' '|')|latest|*-$ARCH.AppImage.zsync"
LIB4BN="https://raw.githubusercontent.com/VHSgunzo/sharun/refs/heads/main/lib4bin"
VERSION="$(echo "$GITHUB_SHA" | cut -c 1-9)-anylinux"

# Get iculess qt6-base, removes a 30 MiB lib from the appimage
if [ "$ARCH" = 'x86_64' ]; then
	PKG_TYPE='x86_64.pkg.tar.zst'
else
	PKG_TYPE='aarch64.pkg.tar.xz'
fi
QT6_URL="https://github.com/pkgforge-dev/llvm-libs-debloated/releases/download/continuous/qt6-base-iculess-$PKG_TYPE"
wget --retry-connrefused --tries=30 "$QT6_URL" -O ./qt6-base-iculess.pkg.tar.zst
pacman -U --noconfirm ./qt6-base-iculess.pkg.tar.zst
rm -f ./qt6-base-iculess.pkg.tar.zst

# Prepare AppDir
mkdir -p ./AppDir/shared/bin
cp -v ./dist/QDiskInfo.desktop      ./AppDir
cp -v ./dist/QDiskInfo-256x256.png  ./AppDir/QDiskInfo.png
cp -v ./dist/QDiskInfo-256x256.png  ./AppDir/.DirIcon
cp -v ./build/QDiskInfo             ./AppDir/shared/bin
cd ./AppDir

# ADD LIBRARIES
wget "$LIB4BN" -O ./lib4bin
chmod +x ./lib4bin
./lib4bin -p -v -s -k  \
	./shared/bin/QDiskInfo \
	/usr/lib/qt6/plugins/iconengines/* \
	/usr/lib/qt6/plugins/imageformats/* \
	/usr/lib/qt6/plugins/platforms/* \
	/usr/lib/qt6/plugins/platformthemes/* \
	/usr/lib/qt6/plugins/styles/* \
	/usr/lib/qt6/plugins/xcbglintegrations/* \
	/usr/lib/qt6/plugins/wayland-*/*

# also use lib4bin to make a portable smartctl with wrappe 
./lib4bin -s --with-wrappe "$(command -v smartctl)"  

# prepare sharun
echo '#!/bin/sh
CURRENTDIR="$(dirname "$(readlink -f "$0")")"
CACHEDIR="${XDG_CACHE_HOME:-$HOME/.cache}"
export QT_STYLE_OVERRIDE=${QT_STYLE_OVERRIDE:-Breeze}
[ -f "$APPIMAGE".stylesheet ] && APPIMAGE_QT_THEME="$APPIMAGE.stylesheet"
[ -f "$APPIMAGE_QT_THEME" ] && set -- "$@" "-stylesheet" "$APPIMAGE_QT_THEME"

if ! command -v smartctl >/dev/null 2>&1; then
	echo "smartctl is not on the system, using bundled binary..."
	export PATH="$PATH:"$CACHEDIR"/qdiskinfo-appimage"
	mkdir -p "$CACHEDIR"/qdiskinfo-appimage
	cp -v "$CURRENTDIR"/smartctl "$CACHEDIR"/qdiskinfo-appimage
	chmod +x "$CACHEDIR"/qdiskinfo-appimage/smartctl
fi

exec "$CURRENTDIR"/bin/QDiskInfo "$@"' > ./AppRun
chmod +x ./AppRun
./sharun -g

# Make AppImage with uruntime
cd ..
wget "$APPIMAGETOOL" -O ./appimagetool
chmod +x ./appimagetool

./appimagetool --no-appstream -u "$UPINFO" \
	"$PWD"/AppDir "$PWD"/"$APP"-"$VERSION"-"$ARCH".AppImage
