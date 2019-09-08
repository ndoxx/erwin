set(SRC_SANDBOX
    source/sandbox_app.cpp
   )

add_executable(sandbox ${SRC_SANDBOX})
target_include_directories(sandbox PRIVATE "${CMAKE_SOURCE_DIR}/source/Erwin")
target_include_directories(sandbox PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/ctti/include")


set_target_properties(sandbox
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

target_link_libraries(sandbox
					  ierwin
                      pthread
                      stdc++fs
                      )
cotire(sandbox)