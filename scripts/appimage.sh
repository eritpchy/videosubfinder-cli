#!/bin/sh
set -eu
mkdir -p build/AppDir/usr/bin
cp -a dist/. build/AppDir/usr/bin/
cat > build/AppDir/AppRun <<'LAUNCH'
#!/bin/sh
VSF_APPDIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
exec "$VSF_APPDIR/usr/bin/VideoSubFinderCli.run" "$@"
LAUNCH
chmod 755 build/AppDir/AppRun
cat > build/AppDir/videosubfinder-cli.desktop <<'DESKTOP'
[Desktop Entry]
Type=Application
Name=VideoSubFinder CLI
Exec=VideoSubFinderCli.run
Icon=videosubfinder-cli
Terminal=true
Categories=AudioVideo;
DESKTOP
cat > build/AppDir/videosubfinder-cli.svg <<'ICON'
<svg xmlns="http://www.w3.org/2000/svg" width="128" height="128" viewBox="0 0 128 128"><rect width="128" height="128" rx="20" fill="#183649"/><path d="M32 40h64v8H32zm0 24h64v8H32zm0 24h40v8H32z" fill="white"/></svg>
ICON
case "${VSF_ARCH:-x64}" in
    x64) APPIMAGE_ARCH=x86_64; TOOL_SHA=95cbe7cce9717fce90c484e34052ee7c7f1d7635b33c12525b4776826a7d29b6 ;;
    arm64) APPIMAGE_ARCH=aarch64; TOOL_SHA=a595ea34cd6136c7f595e9dcbb16f3e9725d7610efb9e43b38c3c6e86cafc270 ;;
    *) echo 'Unsupported AppImage architecture' >&2; exit 1 ;;
esac
curl -fsSL "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-$APPIMAGE_ARCH.AppImage" -o build/appimagetool
echo "$TOOL_SHA  build/appimagetool" | sha256sum -c -
chmod 755 build/appimagetool
ARCH=$APPIMAGE_ARCH APPIMAGE_EXTRACT_AND_RUN=1 build/appimagetool build/AppDir "videosubfinder-cli-linux-${VSF_ARCH:-x64}.AppImage"
APPIMAGE_EXTRACT_AND_RUN=1 python3 tests/test_cli.py "$PWD/videosubfinder-cli-linux-${VSF_ARCH:-x64}.AppImage"
