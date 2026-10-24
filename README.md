# videosubfinder-cli

Headless hard-subtitle detection for Linux, macOS and Windows. The portable core
combines the latest SourceForge VideoSubFinder 6.10 source, the CLI 6.10.2 fixes,
and the standard C++ port used by Subrazen (`b869ea8`). wxWidgets, GUI frameworks,
Intel's compiler, Eigen, OpenEXR development packages, Torch and OCR engines are
not required by this CLI. OpenCV supplies image processing and video decoding.
Two pinned, header-only submodules supply JSON callback data and task queues.

## Downloads

GitHub Releases contain Linux x64/arm64 bundles and AppImages, macOS x64/arm64
and Universal bundles, a Windows x64 CLI ZIP, and `SHA256SUMS`. Linux CPU binaries
link musl, OpenCV and FFmpeg statically and do not depend on the host glibc or
multimedia libraries. macOS bundles include
their transitive non-system libraries and do not require Homebrew at runtime.
The Universal executable contains both Intel and Apple Silicon slices, with
architecture-specific runtime libraries isolated in separate directories. The Windows ZIP uses static libraries and includes `VideoSubFinderCli.exe`.
Worker-specific dynamic archives are built separately for Ubuntu 20.04, 22.04 and
24.04 on x64/arm64. They include only four minimal OpenCV shared libraries and
reuse the matching system FFmpeg ABI. They need the `ffmpeg` runtime package;
choose the archive matching the worker's Ubuntu version. No system OpenCV,
Homebrew or development packages are required at runtime.

```sh
./VideoSubFinderCli.run -c -r -i './input video.mkv' -o './Results' \
  -ces './Results/subtitles.srt' -te 0.4 -be 0 -le 0 -re 1 -nthr 4
./VideoSubFinderCli.run -ccti -o './Results'
./videosubfinder-cli-linux-x64.AppImage --help
```

On Windows use `VideoSubFinderCli.exe` with the same arguments. Existing single-dash
multi-character options and double-dash long options remain accepted. FFmpeg via
OpenCV is the default video backend; `-ovocv` selects OpenCV's automatic backend.
Use `-dsi` for timestamp-only extraction. Progress and `FINISH` are written to
stderr. Generated image filenames use portable underscores and upstream crop
metadata. The wrapper preserves the caller's working directory and isolates the
bundled Linux libraries from inherited `LD_LIBRARY_PATH` entries.

Settings are loaded from `settings/general.cfg` beside the executable, or from
`-gs PATH`. The default edge-difference threshold is 800, matching the algorithm's
built-in default and avoiding false subtitles on uniform frames. The CLI validates
crop, time, thread and settings values, reports decode failures with nonzero exit
codes, and prevents generated image directories from following symbolic links.

## Build and test

```sh
git clone --recurse-submodules https://github.com/eritpchy/videosubfinder-cli.git
cmake -S videosubfinder-cli -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
cmake --install build --prefix dist
```

Install a C++17 compiler, CMake 3.24+, Python 3 and OpenCV 4 development packages.
Tests additionally use FFmpeg and a system TrueType font to generate independently
timed MKV and MP4 subtitle fixtures. Set `VSF_TEST_MEDIA` to a directory containing
pre-generated `input video.mkv` and `input video.mp4` to run tests without FFmpeg.
For fully static Linux CPU builds, run `sh scripts/build-static-linux.sh` in
Alpine 3.22 with the dependencies listed in the CI workflow. OpenCV 4.14.0 and
FFmpeg 8.0.1 source commits are pinned in that script.
On Windows use the checked-in vcpkg manifest with `x64-windows-static` and
`CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded`. CUDA 12 builds use `-DUSE_CUDA=ON`;
CPU binaries reject `--use_cuda` explicitly. The separate Linux x64 CUDA 12
archive includes CUDA runtime libraries and targets Ubuntu 22.04 / glibc 2.35 or
newer; it requires a compatible NVIDIA driver. Run it with `--use_cuda`.

All builds run remotely or in GitHub Actions. The CI tests installed artifacts,
not only source-tree executables. Linux distribution checks run the tar bundle
and AppImage in Ubuntu 16.04-26.04 LTS, Debian 10-13, CentOS 7, AlmaLinux 8-10,
Rocky Linux 8/9, Fedora, openSUSE Leap/Tumbleweed, Arch and Alpine containers
without installing guest packages. Host Python drives the guest executable, so
EOL package mirrors and old Python versions do not affect runtime validation. macOS runs native Intel/ARM tests and verifies the
Universal slices. Windows runs the actual installed CLI binary. ASan/UBSan execute
the same extraction and cleared-text tests. CUDA CI compiles against CUDA 12 and
exercises the CPU path; GPU execution requires a CUDA-capable test host. Set `VSF_TEST_CUDA=1` to run
the extraction and CCTI regressions on an actual GPU.

Pushes and pull requests build artifacts. A `v*` tag publishes a release only after
all required platform, distribution and sanitizer jobs pass. No PR publishes a
release, and release creation uses one job to avoid concurrent asset overwrites.
