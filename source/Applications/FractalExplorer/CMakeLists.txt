set(SRC_FRACTAL_EXPLORER
    source/fractal_explorer.cpp
   )

add_executable(fractal ${SRC_FRACTAL_EXPLORER})
target_include_directories(fractal PRIVATE "${CMAKE_SOURCE_DIR}/source/Erwin")
target_include_directories(fractal PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/glm")
target_include_directories(fractal PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/imgui")
target_include_directories(fractal PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/ctti/include")


set_target_properties(fractal
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

target_link_libraries(fractal
					            ierwin
                      pthread
                      stdc++fs
                      eastl
                      )
cotire(fractal)