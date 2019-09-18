set(SRC_INTERNSTR
    source/atlaspack.cpp
    source/stb_build.cpp
    ${CMAKE_SOURCE_DIR}/source/vendor/inih/ini.c
    ${CMAKE_SOURCE_DIR}/source/vendor/inih/cpp/INIReader.cpp
   )

add_executable(atlaspack ${SRC_INTERNSTR})

set_target_properties(atlaspack
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
target_include_directories(atlaspack PUBLIC "${CMAKE_SOURCE_DIR}/source/Erwin")
target_include_directories(atlaspack PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor")
target_include_directories(atlaspack PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/freetype/include")

add_library(freetype STATIC IMPORTED)
set_target_properties(freetype PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libfreetype.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/source/vendor/freetype"
)

target_link_libraries(atlaspack
                      stdc++fs
                      png
                      z
                      bz2
                      harfbuzz
                      freetype
                      )
cotire(atlaspack)
