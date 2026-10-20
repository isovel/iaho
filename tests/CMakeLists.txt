include(FetchContent)

FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(Catch2)

add_executable(iaho_tests
    LegacyIniTest.cpp
    SettingsTest.cpp
)
target_link_libraries(iaho_tests PRIVATE iaho_core Catch2::Catch2WithMain)

if(MSVC)
    target_compile_options(iaho_tests PRIVATE /W4)
else()
    target_compile_options(iaho_tests PRIVATE -Wall -Wextra)
endif()

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(iaho_tests)
