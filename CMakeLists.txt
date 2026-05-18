cmake_minimum_required(VERSION 3.18)
project(Donut VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#  Linux: Auto-install xterm and create system user

if(UNIX AND NOT APPLE)

    find_program(XTERM_BIN xterm)

    if(NOT XTERM_BIN)
        message(STATUS "[Donut] xterm not found — attempting to install via apt-get...")

        # Detect the available package manager
        find_program(APT_BIN   apt-get)
        find_program(DNF_BIN   dnf)
        find_program(ZYPPER_BIN zypper)
        find_program(PACMAN_BIN pacman)

        if(APT_BIN)
            execute_process(
                COMMAND sudo apt-get install -y xterm
                RESULT_VARIABLE XTERM_INSTALL_RESULT
                OUTPUT_QUIET
            )
        elseif(DNF_BIN)
            execute_process(
                COMMAND sudo dnf install -y xterm
                RESULT_VARIABLE XTERM_INSTALL_RESULT
                OUTPUT_QUIET
            )
        elseif(ZYPPER_BIN)
            execute_process(
                COMMAND sudo zypper install -y xterm
                RESULT_VARIABLE XTERM_INSTALL_RESULT
                OUTPUT_QUIET
            )
        elseif(PACMAN_BIN)
            execute_process(
                COMMAND sudo pacman -S --noconfirm xterm
                RESULT_VARIABLE XTERM_INSTALL_RESULT
                OUTPUT_QUIET
            )
        else()
            message(WARNING "[Donut] No supported package manager found. "
                            "Please install xterm manually.")
            set(XTERM_INSTALL_RESULT 1)
        endif()

        if(XTERM_INSTALL_RESULT EQUAL 0)
            message(STATUS "[Donut] xterm installed successfully.")
        else()
            message(WARNING "[Donut] xterm installation failed (exit code: "
                            "${XTERM_INSTALL_RESULT}). "
                            "The application may not function correctly without it.")
        endif()

    else()
        message(STATUS "[Donut] xterm found at: ${XTERM_BIN}")
    endif()

    # Create system user 
    set(DONUT_SYSTEM_USER "donut"
        CACHE STRING "System user account created for the Donut service")

    # Check whether the user already exists (getent returns 0 if found)
    execute_process(
        COMMAND getent passwd ${DONUT_SYSTEM_USER}
        RESULT_VARIABLE USER_EXISTS
        OUTPUT_QUIET
        ERROR_QUIET
    )

    if(NOT USER_EXISTS EQUAL 0)
        message(STATUS "[Donut] Creating system user '${DONUT_SYSTEM_USER}'...")

        execute_process(
            COMMAND sudo useradd
                        --system                         # no login shell / no aging
                        --no-create-home                 # service account — no home dir
                        --shell /usr/sbin/nologin        # prevent interactive login
                        --comment "Donut service account"
                        ${DONUT_SYSTEM_USER}
            RESULT_VARIABLE USER_CREATE_RESULT
            OUTPUT_QUIET
            ERROR_QUIET
        )

        if(USER_CREATE_RESULT EQUAL 0)
            message(STATUS "[Donut] System user '${DONUT_SYSTEM_USER}' created.")
        else()
            message(WARNING "[Donut] Failed to create system user '${DONUT_SYSTEM_USER}' "
                            "(exit code: ${USER_CREATE_RESULT}). "
                            "Run: sudo useradd --system --no-create-home "
                            "--shell /usr/sbin/nologin ${DONUT_SYSTEM_USER}")
        endif()

    else()
        message(STATUS "[Donut] System user '${DONUT_SYSTEM_USER}' already exists.")
    endif()

endif() 

#  DonutMath

add_library(DonutMath SHARED
    DonutMath.cpp
    DonutMath.h
)

target_include_directories(DonutMath PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(DonutMath PRIVATE
    $<$<PLATFORM_ID:Windows>:DONUTMATH_EXPORTS UNICODE _UNICODE>
    $<$<NOT:$<PLATFORM_ID:Windows>>:BUILD_LIBTYPE_SHARED>
)

set_target_properties(DonutMath PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

#  DonutAPI
add_library(DonutAPI SHARED
    DonutAPI.cpp
    DonutAPI.h
)

target_link_libraries(DonutAPI PRIVATE DonutMath)

target_include_directories(DonutAPI PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(DonutAPI PRIVATE
    $<$<PLATFORM_ID:Windows>:DONUTAPI_EXPORTS UNICODE _UNICODE>
    $<$<NOT:$<PLATFORM_ID:Windows>>:BUILD_LIBTYPE_SHARED>
)

#  Platform-specific audio and system libs

if(WIN32)
    # WASAPI + Win32 console APIs
    target_link_libraries(DonutAPI PRIVATE
        ole32        # CoInitializeEx, CoCreateInstance
        mmdevapi     # IMMDeviceEnumerator
    )

elseif(APPLE)
    # AudioToolbox framework for AudioQueue
    find_library(AUDIOTOOLBOX_LIB AudioToolbox REQUIRED)
    target_link_libraries(DonutAPI PRIVATE
        ${AUDIOTOOLBOX_LIB}
        pthread
    )

elseif(UNIX)
    # ALSA for audio on Linux
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ALSA REQUIRED alsa)

    target_include_directories(DonutAPI PRIVATE ${ALSA_INCLUDE_DIRS})
    target_link_libraries(DonutAPI PRIVATE
        ${ALSA_LIBRARIES}
        util        # openpty / login_tty (pty.h) — part of libutil on older distros, libc on newer ones
        pthread
    )
endif()

#  Fix output names and properties
set_target_properties(DonutMath PROPERTIES
    OUTPUT_NAME "DonutMath"
    DEBUG_POSTFIX "_d"
)

set_target_properties(DonutAPI PROPERTIES
    OUTPUT_NAME "DonutAPI"
    DEBUG_POSTFIX "_d"
)

#  Language Bindings

#  13 languages are handled by SWIG (DonutAPI.i):
#      Python, Java, JavaScript, Octave, Ruby, Perl, Lua, PHP, Go, C#, R, D

#  4 languages use the plain-C wrapper (DonutAPI_C.h / .cpp) because
#  they cannot consume C++ types via SWIG:
#      C, Swift, Rust, Zig

#  Enable individual languages at configure time, e.g.:
#      cmake -DBIND_PYTHON=ON -DBIND_JAVA=ON ..

#  All binding options default to OFF so a vanilla build is unaffected.

# Master toggle to enable all language bindings at once
option(BIND_ALL "Build all language bindings (Python, Java, JS, etc.)" OFF)

# Per-language opt-in flags
option(BIND_PYTHON      "Build Python bindings (SWIG)"      OFF)
option(BIND_JAVA        "Build Java bindings (SWIG)"        OFF)
option(BIND_JAVASCRIPT  "Build Node.js/JS bindings (SWIG)"  OFF)
option(BIND_OCTAVE      "Build Octave bindings (SWIG)"      OFF)
option(BIND_RUBY        "Build Ruby bindings (SWIG)"        OFF)
option(BIND_PERL        "Build Perl bindings (SWIG)"        OFF)
option(BIND_LUA         "Build Lua bindings (SWIG)"         OFF)
option(BIND_PHP         "Build PHP bindings (SWIG)"         OFF)
option(BIND_GO          "Build Go bindings (SWIG)"          OFF)
option(BIND_CSHARP      "Build C# bindings (SWIG)"          OFF)
option(BIND_R           "Build R bindings (SWIG)"           OFF)
option(BIND_D           "Build D bindings (SWIG)"           OFF)

# C / Swift / Rust / Zig all share the same C wrapper library
option(BUILD_CAPI       "Build C-compatible wrapper (C · Swift · Rust · Zig)" OFF)

# If BIND_ALL is enabled, force all individual bindings to ON
if(BIND_ALL)
    set(BIND_PYTHON ON CACHE BOOL "" FORCE)
    set(BIND_JAVA ON CACHE BOOL "" FORCE)
    set(BIND_JAVASCRIPT ON CACHE BOOL "" FORCE)
    set(BIND_OCTAVE ON CACHE BOOL "" FORCE)
    set(BIND_RUBY ON CACHE BOOL "" FORCE)
    set(BIND_PERL ON CACHE BOOL "" FORCE)
    set(BIND_LUA ON CACHE BOOL "" FORCE)
    set(BIND_PHP ON CACHE BOOL "" FORCE)
    set(BIND_GO ON CACHE BOOL "" FORCE)
    set(BIND_CSHARP ON CACHE BOOL "" FORCE)
    set(BIND_R ON CACHE BOOL "" FORCE)
    set(BIND_D ON CACHE BOOL "" FORCE)
    set(BUILD_CAPI ON CACHE BOOL "" FORCE)
endif()

# C wrapper library (DonutAPI_C) 
#   C to link against DonutAPI_C and #include "DonutAPI_C.h"
#   Swift to add DonutAPI_C to the bridging header; no SWIG needed
#   Rust tp point bindgen at DonutAPI_C.h in build.rs; link DonutAPI_C
#   Zig tp @cImport(@cInclude("DonutAPI_C.h")) and link DonutAPI_C
if(BUILD_CAPI)
    add_library(DonutAPI_C SHARED
        DonutAPI_C.cpp
        DonutAPI_C.h
    )
    target_include_directories(DonutAPI_C PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(DonutAPI_C PRIVATE DonutAPI)
    target_compile_definitions(DonutAPI_C PRIVATE
        $<$<PLATFORM_ID:Windows>:DONUTCAPI_EXPORTS UNICODE _UNICODE>
        $<$<NOT:$<PLATFORM_ID:Windows>>:BUILD_LIBTYPE_SHARED>
    )
    set_target_properties(DonutAPI_C PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        DEBUG_POSTFIX "_d"
    )
    message(STATUS "[Donut] C API wrapper enabled (DonutAPI_C)")
endif()

# SWIG setup
# Check whether any SWIG language was requested.
set(_SWIG_NEEDED FALSE)
foreach(_lang PYTHON JAVA JAVASCRIPT OCTAVE RUBY PERL LUA PHP GO CSHARP R D)
    if(BIND_${_lang})
        set(_SWIG_NEEDED TRUE)
    endif()
endforeach()

# Package-manager detection (used by auto-install helpers below) 
# Runs once and caches the result.  Works on Linux (apt/dnf/zypper/pacman),
# macOS (brew), and Windows (choco / winget).
if(NOT DEFINED _DONUT_PKG_MGR)
    find_program(_APT_BIN    apt-get  QUIET)
    find_program(_DNF_BIN    dnf      QUIET)
    find_program(_ZYPPER_BIN zypper   QUIET)
    find_program(_PACMAN_BIN pacman   QUIET)
    find_program(_BREW_BIN   brew     QUIET)
    find_program(_CHOCO_BIN  choco    QUIET)
    find_program(_WINGET_BIN winget   QUIET)

    if(_APT_BIN)
        set(_DONUT_PKG_MGR "apt-get" CACHE INTERNAL "")
    elseif(_DNF_BIN)
        set(_DONUT_PKG_MGR "dnf"     CACHE INTERNAL "")
    elseif(_ZYPPER_BIN)
        set(_DONUT_PKG_MGR "zypper"  CACHE INTERNAL "")
    elseif(_PACMAN_BIN)
        set(_DONUT_PKG_MGR "pacman"  CACHE INTERNAL "")
    elseif(_BREW_BIN)
        set(_DONUT_PKG_MGR "brew"    CACHE INTERNAL "")
    elseif(_CHOCO_BIN)
        set(_DONUT_PKG_MGR "choco"   CACHE INTERNAL "")
    elseif(_WINGET_BIN)
        set(_DONUT_PKG_MGR "winget"  CACHE INTERNAL "")
    else()
        set(_DONUT_PKG_MGR "none"    CACHE INTERNAL "")
    endif()
endif()

# _donut_install_pkg(<pkg-apt> <pkg-dnf> <pkg-zypper> <pkg-pacman> <pkg-brew> <pkg-choco> <pkg-winget> <label>) 
# Attempts to install a package via the detected package manager.
# Returns _DONUT_INSTALL_OK = TRUE/FALSE in the parent scope.
function(_donut_install_pkg _apt _dnf _zyp _pac _brew _choco _winget _label)
    set(_DONUT_INSTALL_OK FALSE PARENT_SCOPE)
    set(_r 1)

    if(_DONUT_PKG_MGR STREQUAL "apt-get" AND _apt)
        execute_process(COMMAND sudo apt-get install -y ${_apt} RESULT_VARIABLE _r)
    elseif(_DONUT_PKG_MGR STREQUAL "dnf" AND _dnf)
        execute_process(COMMAND sudo dnf install -y ${_dnf} RESULT_VARIABLE _r)
    elseif(_DONUT_PKG_MGR STREQUAL "zypper" AND _zyp)
        execute_process(COMMAND sudo zypper install -y ${_zyp} RESULT_VARIABLE _r)
    elseif(_DONUT_PKG_MGR STREQUAL "pacman" AND _pac)
        execute_process(COMMAND sudo pacman -S --noconfirm ${_pac} RESULT_VARIABLE _r)
    elseif(_DONUT_PKG_MGR STREQUAL "brew" AND _brew)
        execute_process(COMMAND brew install ${_brew} RESULT_VARIABLE _r)
    elseif(_DONUT_PKG_MGR STREQUAL "choco" AND _choco)
        execute_process(COMMAND choco install -y ${_choco} RESULT_VARIABLE _r)
    elseif(_DONUT_PKG_MGR STREQUAL "winget" AND _winget)
        execute_process(COMMAND winget install --id ${_winget} -e --silent --accept-source-agreements --accept-package-agreements RESULT_VARIABLE _r)
    endif()

    if(_r EQUAL 0)
        message(STATUS "[Donut] ${_label} installed successfully.")
        set(_DONUT_INSTALL_OK TRUE PARENT_SCOPE)
    else()
        message(WARNING "[Donut] Auto-install of ${_label} failed or no package defined for ${_DONUT_PKG_MGR}.")
    endif()
endfunction()

# Global auto-install toggle
# Set this ON to let CMake install missing tools/runtimes automatically.
# Defaults to OFF so normal builds are completely unaffected.
option(DONUT_AUTO_INSTALL
    "Automatically install missing SWIG and language runtimes via the system package manager"
    OFF)

# SWIG soft-find + optional auto-install 
if(_SWIG_NEEDED)
    find_package(SWIG 4.0 QUIET)

    if(NOT SWIG_FOUND)
        message(STATUS "[Donut] SWIG 4.0+ not found on this machine.")

        if(DONUT_AUTO_INSTALL)
            message(STATUS "[Donut] DONUT_AUTO_INSTALL=ON — attempting to install SWIG...")

            set(SWIG_INSTALL_SUCCESS FALSE)

            # 1. Try Chocolatey (most reliable for developers)
            if(_CHOCO_BIN)
                message(STATUS "[Donut] Installing SWIG via Chocolatey...")
                execute_process(
                    COMMAND choco install -y swig --version=4.2.1
                    RESULT_VARIABLE _r 
                    OUTPUT_QUIET ERROR_QUIET
                )
                if(_r EQUAL 0)
                    set(SWIG_INSTALL_SUCCESS TRUE)
                endif()
            endif()

            # 2. Fallback: Winget
            if(NOT SWIG_INSTALL_SUCCESS AND _WINGET_BIN)
                message(STATUS "[Donut] Installing SWIG via Winget...")
                execute_process(
                    COMMAND winget install --id SWIG.SWIG -e --silent --accept-source-agreements --accept-package-agreements
                    RESULT_VARIABLE _r 
                    OUTPUT_QUIET ERROR_QUIET
                )
                if(_r EQUAL 0)
                    set(SWIG_INSTALL_SUCCESS TRUE)
                endif()
            endif()

            # 3. Ultimate fallback: Download official Windows binary
            if(NOT SWIG_INSTALL_SUCCESS)
                message(STATUS "[Donut] Downloading official SWIG 4.2.1 binary...")
                set(SWIG_ZIP "${CMAKE_BINARY_DIR}/swigwin-4.2.1.zip")
                set(SWIG_DIR "${CMAKE_BINARY_DIR}/swigwin-4.2.1")

                file(DOWNLOAD 
                    "https://sourceforge.net/projects/swig/files/swigwin/swigwin-4.2.1/swigwin-4.2.1.zip"
                    "${SWIG_ZIP}"
                    STATUS _dl_status SHOW_PROGRESS
                )

                list(GET _dl_status 0 _dl_ok)
                if(_dl_ok EQUAL 0)
                    file(ARCHIVE_EXTRACT INPUT "${SWIG_ZIP}" DESTINATION "${CMAKE_BINARY_DIR}")
                    if(EXISTS "${SWIG_DIR}/swig.exe")
                        set(SWIG_EXECUTABLE "${SWIG_DIR}/swig.exe" CACHE FILEPATH "SWIG executable" FORCE)
                        set(SWIG_INSTALL_SUCCESS TRUE)
                        message(STATUS "[Donut] SWIG downloaded and extracted successfully.")
                    endif()
                else()
                    message(WARNING "[Donut] Failed to download SWIG zip.")
                endif()
            endif()

            # Refresh find_package after installation attempt
            if(SWIG_INSTALL_SUCCESS)
                find_package(SWIG 4.0 QUIET)
            endif()
        endif()

        # Final check
        if(NOT SWIG_FOUND)
            message(FATAL_ERROR
                "[Donut] SWIG 4.0 or newer is required.\n"
                "Options:\n"
                "   1. Run again with -DDONUT_AUTO_INSTALL=ON (recommended)\n"
                "   2. Install manually from https://www.swig.org/download.html\n"
                "   3. Disable bindings: -DBIND_PYTHON=OFF")
        endif()
    else()
        message(STATUS "[Donut] Found SWIG: ${SWIG_EXECUTABLE}")
    endif()

    include(UseSWIG)
    
    # Ensure SWIG targets inherit include directories from their dependencies
    set(SWIG_USE_TARGET_INCLUDE_DIRECTORIES TRUE)
    # Use standard naming (e.g. _DonutAPI.so for Python)
    set(UseSWIG_TARGET_NAME_PREFERENCE STANDARD)
       
    # Mark the interface file as C++ so SWIG generates a C++ wrapper
    set_property(SOURCE DonutAPI.i PROPERTY CPLUSPLUS ON)
    set_property(SOURCE DonutAPI.i PROPERTY
        INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})

    # Helper macro: create one SWIG target 
    # Usage: _donut_swig_target(<TargetName> <language> <extra_link_libs>)
    macro(_donut_swig_target _tgt _lang _extra_libs)
        swig_add_library(${_tgt}
            TYPE    SHARED
            LANGUAGE ${_lang}
            SOURCES DonutAPI.i
        )
        target_include_directories(${_tgt} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
        )
        target_link_libraries(${_tgt} PRIVATE DonutAPI ${_extra_libs})
        
        # Ensure SWIG targets inherit the same UNICODE and DLL export definitions
        target_compile_definitions(${_tgt} PRIVATE
            $<$<PLATFORM_ID:Windows>:UNICODE _UNICODE>
        )

        set_target_properties(${_tgt} PROPERTIES DEBUG_POSTFIX "_d")
        message(STATUS "[Donut] SWIG binding enabled: ${_lang} → ${_tgt}")
    endmacro()

    # Helper: soft find_package with auto-install fallback 
    # Usage: _donut_require_pkg(<CMakePackage> <components> <apt> <dnf> <zyp> <pac> <brew> <choco> <winget> <label> <foundVar>)
    macro(_donut_require_pkg _pkg _comps _apt _dnf _zyp _pac _brew _choco _winget _label _foundVar)
        if(_comps)
            find_package(${_pkg} QUIET COMPONENTS ${_comps})
        else()
            find_package(${_pkg} QUIET)
        endif()
        if(NOT ${_pkg}_FOUND AND DONUT_AUTO_INSTALL)
            message(STATUS "[Donut] ${_label} not found — attempting auto-install...")
            _donut_install_pkg("${_apt}" "${_dnf}" "${_zyp}" "${_pac}" "${_brew}" "${_choco}" "${_winget}" "${_label}")
            if(_comps)
                find_package(${_pkg} QUIET COMPONENTS ${_comps})
            else()
                find_package(${_pkg} QUIET)
            endif()
        endif()
        if(${_pkg}_FOUND)
            set(${_foundVar} TRUE)
        else()
            set(${_foundVar} FALSE)
            message(WARNING
                "[Donut] ${_label} not found — ${_pkg} binding will be skipped.\n"
                "  To install automatically: re-run CMake with -DDONUT_AUTO_INSTALL=ON\n"
                "  To install manually:      install ${_apt} (apt) / ${_dnf} (dnf) / ${_brew} (brew)")
        endif()
    endmacro()

    # Python
    if(BIND_PYTHON)
        # On Windows, we often need to be explicit about finding the Development component
        _donut_require_pkg(Python3 "Interpreter;Development"
            "python3-dev" "python3-devel" "python3-devel" "python3" "python3" "python3" "Python.Python.3"
            "Python 3 development headers" _PY_OK)

        if(_PY_OK)
            _donut_swig_target(DonutAPI_python python "")

            # Modern CMake approach: link to the Python3::Python target
            # This automatically handles include directories and libraries.
            if(TARGET Python3::Python)
                target_link_libraries(DonutAPI_python PRIVATE Python3::Python)
            endif()

            # Fallback: manually add include directories if the target didn't provide them
            if(Python3_INCLUDE_DIRS)
                target_include_directories(DonutAPI_python PRIVATE ${Python3_INCLUDE_DIRS})
            endif()

            # Ensure the output name is correct for Python (usually _DonutAPI)
            set_target_properties(DonutAPI_python PROPERTIES
                SWIG_OUTFILE_DIR ${CMAKE_CURRENT_BINARY_DIR}
                OUTPUT_NAME "DonutAPI")
        endif()
    endif()

    # Java
    # Output: libDonutAPI_java.so and DonutAPI.java (compile into a JAR)
    # Usage:  import DonutAPI.*; System.loadLibrary("DonutAPI_java");
    if(BIND_JAVA)
        _donut_require_pkg(Java "Runtime" "" "" "" "" "" "" "Oracle.JDK.21" "Java Runtime" _JAVA_OK)
        _donut_require_pkg(JNI  ""
            "default-jdk" "java-devel" "java-devel" "jdk-openjdk" "openjdk" "openjdk" "Oracle.JDK.21"
            "JNI (Java Native Interface)" _JNI_OK)
        if(_JAVA_OK AND _JNI_OK)
            _donut_swig_target(DonutAPI_java java "${JNI_LIBRARIES}")
            target_include_directories(DonutAPI_java PRIVATE ${JNI_INCLUDE_DIRS})
            
            # Use target-specific SWIG flags instead of global source property
            # to avoid leaking flags to other language targets (like C#)
            set_property(TARGET DonutAPI_java PROPERTY
                SWIG_FLAGS "-package" "com.donut.api")
                
            set_target_properties(DonutAPI_java PROPERTIES
                SWIG_OUTFILE_DIR ${CMAKE_CURRENT_BINARY_DIR}/java)
        endif()
    endif()

    # JavaScript (Node.js) 
    # Output: DonutAPI.node (native addon)
    # Usage:  const dn = require('./DonutAPI.node');
    if(BIND_JAVASCRIPT)
        find_program(NODE_BIN node QUIET)
        if(NOT NODE_BIN AND DONUT_AUTO_INSTALL)
            message(STATUS "[Donut] Node.js not found — attempting auto-install...")
            _donut_install_pkg("nodejs" "nodejs" "nodejs" "nodejs" "node" "nodejs" "OpenJS.NodeJS" "Node.js")
            find_program(NODE_BIN node QUIET)
        endif()
        if(NODE_BIN)
            execute_process(
                COMMAND ${NODE_BIN} -e
                    "process.stdout.write(require('path').join(process.execPath,'../../include/node'))"
                OUTPUT_VARIABLE NODE_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
            if(NOT IS_DIRECTORY "${NODE_INCLUDE_DIR}")
                set(NODE_INCLUDE_DIR "/usr/include/node")
            endif()
            _donut_swig_target(DonutAPI_js javascript "")
            target_include_directories(DonutAPI_js PRIVATE "${NODE_INCLUDE_DIR}")
            set_property(SOURCE DonutAPI.i PROPERTY
                SWIG_FLAGS "-node" "-DSWIGJAVASCRIPT")
            set_target_properties(DonutAPI_js PROPERTIES
                OUTPUT_NAME "DonutAPI" SUFFIX ".node")
        else()
            message(WARNING "[Donut] Node.js not found — JavaScript binding skipped.\n"
                            "  Install with: -DDONUT_AUTO_INSTALL=ON or install nodejs manually.")
        endif()
    endif()

    # Octave 
    # Output: DonutAPI.oct
    # Usage:  DonutAPI; (source the .oct file in Octave)
    if(BIND_OCTAVE)
        _donut_require_pkg(Octave "Development"
            "liboctave-dev" "octave-devel" "octave-devel" "octave" "octave" "octave" "GNU.Octave"
            "Octave development headers" _OCT_OK)
        if(_OCT_OK)
            _donut_swig_target(DonutAPI_octave octave "${Octave_LIBRARIES}")
            target_include_directories(DonutAPI_octave PRIVATE ${Octave_INCLUDE_DIRS})
            set_target_properties(DonutAPI_octave PROPERTIES
                OUTPUT_NAME "DonutAPI" SUFFIX ".oct")
        endif()
    endif()

    # Ruby
    # Output: DonutAPI.so
    # Usage:  require_relative 'DonutAPI'
    if(BIND_RUBY)
        _donut_require_pkg(Ruby ""
            "ruby-dev" "ruby-devel" "ruby-devel" "ruby" "ruby" "ruby" "RubyInstallerTeam.Ruby.3.3"
            "Ruby development headers" _RUBY_OK)
        if(_RUBY_OK)
            _donut_swig_target(DonutAPI_ruby ruby "${RUBY_LIBRARY}")
            target_include_directories(DonutAPI_ruby PRIVATE ${RUBY_INCLUDE_DIRS})
        endif()
    endif()

    # Perl 
    # Output: DonutAPI.so and DonutAPI.pm
    # Usage:  use DonutAPI;
    if(BIND_PERL)
        find_program(PERL_BIN perl QUIET)
        if(NOT PERL_BIN AND DONUT_AUTO_INSTALL)
            _donut_install_pkg("libperl-dev" "perl-ExtUtils-Embed" "perl-devel" "perl" "perl" "strawberryperl" "StrawberryPerl.StrawberryPerl" "Perl")
            find_program(PERL_BIN perl QUIET)
        endif()
        if(PERL_BIN)
            execute_process(COMMAND ${PERL_BIN} -MConfig -e "print ${archlib}"
                            OUTPUT_VARIABLE PERL_ARCH_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
            set(PERL_INCLUDE_DIR "${PERL_ARCH_DIR}/CORE")
            _donut_swig_target(DonutAPI_perl perl "")
            target_include_directories(DonutAPI_perl PRIVATE "${PERL_INCLUDE_DIR}")
            target_link_directories(DonutAPI_perl PRIVATE "${PERL_ARCH_DIR}/CORE")
        else()
            message(WARNING "[Donut] Perl not found — Perl binding skipped.\n"
                            "  Install with -DDONUT_AUTO_INSTALL=ON or install perl manually.")
        endif()
    endif()

    # Lua
    # Output: DonutAPI.so
    # Usage:  local dn = require('DonutAPI')
    if(BIND_LUA)
        _donut_require_pkg(Lua ""
            "liblua5.4-dev" "lua-devel" "lua-devel" "lua" "lua" "lua" "Lua.Lua"
            "Lua development headers" _LUA_OK)
        if(_LUA_OK)
            _donut_swig_target(DonutAPI_lua lua "${LUA_LIBRARIES}")
            target_include_directories(DonutAPI_lua PRIVATE ${LUA_INCLUDE_DIR})
        endif()
    endif()

    # PHP 
    # Output: DonutAPI.so and DonutAPI.php
    # Usage:  include 'DonutAPI.php';
    if(BIND_PHP)
        find_program(PHP_CONFIG php-config QUIET)
        if(NOT PHP_CONFIG AND DONUT_AUTO_INSTALL)
            _donut_install_pkg("php-dev" "php-devel" "php-devel" "php" "php" "php" "PHP.PHP" "PHP dev headers")
            find_program(PHP_CONFIG php-config QUIET)
        endif()
        if(PHP_CONFIG)
            execute_process(COMMAND ${PHP_CONFIG} --includes
                            OUTPUT_VARIABLE PHP_INCLUDE_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
            string(REPLACE "-I" "" PHP_INCLUDE_LIST "${PHP_INCLUDE_FLAGS}")
            string(REPLACE " " ";" PHP_INCLUDE_LIST "${PHP_INCLUDE_LIST}")
            _donut_swig_target(DonutAPI_php php "")
            target_include_directories(DonutAPI_php PRIVATE ${PHP_INCLUDE_LIST})
        else()
            message(WARNING "[Donut] php-config not found — PHP binding skipped.\n"
                            "  Install with -DDONUT_AUTO_INSTALL=ON or install php-dev manually.")
        endif()
    endif()

    # Go 
    # Usage:  import "DonutAPI"  (copy generated files into your Go module)
    if(BIND_GO)
        find_program(GO_BIN go QUIET)
        if(NOT GO_BIN AND DONUT_AUTO_INSTALL)
            _donut_install_pkg("golang" "golang" "golang" "go" "go" "go" "Google.Go" "Go toolchain")
            find_program(GO_BIN go QUIET)
        endif()
        if(GO_BIN)
            set(_GO_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/go_bindings)
            file(MAKE_DIRECTORY ${_GO_OUT_DIR})
            add_custom_target(DonutAPI_go ALL
                COMMAND ${SWIG_EXECUTABLE}
                        -go -cgo -intgosize 64 -c++ -cpperraswarn
                        -I${CMAKE_CURRENT_SOURCE_DIR}
                        -outdir ${_GO_OUT_DIR}
                        -o ${_GO_OUT_DIR}/DonutAPI_wrap.cxx
                        ${CMAKE_CURRENT_SOURCE_DIR}/DonutAPI.i
                COMMENT "[Donut] Generating Go SWIG bindings" VERBATIM)
            add_custom_command(TARGET DonutAPI_go POST_BUILD
                COMMAND ${GO_BIN} build -buildmode=c-shared
                        -o ${_GO_OUT_DIR}/DonutAPI.so .
                WORKING_DIRECTORY ${_GO_OUT_DIR}
                COMMENT "[Donut] Building Go shared library")
            message(STATUS "[Donut] Go binding target: DonutAPI_go → ${_GO_OUT_DIR}")
        else()
            message(WARNING "[Donut] Go not found — Go binding skipped.\n"
                            "  Install with -DDONUT_AUTO_INSTALL=ON or install golang manually.")
        endif()
    endif()

    # C# 
    # Output: DonutAPIPINVOKE.cs and DonutAPI.cs (compile into your .csproj)
    # Usage:  DonutAPI.InitWindow(120, 30, 8, 16);
    if(BIND_CSHARP)
        _donut_swig_target(DonutAPI_csharp csharp "")
        set_target_properties(DonutAPI_csharp PROPERTIES
            SWIG_OUTFILE_DIR ${CMAKE_CURRENT_BINARY_DIR}/csharp)
    endif()

    # R
    # Output: DonutAPI.so  loaded via dyn.load()
    # Usage:  dyn.load('DonutAPI.so'); .Call('DonutAPI_InitWindow', ...)
    if(BIND_R)
        find_program(R_BIN R QUIET)
        if(NOT R_BIN AND DONUT_AUTO_INSTALL)
            _donut_install_pkg("r-base-dev" "R-devel" "R-devel" "r" "r" "r" "RProject.R" "R")
            find_program(R_BIN R QUIET)
        endif()
        if(R_BIN)
            execute_process(COMMAND ${R_BIN} CMD config --cppflags
                            OUTPUT_VARIABLE R_CPP_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
            string(REPLACE "-I" "" R_INCLUDE_LIST "${R_CPP_FLAGS}")
            string(REPLACE " " ";" R_INCLUDE_LIST "${R_INCLUDE_LIST}")
            _donut_swig_target(DonutAPI_r r "")
            target_include_directories(DonutAPI_r PRIVATE ${R_INCLUDE_LIST})
        else()
            message(WARNING "[Donut] R not found — R binding skipped.\n"
                            "  Install with -DDONUT_AUTO_INSTALL=ON or install r-base-dev manually.")
        endif()
    endif()

    # D
    # Output: DonutAPI.d  +  libDonutAPI_d.so
    # Usage:  import DonutAPI;
    if(BIND_D)
        find_program(DMD_BIN NAMES dmd ldc2 QUIET)
        if(NOT DMD_BIN AND DONUT_AUTO_INSTALL)
            # dmd is not in most package managers — guide the user
            _donut_install_pkg("dmd" "dmd" "dmd" "dmd" "dmd" "dmd" "DLang.DMD" "D compiler (dmd)")
            find_program(DMD_BIN NAMES dmd ldc2 QUIET)
        endif()
        if(DMD_BIN)
            _donut_swig_target(DonutAPI_d d "")
            set_target_properties(DonutAPI_d PROPERTIES
                SWIG_OUTFILE_DIR ${CMAKE_CURRENT_BINARY_DIR}/d)
        else()
            message(WARNING "[Donut] D compiler (dmd/ldc2) not found — D binding skipped.\n"
                            "  Install dmd from https://dlang.org/download.html, then re-run CMake.")
        endif()
    endif()

endif() # _SWIG_NEEDED