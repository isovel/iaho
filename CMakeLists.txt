cmake_minimum_required(VERSION 3.22)
project(IAlreadyHaveOne)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# UE4SS requires one of its custom configurations
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Game__Shipping__Win64" CACHE STRING "Choose the type of build." FORCE)
endif()

# clang-cl has no /std:c++23, so CMake's CXX_STANDARD 23 maps to /std:c++latest
# (C++26), where UE4SS's Unreal headers hit P2864: bitwise ops between different
# enum types become a hard error. Emit a real C++23 flag instead.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(CMAKE_CXX23_STANDARD_COMPILE_OPTION "/clang:-std=c++23")
    set(CMAKE_CXX23_EXTENSION_COMPILE_OPTION "/clang:-std=gnu++23")
    add_compile_options(-Wno-deprecated-enum-enum-conversion)
endif()

add_subdirectory(deps/ue4ss)
add_subdirectory(mod)
