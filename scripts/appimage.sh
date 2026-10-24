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
    x64) RUNTIME_SHA=617aa77314d57c8b8e4fae9c3f0b35af7b30d5529cb69e98f64f92510ed98a46; APPIMAGE_ARCH=x86_64; TOOL_SHA=ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0 ;;
    arm64) RUNTIME_SHA=9a72a744d221576c9347ec074243acc5c3c7568896b12714e0e82e9f0bf47900; APPIMAGE_ARCH=aarch64; TOOL_SHA=f0837e7448a0c1e4e650a93bb3e85802546e60654ef287576f46c71c126a9158 ;;
    *) echo 'Unsupported AppImage architecture' >&2; exit 1 ;;
esac
if ! echo "$TOOL_SHA  build/appimagetool" | sha256sum -c - >/dev/null 2>&1; then
    curl -fsSL --connect-timeout 15 --max-time 180 --retry 3 "https://github.com/AppImage/appimagetool/releases/download/1.9.1/appimagetool-$APPIMAGE_ARCH.AppImage" -o build/appimagetool
fi
echo "$TOOL_SHA  build/appimagetool" | sha256sum -c -
chmod 755 build/appimagetool
if ! echo "$RUNTIME_SHA  build/appimage-runtime" | sha256sum -c - >/dev/null 2>&1; then
    curl -fsSL --connect-timeout 15 --max-time 180 --retry 3 "https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-$APPIMAGE_ARCH" -o build/appimage-runtime
fi
echo "$RUNTIME_SHA  build/appimage-runtime" | sha256sum -c -
ARCH=$APPIMAGE_ARCH APPIMAGE_EXTRACT_AND_RUN=1 build/appimagetool --runtime-file build/appimage-runtime build/AppDir "videosubfinder-cli-linux-${VSF_ARCH:-x64}.AppImage"
APPIMAGE_EXTRACT_AND_RUN=1 python3 tests/test_cli.py "$PWD/videosubfinder-cli-linux-${VSF_ARCH:-x64}.AppImage"
