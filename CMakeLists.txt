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
#   1. vcpkg CMake config (Windows)
#   2. pkg-config (Linux, macOS when PKG_CONFIG_PATH is set)
#   3. find_path / find_library using CMAKE_PREFIX_PATH (macOS Homebrew fallback)
find_package(capstone CONFIG QUIET)
if(TARGET capstone::capstone)
    target_link_libraries(slopreveal PRIVATE capstone::capstone)
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(CAPSTONE QUIET capstone)
    endif()

    if(CAPSTONE_FOUND)
        target_include_directories(slopreveal PRIVATE ${CAPSTONE_INCLUDE_DIRS})
        target_link_libraries(slopreveal PRIVATE ${CAPSTONE_LIBRARIES})
        target_link_directories(slopreveal PRIVATE ${CAPSTONE_LIBRARY_DIRS})
    else()
        # Homebrew / manual install: honour CMAKE_PREFIX_PATH
        find_path(CAPSTONE_INCLUDE_DIR capstone/capstone.h)
        find_library(CAPSTONE_LIBRARY NAMES capstone)
        if(NOT CAPSTONE_INCLUDE_DIR OR NOT CAPSTONE_LIBRARY)
            message(FATAL_ERROR
                "Capstone not found.\n"
                "  Linux:  sudo apt install libcapstone-dev\n"
                "  macOS:  brew install capstone\n"
                "  Windows: add capstone to vcpkg.json and pass the toolchain file")
        endif()
        target_include_directories(slopreveal PRIVATE ${CAPSTONE_INCLUDE_DIR})
        target_link_libraries(slopreveal PRIVATE ${CAPSTONE_LIBRARY})
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
