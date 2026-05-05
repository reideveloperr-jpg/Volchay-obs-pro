cmake_minimum_required(VERSION 3.16)

project(lumen-stream
    VERSION 0.1.0
    DESCRIPTION "Modern C++ Twitch streaming client"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Qt6 6.2 COMPONENTS Widgets Network REQUIRED)

# qt_standard_project_setup() is Qt 6.3+; emulate the bits we need so we
# stay buildable on Ubuntu 22.04's Qt 6.2.
if(COMMAND qt_standard_project_setup)
    qt_standard_project_setup()
endif()

set(LUMEN_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/SettingsDialog.cpp
    src/SettingsDialog.h
    src/PresetManager.cpp
    src/PresetManager.h
    src/StreamEngine.cpp
    src/StreamEngine.h
    src/ThemeManager.cpp
    src/ThemeManager.h
    src/PreviewWidget.cpp
    src/PreviewWidget.h
    src/AccentBadge.cpp
    src/AccentBadge.h
    src/Devices.cpp
    src/Devices.h
    src/Source.cpp
    src/Source.h
    src/SourcesPanel.cpp
    src/SourcesPanel.h
    src/SourceConfigDialog.cpp
    src/SourceConfigDialog.h
)

qt_add_executable(lumen-stream WIN32 MACOSX_BUNDLE ${LUMEN_SOURCES})

qt_add_resources(lumen-stream "lumen_resources"
    PREFIX "/"
    FILES
        resources/themes/light.qss
        resources/themes/blackout.qss
        resources/themes/rgb.qss
        resources/icons/logo.svg
        resources/icons/stream.svg
        resources/icons/settings.svg
        resources/icons/info.svg
        resources/icons/play.svg
        resources/icons/stop.svg
)

target_include_directories(lumen-stream PRIVATE src)

target_link_libraries(lumen-stream PRIVATE
    Qt6::Widgets
    Qt6::Network
)

if(WIN32)
    set_target_properties(lumen-stream PROPERTIES
        WIN32_EXECUTABLE TRUE
        OUTPUT_NAME "LumenStream"
    )
endif()

install(TARGETS lumen-stream
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION bin
)
