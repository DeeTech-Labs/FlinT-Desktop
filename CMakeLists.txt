cmake_minimum_required(VERSION 3.21)

project(FlintDesktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 SerialPort)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(flint_desktop
    src/main.cpp
    src/device/DeviceSession.cpp
    src/device/DeviceSession.h
)

set_source_files_properties(qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)
qt_add_qml_module(flint_desktop
    URI Flint.Desktop
    VERSION 1.0
    QML_FILES qml/Main.qml
)

target_link_libraries(flint_desktop PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::SerialPort)
target_compile_definitions(flint_desktop PRIVATE
    FLINT_DESKTOP_VERSION="${PROJECT_VERSION}"
)

set_target_properties(flint_desktop PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "labs.deetech.flint.desktop"
    MACOSX_BUNDLE_BUNDLE_NAME "FlinT Desktop"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)

include(GNUInstallDirs)
install(TARGETS flint_desktop
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
