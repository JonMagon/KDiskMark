cmake_minimum_required(VERSION 3.10)

project(kdiskmark VERSION 3.2.0 LANGUAGES CXX)

set(CMAKE_PROJECT_HOMEPAGE_URL "https://github.com/JonMagon/KDiskMark")
set(CMAKE_PROJECT_DESCRIPTION "A simple open source disk benchmark tool for Linux distros")

set(APPLICATION_NAME "KDiskMark")
set(DESKTOP_FILE kdiskmark.desktop)

configure_file(src/cmake.h.in cmake.h)
include_directories(${PROJECT_BINARY_BIN})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(USE_QT5 "Use Qt5 instead of Qt6" OFF)

if(USE_QT5)
    find_package(Qt5 COMPONENTS Widgets LinguistTools DBus REQUIRED)
    find_package(PolkitQt5-1 REQUIRED)
    set(QT_LIBS Qt5::Widgets Qt5::DBus)
    set(POLKIT_LIB PolkitQt5-1::Core)
else()
    find_package(Qt6 COMPONENTS Widgets LinguistTools DBus REQUIRED)
    find_package(PolkitQt6-1 REQUIRED)
    set(QT_LIBS Qt6::Widgets Qt6::DBus)
    set(POLKIT_LIB PolkitQt6-1::Core)
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CPACK_PACKAGE_VENDOR ${APPLICATION_NAME})
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${APPLICATION_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT jonmagon@gmail.com)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY >=)
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_LICENSE GPLv3)
set(CPACK_RPM_PACKAGE_GROUP Applications/System)
set(CPACK_RPM_PACKAGE_REQUIRES "fio >= 3.1")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "fio (>= 3.1)")
set(CPACK_RPM_COMPRESSION_TYPE lzma)
set(CPACK_NSIS_DISPLAY_NAME ${APPLICATION_NAME})

set(QAPPLICATION_CLASS QApplication)
add_subdirectory(src/singleapplication)

find_package(ECM REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(CPack)
include(GNUInstallDirs)
include(ECMInstallIcons)
include(ECMConfiguredInstall)
include(KDEInstallDirs)

set(helper_interface_xml dev.jonmagon.kdiskmark.helperinterface.xml)

# Generate DBus Interface
if(USE_QT5)
    qt5_generate_dbus_interface(
        src/helper.h
        ${helper_interface_xml}
        OPTIONS -a
    )
    qt5_add_dbus_interface(HelperInterface_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${helper_interface_xml} helper_interface)
else()
    qt6_generate_dbus_interface(
        src/helper.h
        ${helper_interface_xml}
        OPTIONS -a
    )
    qt6_add_dbus_interface(HelperInterface_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${helper_interface_xml} helper_interface)
endif()

set(TS_FILES
    data/translations/${PROJECT_NAME}_cs_CZ.ts
    data/translations/${PROJECT_NAME}_de_DE.ts
    data/translations/${PROJECT_NAME}_es_ES.ts
    data/translations/${PROJECT_NAME}_es_MX.ts
    data/translations/${PROJECT_NAME}_fi_FI.ts
    data/translations/${PROJECT_NAME}_fr_FR.ts
    data/translations/${PROJECT_NAME}_hi_IN.ts
    data/translations/${PROJECT_NAME}_hu_HU.ts
    data/translations/${PROJECT_NAME}_it_IT.ts
    data/translations/${PROJECT_NAME}_ja_JP.ts
    data/translations/${PROJECT_NAME}_nl_NL.ts
    data/translations/${PROJECT_NAME}_pl_PL.ts
    data/translations/${PROJECT_NAME}_pt_BR.ts
    data/translations/${PROJECT_NAME}_ru_RU.ts
    data/translations/${PROJECT_NAME}_sk_SK.ts
    data/translations/${PROJECT_NAME}_sv_SE.ts
    data/translations/${PROJECT_NAME}_tr_TR.ts
    data/translations/${PROJECT_NAME}_uk_UA.ts
    data/translations/${PROJECT_NAME}_zh_CN.ts
    data/translations/${PROJECT_NAME}_zh_TW.ts
)

if(USE_QT5)
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    qt6_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

add_executable(${PROJECT_NAME}
    ${QM_FILES}
    ${HelperInterface_SRCS}
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/mainwindow.ui
    src/storageitemdelegate.h
    src/storageitemdelegate.cpp
    src/benchmark.h
    src/benchmark.cpp
    src/about.h
    src/about.cpp
    src/about.ui
    src/appsettings.h
    src/appsettings.cpp
    src/settings.h
    src/settings.cpp
    src/settings.ui
    src/global.h
    src/global.cpp
    src/diskdriveinfo.h
    src/diskdriveinfo.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SingleApplication::SingleApplication
    ${QT_LIBS}
)

set(APP_ICONS
    data/icons/16-apps-kdiskmark.png
    data/icons/24-apps-kdiskmark.png
    data/icons/32-apps-kdiskmark.png
    data/icons/48-apps-kdiskmark.png
    data/icons/64-apps-kdiskmark.png
    data/icons/128-apps-kdiskmark.png
    data/icons/256-apps-kdiskmark.png
    data/icons/512-apps-kdiskmark.png
)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/${PROJECT_NAME}/translations)
install(FILES data/${DESKTOP_FILE} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)

ecm_install_icons(ICONS ${APP_ICONS} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons)

# Helper
add_executable(${PROJECT_NAME}_helper
    src/helper.cpp
    src/helper.h
)

target_link_libraries(${PROJECT_NAME}_helper
    ${QT_LIBS}
    ${POLKIT_LIB}
)

install(TARGETS ${PROJECT_NAME}_helper DESTINATION ${KDE_INSTALL_LIBEXECDIR})
install(FILES data/dev.jonmagon.kdiskmark.helperinterface.conf DESTINATION ${KDE_INSTALL_DBUSDIR}/system.d)

install(FILES data/dev.jonmagon.kdiskmark.helper.policy DESTINATION ${POLKITQT-1_POLICY_FILES_INSTALL_DIR})
ecm_install_configured_files(
    INPUT data/dev.jonmagon.kdiskmark.helperinterface.service.in
    DESTINATION ${KDE_INSTALL_DBUSDIR}/system-services
)
