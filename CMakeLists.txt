# CineWindows - Video Player
# Copyright (c) 2026 Ritesh Pandit
#
# CineWindows Community License
#
# This source code is made available for personal, non-commercial
# use only. Organizations may not use, copy, modify, or distribute
# this code without written permission from Ritesh Pandit.
#
# See the LICENSE.md file for full license terms.
#
# Project: Cinewindows
# Author:  Ritesh Pandit
# Last modified: 2026-09-10
# Modified by: Ritesh Pandit
#
cmake_minimum_required(VERSION 3.25)

project(CineWindows VERSION 0.0.1 LANGUAGES CXX)

set(CINEWINDOWS_APP_ID "com.gyrolet.CineWindows")
set(CINEWINDOWS_DISPLAY_NAME "CineWindows" CACHE STRING "Application display name")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    add_compile_options($<$<CONFIG:Release>:/O2>)
else()
    add_compile_options($<$<CONFIG:Release>:-O3>)
endif()

option(CINEWINDOWS_FETCH_MPVQT "Fetch MpvQt v1.2.0 during configure if no package install is found" OFF)
option(CINEWINDOWS_ENABLE_IPO "Enable interprocedural optimization for release builds when supported" ON)
set(CINEWINDOWS_MPVQT_PREFIX
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/mpvqt-install"
    CACHE PATH
    "Local MpvQt install prefix used by scripts/setup/bootstrap_mpvqt_windows.ps1"
)
set(CINEWINDOWS_LIBMPV_PREFIX
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/libmpv-static"
    CACHE PATH
    "MSYS2 prefix containing libmpv headers and import library"
)
set(CINEWINDOWS_LIBMPV_RUNTIME
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/libmpv-static/bin/libmpv-2.dll"
    CACHE FILEPATH
    "Path to libmpv-2.dll for local Windows runs"
)

if(APPLE)
    set(CINEWINDOWS_MACOS_ICON
        "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/CineWindows.icns"
        CACHE FILEPATH
        "macOS application bundle icon"
    )
    set(CINEWINDOWS_MACOS_ARCHITECTURES
        ""
        CACHE STRING
        "Optional macOS architectures (for example arm64 or x86_64); defaults to the build host"
    )
    if(CINEWINDOWS_MACOS_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES
            "${CINEWINDOWS_MACOS_ARCHITECTURES}"
            CACHE STRING
            "macOS target architectures"
            FORCE
        )
    endif()
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS
    Concurrent
    Core
    Gui
    Network
    Qml
    Quick
    QuickControls2
    QuickDialogs2
    QuickWidgets
    Sql
    Svg
    Widgets
    LinguistTools
)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/qtlogger/src/qtlogger/CMakeLists.txt")
    message(FATAL_ERROR "QtLogger is missing. Run: git submodule update --init --recursive")
endif()
set(QTLOGGER_LIBRARY OFF CACHE BOOL "Build QtLogger as a static library" FORCE)
set(QTLOGGER_DEBUG_OUTPUT OFF CACHE BOOL "Disable QtLogger internal diagnostics" FORCE)
set(QTLOGGER_NO_THREAD OFF CACHE BOOL "Enable QtLogger thread-safe logging support" FORCE)
set(QTLOGGER_NETWORK OFF CACHE BOOL "Disable QtLogger remote logging sinks" FORCE)
set(QTLOGGER_JOURNAL OFF CACHE BOOL "Disable the optional systemd journal sink" FORCE)
add_subdirectory(third_party/qtlogger/src/qtlogger EXCLUDE_FROM_ALL)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Qt-Advanced-Docking-System/CMakeLists.txt")
    message(FATAL_ERROR "Qt Advanced Docking System is missing. Run: git submodule update --init --recursive")
endif()
block(SCOPE_FOR VARIABLES)
    set(BUILD_EXAMPLES OFF)
    set(BUILD_STATIC OFF)
    add_subdirectory(third_party/Qt-Advanced-Docking-System EXCLUDE_FROM_ALL)
endblock()

if(EXISTS "${CINEWINDOWS_MPVQT_PREFIX}/lib/cmake/MpvQt")
    list(PREPEND CMAKE_PREFIX_PATH "${CINEWINDOWS_MPVQT_PREFIX}")
endif()

# Provide our shim LibmpvConfig so MpvQt's find_package(Libmpv) succeeds
# on distros that ship libmpv-dev without a cmake config file.
list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(WIN32
        AND EXISTS "${CINEWINDOWS_LIBMPV_PREFIX}/include/mpv/client.h"
        AND EXISTS "${CINEWINDOWS_LIBMPV_PREFIX}/lib/libmpv.dll.a")
    set(Libmpv_INCLUDE_DIRS
        "${CINEWINDOWS_LIBMPV_PREFIX}/include"
        CACHE PATH
        "libmpv include root"
        FORCE
    )
    set(Libmpv_LIBRARIES
        "${CINEWINDOWS_LIBMPV_PREFIX}/lib/libmpv.dll.a"
        CACHE FILEPATH
        "libmpv import library"
        FORCE
    )
elseif(APPLE)
    find_library(Libmpv_LIBRARIES mpv)
    if(Libmpv_LIBRARIES)
        get_filename_component(Libmpv_LIB_DIR "${Libmpv_LIBRARIES}" DIRECTORY)
        get_filename_component(Libmpv_PREFIX "${Libmpv_LIB_DIR}/.." ABSOLUTE)
        set(Libmpv_INCLUDE_DIRS "${Libmpv_PREFIX}/include" CACHE PATH "libmpv include root" FORCE)
    endif()
elseif(UNIX)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(PC_LIBMPV libmpv)
        if(PC_LIBMPV_FOUND)
            set(Libmpv_INCLUDE_DIRS "${PC_LIBMPV_INCLUDE_DIRS}" CACHE PATH "libmpv include root" FORCE)
            set(Libmpv_LIBRARIES "${PC_LIBMPV_LIBRARIES}" CACHE STRING "libmpv libraries" FORCE)
        endif()
    endif()
    if(NOT Libmpv_LIBRARIES)
        find_library(Libmpv_LIBRARIES mpv)
        if(Libmpv_LIBRARIES)
            find_path(Libmpv_INCLUDE_DIRS mpv/client.h PATH_SUFFIXES include)
        endif()
    endif()
endif()

find_package(MpvQt 1.2.0 CONFIG QUIET)
if(NOT MpvQt_FOUND AND CINEWINDOWS_FETCH_MPVQT)
    add_subdirectory(third_party/mpvqt EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET MpvQt::MpvQt)
    if(WIN32)
        set(MpvQt_MSG
            "Run 'powershell -ExecutionPolicy Bypass -File scripts/setup/bootstrap_mpvqt_windows.ps1' "
            "to build it into ${CINEWINDOWS_MPVQT_PREFIX}"
        )
    elseif(APPLE)
        set(MpvQt_MSG
            "Run 'bash scripts/setup/bootstrap_macos.sh' to install dependencies, or set "
            "CINEWINDOWS_FETCH_MPVQT=ON to build MpvQt from third_party/mpvqt"
        )
    else()
        set(MpvQt_MSG
            "Run 'bash scripts/setup/bootstrap_linux.sh' to install dependencies, or set "
            "CINEWINDOWS_FETCH_MPVQT=ON to build MpvQt from third_party/mpvqt"
        )
    endif()
    message(FATAL_ERROR
        "MpvQt 1.2.0 was not found. ${MpvQt_MSG}, or set MpvQt_DIR to an existing "
        "MpvQtConfig.cmake directory."
    )
endif()

qt_standard_project_setup(
    REQUIRES 6.8
    I18N_SOURCE_LANGUAGE en
    I18N_TRANSLATED_LANGUAGES
        en
        af ar ast az
        be bg bn bs
        ca cs cy
        da de
        el es et eu
        fa fi fil fr
        ga gd gl gu
        he hi hr hu hy
        id is it
        ja
        ka kk km kn ko ky
        lt lv
        mi mk ml mn mr ms mt my
        nb ne nl nn
        pa pl ps pt
        ro ru
        si sk sl sq sr sv sw
        ta te tg th tk tl tr
        ug uk ur uz
        vi
        wa
        xh
        yi
        zh_CN zh_TW zu
)

set(CINEWINDOWS_QML_FILES
    qml/App.qml
    qml/controls/CineButton.qml
    qml/controls/CineIcon.qml
    qml/controls/CineTooltip.qml
    qml/controls/CineSpinner.qml
    qml/controls/CineSwitch.qml
    qml/controls/CineRangeSlider.qml
    qml/controls/LazyPopupLoader.qml
    qml/controls/ResponsivePopup.qml
    qml/controls/CineMenu.qml
    qml/controls/CineMenuItem.qml
    qml/controls/TrackSelectionMenu.qml
    qml/controls/VolumePopup.qml
    qml/controls/CineSeekbar.qml
    qml/controls/ControlBar.qml
    qml/controls/HeaderBar.qml
    qml/dialogs/AboutDialog.qml
    qml/dialogs/ShortcutsDialog.qml
    qml/dialogs/SubtitleSearchDialog.qml
    qml/dialogs/RemoteControlDialog.qml
    qml/dialogs/CastDialog.qml
    qml/dialogs/UrlDialog.qml
    qml/dialogs/YouTubeSearchDialog.qml
    qml/dialogs/FontPickerDialog.qml
    qml/dialogs/SwatchColorDialog.qml
    qml/dialogs/HsvColorDialog.qml
    qml/input/KeyBindings.qml
    qml/library/MediaHub.qml
    qml/overlays/AudioOnlyIndicator.qml
    qml/overlays/ChromeGradient.qml
    qml/overlays/DropOverlay.qml
    qml/overlays/OsdDisplay.qml
    qml/overlays/Toast.qml
    qml/overlays/VolumeOsd.qml
    qml/playlist/PlaylistDrawer.qml
    qml/preferences/PreferencesDialog.qml
    qml/preferences/EqualizerDialog.qml
    qml/preferences/OptionsDialog.qml
    qml/style/Theme.qml
    qml/style/ViewportMetrics.qml
    qml/utils/TimeUtils.qml
    qml/window/ResizeHandles.qml
    qml/window/WindowSurface.qml
    qml/workspace/DockedVideo.qml
)

set_source_files_properties(qml/style/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/input/KeyBindings.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/utils/TimeUtils.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

set(CINEWINDOWS_SOURCES
    src/app/ApplicationLog.cpp src/app/ApplicationLog.h
    src/app/LoggingCategories.cpp src/app/LoggingCategories.h
    src/app/SettingsManager.cpp src/app/SettingsManager.h
    src/app/StartupShell.cpp src/app/StartupShell.h
    src/controls/QrCodeItem.cpp src/controls/QrCodeItem.h
    src/models/ChapterModel.cpp src/models/ChapterModel.h
    src/models/FileBrowserModel.cpp src/models/FileBrowserModel.h
    src/models/PlaylistModel.cpp src/models/PlaylistModel.h
    src/models/TrackModel.cpp src/models/TrackModel.h
    src/player/CineMpvItem.cpp src/player/CineMpvItem.h
    src/player/AdvancedPlaybackController.cpp src/player/AdvancedPlaybackController.h
    src/player/InputRouter.cpp src/player/InputRouter.h
    src/player/IpcProtocol.cpp src/player/IpcProtocol.h
    src/player/IpcServer.cpp src/player/IpcServer.h
    src/player/MpvConfig.cpp src/player/MpvConfig.h
    src/player/MpvConfigFilter.cpp src/player/MpvConfigFilter.h
    src/player/PlaybackController.cpp src/player/PlaybackController.h
    src/player/ThumbnailController.cpp src/player/ThumbnailController.h
    src/player/VideoOptionsController.cpp src/player/VideoOptionsController.h
    src/services/FileService.cpp src/services/FileService.h
    src/services/LibraryDatabase.cpp src/services/LibraryDatabase.h
    src/services/MediaLibraryService.cpp src/services/MediaLibraryService.h
    src/services/MediaThumbnailService.cpp src/services/MediaThumbnailService.h
    src/services/RemoteControlAssets.cpp src/services/RemoteControlAssets.h
    src/services/RemoteControlService.cpp src/services/RemoteControlService.h
    src/services/CastService.cpp src/services/CastService.h
    src/services/SessionManager.cpp src/services/SessionManager.h
    src/services/SubtitleService.cpp src/services/SubtitleService.h
    src/services/TmdbMetadataService.cpp src/services/TmdbMetadataService.h
    src/services/UpdateService.cpp src/services/UpdateService.h
    src/services/YouTubeSearchService.cpp src/services/YouTubeSearchService.h
    src/utils/MediaUtils.cpp src/utils/MediaUtils.h
    src/utils/PathUtils.cpp src/utils/PathUtils.h
    src/window/WindowCornerPreference.cpp src/window/WindowCornerPreference.h
    src/workspace/WorkspaceController.cpp src/workspace/WorkspaceController.h
    third_party/QR-Code-generator/cpp/qrcodegen.cpp third_party/QR-Code-generator/cpp/qrcodegen.hpp
)

# Keep strict warnings actionable for CineWindows-owned code. The pinned
# third-party QR implementation is built unmodified under its own warning policy.
if(MSVC)
    set_source_files_properties(third_party/QR-Code-generator/cpp/qrcodegen.cpp PROPERTIES COMPILE_OPTIONS "/w")
else()
    set_source_files_properties(third_party/QR-Code-generator/cpp/qrcodegen.cpp PROPERTIES COMPILE_OPTIONS "-w")
endif()

set(CINEWINDOWS_WINDOWS_RC "")
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/windows/CineWindows.rc")
    set(CINEWINDOWS_WINDOWS_RC resources/windows/CineWindows.rc)
endif()

if(APPLE AND EXISTS "${CINEWINDOWS_MACOS_ICON}")
    set_source_files_properties("${CINEWINDOWS_MACOS_ICON}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
    set(CINEWINDOWS_MACOS_ICON_FILE "${CINEWINDOWS_MACOS_ICON}")
else()
    set(CINEWINDOWS_MACOS_ICON_FILE "")
endif()

qt_add_executable(CineWindows
    src/main.cpp
    resources/cinewindows.qrc
    ${CINEWINDOWS_WINDOWS_RC}
    ${CINEWINDOWS_MACOS_ICON_FILE}
)

qt_add_qml_module(CineWindows
    URI CineWindows
    VERSION 1.0
    QML_FILES ${CINEWINDOWS_QML_FILES}
    SOURCES ${CINEWINDOWS_SOURCES}
)

qt_add_translations(CineWindows
    TS_FILE_BASE CineWindows
    TS_FILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/i18n"
    RESOURCE_PREFIX "/i18n"
)

target_include_directories(CineWindows PRIVATE
    src
    src/app
    src/controls
    src/models
    src/player
    src/services
    src/utils
    src/window
    src/workspace
    third_party/QR-Code-generator/cpp
    ${Libmpv_INCLUDE_DIRS}
)

target_link_libraries(CineWindows PRIVATE
    ads::qtadvanceddocking-qt6
    qtlogger::qtlogger
    Qt6::Concurrent
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickDialogs2
    Qt6::QuickWidgets
    Qt6::Sql
    Qt6::Svg
    Qt6::Widgets
    MpvQt::MpvQt
    ${Libmpv_LIBRARIES}
)

if(WIN32)
    target_link_libraries(CineWindows PRIVATE dwmapi)
endif()

target_compile_definitions(CineWindows PRIVATE
    APP_ID="${CINEWINDOWS_APP_ID}"
    APP_DISPLAY_NAME="${CINEWINDOWS_DISPLAY_NAME}"
    APP_VERSION="${PROJECT_VERSION}"
    QT_NO_KEYWORDS
)

if(MSVC)
    target_compile_options(CineWindows PRIVATE /utf-8)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    target_compile_options(CineWindows PRIVATE
        -finput-charset=UTF-8
        $<$<CONFIG:Release>:-DNDEBUG>
        $<$<CONFIG:Release>:-ffunction-sections>
        $<$<CONFIG:Release>:-fdata-sections>
        $<$<CONFIG:MinSizeRel>:-ffunction-sections>
        $<$<CONFIG:MinSizeRel>:-fdata-sections>
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Wmisleading-indentation
        -Wno-null-dereference
        -Wno-unknown-pragmas
    )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(CineWindows PRIVATE
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wuseless-cast
        -Wno-useless-cast
    )
endif()

if(CINEWINDOWS_ENABLE_IPO AND NOT MINGW)
    include(CheckIPOSupported)
    check_ipo_supported(
        RESULT CINEWINDOWS_IPO_SUPPORTED
        OUTPUT CINEWINDOWS_IPO_OUTPUT
        LANGUAGES CXX
    )
    if(CINEWINDOWS_IPO_SUPPORTED)
        set_property(TARGET CineWindows PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        set_property(TARGET CineWindows PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
    else()
        message(STATUS "IPO/LTO not enabled: ${CINEWINDOWS_IPO_OUTPUT}")
    endif()
elseif(CINEWINDOWS_ENABLE_IPO AND MINGW)
    message(STATUS "IPO/LTO disabled for MinGW builds; keeping section-gc/strip release optimizations")
endif()

if(MSVC)
    target_link_options(CineWindows PRIVATE
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
        $<$<CONFIG:MinSizeRel>:/OPT:REF>
        $<$<CONFIG:MinSizeRel>:/OPT:ICF>
    )
elseif(APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_link_options(CineWindows PRIVATE
        $<$<CONFIG:Release>:-Wl,-dead_strip>
        $<$<CONFIG:MinSizeRel>:-Wl,-dead_strip>
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
    target_link_options(CineWindows PRIVATE
        $<$<CONFIG:Release>:-Wl,--gc-sections>
        $<$<CONFIG:Release>:-s>
        $<$<CONFIG:MinSizeRel>:-Wl,--gc-sections>
        $<$<CONFIG:MinSizeRel>:-s>
    )
endif()

set_target_properties(CineWindows PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    WIN32_EXECUTABLE "$<NOT:$<CONFIG:Debug>>"
)
if(MINGW)
    set_property(TARGET CineWindows PROPERTY QT_NO_ENTRYPOINT TRUE)
endif()
if(APPLE)
    set_target_properties(CineWindows PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.gyrolet.CineWindows"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
    if(CINEWINDOWS_MACOS_ICON_FILE)
        set_target_properties(CineWindows PROPERTIES MACOSX_BUNDLE_ICON_FILE CineWindows.icns)
    endif()
endif()

if(WIN32)
    add_custom_command(TARGET CineWindows POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:qtadvanceddocking-qt6>"
            "$<TARGET_FILE_DIR:CineWindows>"
    )
    if(EXISTS "${CINEWINDOWS_MPVQT_PREFIX}/bin/libMpvQt.dll")
        add_custom_command(TARGET CineWindows POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CINEWINDOWS_MPVQT_PREFIX}/bin/libMpvQt.dll"
                "$<TARGET_FILE_DIR:CineWindows>"
        )
    endif()
    if(EXISTS "${CINEWINDOWS_LIBMPV_RUNTIME}")
        add_custom_command(TARGET CineWindows POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CINEWINDOWS_LIBMPV_RUNTIME}"
                "$<TARGET_FILE_DIR:CineWindows>"
        )
    endif()
endif()

include(GNUInstallDirs)
install(TARGETS CineWindows
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(UNIX AND NOT APPLE)
    set_property(TARGET CineWindows APPEND PROPERTY INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
    install(TARGETS qtadvanceddocking-qt6
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(FILES packaging/linux/CineWindows.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
        RENAME ${CINEWINDOWS_APP_ID}.desktop
    )
    install(FILES resources/icons/apps/CineWindows.svg
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
        RENAME ${CINEWINDOWS_APP_ID}.svg
    )
    install(FILES resources/icons/apps/CineWindows.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps
        RENAME ${CINEWINDOWS_APP_ID}.png
    )
    install(FILES third_party/Qt-Advanced-Docking-System/LICENSE
        third_party/Qt-Advanced-Docking-System/gnu-lgpl-v2.1.md
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/licenses/${CINEWINDOWS_APP_ID}/Qt-Advanced-Docking-System
    )
    install(FILES packaging/linux/${CINEWINDOWS_APP_ID}.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
    )
    install(FILES LICENSE.md THIRD_PARTY_NOTICES.md
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/licenses/${CINEWINDOWS_APP_ID}
    )
endif()

if(APPLE)
    qt_generate_deploy_qml_app_script(
        TARGET CineWindows
        OUTPUT_SCRIPT deploy_script
        MACOS_BUNDLE_POST_BUILD
        NO_UNSUPPORTED_PLATFORM_ERROR
        DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
    )
    install(SCRIPT ${deploy_script})
endif()

