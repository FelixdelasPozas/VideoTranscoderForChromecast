Video Transcoder For Chromecast
===============================

# Summary
- [Description](#description)
- [Compilation](#compilation-requirements)
- [Install](#install)
- [Screenshots](#screenshots)
- [Repository information](#repository-information)

# Description
Simple transcoder program to transcode video files to formats supported by Chromescast devices using the NVidia NVENC encoder.   

** TODO **

## Options
The tool can be configured:
* The output codecs are H.264, HEVC (H.265), VP8 and VP9. Only the first two are hardware accelerated using NVENC.
* Extract subtitle files from the input files with language preferences.
* Select output audio language by preferences.

## Input file formats
The input videos recognized by the tool are the same recognized by libav (ffmpeg) library. 

# Compilation requirements
## To build the tool:
* cross-platform build system: [CMake](http://www.cmake.org/cmake/resources/software.html).
* compiler: [Mingw64](http://sourceforge.net/projects/mingw-w64/) on Windows or [gcc](http://gcc.gnu.org/) on Linux.

## External dependencies:
The following libraries are required:
* [libav](https://libav.org/) - Open source audio and video processing tools.
* [libvpx](https://www.webmproject.org/) - WebM project VPx codec implementation. 
* [Qt opensource framework](http://www.qt.io/).

# Install

Download and execute the ![latest release](https://github.com/FelixdelasPozas/transcodertomp3/releases) installer.

# Screenshots
Configuration dialog.

![Configuration Dialog](https://cloud.githubusercontent.com/assets/12167134/14055036/94e9906a-f2de-11e5-8f8c-5989a96dc791.jpg)

Simple main dialog.

![Main dialog](https://cloud.githubusercontent.com/assets/12167134/7867872/e2fd4c28-0578-11e5-93bb-56c7ee8b26df.jpg)

Dialog shown while processing files.

![Process Dialog](https://cloud.githubusercontent.com/assets/12167134/7867873/e48c0714-0578-11e5-8de4-ba1b44b1b72f.jpg)

# Repository information
**Version**: 1.2.2

**Status**: finished

**License**: GNU General Public License 3

**cloc statistics**

| Language                     |files          |blank        |comment           |code  |
|:-----------------------------|--------------:|------------:|-----------------:|-----:|
| C++                          |   11          |  601        |    384           |2458  |
| C/C++ Header                 |   10          |  243        |    693           | 463  |
| CMake                        |    1          |   22        |     13           |  95  |
| **Total**                    |   **22**      |  **866**    |   **1090**       |**3016**|
