cmake_minimum_required(VERSION 3.20)
project(ChirikSIP VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(WIN32)
    add_executable(chiriksip WIN32
        src/main.cpp
        src/mainwindow.cpp
        src/settingsdialog.cpp
        src/setupwizard.cpp
        src/sipclient.cpp
        src/audiobridge.cpp
        src/ringtone.cpp
        src/callnotification.cpp
        windows/chiriksip.rc
    )
else()
    add_executable(chiriksip
        src/main.cpp
        src/mainwindow.cpp
        src/settingsdialog.cpp
        src/setupwizard.cpp
        src/sipclient.cpp
        src/audiobridge.cpp
        src/ringtone.cpp
        src/callnotification.cpp
    )
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

if(WIN32)
    find_path(PJSIP_INCLUDE_DIR pjsua.h)
    find_library(PJSIP_LIB NAMES pjproject pj pjsua)
    find_library(PJSIP_LIB NAMES pjmedia)
    find_library(PJSIP_LIB NAMES pjsip)
    find_library(PA_LIB NAMES portaudio)

    find_library(PJ_LIB NAMES pj)
    find_library(PJNATH_LIB NAMES pjnath)
    find_library(PJLIB_UTIL_LIB NAMES pjlib-util)

    target_include_directories(chiriksip PRIVATE ${PJSIP_INCLUDE_DIR})
    target_link_libraries(chiriksip PRIVATE
        Qt6::Core Qt6::Widgets
        ${PJSIP_LIB} ${PJ_LIB} ${PJNATH_LIB} ${PJLIB_UTIL_LIB}
        ${PA_LIB}
        ws2_32 winmm ole32 uuid
    )
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PJSIP REQUIRED IMPORTED_TARGET libpjproject)
    target_include_directories(chiriksip PRIVATE ${PJSIP_INCLUDE_DIRS})
    target_link_libraries(chiriksip PRIVATE
        Qt6::Core Qt6::Widgets
        ${PJSIP_LIBRARIES}
        portaudio
    )
    target_compile_options(chiriksip PRIVATE ${PJSIP_CFLAGS_OTHER})
endif()

if(UNIX AND NOT APPLE)
    install(TARGETS chiriksip DESTINATION bin)
    install(FILES resources/chiriksip.desktop DESTINATION share/applications)
    install(FILES resources/icons/chiriksip.png DESTINATION share/icons/hicolor/256x256/apps)
    install(FILES resources/icons/128x128/chiriksip.png DESTINATION share/icons/hicolor/128x128/apps)
    install(FILES resources/icons/64x64/chiriksip.png DESTINATION share/icons/hicolor/64x64/apps)
    install(FILES resources/icons/48x48/chiriksip.png DESTINATION share/icons/hicolor/48x48/apps)
    install(FILES resources/icons/32x32/chiriksip.png DESTINATION share/icons/hicolor/32x32/apps)
    install(FILES resources/icons/16x16/chiriksip.png DESTINATION share/icons/hicolor/16x16/apps)
elseif(WIN32)
    install(TARGETS chiriksip DESTINATION .)
    install(FILES resources/icons/chiriksip.png DESTINATION .)
endif()
