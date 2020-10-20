# KDiskMark
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-orange.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![GitHub (pre-)release](https://img.shields.io/github/release/JonMagon/KDiskMark/all.svg)](https://github.com/JonMagon/KDiskMark/releases)
[![Build Status](https://travis-ci.com/JonMagon/KDiskMark.svg?branch=master)](https://travis-ci.com/JonMagon/KDiskMark)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/d4457b2f0d2947be95414218e37ce19f)](https://app.codacy.com/manual/JonMagon/KDiskMark?utm_source=github.com&utm_medium=referral&utm_content=JonMagon/KDiskMark&utm_campaign=Badge_Grade_Dashboard)
![GitHub All Releases](https://img.shields.io/github/downloads/JonMagon/KDiskMark/total?color=blue)
![PPA installations](https://ppa-downloads-badge.herokuapp.com/?ppa=jonmagon/kdiskmark)

KDiskMark is an HDD and SSD benchmark tool with a very friendly graphical user interface. KDiskMark with its presets and powerful GUI calls [Flexible I/O Tester](https://github.com/axboe/fio) and handles the output to provide an easy to view and interpret comprehensive benchmark result.

<p align="center">
   <img src="https://raw.githubusercontent.com/JonMagon/KDiskMark/master/assets/images/kdiskmark.png"/>
</p>

## Features
* Configurable block size, queues, and threads count for each test
* Many languages support
* Report generation

## Report Example
```
                     KDiskMark (1.6.0): https://github.com/JonMagon/KDiskMark
                 Flexible I/O Tester (fio-3.16): https://github.com/axboe/fio
-----------------------------------------------------------------------------
* MB/s = 1,000,000 bytes/s [SATA/600 = 600,000,000 bytes/s]
* KB = 1000 bytes, KiB = 1024 bytes

[Read]
Sequential 1 MiB (Q= 8, T= 1):   542.516 MB/s [    529.8 IOPS] < 14415.61 us>
Sequential 1 MiB (Q= 1, T= 1):   452.596 MB/s [    442.0 IOPS] <  2248.08 us>
    Random 4 KiB (Q=32, T=16):   271.553 MB/s [  67889.0 IOPS] <  1955.57 us>
    Random 4 KiB (Q= 1, T= 1):    43.252 MB/s [  10813.1 IOPS] <    90.34 us>

[Write]
Sequential 1 MiB (Q= 8, T= 1):   513.605 MB/s [    501.6 IOPS] < 15319.33 us>
Sequential 1 MiB (Q= 1, T= 1):   428.900 MB/s [    418.8 IOPS] <  2369.68 us>
    Random 4 KiB (Q=32, T=16):   165.142 MB/s [  41286.6 IOPS] <  3091.38 us>
    Random 4 KiB (Q= 1, T= 1):   103.696 MB/s [  25924.1 IOPS] <    36.71 us>

Profile: Default
   Test: 32 MiB (x5) [Interval: 5 sec]
   Date: 2020/09/05 18:31:47
     OS: neon 20.04 [linux 5.4.0-42-generic]
```

## Dependencies
### Required
* GCC/Clang C++17 (or later)
* [CMake](https://cmake.org/) >= 3.5
* [Extra CMake Modules](https://github.com/KDE/extra-cmake-modules)
* Qt with Widgets >= 5.9
* [Flexible I/O Tester](https://github.com/axboe/fio) with libaio >= 3.1
    * If you build FIO from source, install `libaio-dev` package.

## Installation
Binaries are available on the [Releases](https://github.com/JonMagon/KDiskMark/releases/latest) page. 

### Ubuntu 20.04 focal based distros
```bash
sudo add-apt-repository ppa:jonmagon/kdiskmark
sudo apt update
sudo apt install kdiskmark
```

### Arch based distros

KDiskMark is included in the official [community](https://www.archlinux.org/packages/community/x86_64/kdiskmark/) repo. You can install it like any other package:
```bash
sudo pacman -Syu kdiskmark
```

Development version can be installed from AUR `kdiskmark-git` package.
```bash
git clone https://aur.archlinux.org/kdiskmark-git.git
cd kdiskmark-git
makepkg -si
```

### Fedora
```bash
sudo dnf copr enable atim/kdiskmark -y
sudo dnf install kdiskmark
```

## Building
### Building executable
You can build **KDiskMark** by using the following commands:

```bash
mkdir build && cd build
cmake -D CMAKE_BUILD_TYPE=Release ..
cmake --build .
```

## Localization
To help with localization you can use [Crowdin](https://crowdin.com/project/kdiskmark) or translate files in `data/translations` with [Qt Linguist](https://doc.qt.io/Qt-5/linguist-translators.html) directly. To add a new language, copy `data/translations/kdiskmark.ts` to `data/translations/kdiskmark_<ISO 639-1 language code>_<ISO 3166-1 alpha-2 language code>.ts`, translate it, then add the file to the TS_FILES variable in CMakeLists.txt, and create a pull request. It is also possible to add localized Comment and Keywords sections into `data/kdiskmark.desktop`.

Languages currently available:
* Chinese (Simplified)
* Czech
* English (default)
* French
* German
* Italian
* Portuguese (Brazilian)
* Russian

## TODO
- [ ] Windows compatibility
- [x] Performance profiles (mix, peak, real-world)

## Credits
Application Icon  
Copyright (c) https://www.iconfinder.com/baitisstudio

If you have any ideas, critics, suggestions or whatever you want to call it, please open an issue.
