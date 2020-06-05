add_executable(rtest)

file(GLOB_RECURSE SRC_RTEST "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

target_sources(rtest
    PRIVATE
      ${SRC_RTEST}
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