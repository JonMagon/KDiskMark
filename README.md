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
                        KDiskMark (3.0.0): https://github.com/JonMagon/KDiskMark
                    Flexible I/O Tester (fio-3.30): https://github.com/axboe/fio
--------------------------------------------------------------------------------
* MB/s = 1,000,000 bytes/s [SATA/600 = 600,000,000 bytes/s]
* KB = 1000 bytes, KiB = 1024 bytes

[Read]
Sequential   1 MiB (Q=  8, T= 1):   508.897 MB/s [    497.0 IOPS] < 13840.05 us>
Sequential   1 MiB (Q=  1, T= 1):   438.278 MB/s [    428.0 IOPS] <  2280.14 us>
    Random   4 KiB (Q= 32, T= 1):   354.657 MB/s [  88664.6 IOPS] <   352.37 us>
    Random   4 KiB (Q=  1, T= 1):    44.166 MB/s [  11041.6 IOPS] <    88.48 us>

[Write]
Sequential   1 MiB (Q=  8, T= 1):   460.312 MB/s [    449.5 IOPS] < 15153.11 us>
Sequential   1 MiB (Q=  1, T= 1):   333.085 MB/s [    325.3 IOPS] <  2349.82 us>
    Random   4 KiB (Q= 32, T= 1):   315.170 MB/s [  78792.5 IOPS] <   383.86 us>
    Random   4 KiB (Q=  1, T= 1):    91.040 MB/s [  22760.3 IOPS] <    39.80 us>

Profile: Default
   Test: 1 GiB (x5) [Measure: 5 sec / Interval: 5 sec]
   Date: 2022-08-24 16:10:33
     OS: opensuse-tumbleweed 20220821 [linux 5.19.2-1-default]
```

## Dependencies
### Required
* GCC/Clang C++17 (or later)
* [CMake](https://cmake.org/) >= 3.12
* [Extra CMake Modules](https://github.com/KDE/extra-cmake-modules) >= 5.73
* [Qt](https://www.qt.io/) with Widgets and DBus >= 5.9
* [PolicyKit](https://gitlab.freedesktop.org/polkit/polkit) Agent
    * `PolkitQt-1` bindings.
* [Flexible I/O Tester](https://github.com/axboe/fio) with libaio >= 3.1
    * `libaio` development package.

### External libraries
* [SingleApplication](https://github.com/itay-grudev/SingleApplication) prevents launch of multiple application instances.

## Installation
Binaries are available on the [Releases](https://github.com/JonMagon/KDiskMark/releases/latest) page. 

### Install from the Snap Store
[![Get it from the Snap Store](https://snapcraft.io/static/images/badges/en/snap-store-white.svg)](https://snapcraft.io/kdiskmark)
```bash
sudo snap install kdiskmark
sudo snap connect kdiskmark:removable-media # external storages
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

## Localization [![Crowdin](https://badges.crowdin.net/kdiskmark/localized.svg)](https://crowdin.com/project/kdiskmark)
To help with localization you can use [Crowdin](https://crowdin.com/project/kdiskmark) or translate files in `data/translations` with [Qt Linguist](https://doc.qt.io/Qt-5/linguist-translators.html) directly. To add a new language, copy `data/translations/kdiskmark.ts` to `data/translations/kdiskmark_<ISO 639-1 language code>_<ISO 3166-1 alpha-2 language code>.ts`, translate it, then add the file to the TS_FILES variable in CMakeLists.txt, and create a pull request. It is also possible to add localized Comment and Keywords sections into `data/kdiskmark.desktop` and message for PolicyKit authorization into `data/dev.jonmagon.kdiskmark.helper.policy`.

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
- [ ] Text-based user interface
- [x] Performance profiles (mix, peak, real-world)

## Special Thanks
* Artem Grinev (<agrinev@manjaro.org>) for his help with assembling the AppImage package.

Thanks to the package maintainers, translators, and all users for supporting the project.

## Credits
**Application Icon**  
Copyright (c) https://www.iconfinder.com/baitisstudio

**FlagKit**  
https://github.com/madebybowtie/FlagKit

If you have any ideas, critics, suggestions or whatever you want to call it, please open an issue.
