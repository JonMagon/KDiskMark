# KDiskMark
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-orange.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![GitHub (pre-)release](https://img.shields.io/github/release/JonMagon/KDiskMark/all.svg)](https://github.com/JonMagon/KDiskMark/releases)
[![Main](https://github.com/JonMagon/KDiskMark/actions/workflows/main.yml/badge.svg)](https://github.com/JonMagon/KDiskMark/actions/workflows/main.yml)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/d4457b2f0d2947be95414218e37ce19f)](https://app.codacy.com/manual/JonMagon/KDiskMark?utm_source=github.com&utm_medium=referral&utm_content=JonMagon/KDiskMark&utm_campaign=Badge_Grade_Dashboard)

**KDiskMark** is an HDD and SSD benchmark tool with a very friendly graphical user interface. **KDiskMark** with its presets and powerful GUI calls [Flexible I/O Tester](https://github.com/axboe/fio) and handles the output to provide an easy to view and interpret comprehensive benchmark result. The application is written in C++ with Qt and *doesn't have* any KDE dependencies.

<p align="center">
   <img src="https://raw.githubusercontent.com/JonMagon/KDiskMark/master/assets/images/kdiskmark.png"/>
</p>

## Features
* Configurable block size, queues, and threads count for each test
* Many languages support
* Report generation

## Report Example
```
                     KDiskMark (2.3.0): https://github.com/JonMagon/KDiskMark
                 Flexible I/O Tester (fio-3.28): https://github.com/axboe/fio
-----------------------------------------------------------------------------
* MB/s = 1,000,000 bytes/s [SATA/600 = 600,000,000 bytes/s]
* KB = 1000 bytes, KiB = 1024 bytes

[Read]
Sequential 1 MiB (Q= 8, T= 1):   550.145 MB/s [    537.3 IOPS] < 14840.12 us>
Sequential 1 MiB (Q= 1, T= 1):   456.261 MB/s [    445.6 IOPS] <  2233.53 us>
    Random 4 KiB (Q=32, T= 1):   339.953 MB/s [  84988.3 IOPS] <   377.48 us>
    Random 4 KiB (Q= 1, T= 1):    43.042 MB/s [  10760.6 IOPS] <    90.69 us>

[Write]
Sequential 1 MiB (Q= 8, T= 1):   522.410 MB/s [    510.2 IOPS] < 15210.65 us>
Sequential 1 MiB (Q= 1, T= 1):   371.278 MB/s [    362.6 IOPS] <  2248.00 us>
    Random 4 KiB (Q=32, T= 1):   330.205 MB/s [  82551.5 IOPS] <   387.88 us>
    Random 4 KiB (Q= 1, T= 1):   100.602 MB/s [  25150.7 IOPS] <    36.40 us>

Profile: Default
   Test: 1 GiB (x5) [Interval: 5 sec]
   Date: 2021-11-02 13:17:41
     OS: opensuse-tumbleweed 20211031 [linux 5.14.14-1-default]
```

## Dependencies
### Build
* GCC/Clang C++17 (or later)
* [CMake](https://cmake.org/) >= 3.5
* [Extra CMake Modules](https://github.com/KDE/extra-cmake-modules)
* [KAuth](https://github.com/KDE/kauth) (optional)
* Qt with Widgets >= 5.9
### Runtime
* Qt with Widgets >= 5.9
* [Flexible I/O Tester](https://github.com/axboe/fio) with libaio >= 3.1
    * If you build FIO from source, install `libaio-dev` package.
* KAuth Library

## Installation
Binaries are available on the [Releases](https://github.com/JonMagon/KDiskMark/releases/latest) page. 

### Install from the Snap Store
[![Get it from the Snap Store](https://snapcraft.io/static/images/badges/en/snap-store-white.svg)](https://snapcraft.io/kdiskmark)
```bash
sudo snap install kdiskmark
````

### Ubuntu based distros
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

KDiskMark is included in the official [Fedora](https://src.fedoraproject.org/rpms/kdiskmark/) repo. You can install it like any other package:
```bash
sudo dnf install kdiskmark
```

### openSUSE Tumbleweed

```bash
sudo zypper install kdiskmark
```

## Building
### Building a package using CPack
You can build **KDiskMark** by using the following commands:

```bash
mkdir build && cd build
cmake -D CMAKE_BUILD_TYPE=Release ..
cpack -G DEB # Or RPM, ZIP etc.
```

### Build parameters
* `BUILD_WITH_PAGECACHE_CLEARING_SUPPORT` enables pagecache clearing functionality (default is ON). If disabled, the application will not be able to clear the pagecache in any way.
* `PERFORM_PAGECACHE_CLEARING_USING_KF5AUTH` determines whether a helper will be built (default is ON). If the KF5Auth helper is not built, the application will be able to clear the pagecache if it is run as root.

Build parameters are passed at configuration stage:  
`cmake -D PERFORM_PAGECACHE_CLEARING_USING_KF5AUTH=OFF ..`

## Localization [![Crowdin](https://badges.crowdin.net/kdiskmark/localized.svg)](https://crowdin.com/project/kdiskmark)
To help with localization you can use [Crowdin](https://crowdin.com/project/kdiskmark) or translate files in `data/translations` with [Qt Linguist](https://doc.qt.io/Qt-5/linguist-translators.html) directly. To add a new language, copy `data/translations/kdiskmark.ts` to `data/translations/kdiskmark_<ISO 639-1 language code>_<ISO 3166-1 alpha-2 language code>.ts`, translate it, then add the file to the TS_FILES variable in CMakeLists.txt, and create a pull request. It is also possible to add localized Comment and Keywords sections into `data/kdiskmark.desktop`.

Languages currently available:
* Chinese (Simplified)
* Czech
* English (default)
* French
* German
* Hindi
* Hungarian
* Italian
* Polish
* Portuguese (Brazilian)
* Russian
* Slovak
* Spanish (Mexico)
* Turkish
* Ukrainian

## TODO
- [ ] CLI
- [ ] Windows compatibility
- [x] Performance profiles (mix, peak, real-world)

## Special Thanks
* Artem Grinev (<agrinev@manjaro.org>) for his help with assembling the AppImage package.

Thanks to the package maintainers, translators, and all users for supporting the project.

## Credits
Application Icon  
Copyright (c) https://www.iconfinder.com/baitisstudio

If you have any ideas, critics, suggestions or whatever you want to call it, please open an issue.
