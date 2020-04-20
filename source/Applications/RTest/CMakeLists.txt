add_executable(rtest)
target_sources(rtest
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/source/rtest_app.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/source/layer_test.cpp"
    )
target_include_directories(rtest
    PRIVATE
        "${CMAKE_SOURCE_DIR}/source/Applications/Sandbox/source"
        "${CMAKE_SOURCE_DIR}/source/Erwin"
    )

set_target_properties(rtest
    PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    )

target_link_libraries(rtest
    PRIVATE
        erwin
        pthread
        stdc++fs
        eastl
    )

cotire(rtest)