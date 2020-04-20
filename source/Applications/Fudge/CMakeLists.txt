file(GLOB_RECURSE SRC_FUDGE "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

add_executable(fudge)
target_sources(fudge
    PRIVATE
        ${SRC_FUDGE}
    )
set_target_properties(fudge
    PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
target_include_directories(fudge
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/source"
        "${CMAKE_SOURCE_DIR}/source/Erwin"
        "${CMAKE_SOURCE_DIR}/source/vendor"
        "${CMAKE_SOURCE_DIR}/source/vendor/freetype/include"
    )

add_library(freetype STATIC IMPORTED)
set_target_properties(freetype
    PROPERTIES
        IMPORTED_LOCATION             "${CMAKE_SOURCE_DIR}/lib/libfreetype.a"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/source/vendor/freetype"
)

target_link_libraries(fudge
    PRIVATE
        erwin
        stdc++fs
        pthread
        png
        z
        bz2
        harfbuzz
        freetype
        eastl
    )

cotire(fudge)
