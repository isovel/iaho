cmake_minimum_required(VERSION 3.22)
project(IAlreadyHaveOne)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR AND NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(IAHO_TESTS_DEFAULT ON)
else()
    set(IAHO_TESTS_DEFAULT OFF)
endif()
option(IAHO_BUILD_TESTS "Build host tests" ${IAHO_TESTS_DEFAULT})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# UE4SS requires one of its custom configurations
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Game__Shipping__Win64" CACHE STRING "Choose the type of build." FORCE)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/deps/pal-core/cmake/PalCoreCompiler.cmake")

add_subdirectory(deps/pal-config-lib)

# The logic that needs no game: built for the host too, and covered by tests.
add_library(iaho_core STATIC
    core/src/LegacyIni.cpp
)
target_include_directories(iaho_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/core/include")
target_link_libraries(iaho_core PUBLIC PalCfg::Core)

if(IAHO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    return()
endif()

add_subdirectory(deps/ue4ss)
add_subdirectory(deps/pal-core)
add_subdirectory(mod)
