set(SRC_PUYO
    source/puyo_app.cpp
    source/dungeon.cpp
    source/game.cpp
    source/layer_2d.cpp
    source/layer_presentation.cpp
    source/layer_debug.cpp
   )

add_executable(puyo ${SRC_PUYO})
target_include_directories(puyo PRIVATE "${CMAKE_SOURCE_DIR}/source/Erwin")
target_include_directories(puyo PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/glm")
target_include_directories(puyo PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/imgui")
target_include_directories(puyo PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/ctti/include")


set_target_properties(puyo
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

add_library(ierwin SHARED IMPORTED)
set_target_properties(ierwin PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/liberwin.so"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/source/"
)

target_link_libraries(puyo
					            ierwin
                      pthread
                      stdc++fs
                      eastl
                      )
cotire(puyo)