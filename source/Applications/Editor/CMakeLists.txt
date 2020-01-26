set(SRC_EDITOR
    source/editor_app.cpp
    source/layer_game.cpp
    source/game/game_components.cpp
    source/game/pbr_deferred_render_system.cpp
    source/game/forward_sun_render_system.cpp
    source/game/bounding_box_system.cpp
   )

add_executable(editor ${SRC_EDITOR})
target_include_directories(editor PRIVATE "${CMAKE_SOURCE_DIR}/source/Applications/Editor/source")
target_include_directories(editor PRIVATE "${CMAKE_SOURCE_DIR}/source/Erwin")
target_include_directories(editor PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/glm")
target_include_directories(editor PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/imgui")
target_include_directories(editor PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/ctti/include")


set_target_properties(editor
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

target_link_libraries(editor
					            erwin
                      pthread
                      stdc++fs
                      eastl
                      )
cotire(editor)