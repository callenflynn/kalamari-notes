cmake_minimum_required(VERSION 3.28)
project(Kalamari VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# VS 2026: cmake -S . -B build -G "Visual Studio 18 2026" -A x64

# ---------------------------------------------------------------------------
# SDL 3
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.4.12
    GIT_SHALLOW    TRUE
)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST   OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

# ---------------------------------------------------------------------------
# Dear ImGui
# ---------------------------------------------------------------------------
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.92.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

# Build ImGui as a static library
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC SDL3::SDL3)

# ---------------------------------------------------------------------------
# Sentry Native SDK (crash monitoring)
# ---------------------------------------------------------------------------
FetchContent_Declare(
    sentry
    GIT_REPOSITORY https://github.com/getsentry/sentry-native.git
    GIT_TAG        0.15.3
    GIT_SHALLOW    TRUE
)
set(SENTRY_BACKEND "crashpad" CACHE STRING "Sentry crash backend")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SENTRY_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SENTRY_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(sentry)

# ---------------------------------------------------------------------------
# Kalamari executable
# ---------------------------------------------------------------------------
add_executable(Kalamari WIN32 MACOSX_BUNDLE
    src/main.cpp
)
target_link_libraries(Kalamari PRIVATE SDL3::SDL3 imgui_lib sentry::sentry)

# ---------------------------------------------------------------------------
# Copy assets next to the executable (for development / running from build)
# ---------------------------------------------------------------------------
add_custom_command(TARGET Kalamari POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/assets
        $<TARGET_FILE_DIR:Kalamari>/assets
    COMMENT "Copying assets/ to output directory"
)

# ---------------------------------------------------------------------------
# Platform-specific flags
# ---------------------------------------------------------------------------
if(WIN32)
    # Copy SDL3.dll and crashpad_handler.exe next to the executable
    add_custom_command(TARGET Kalamari POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:Kalamari>
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:sentry::sentry>
            $<TARGET_FILE_DIR:Kalamari>
        COMMENT "Copying DLLs to output directory"
    )
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(Kalamari PRIVATE stdc++fs)
endif()

if(APPLE)
    set_target_properties(Kalamari PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "Kalamari"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.kalamari.notes"
        MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2026 Kalamari"
    )
endif()

# ===========================================================================
# Install rules (used by CPack)
# ===========================================================================
include(GNUInstallDirs)

install(TARGETS Kalamari
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)

# Install assets alongside the executable
install(DIRECTORY src/assets/
    DESTINATION ${CMAKE_INSTALL_BINDIR}/assets
)

# ===========================================================================
# CPack Configuration
# ===========================================================================
set(CPACK_PACKAGE_NAME "Kalamari")
set(CPACK_PACKAGE_VENDOR "Kalamari")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Kalamari - Ink your ideas")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/src/main.cpp")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

# ---------------------------------------------------------------------------
# Windows: WiX MSI Installer
# ---------------------------------------------------------------------------
if(WIN32)
    set(CPACK_GENERATOR "WIX;ZIP")

    # WiX-specific settings
    if(EXISTS "${CMAKE_SOURCE_DIR}/src/assets/kalamari.ico")
        set(CPACK_WIX_PRODUCT_ICON "${CMAKE_SOURCE_DIR}/src/assets/kalamari.ico")
    endif()

    # Installer banner image (493x58 pixels)
    if(EXISTS "${CMAKE_SOURCE_DIR}/src/assets/kalamari_banner.bmp")
        set(CPACK_WIX_UI_BANNER "${CMAKE_SOURCE_DIR}/src/assets/kalamari_banner.bmp")
    endif()

    set(CPACK_WIX_UPGRADE_GUID "A1B2C3D4-E5F6-7890-ABCD-EF1234567890")
    set(CPACK_WIX_PROGRAM_MENU_FOLDER "Kalamari")
    set(CPACK_WIX_INSTALL_DIR "Kalamari")

    # Add SDL3 DLL as a component
    set(CPACK_COMPONENT_KALAMARI_DESCRIPTION "Kalamari Notes Application")
    set(CPACK_COMPONENT_KALAMARI_DISPLAY_NAME "Kalamari")

# ---------------------------------------------------------------------------
# macOS: DMG Installer
# ---------------------------------------------------------------------------
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;ZIP")
    set(CPACK_DMG_VOLUME_NAME "Kalamari")
    set(CPACK_DMG_FORMAT "UDBZ")

# ---------------------------------------------------------------------------
# Linux: DEB + RPM + TAR.GZ
# ---------------------------------------------------------------------------
elseif(UNIX)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")

    # DEB settings
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Kalamari Team")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl3-0")

    # RPM settings
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Editors")
    set(CPACK_RPM_PACKAGE_REQUIRES "SDL3 >= 3.0")
endif()

# Common CPack settings
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_EXECUTABLES "Kalamari" "Kalamari")
set(CPACK_STRIP_FILES TRUE)

include(CPack)
