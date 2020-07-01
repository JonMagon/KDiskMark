cmake_minimum_required(VERSION 3.5)

project(KDiskMark VERSION 1.2.0 LANGUAGES CXX)

configure_file(src/cmake.h.in cmake.h)
include_directories(${PROJECT_BINARY_BIN})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets LinguistTools REQUIRED)

set(TS_FILES data/translations/KDiskMark_ru_RU.ts data/translations/KDiskMark_de_DE.ts)

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

add_executable(KDiskMark
    ${QM_FILES}
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
    src/benchmark.cpp
    src/benchmark.h
    src/about.cpp
    src/about.h
    src/about.ui
    src/appsettings.h
    src/appsettings.cpp
    src/settings.cpp
    src/settings.h
    src/settings.ui
    src/global.cpp
    src/global.h
)

target_link_libraries(KDiskMark PRIVATE Qt5::Widgets)

file(COPY data/icons/kdiskmark.png
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/icons/)

file(COPY data/icons/kdiskmark.svg
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/icons/)
