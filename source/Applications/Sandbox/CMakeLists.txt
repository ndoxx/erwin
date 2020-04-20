file(GLOB_RECURSE SRC_SANDBOX "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

add_executable(sandbox)
target_sources(sandbox
    PRIVATE
        ${SRC_SANDBOX}
    )
target_include_directories(sandbox
    PRIVATE
        "${CMAKE_SOURCE_DIR}/source/Applications/Sandbox/source"
        "${CMAKE_SOURCE_DIR}/source/Erwin"
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