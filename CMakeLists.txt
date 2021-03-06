cmake_minimum_required(VERSION 3.10)

project(erwin VERSION 1.0.0 DESCRIPTION "Erwin engine")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
include(cotire)
include(standard_settings)
include(compiler_warnings) # Hardcore warnings
include(sanitizers)
include(doxygen)

# Interface library to enable us to pass on options to multiple projects
add_library(project_warnings INTERFACE)
set_project_warnings(project_warnings)

add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_20)
enable_sanitizers(project_options)

# Global definitions & options
option(ENABLE_LOGGING "Log stuff" ON)
if (ENABLE_LOGGING)
	add_definitions(-DLOGGING_ENABLED=1)
endif()

add_definitions(-DW_PROFILE)
add_definitions(-DW_PROFILE_RENDER)
if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	add_definitions(-DW_DEBUG)
	add_definitions(-DK_DEBUG)
	add_definitions(-DK_ENABLE_ASSERT)
	add_definitions(-DLOGGING_ENABLED=1)
endif()

# I can't link EASTL statically in liberwin (I tried hard building it with -fPIC but
# it does not seem to work). As a result, for now, each client applications
# MUST link against EASTL
# add_library(eastl STATIC IMPORTED)


subdirs(source)
