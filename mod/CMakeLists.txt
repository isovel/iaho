set(TARGET IAlreadyHaveOne)

add_library(${TARGET} SHARED
    src/dllmain.cpp
    src/Config.cpp
    src/Reflection.cpp
    src/Inventory.cpp
    src/RecipeFilter.cpp
    src/Hooks.cpp
)

target_include_directories(${TARGET} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/deps/ue4ss/UE4SS/include"
    "${CMAKE_SOURCE_DIR}/deps/ue4ss/deps/first/Unreal/include"
    "${CMAKE_SOURCE_DIR}/deps/ue4ss/deps/first/DynamicOutput/include"
    "${CMAKE_SOURCE_DIR}/deps/ue4ss/deps/first/Function-Pointer-Wrapper/include"
)

target_link_libraries(${TARGET} PUBLIC UE4SS)

if(MSVC)
    target_compile_options(${TARGET} PRIVATE /FS /Zi)
    target_link_options(${TARGET} PRIVATE "/FORCE:MULTIPLE" "/DEBUG")
endif()

# UE4SS loads C++ mods as <ModName>/dlls/main.dll
add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dlls"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${TARGET}>" "${CMAKE_SOURCE_DIR}/dlls/main.dll"
    COMMENT "Copying main.dll into dlls/"
)
