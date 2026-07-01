cmake_minimum_required (VERSION 3.18)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
endif()

project("TradeBot")
add_executable(tradebot "conf-src/sanitizers_configure.cpp" "src/main.cpp" "src/indicators.cpp" "src/indicators.h" "src/test.cpp" "src/test.h"  "src/loader.cpp" "src/loader.h")
# add_executable(tradebot "conf-src/sanitizers_configure.cpp" "src/main.cpp" "src/loader.cpp" "src/loader.h")
include(FetchContent)

find_package(CURL REQUIRED)
target_link_libraries(tradebot PRIVATE CURL::libcurl)

FetchContent_Declare(
    SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT TAG        origin/master
)
FetchContent_MakeAvailable(SQLiteCpp)

target_link_libraries(tradebot PRIVATE SQLiteCpp)

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(json)

target_link_libraries(tradebot PRIVATE nlohmann_json::nlohmann_json)

set_property(TARGET tradebot PROPERTY CXX_STANDARD 23)

# Enabling most errors
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(tradebot PRIVATE /W4 /permissive-)
    if(TREAT_WARNINGS_AS_ERRORS)
        target_compile_options(tradebot PRIVATE /WX)
    endif()
    target_compile_options(tradebot PRIVATE /wd4244 /wd4702)
    target_compile_definitions(tradebot PRIVATE -D_CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(tradebot PRIVATE -Wall -pedantic -Wextra)
    target_compile_options(tradebot PRIVATE -Wold-style-cast)
    target_compile_options(tradebot PRIVATE -Wextra-semi)
    target_compile_options(tradebot PRIVATE -Woverloaded-virtual)
    target_compile_options(tradebot PRIVATE -Wzero-as-null-pointer-constant)
    if(TREAT_WARNINGS_AS_ERRORS)
        target_compile_options(tradebot PRIVATE -Werror -pedantic-errors)
    endif()
endif()

# Compiler specific warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tradebot PRIVATE -Wshadow=compatible-local)
    target_compile_options(tradebot PRIVATE -Wduplicated-branches)
    target_compile_options(tradebot PRIVATE -Wduplicated-cond)
    # Disabled due to GCC bug
    # target_compile_options(tradebot PRIVATE -Wnull-dereference)
    target_compile_options(tradebot PRIVATE -Wno-maybe-uninitialized)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(tradebot PRIVATE -Wshadow-uncaptured-local)
    target_compile_options(tradebot PRIVATE -Wloop-analysis)
    target_compile_options(tradebot PRIVATE -Wno-self-assign-overloaded)
    target_compile_options(tradebot PRIVATE -Wno-missing-braces)
endif()

# Enabling debug standard library for Debug and Sanitized Debug on linux
if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT WIN32)
    message(STATUS "Enabling _GLIBCXX_DEBUG")
    add_compile_definitions(_GLIBCXX_DEBUG)
endif()

option(USE_SANITIZERS "Enable to build with undefined, address and leak sanitizers" OFF)
if(USE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        message(STATUS "Enabling ASAN")
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "Embedded")
        target_compile_options(tradebot PUBLIC /fsanitize=address)
        target_compile_definitions(tradebot PUBLIC _DISABLE_STRING_ANNOTATION=1 _DISABLE_VECTOR_ANNOTATION=1)
    else()
        message(STATUS "Enabling UBSan, ASan and LSan")
        target_compile_options(tradebot PUBLIC -fsanitize=undefined,address,pointer-compare,pointer-subtract,leak)
        target_link_options(tradebot PUBLIC -fsanitize=undefined,address,pointer-compare,pointer-subtract,leak)

        target_compile_options(tradebot PUBLIC -fno-sanitize-recover=all -fno-optimize-sibling-calls -fno-omit-frame-pointer)
    endif()
else()
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "EditAndContinue")
    endif()
endif()
