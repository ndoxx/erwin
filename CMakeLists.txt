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
    ${CMAKE_SOURCE_DIR}/source/Erwin/core/layer.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/core/layer_stack.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/imgui/imgui_layer.cpp
  )

set(SRC_MATH
    ${CMAKE_SOURCE_DIR}/source/Erwin/math/color.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/math/math3d.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/math/quaternion.cpp
  )

set(SRC_LOG
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/stack_trace.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/logger_common.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/logger_thread.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/logger.cpp
  )

set(SRC_RENDERER
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/render_queue.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/render_thread.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/vertex_layout.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/mesh.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/wmesh.cpp
  )

set(SRC_PLATFORM
    ${CMAKE_SOURCE_DIR}/source/platform/ogl_context.cpp
    ${CMAKE_SOURCE_DIR}/source/platform/glfw_window.cpp
  )

set(SRC_IMGUI
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/imgui/imgui_build.cpp)

add_definitions(-D__DEBUG__)
add_definitions(-DLOGGING_ENABLED=1)

if(DEFINED __DEBUG__)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--export-dynamic")
endif()


add_library(erwin SHARED
            ${SRC_CORE}
            ${SRC_MATH}
            ${SRC_LOG}
            ${SRC_RENDERER}
            ${SRC_PLATFORM}
            ${SRC_IMGUI}
            )

target_include_directories(erwin PUBLIC "${CMAKE_SOURCE_DIR}/source")
target_include_directories(erwin PUBLIC "${CMAKE_SOURCE_DIR}/source/Erwin")
target_include_directories(erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor")
target_include_directories(erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/imgui")
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


# GLFW library as a static import
target_include_directories(erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/glfw/include")
add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libglfw3.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/source/vendor/glfw"
)

# GLAD library as a static import
target_include_directories(erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/glad/include")
add_library(glad STATIC IMPORTED)
set_target_properties(glad PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libglad.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/source/vendor/glad"
)

target_link_libraries(erwin
                      m
                      stdc++fs
                      pthread
                      X11
                      glad
                      glfw)
