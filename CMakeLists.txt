cmake_minimum_required(VERSION 3.20)
project(ChirikSIP VERSION 1.0.1 LANGUAGES CXX)

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

add_executable(chiriksip ${APP_SOURCES})

find_package(Qt6 REQUIRED COMPONENTS Core Widgets DBus Multimedia)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PJSIP REQUIRED IMPORTED_TARGET libpjproject)
target_link_libraries(chiriksip PRIVATE
    Qt6::Core Qt6::Widgets Qt6::DBus Qt6::Multimedia
    PkgConfig::PJSIP
)
target_compile_options(chiriksip PRIVATE ${PJSIP_CFLAGS_OTHER})

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

install(TARGETS chiriksip DESTINATION bin)
install(FILES resources/chiriksip.desktop DESTINATION share/applications)
install(FILES resources/icons/chiriksip.png DESTINATION share/icons/hicolor/256x256/apps)
install(FILES resources/icons/128x128/chiriksip.png DESTINATION share/icons/hicolor/128x128/apps)
install(FILES resources/icons/64x64/chiriksip.png DESTINATION share/icons/hicolor/64x64/apps)
install(FILES resources/icons/48x48/chiriksip.png DESTINATION share/icons/hicolor/48x48/apps)
install(FILES resources/icons/32x32/chiriksip.png DESTINATION share/icons/hicolor/32x32/apps)
install(FILES resources/icons/16x16/chiriksip.png DESTINATION share/icons/hicolor/16x16/apps)

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
    Qt6::Core Qt6::Widgets Qt6::Test Qt6::Multimedia
    PkgConfig::PJSIP
)
target_include_directories(test_parsing PRIVATE src)
target_compile_definitions(test_parsing PRIVATE PROJECT_VERSION="${PROJECT_VERSION}")
add_test(NAME test_parsing COMMAND test_parsing)
