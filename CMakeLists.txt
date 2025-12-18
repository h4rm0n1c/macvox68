#    Copyright 2015 Wolfgang Thaller.
#
#    This file is part of Retro68.
#
#    Retro68 is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    Retro68 is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with Retro68.  If not, see <http://www.gnu.org/licenses/>.

# To use this example as a standalone project using CMake:
# mkdir build
# cd build
# cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
# make

cmake_minimum_required(VERSION 3.16)
project(MacVox68 C)

# Allow overriding the icon Rez path (defaults to macvox68_icon.r in the source root).
set(_default_icon_rez "${CMAKE_CURRENT_SOURCE_DIR}/macvox68_icon.r")
set(_legacy_icon_rez "${CMAKE_CURRENT_SOURCE_DIR}/docs/macvox68_icon.r")
set(MACVOX68_ICON_REZ "" CACHE FILEPATH "Path to icon Rez source (defaults to macvox68_icon.r next to the sources)")

if(NOT MACVOX68_ICON_REZ)
    if(EXISTS "${_default_icon_rez}")
        set(MACVOX68_ICON_REZ "${_default_icon_rez}")
    elseif(EXISTS "${_legacy_icon_rez}")
        message(STATUS "Using legacy icon Rez at ${_legacy_icon_rez}; prefer macvox68_icon.r in project root")
        set(MACVOX68_ICON_REZ "${_legacy_icon_rez}")
    else()
        set(MACVOX68_ICON_REZ "${_default_icon_rez}")
    endif()
endif()

set(MACVOX68_ICON_SOURCES)
if(EXISTS "${MACVOX68_ICON_REZ}")
    list(APPEND MACVOX68_ICON_SOURCES "${MACVOX68_ICON_REZ}")
    message(STATUS "Including icon resources from ${MACVOX68_ICON_REZ}")
else()
    message(STATUS "No icon Rez found (expected at ${MACVOX68_ICON_REZ} by default); building without custom icon")
endif()

add_application(MacVox68
    main.c
    main_window.c
    window_ui.c
    ${MACVOX68_ICON_SOURCES}
    TYPE APPL
    CREATOR MV68
    )

# Enable -ffunction-sections and -gc-sections to make the app as small as possible
# On 68K, also enable --mac-single to build it as a single-segment app (so that this code path doesn't rot)
set_target_properties(MacVox68 PROPERTIES COMPILE_OPTIONS -ffunction-sections)
if(CMAKE_SYSTEM_NAME MATCHES Retro68)
    set_target_properties(MacVox68 PROPERTIES LINK_FLAGS "-Wl,-gc-sections -Wl,--mac-single")

else()
    set_target_properties(MacVox68 PROPERTIES LINK_FLAGS "-Wl,-gc-sections")
endif()
