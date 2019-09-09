cmake_minimum_required(VERSION 3.10)
project(glad VERSION 1.0.0 DESCRIPTION "GLAD OpenGL loader")

add_library(glad STATIC
			src/glad.c)
target_include_directories(glad PUBLIC "include")

set_target_properties(glad
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "../../../../lib"
    LIBRARY_OUTPUT_DIRECTORY "../../../../lib"
)
set_property(TARGET glad PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(glad
					  GL)