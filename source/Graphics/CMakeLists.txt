file(GLOB_RECURSE SRC_GRAPHICS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE SRC_PLATFORM "${CMAKE_CURRENT_SOURCE_DIR}/Platform/*.cpp")

add_library(erwin_graphics STATIC)
target_sources(erwin_graphics
    PRIVATE
        ${SRC_GRAPHICS}
        ${SRC_PLATFORM}
    )
target_include_directories(erwin_graphics
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )
target_include_directories(erwin_graphics
    SYSTEM PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/glad/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/glfw/include"
    )
set_target_properties(erwin_graphics
    PROPERTIES
        VERSION                  "0.0.1"
        SOVERSION                1
        PUBLIC_HEADER            "${CMAKE_CURRENT_SOURCE_DIR}/graphics.h"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    )

# GLFW library as a static import
add_library(glfw STATIC IMPORTED)
set_target_properties(glfw
    PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libglfw3.a"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/source/vendor/glfw"
    )

# GLAD
add_library(glad
    STATIC
        ${CMAKE_SOURCE_DIR}/source/vendor/glad/src/glad.c
    )
target_include_directories(glad PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/glad/include")
set_target_properties(glad
    PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
        POSITION_INDEPENDENT_CODE ON
    )
target_link_libraries(glad GL)

target_link_libraries(erwin_graphics
    PRIVATE
        project_warnings
        project_options
        m
        X11
        glad
        glfw
        kibble
    )

cotire(erwin_graphics)