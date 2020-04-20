cmake_minimum_required(VERSION 3.10)

# GLAD
project(glad VERSION 1.0.0 DESCRIPTION "GLAD OpenGL loader")

add_library(glad STATIC
            ${CMAKE_SOURCE_DIR}/source/vendor/glad/src/glad.c)
target_include_directories(glad PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/glad/include")

set_target_properties(glad
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
)
set_property(TARGET glad PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(glad GL)


set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
include(cotire)

# Needed here to avoid weird cmake loop
project(erwin VERSION 1.0.0 DESCRIPTION "Erwin engine shared library")

set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE MinSizeRel)
# set(CMAKE_BUILD_TYPE RelWithDebInfo)

include(standard_settings)

if(DEFINED CLANG6)
  include(toolchain_clang6)
elseif(DEFINED CLANG7)
  include(toolchain_clang7)
elseif(DEFINED CLANG8)
  include(toolchain_clang8)
else()
  include(toolchain_clang_local)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a")

# Interface library to enable us to pass on options to multiple projects
add_library(project_warnings INTERFACE)
include(compiler_warnings) # Hardcore warnings
set_project_warnings(project_warnings)

# Global definitions
add_definitions(-DW_DEBUG)
add_definitions(-DW_ENABLE_ASSERT)
add_definitions(-DLOGGING_ENABLED=1)


# I can't link EASTL statically in liberwin (I tried hard building it with -fPIC but
# it does not seem to work). As a result, for now, each client applications
# MUST link against EASTL
set(INC_EASTL
    ${CMAKE_SOURCE_DIR}/source/vendor/eastl/include
    ${CMAKE_SOURCE_DIR}/source/vendor/eastl
    ${CMAKE_SOURCE_DIR}/source/vendor/eastl/test/packages/EAAssert/include
    ${CMAKE_SOURCE_DIR}/source/vendor/eastl/test/packages/EABase/include/Common
    ${CMAKE_SOURCE_DIR}/source/vendor/eastl/test/packages/EAMain/include
    ${CMAKE_SOURCE_DIR}/source/vendor/eastl/test/packages/EAStdC/include
    ${CMAKE_SOURCE_DIR}/source/vendor/eastl/test/packages/EATest/include
    ${CMAKE_SOURCE_DIR}/source/vendor/eastl/test/packages/EAThread/include)
add_library(eastl STATIC IMPORTED)
set_target_properties(eastl PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libEASTL.a"
  INTERFACE_INCLUDE_DIRECTORIES "${INC_EASTL}"
)


subdirs(source)
