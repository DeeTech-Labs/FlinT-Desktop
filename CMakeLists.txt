cmake_minimum_required(VERSION 3.21)

project(FlintDesktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 SerialPort)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(flint_desktop
    src/main.cpp
    src/device/DeviceSession.cpp
    src/localization/Localization.cpp
    src/localization/Localization.h
    src/device/DeviceSession.h
)

set(FLINT_QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/components/AppButton.qml
        qml/components/AppCard.qml
        qml/components/AppPage.qml
        qml/components/Caption.qml
        qml/components/LogView.qml
        qml/components/MetricCard.qml
        qml/components/NavigationButton.qml
        qml/components/AppComboBox.qml
        qml/features/device/ConnectionCard.qml
        qml/features/device/DeviceHero.qml
        qml/features/device/DeviceInfoCard.qml
        qml/features/device/DevicePage.qml
        qml/features/device/OverviewPage.qml
        qml/features/device/StatusCard.qml
        qml/features/diagnostics/DiagnosticsPage.qml
        qml/features/diagnostics/UsbLogCard.qml
        qml/models/DevicePresentation.qml
        qml/shell/AppNavigation.qml
        qml/shell/AppWindow.qml
)
# Flat resource aliases keep imports independent of the source folder layout.
foreach(qml_file IN LISTS FLINT_QML_FILES)
    get_filename_component(qml_name "${qml_file}" NAME)
    set_source_files_properties(${qml_file} PROPERTIES QT_RESOURCE_ALIAS "${qml_name}")
endforeach()
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(flint_desktop
    URI Flint.Desktop
    VERSION 1.0
    QML_FILES ${FLINT_QML_FILES}
)
file(GLOB_RECURSE FLINT_TRANSLATION_FILES CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} translations/*.json)
qt_add_resources(flint_desktop translations
    PREFIX "/"
    FILES ${FLINT_TRANSLATION_FILES}
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

qt_generate_deploy_qml_app_script(
    TARGET flint_desktop
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(flint_ui_tests
        tests/ui_smoke.cpp
        src/device/DeviceSession.cpp src/device/DeviceSession.h
        src/localization/Localization.cpp src/localization/Localization.h
    )
    target_include_directories(flint_ui_tests PRIVATE src)
    target_link_libraries(flint_ui_tests PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::SerialPort Qt6::Test)
    # Test the same QML and module manifest as the shipped executable.
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/Flint/Desktop/qmldir PROPERTIES QT_RESOURCE_ALIAS qmldir)
    qt_add_resources(flint_ui_tests test_ui
        PREFIX "/qt/qml/Flint/Desktop"
        FILES ${FLINT_QML_FILES} ${CMAKE_CURRENT_BINARY_DIR}/Flint/Desktop/qmldir
    )
    qt_add_resources(flint_ui_tests test_translations
        PREFIX "/" FILES ${FLINT_TRANSLATION_FILES}
    )
    add_test(NAME ui_smoke COMMAND flint_ui_tests)
    set_tests_properties(ui_smoke PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
endif()
