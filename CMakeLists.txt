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

set(APPEARANCE_SDK_ROOT "" CACHE PATH "Path to Appearance SDK headers root (contains CIncludes and RIncludes)")

add_application(MacVox68
    about_box.c
    main.c
    main_window.c
    network.c
    network_backend_ot.c
    speech.c
    ui_input.c
    ui_layout.c
    ui_theme.c
    ui_text_fields.c
    ui_windows.c
    window_ui.c
    macvox68_icon.r
    TYPE APPL
    CREATOR MV68
    )

if(CMAKE_SYSTEM_NAME MATCHES Retro68)
    get_filename_component(_retro68_cmake_dir "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
    get_filename_component(_retro68_target_root "${_retro68_cmake_dir}/.." ABSOLUTE)
    get_filename_component(_retro68_root "${_retro68_target_root}/.." ABSOLUTE)

    set(_speech_import_dirs)

    # Allow the operator to override the Speech Manager import library location.
    if(SPEECH_IMPORT_LIB_DIR)
        list(APPEND _speech_import_dirs "${SPEECH_IMPORT_LIB_DIR}")
    endif()

    # Common Retro68 installation layouts.
    list(APPEND _speech_import_dirs
        "${_retro68_root}/ImportLibraries"
        "${_retro68_target_root}/ImportLibraries"
        "/opt/Retro68/ImportLibraries"
    )

    # MPW kits are sometimes unpacked into Interfaces&Libraries/Libraries
    # instead of ImportLibraries. Cover both spellings (with and without the
    # ampersand) and common subfolders, plus “Interfaces and Libraries” with
    # spaces.
    list(APPEND _speech_import_dirs
        "${_retro68_root}/Interfaces&Libraries/Libraries"
        "${_retro68_root}/Interfaces&Libraries/Libraries/SharedLibraries"
        "${_retro68_root}/InterfacesAndLibraries/Libraries"
        "${_retro68_root}/InterfacesAndLibraries/Libraries/SharedLibraries"
        "${_retro68_root}/Interfaces and Libraries/Libraries"
        "${_retro68_root}/Interfaces and Libraries/Libraries/Shared Libraries"
    )

    if(DEFINED ENV{RETRO68_ROOT} AND NOT "$ENV{RETRO68_ROOT}" STREQUAL "")
        list(APPEND _speech_import_dirs "$ENV{RETRO68_ROOT}/ImportLibraries")
        list(APPEND _speech_import_dirs
            "$ENV{RETRO68_ROOT}/Interfaces&Libraries/Libraries"
            "$ENV{RETRO68_ROOT}/Interfaces&Libraries/Libraries/SharedLibraries"
            "$ENV{RETRO68_ROOT}/InterfacesAndLibraries/Libraries"
            "$ENV{RETRO68_ROOT}/InterfacesAndLibraries/Libraries/SharedLibraries"
            "$ENV{RETRO68_ROOT}/Interfaces and Libraries/Libraries"
            "$ENV{RETRO68_ROOT}/Interfaces and Libraries/Libraries/Shared Libraries"
        )
    endif()

    list(REMOVE_DUPLICATES _speech_import_dirs)

    set(_speechlib_names SpeechLib Speech SpeechSynthesis SpeechLib68K)
    list(JOIN _speech_import_dirs "\n        " _speech_import_dirs_bulleted)
    list(JOIN _speechlib_names ", " _speechlib_names_joined)

    # Search common import library layouts for the classic 68K import archive.
    #
    # Classic MPW distributions sometimes name this import archive `SpeechSynthesis.a`
    # instead of `SpeechLib.a`, so include both families of names here.
    find_library(SPEECHLIB_LIBRARY
        NAMES ${_speechlib_names}
        HINTS ${_speech_import_dirs}
        PATH_SUFFIXES "" 68k 68K m68k powerpc PowerPC PPC Libraries Libraries/SharedLibraries
        NO_DEFAULT_PATH
    )

    if(NOT SPEECHLIB_LIBRARY)
        message(STATUS "Speech Manager import library search paths:\n        ${_speech_import_dirs_bulleted}")
        message(FATAL_ERROR "Speech Manager import library not found. Expected one of: ${_speechlib_names_joined}. Install Retro68/MPW Interfaces&Libraries/ImportLibraries that include the 68K SpeechLib/SpeechSynthesis import archive or set SPEECH_IMPORT_LIB_DIR to its location.")
    else()
        message(STATUS "Speech Manager import library: ${SPEECHLIB_LIBRARY}")
    endif()

    target_link_directories(MacVox68 PRIVATE ${_speech_import_dirs})
    target_link_libraries(MacVox68 -lOpenTransport -lOpenTransportApp -lOpenTptInet ${SPEECHLIB_LIBRARY})
endif()

if(APPEARANCE_SDK_ROOT)
    target_include_directories(MacVox68 PRIVATE
        "${APPEARANCE_SDK_ROOT}/CIncludes"
        "${APPEARANCE_SDK_ROOT}/RIncludes"
    )
endif()

# Enable -ffunction-sections and -gc-sections to make the app as small as possible
# On 68K, also enable --mac-single to build it as a single-segment app (so that this code path doesn't rot)
set_target_properties(MacVox68 PROPERTIES COMPILE_OPTIONS -ffunction-sections)
if(CMAKE_SYSTEM_NAME MATCHES Retro68)
    set_target_properties(MacVox68 PROPERTIES LINK_FLAGS "-Wl,-gc-sections -Wl,--mac-single")

else()
    set_target_properties(MacVox68 PROPERTIES LINK_FLAGS "-Wl,-gc-sections")
endif()
