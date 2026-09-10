set(TARGET IAlreadyHaveOne)

add_library(${TARGET} SHARED
    src/dllmain.cpp
    src/Config.cpp
    src/Reflection.cpp
    src/Inventory.cpp
    src/RecipeFilter.cpp
    src/Hooks.cpp
)

target_include_directories(${TARGET} PRIVATE src)
target_link_libraries(${TARGET} PUBLIC UE4SS)

if(DEFINED IAHO_GAME_MODS_DIR)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${IAHO_GAME_MODS_DIR}/${TARGET}/dlls"
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARGET}> "${IAHO_GAME_MODS_DIR}/${TARGET}/dlls/main.dll"
        COMMENT "Installing ${TARGET} to ${IAHO_GAME_MODS_DIR}"
    )
endif()
