VideoSubFinder is distributed under GPL version 2 as declared by the official
SourceForge project: https://sourceforge.net/projects/videosubfinder/.
The original author is skosnits (Dmitry Kosnitskiy). The source history is retained
from https://git.code.sf.net/p/videosubfinder/src.

The portable CLI includes code from the Subrazen standard C++ port and
https://github.com/eritpchy/videosubfinder-cli. Complete CLI source and build
instructions are available in this repository and the release source archive.

The Linux static build uses these source revisions:

- OpenCV 4.14.0: https://github.com/opencv/opencv/tree/0654a42e19215ef25b1d367d822f3c630447e7c7
- FFmpeg 8.0.1: https://github.com/FFmpeg/FFmpeg/tree/894da5ca7d742e4429ffb2af534fcda0103ef593
- musl: https://musl.libc.org/ (Alpine 3.22 build toolchain)
- GCC runtime: https://gcc.gnu.org/ (Alpine 3.22 build toolchain)

FFmpeg is built without GPL or nonfree components and with external dependency
autodetection disabled. OpenCV's bundled JPEG, PNG and zlib implementations retain
their original notices. Dependency license files are included in Linux packages.
Header-only JSON and thread-pool dependency licenses are included in every package.
Windows dependency versions are pinned by vcpkg.json. macOS dependencies are
provided by Homebrew and bundled with the executable.
