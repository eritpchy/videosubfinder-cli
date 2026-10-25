# videosubfinder-cli

Find hardcoded subtitles in a video, save subtitle images for OCR, and export
subtitle timing to SRT. Runs from a terminal or over SSH on Linux, macOS and
Windows, without a desktop session.

The CLI detects text and its timing. To obtain the actual subtitle words, run an
OCR tool on the exported images. The SRT produced by `-ces` contains timing and
placeholder text, not recognized dialogue.

- [Download and install](#download-and-install)
- [Extract your first video](#extract-your-first-video)
- [Select the subtitle area and time range](#select-the-subtitle-area-and-time-range)
- [Prepare images for OCR](#prepare-images-for-ocr)
- [Command-line reference](#command-line-reference)
- [Troubleshooting](#troubleshooting)
- [Build from source](#build-from-source)
- [Donate](#donate)

## Download and install

Get a prebuilt package from [the latest release](https://github.com/eritpchy/videosubfinder-cli/releases/latest).
For normal CPU use, choose the package matching your operating system and CPU:

| Your system | Download | Runtime requirements |
| --- | --- | --- |
| Linux Intel/AMD 64-bit | `videosubfinder-cli-linux-x64.tar.gz` or `videosubfinder-cli-linux-x64.AppImage` | CPU libraries are included and statically linked; no system OpenCV, FFmpeg or glibc version requirement. |
| Linux ARM 64-bit | `videosubfinder-cli-linux-arm64.tar.gz` or `videosubfinder-cli-linux-arm64.AppImage` | Same portable CPU runtime as x64. |
| macOS Apple Silicon | `videosubfinder-cli-macos-arm64.tar.gz` | Bundled libraries; Homebrew is not needed. |
| macOS Intel | `videosubfinder-cli-macos-x64.tar.gz` | Bundled libraries; Homebrew is not needed. |
| macOS, either CPU | `videosubfinder-cli-macos-universal.tar.gz` | Includes both CPU architectures and their libraries. |
| Windows Intel/AMD 64-bit | `videosubfinder-cli-windows-x64.zip` | Includes `VideoSubFinderCli.exe`; no separate OpenCV, FFmpeg or Visual C++ runtime installation. |
| Windows ARM64 | `videosubfinder-cli-windows-arm64.zip` | Native ARM64 executable with static dependencies; no x64 emulation or separate runtime installation. |

Linux CPU packages are tested in Ubuntu 16.04-26.04 LTS, Debian 10-13, CentOS 7,
AlmaLinux 8-10, Rocky Linux 8/9, Fedora, openSUSE Leap/Tumbleweed, Arch and Alpine
containers. ARM64 uses the supported subset in the [CI matrix](.github/workflows/portable.yml).
ARM64 packages are built and tested on native ARM64 runners, including Windows 11.
Linux container tests validate old user-space on a current host kernel; they do
not validate every old kernel.
macOS packages are tested on macOS 15 on both architectures; compatibility with
older macOS versions has not been verified. Windows binaries are tested on
Windows Server 2022 (x64) and Windows 11 (native ARM64).

### Linux and macOS archives

Extract the archive into its own directory, then run the launcher there:

```sh
mkdir videosubfinder
tar -xzf videosubfinder-cli-linux-x64.tar.gz -C videosubfinder
cd videosubfinder
chmod +x VideoSubFinderCli.run
./VideoSubFinderCli.run --help
```

On macOS, replace the archive name with your macOS download. Keep the executable,
`lib` directory and `settings` directory together. Relative input and output paths
are resolved from your terminal's current directory.

### Linux AppImage

```sh
chmod +x videosubfinder-cli-linux-x64.AppImage
./videosubfinder-cli-linux-x64.AppImage --help
```

If FUSE is unavailable, use extraction mode:

```sh
APPIMAGE_EXTRACT_AND_RUN=1 ./videosubfinder-cli-linux-x64.AppImage --help
```

Use the same extraction-mode prefix with your normal extraction arguments.

### Windows

Extract the ZIP, open PowerShell in the extracted directory, and run:

```powershell
.\VideoSubFinderCli.exe --help
```

All examples below use the same arguments on Windows. Replace
`./VideoSubFinderCli.run` with `.\VideoSubFinderCli.exe`. Keep paths in quotes when
they contain spaces.

### Optional CUDA and worker packages

For NVIDIA GPU processing on Linux x64, download
`videosubfinder-cli-linux-cuda12-x64.tar.gz` and add `--use_cuda` to the command.
It includes CUDA 12 runtime libraries, targets Ubuntu 22.04 / glibc 2.35 or newer,
and requires a compatible NVIDIA driver. GPU extraction and cleared-text image
creation have also been tested on a real CUDA device. CPU packages reject
`--use_cuda` with an explicit error.

For an Ubuntu worker/container that already provides FFmpeg, use the smaller
`videosubfinder-cli-linux-dynamic-ubuntu<VERSION>-<ARCH>.tar.gz` archive. Choose
**the exact Ubuntu version** (20.04, 22.04 or 24.04) and architecture (`x64` or
`arm64`), then install the runtime package:

```sh
apt-get update
apt-get install -y --no-install-recommends ffmpeg
```

These archives include four minimal OpenCV shared libraries and use the matching
system FFmpeg ABI. Use a portable CPU package on other distributions.

### Verify your download

Download `SHA256SUMS` from the same release. From the directory containing both
files, compare the downloaded archive's SHA-256 with its entry in `SHA256SUMS`:

```sh
# Linux
sha256sum videosubfinder-cli-linux-x64.tar.gz
# macOS
shasum -a 256 videosubfinder-cli-macos-universal.tar.gz
```

```powershell
Get-FileHash .\videosubfinder-cli-windows-x64.zip -Algorithm SHA256
```

## Extract your first video

Search a video and save images plus an SRT timing file:

```sh
./VideoSubFinderCli.run -r -i './input video.mkv' -o './Results' \
  -ces './Results/subtitles.srt' -nthr 4
```

The default search covers the full frame and the whole video. MP4 and MKV are
covered by regression tests. Decoding uses OpenCV's FFmpeg backend by default.

| Output | What you can use it for |
| --- | --- |
| `Results/RGBImages/` | Detected subtitle frames, with timestamps and crop metadata in their filenames. |
| `Results/ISAImages/`, `Results/ILAImages/` | Intermediate images used by subtitle processing. |
| `Results/subtitles.srt` | Detected intervals with placeholder text (`[sub_duration]` in the supplied settings). |
| `Results/TXTImages/` | Cleared text images after running `-ccti`. |

To rerun into an existing output directory, add `-c` to clear generated image
files first. **This deletes existing images in the generated image directories.**
Use a separate output directory for each video.

For timing only, skip image output:

```sh
./VideoSubFinderCli.run -r -i './input video.mkv' -o './Timing' \
  -ces './Timing/subtitles.srt' -dsi -nthr 4
```

### Monitor completion

Progress, elapsed seconds, `Frame:` events and the final `FINISH` marker are
written to **stderr**. The process exits with code `0` on success and a nonzero
code on error. In scripts, wait for the process and check its exit code.
`Progress: 100%` marks the end of searching; a combined CCTI operation can continue
until `FINISH`.

```sh
./VideoSubFinderCli.run -r -i './input video.mkv' -o './Results' 2> './vsf.log'
```

## Select the subtitle area and time range

Restrict the search to the area where subtitles appear to reduce processing time
and false detections. All crop values are fractions from `0` to `1`:

- `-le` and `-re` measure the left and right boundaries from the **left edge**.
- `-be` and `-te` measure the bottom and top boundaries from the **bottom edge**.
- Valid values satisfy `0 <= le < re <= 1` and `0 <= be < te <= 1`.

For subtitles in the bottom 40% of the frame, across its full width:

```sh
./VideoSubFinderCli.run -r -i './input video.mp4' -o './Results' \
  -te 0.4 -be 0 -le 0 -re 1
```

Search only from 10.300 seconds to 13.100 seconds, with a narrower crop:

```sh
./VideoSubFinderCli.run -r -i './input video.mp4' -o './Preview' \
  -te 0.5 -be 0.1 -le 0.1 -re 0.9 -s 0:00:10:300 -e 0:00:13:100
```

Time arguments use `hour:min:sec:millisec`, with a colon before milliseconds.
For example, `0:00:01:5` means 1 second and 5 milliseconds.

## Prepare images for OCR

After searching, convert the saved RGB frames into cleared text images:

```sh
./VideoSubFinderCli.run -ccti -o './Results'
```

Read the resulting `Results/TXTImages/` with your preferred OCR tool. You can
combine searching and image clearing in one command:

```sh
./VideoSubFinderCli.run -r -ccti -i './input video.mkv' -o './Results' \
  -ces './Results/subtitles.srt' -te 0.4 -nthr 4
```

CCTI needs saved RGB images, so use image output when you plan to run it. When
clearing images from an earlier run, omit `-c` so those images remain available.

## Command-line reference

Short options from earlier releases remain accepted. Value options support both
`-i 'video.mkv'` and `--input_video='video.mkv'` syntax. Options are case-sensitive.
Use `-ccti` or `--create_cleared_text_images`, including on Windows.

The release's `--help` output is:

```text
VideoSubFinderCli 6.10.5
Usage: VideoSubFinderCli -r -i VIDEO -o OUTPUT [options]
  -c, --clear_dirs                 Clear generated image files
  -r, --run_search                 Search for hardcoded subtitles
  -ces, --create_empty_sub PATH    Write SRT intervals
  -ccti, --create_cleared_text_images  Create TXTImages from RGBImages
  -dsi, --dont_save_images         Write intervals without images
  -gs, --general_settings PATH     Load settings/general.cfg
  -nthr, --num_threads N           Search threads (2-64)
  -s, --start_time H:M:S:MS        Start timestamp
  -e, --end_time H:M:S:MS          End timestamp
  -te, -be, -le, -re NUMBER        Crop fractions (0-1)
  -ovffmpeg, --open_video_ffmpeg   Use OpenCV's FFmpeg backend
  -ovocv, --open_video_opencv      Select the available OpenCV backend
  -uc, --use_cuda                 Require CUDA support
  --verbose --version -h --help
```

The full option list, including the long crop names from the earlier README:

| Short option | Long option | Meaning and default |
| --- | --- | --- |
| `-h` | `--help` | Show help and exit. |
| | `--version` | Show the CLI version and exit. |
| | `--verbose` | Accepted for compatibility; progress and errors are reported without it. |
| `-c` | `--clear_dirs` | Clear generated `.jpeg`, `.jpg`, `.png` and `.bmp` files before processing. Default: keep existing files. |
| `-r` | `--run_search` | Search the input video for hardcoded subtitles. |
| `-i PATH` | `--input_video PATH` | Input video. Required for `-r`. |
| `-o PATH` | `--output_dir PATH` | Output root directory. Default: current working directory. |
| `-ces PATH` | `--create_empty_sub PATH` | Write SRT intervals with placeholder text after searching. Default: no SRT file. |
| `-ccti` | `--create_cleared_text_images` | Create `TXTImages` from `RGBImages` in the output directory. Can run alone or after `-r`. |
| `-dsi` | `--dont_save_images` | Skip image output during searching; useful with `-ces`. Default: save images. |
| `-gs PATH` | `--general_settings PATH` | Read a settings file. Default: `settings/general.cfg` beside the executable, when present. |
| `-nthr N` | `--num_threads N` | Search threads, 2-64. Default: the settings value (`-1` selects automatic thread count). |
| `-s H:M:S:MS` | `--start_time H:M:S:MS` | Start timestamp. Default: `0:00:00:000`. |
| `-e H:M:S:MS` | `--end_time H:M:S:MS` | End timestamp. Default: video length. |
| `-te NUMBER` | `--top_video_image_percent_end NUMBER` | Top crop boundary measured from the bottom. Default: `1.0`. |
| `-be NUMBER` | `--bottom_video_image_percent_end NUMBER` | Bottom crop boundary measured from the bottom. Default: `0.0`. |
| `-le NUMBER` | `--left_video_image_percent_end NUMBER` | Left crop boundary measured from the left. Default: `0.0`. |
| `-re NUMBER` | `--right_video_image_percent_end NUMBER` | Right crop boundary measured from the left. Default: `1.0`. |
| `-ovffmpeg` | `--open_video_ffmpeg` | Use OpenCV's FFmpeg backend. This is the default. |
| `-ovocv` | `--open_video_opencv` | Let OpenCV select an available video backend automatically. |
| `-uc` | `--use_cuda` | Enable CUDA processing. Requires a CUDA build and a compatible GPU/driver. Default: CPU. |

At least one processing action (`-r` or `-ccti`) is required. Settings can be tuned
with a copy of the supplied [general.cfg](Settings/general.cfg); use `-gs` to
select that copy.

## Troubleshooting

| Symptom | What to try |
| --- | --- |
| No images or empty SRT | Check the crop, try a short time range with full-frame defaults, and confirm the video contains visible hardcoded text. `-dsi` intentionally skips images. |
| Processing appears stuck | Read stderr for progress and elapsed time. Try `-e 0:00:10:000` on a longer video to check a short sample. |
| Cannot decode input video | Check the path and permissions. Use a current portable package; try `-ovocv` if the default FFmpeg backend cannot open your codec. |
| Old macOS `dyld` / missing OpenCV or FFmpeg library error | Extract a complete current macOS package into a new directory and keep its `lib` folder. No Homebrew downgrades are needed. |
| Old Linux package crashes or reports missing libraries | Replace the old package with the current portable CPU archive. The launcher isolates bundled libraries from inherited `LD_LIBRARY_PATH`. |
| Dynamic worker package reports an FFmpeg library error | Install `ffmpeg` and match the archive's Ubuntu version and CPU architecture to the container. |
| AppImage reports a FUSE error | Use `APPIMAGE_EXTRACT_AND_RUN=1` as shown above. |
| CUDA is unavailable | Check the CUDA package, NVIDIA driver and GPU availability, or use a CPU package without `--use_cuda`. |

If the problem persists, [open an issue](https://github.com/eritpchy/videosubfinder-cli/issues)
with the release version, OS/architecture, full command, exit code and stderr.
For a detection problem, include a short shareable video sample and the crop/time
range needed to reproduce it.

## Build from source

Prebuilt packages are the easiest way to use the CLI. To build your own, install
a C++17 compiler, CMake 3.24+, Python 3.8+ and OpenCV 4 development packages:

```sh
git clone --recurse-submodules https://github.com/eritpchy/videosubfinder-cli.git
cmake -S videosubfinder-cli -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
cmake --install build --prefix dist
```

Tests use FFmpeg and a system TrueType font to generate timed MKV, MPEG-4 and
H.264 fixtures. Alternatively, set `VSF_TEST_MEDIA` to a directory containing
`input video.mkv`, `input video.mp4` and `input h264.mp4`.

Packaging workflows and dependencies are in [portable.yml](.github/workflows/portable.yml).
For fully static Linux CPU builds, use `scripts/build-static-linux.sh` in Alpine
3.22. For Windows, use the checked-in vcpkg manifest with `x64-windows-static` or
`arm64-windows-static`, the matching CMake architecture (`-A x64` / `-A ARM64`),
and `CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded`. CUDA builds use `-DUSE_CUDA=ON`.

CI tests the installed packages on Linux, macOS and Windows, including AppImage,
Universal, dynamic-worker and sanitizer checks. CUDA CI exercises the CPU path;
set `VSF_TEST_CUDA=1` on a CUDA host to run GPU extraction and CCTI regressions.
A `v*` tag publishes a release only after the required jobs pass.
Fork releases remain in the upstream VideoSubFinder `6.10.x` series. CMake
reads the version header; the CLI, dependency manifest, README help and release
tag must match. Version checks reject releases outside this series.

## Credits and license

Based on [VideoSubFinder 6.10](https://sourceforge.net/projects/videosubfinder/),
with the CLI fixes and the standard C++ core used by Subrazen. The CLI runs
without wxWidgets, GUI frameworks, Intel's compiler, Eigen, OpenEXR development
packages, Torch or an OCR engine.

Licensed under [GPL-2.0](LICENSE). Bundled library notices are in
[THIRD_PARTY.md](THIRD_PARTY.md) and the release packages' `licenses` directory.
Matching source is available as `videosubfinder-cli-source.tar.gz` in each release.

## Donate

If you use and love videosubfinder-cli, please consider sending some Bitcoin (BTC) to bc1q9t4t6g42hge2stvu96rzr6kn7ynq52al5uv0pg
In case you want to be mentioned as a sponsor, let me know!
