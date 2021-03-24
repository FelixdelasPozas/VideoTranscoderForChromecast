Video Transcoder For Chromecast
===============================

# Summary
- [Description](#description)
- [Compilation](#compilation-requirements)
- [Install](#install)
- [Screenshots](#screenshots)
- [Repository information](#repository-information)

# Description
Simple program to transcode video files to formats supported by Chromescast Ultra devices using the NVidia NVENC encoder
for hardware acceleration.   

## Options
The tool can be configured:
* The output video codecs are H.264, HEVC (H.265), VP8 and VP9. Only the first two are hardware accelerated using NVENC.
* The output audio codecs are AAC for H.264/H.265 and Vorbis OGG for VP8/VP9.
* Extract subtitles from the input files with language preferences. Only SRT subtitles are supported, other subtitle formats are ignored.
* Select output audio language by preferences.

## Input file formats
The input videos recognized by the tool are the same recognized by libav (ffmpeg) library.

## Output file formats
The resulting transcoded files are in Matroska MKV format.  

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
There will never be any binary release of this program, as my libav is compiled with '--enable-nonfree' flag thus
making it unredistributable. The source code releases can be downloaded from the
![releases page](https://github.com/FelixdelasPozas/VideoTranscoderForChromecast/releases/).

# Screenshots
Configuration dialog.

![Configuration Dialog](https://user-images.githubusercontent.com/12167134/103494167-aeee0600-4e35-11eb-8f26-7ec2970a4675.png)

Simple main dialog.

![Main dialog](https://user-images.githubusercontent.com/12167134/103810436-88111900-505b-11eb-88bf-642da3d46778.png)

Dialog shown while processing files.

![Process Dialog](https://user-images.githubusercontent.com/12167134/103494169-af869c80-4e35-11eb-9dd7-7adf64bf9f59.png)

# Repository information
**Version**: 1.1.3

**Status**: finished

**License**: GNU General Public License 3

**cloc statistics**

| Language                     |files          |blank        |comment          |code  |
|:-----------------------------|--------------:|------------:|----------------:|-----:|
| C++                          |    7          |  421        |    247          |1858  |
| C/C++ Header                 |    6          |  163        |    457          | 314  |
| CMake                        |    2          |   26        |     22          |  93  |
| **Total**                    |   **15**      |  **610**    |   **726**       |**2265**|
