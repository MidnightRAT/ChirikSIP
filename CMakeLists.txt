cmake_minimum_required(VERSION 3.20)
project(ChirikSIP VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PJSIP REQUIRED IMPORTED_TARGET libpjproject)

add_executable(chiriksip
    src/main.cpp
    src/mainwindow.cpp
    src/settingsdialog.cpp
    src/sipclient.cpp
    src/audiobridge.cpp
    src/ringtone.cpp
)

target_include_directories(chiriksip PRIVATE ${PJSIP_INCLUDE_DIRS})
target_link_libraries(chiriksip PRIVATE
    Qt6::Core
    Qt6::Widgets
    ${PJSIP_LIBRARIES}
    portaudio
)

target_compile_options(chiriksip PRIVATE ${PJSIP_CFLAGS_OTHER})

install(TARGETS chiriksip DESTINATION bin)
install(FILES resources/chiriksip.desktop DESTINATION share/applications)
install(FILES resources/icons/chiriksip.png DESTINATION share/icons/hicolor/256x256/apps)
install(FILES resources/icons/128x128/chiriksip.png DESTINATION share/icons/hicolor/128x128/apps)
install(FILES resources/icons/64x64/chiriksip.png DESTINATION share/icons/hicolor/64x64/apps)
install(FILES resources/icons/48x48/chiriksip.png DESTINATION share/icons/hicolor/48x48/apps)
install(FILES resources/icons/32x32/chiriksip.png DESTINATION share/icons/hicolor/32x32/apps)
install(FILES resources/icons/16x16/chiriksip.png DESTINATION share/icons/hicolor/16x16/apps)
