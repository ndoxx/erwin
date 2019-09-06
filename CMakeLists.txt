cmake_minimum_required(VERSION 3.10)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
include(cotire)

# Defines a test application with basic scene setup

cmake_minimum_required(VERSION 3.10)
project(erwin VERSION 1.0.0 DESCRIPTION "Erwin engine shared library")

if(DEFINED CLANG6)
  include(toolchain_clang6)
else()
  include(toolchain_clang7)
endif()

set(CMAKE_CXX_STANDARD 17)

# include_directories(BEFORE "/usr/include/c++/8/")
# include_directories(AFTER "/usr/include/x86_64-linux-gnu/c++/8/")

set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE MinSizeRel)
# set(CMAKE_BUILD_TYPE RelWithDebInfo)

subdirs(source)

set(SRC_CORE
    ${CMAKE_SOURCE_DIR}/source/Erwin/core/config.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/core/application.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/core/time_base.cpp
  )

set(SRC_LOG
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/stack_trace.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/logger_common.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/logger_thread.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/logger.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/color.cpp
  )

set(SRC_RENDERER
    ${CMAKE_SOURCE_DIR}/source/Erwin/render_queue.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render_thread.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/vertex_layout.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/mesh.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/wmesh.cpp
  )

add_definitions(-D__DEBUG__)
add_definitions(-DLOGGING_ENABLED=1)

if(DEFINED __DEBUG__)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--export-dynamic")
endif()


add_library(erwin SHARED
            ${SRC_CORE}
            ${SRC_LOG}
            ${SRC_RENDERER})

target_include_directories(erwin PUBLIC "${CMAKE_SOURCE_DIR}/source")
target_include_directories(erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor")
# target_include_directories(erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/imgui")
target_include_directories(erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/ctti/include")
set_target_properties(erwin PROPERTIES PUBLIC_HEADER "${CMAKE_SOURCE_DIR}/source/erwin.h")
set_target_properties(erwin PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(erwin PROPERTIES SOVERSION 1)
set_target_properties(erwin
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
target_compile_definitions(erwin PRIVATE W_BUILD_LIB=1)
