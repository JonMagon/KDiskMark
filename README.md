# KDiskMark
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-orange.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![GitHub (pre-)release](https://img.shields.io/github/release/JonMagon/KDiskMark/all.svg)](https://github.com/JonMagon/KDiskMark/releases)
[![Build Status](https://travis-ci.com/JonMagon/KDiskMark.svg?branch=master)](https://travis-ci.com/JonMagon/KDiskMark) 

KDiskMark is an HDD and SSD benchmark tool with a very friendly graphical user interface. KDiskMark with its presets and powerful GUI calls [Flexible I/O Tester](https://github.com/axboe/fio) and handles the output to provide an easy to view and interpret comprehensive benchmark result.

<p align="center">
   <img src="https://raw.githubusercontent.com/JonMagon/KDiskMark/master/assets/images/kdiskmark.png"/>
</p>

## Dependencies
### Required
* GCC/Clang C++11 (or later)
* [CMake](https://cmake.org/) >= 3.5
* [Extra CMake Modules](https://github.com/KDE/extra-cmake-modules)
* [Flexible I/O Tester](https://github.com/axboe/fio) with libaio >= 3.1
    * If you build FIO from source, install `libaio-dev` package.
* Qt >= 5.9

## Installation
Downloads are available on the [Releases](https://github.com/JonMagon/KDiskMark/releases/latest) page. 

## Localization
To help with localization you can use [Qt Linguist](https://doc.qt.io/Qt-5/linguist-translators.html). Add a new language file to the TS_FILES variable in CMakeLists.txt, translate it and create a pull request. 

Languages currently available:
* English (default)
* Russian
* German

## Credits
Application Icon  
Copyright (c) https://www.iconfinder.com/baitisstudio

If you have any ideas, critics, suggestions or whatever you want to call it, please open an issue.
