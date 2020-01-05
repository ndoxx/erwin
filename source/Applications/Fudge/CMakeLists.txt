set(SRC_FUDGE
    source/fudge.cpp
    source/stb_build.cpp
    source/asset_registry.cpp
    source/dxt_compressor.cpp
    source/atlas_packer.cpp
    source/texture_packer.cpp
    source/shader_packer.cpp
    source/optimal_packing.cpp
   )

add_executable(fudge ${SRC_FUDGE})

set_target_properties(fudge
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
target_include_directories(fudge PUBLIC "${CMAKE_SOURCE_DIR}/source/Erwin")
target_include_directories(fudge PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor")
target_include_directories(fudge PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/glm")
target_include_directories(fudge PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/freetype/include")
target_include_directories(fudge PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/ctti/include")

add_library(freetype STATIC IMPORTED)
set_target_properties(freetype PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libfreetype.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/source/vendor/freetype"
)

target_link_libraries(fudge
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
