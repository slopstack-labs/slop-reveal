cmake_minimum_required(VERSION 3.16)
project(slopreveal CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ── Sources ──────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/config/config.cpp
    src/disasm/disassembler.cpp
    src/http/http_client.cpp
    src/providers/provider.cpp
    src/providers/claude_provider.cpp
    src/providers/ollama_provider.cpp
    src/providers/openai_provider.cpp
    src/providers/gemini_provider.cpp
    src/cache/cache.cpp
    src/output/output.cpp
)

add_executable(slopreveal ${SOURCES})

target_include_directories(slopreveal PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# ── Capstone ─────────────────────────────────────────────────────────────────
# Resolution order:
#   1. Explicit -DCAPSTONE_INCLUDE_DIR / -DCAPSTONE_LIBRARY cache vars (macOS CI)
#      These must come first: capstone 5.x ships cmake config files, so
#      find_package(capstone CONFIG) succeeds on macOS but the imported target
#      does not propagate include directories, silently breaking the build.
#   2. vcpkg CMake config target (Windows via toolchain file)
#   3. pkg-config (Linux, macOS when PKG_CONFIG_PATH is correctly set)
#   4. find_path / find_library via CMAKE_PREFIX_PATH (last resort)
if(CAPSTONE_INCLUDE_DIR AND CAPSTONE_LIBRARY)
    message(STATUS "Capstone: using explicit paths")
    message(STATUS "  include: ${CAPSTONE_INCLUDE_DIR}")
    message(STATUS "  library: ${CAPSTONE_LIBRARY}")
    target_include_directories(slopreveal PRIVATE "${CAPSTONE_INCLUDE_DIR}")
    target_link_libraries(slopreveal PRIVATE "${CAPSTONE_LIBRARY}")
else()
    find_package(capstone CONFIG QUIET)
    if(TARGET capstone::capstone)
        target_link_libraries(slopreveal PRIVATE capstone::capstone)
    else()
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(CAPSTONE QUIET capstone)
        endif()

        if(CAPSTONE_FOUND AND CAPSTONE_INCLUDE_DIRS)
            target_include_directories(slopreveal PRIVATE ${CAPSTONE_INCLUDE_DIRS})
            target_link_libraries(slopreveal PRIVATE ${CAPSTONE_LIBRARIES})
            target_link_directories(slopreveal PRIVATE ${CAPSTONE_LIBRARY_DIRS})
        else()
            find_path(CAPSTONE_INCLUDE_DIR NAMES capstone/capstone.h)
            find_library(CAPSTONE_LIBRARY NAMES capstone)
            if(NOT CAPSTONE_INCLUDE_DIR OR NOT CAPSTONE_LIBRARY)
                message(FATAL_ERROR
                    "Capstone not found.\n"
                    "  Linux:   sudo apt install libcapstone-dev\n"
                    "  macOS:   brew install capstone  (then pass -DCAPSTONE_INCLUDE_DIR / -DCAPSTONE_LIBRARY)\n"
                    "  Windows: add capstone to vcpkg.json and pass -DCMAKE_TOOLCHAIN_FILE")
            endif()
            target_include_directories(slopreveal PRIVATE "${CAPSTONE_INCLUDE_DIR}")
            target_link_libraries(slopreveal PRIVATE "${CAPSTONE_LIBRARY}")
        endif()
    endif()
endif()

# ── libcurl ──────────────────────────────────────────────────────────────────
find_package(CURL REQUIRED)
target_link_libraries(slopreveal PRIVATE CURL::libcurl)

# ── OpenSSL (SHA-256) ────────────────────────────────────────────────────────
find_package(OpenSSL REQUIRED)
target_link_libraries(slopreveal PRIVATE OpenSSL::Crypto)

# ── Platform compiler flags ──────────────────────────────────────────────────
if(MSVC)
    target_compile_options(slopreveal PRIVATE /W4 /WX- /utf-8
        $<$<CONFIG:Release>:/O2>
    )
    target_compile_definitions(slopreveal PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
else()
    target_compile_options(slopreveal PRIVATE
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Debug>:-g -O0>
    )
endif()

# ── Install ──────────────────────────────────────────────────────────────────
install(TARGETS slopreveal DESTINATION bin)
