set(SRC_SANDBOX
    source/editor_app.cpp
    source/layer_game.cpp
    source/game/game_components.cpp
   )

add_executable(sandbox ${SRC_SANDBOX})
target_include_directories(sandbox
    PRIVATE
        "${CMAKE_SOURCE_DIR}/source/Applications/Sandbox/source"
        "${CMAKE_SOURCE_DIR}/source/Erwin"
        "${CMAKE_SOURCE_DIR}/source/vendor/glm"
        "${CMAKE_SOURCE_DIR}/source/vendor/imgui"
        "${CMAKE_SOURCE_DIR}/source/vendor/ctti/include"
        "${CMAKE_SOURCE_DIR}/source/vendor/entt/src"
    )

set_target_properties(sandbox
    PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

target_link_libraries(sandbox
    PRIVATE
        erwin
        pthread
        stdc++fs
        eastl
)

cotire(sandbox)