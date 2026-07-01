cmake_minimum_required(VERSION 3.20)
project(ChirikSIP VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(APP_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/settingsdialog.cpp
    src/setupwizard.cpp
    src/sipclient.cpp
    src/callmanager.cpp
    src/audiobridge.cpp
    src/ringtone.cpp
    src/callnotification.cpp
    src/scrollhelper.cpp
)

if(WIN32)
    set(APP_SOURCES ${APP_SOURCES} windows/chiriksip.rc resources.qrc)
    add_executable(chiriksip WIN32 ${APP_SOURCES})
else()
    add_executable(chiriksip ${APP_SOURCES})
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

if(WIN32)
    add_definitions(-DPJ_NATIVE_STRING_IS_UNICODE=0)

    find_path(PJSIP_INCLUDE_DIR pjsua.h)

    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(PJ_SUFFIX "x86_64-w64-mingw32")
    else()
        set(PJ_SUFFIX "i686-w64-mingw32")
    endif()

    find_library(PJSUA_LIB NAMES pjsua pjsua-${PJ_SUFFIX})
    find_library(PJSIP_LIB NAMES pjsip pjsip-${PJ_SUFFIX})
    find_library(PJSIP_UA_LIB NAMES pjsip-ua pjsip-ua-${PJ_SUFFIX})
    find_library(PJSIP_SIMPLE_LIB NAMES pjsip-simple pjsip-simple-${PJ_SUFFIX})
    find_library(PJMEDIA_LIB NAMES pjmedia pjmedia-${PJ_SUFFIX})
    find_library(PJMEDIA_CODEC_LIB NAMES pjmedia-codec pjmedia-codec-${PJ_SUFFIX})
    find_library(PJMEDIA_AUDIODEV_LIB NAMES pjmedia-audiodev pjmedia-audiodev-${PJ_SUFFIX})
    find_library(PJ_LIB NAMES pj pj-${PJ_SUFFIX})
    find_library(PJNATH_LIB NAMES pjnath pjnath-${PJ_SUFFIX})
    find_library(PJLIB_UTIL_LIB NAMES pjlib-util pjlib-util-${PJ_SUFFIX})
    find_library(PA_LIB NAMES portaudio)

    target_include_directories(chiriksip PRIVATE ${PJSIP_INCLUDE_DIR})
    find_library(ILBC_LIB NAMES ilbccodec ilbccodec-${PJ_SUFFIX})
    find_library(GSM_LIB NAMES gsmcodec gsmcodec-${PJ_SUFFIX})
    find_library(SPEEX_LIB NAMES speex speex-${PJ_SUFFIX})
    find_library(G7221_LIB NAMES g7221codec g7221codec-${PJ_SUFFIX})
    find_library(SRTP_LIB NAMES srtp srtp-${PJ_SUFFIX})
    find_library(RESAMPLE_LIB NAMES resample resample-${PJ_SUFFIX})

    target_link_libraries(chiriksip PRIVATE
        Qt6::Core Qt6::Widgets
        -Wl,--start-group
        ${PJSUA_LIB} ${PJSIP_LIB} ${PJSIP_UA_LIB} ${PJSIP_SIMPLE_LIB}
        ${PJMEDIA_CODEC_LIB} ${PJMEDIA_AUDIODEV_LIB} ${PJMEDIA_LIB}
        ${PJ_LIB} ${PJNATH_LIB} ${PJLIB_UTIL_LIB}
        ${ILBC_LIB} ${GSM_LIB} ${SPEEX_LIB} ${G7221_LIB}
        ${SRTP_LIB} ${RESAMPLE_LIB}
        ${PA_LIB}
        -Wl,--end-group
        ws2_32 winmm ole32 uuid
    )
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PJSIP REQUIRED IMPORTED_TARGET libpjproject)
    pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
    target_link_libraries(chiriksip PRIVATE
        Qt6::Core Qt6::Widgets
        PkgConfig::PJSIP
        ${PORTAUDIO_LIBRARIES}
    )
    target_include_directories(chiriksip PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
    target_compile_options(chiriksip PRIVATE ${PJSIP_CFLAGS_OTHER} ${PORTAUDIO_CFLAGS_OTHER})
endif()

target_compile_definitions(chiriksip PRIVATE PROJECT_VERSION="${PROJECT_VERSION}")

# Debug sanitizers (optional — requires libasan/libubsan)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_LIBRARIES "-fsanitize=address")
    check_cxx_source_compiles("int main(){return 0;}" HAS_ASAN)
    if(HAS_ASAN)
        target_compile_options(chiriksip PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(chiriksip PRIVATE -fsanitize=address,undefined)
    else()
        message(WARNING "AddressSanitizer not available, skipping")
    endif()
    unset(CMAKE_REQUIRED_LIBRARIES)
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

# Tests
find_package(Qt6 REQUIRED COMPONENTS Test)
set(TEST_SOURCES
    tests/test_parsing.cpp
    src/mainwindow.cpp
    src/settingsdialog.cpp
    src/setupwizard.cpp
    src/sipclient.cpp
    src/callmanager.cpp
    src/audiobridge.cpp
    src/ringtone.cpp
    src/callnotification.cpp
    src/scrollhelper.cpp
)
add_executable(test_parsing ${TEST_SOURCES})
target_link_libraries(test_parsing PRIVATE
    Qt6::Core Qt6::Widgets Qt6::Test
    PkgConfig::PJSIP
    ${PORTAUDIO_LIBRARIES}
)
target_include_directories(test_parsing PRIVATE ${PORTAUDIO_INCLUDE_DIRS} src)
target_compile_definitions(test_parsing PRIVATE PROJECT_VERSION="${PROJECT_VERSION}")
add_test(NAME test_parsing COMMAND test_parsing)
