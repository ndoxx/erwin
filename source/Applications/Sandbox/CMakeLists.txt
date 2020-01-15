set(SRC_SANDBOX
    source/sandbox_app.cpp
    source/layer_2d.cpp
    source/layer_3d.cpp
    source/layer_3d_deferred.cpp
    source/layer_presentation.cpp
    source/layer_debug.cpp
   )

add_executable(sandbox ${SRC_SANDBOX})
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