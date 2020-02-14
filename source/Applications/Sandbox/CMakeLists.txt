set(SRC_SANDBOX
    source/editor_app.cpp
    source/layer_game.cpp
    source/game/game_components.cpp
    source/game/gizmo_system.cpp
    source/game/bounding_box_system.cpp
   )

add_executable(sandbox ${SRC_SANDBOX})
target_include_directories(sandbox PRIVATE "${CMAKE_SOURCE_DIR}/source/Applications/Sandbox/source")
target_include_directories(sandbox PRIVATE "${CMAKE_SOURCE_DIR}/source/Erwin")
target_include_directories(sandbox PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/glm")
target_include_directories(sandbox PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/imgui")
target_include_directories(sandbox PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/ctti/include")


set_target_properties(sandbox
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

target_link_libraries(sandbox
					            erwin
                      pthread
                      stdc++fs
                      eastl
                      )
cotire(sandbox)