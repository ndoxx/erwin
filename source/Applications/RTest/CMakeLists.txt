set(SRC_RTEST
    source/rtest_app.cpp
    source/layer_test.cpp
   )

add_executable(rtest ${SRC_RTEST})
target_include_directories(rtest PRIVATE "${CMAKE_SOURCE_DIR}/source/Erwin")
target_include_directories(rtest PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/glm")
target_include_directories(rtest PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/imgui")
target_include_directories(rtest PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/ctti/include")


set_target_properties(rtest
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

target_link_libraries(rtest
					            erwin
                      pthread
                      stdc++fs
                      eastl
                      )
cotire(rtest)