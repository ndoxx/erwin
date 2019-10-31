# GLAD
cmake_minimum_required(VERSION 3.10)
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


# Erwin library
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
include(cotire)

cmake_minimum_required(VERSION 3.10)
project(erwin VERSION 1.0.0 DESCRIPTION "Erwin engine shared library")

if(DEFINED CLANG6)
  include(toolchain_clang6)
elseif(DEFINED CLANG7)
  include(toolchain_clang7)
else()
  include(toolchain_clang8)
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
    ${CMAKE_SOURCE_DIR}/source/Erwin/core/intern_string.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/core/unique_id.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/core/config.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/core/value_map.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/core/application.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/core/time_base.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/core/layer.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/core/layer_stack.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/core/game_clock.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/core/z_wrapper.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/memory/linear_allocator.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/memory/pool_allocator.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/memory/memory_utils.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/memory/handle_pool.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/imgui/imgui_layer.cpp
  )

set(SRC_FILESYSTEM
    ${CMAKE_SOURCE_DIR}/source/Erwin/filesystem/filesystem.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/filesystem/spv_file.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/filesystem/tom_file.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/filesystem/cat_file.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/filesystem/xml_file.cpp
  )

set(SRC_MATH
    ${CMAKE_SOURCE_DIR}/source/Erwin/math/color.cpp
    # ${CMAKE_SOURCE_DIR}/source/Erwin/math/math3d.cpp
    # ${CMAKE_SOURCE_DIR}/source/Erwin/math/quaternion.cpp
  )

set(SRC_DEBUG
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/stack_trace.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/logger_common.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/logger_thread.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/logger_sink.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/logger.cpp
  )

set(SRC_RENDERER
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/stb_build.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/buffer_layout.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/buffer.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/framebuffer.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/framebuffer_layout.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/framebuffer_pool.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/texture.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/texture_atlas.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/render_device.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/render_thread.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/renderer_2d.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/master_renderer.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/WIP/renderer_2d.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/WIP/main_renderer.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/WIP/texture_atlas.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/WIP/framebuffer_pool.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/camera_2d.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/shader.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/query_timer.cpp
    # ${CMAKE_SOURCE_DIR}/source/Erwin/render/mesh.cpp
    # ${CMAKE_SOURCE_DIR}/source/Erwin/render/wmesh.cpp
  )

set(SRC_INPUT
    ${CMAKE_SOURCE_DIR}/source/Erwin/input/camera_2d_controller.cpp
  )

set(SRC_PLATFORM
    ${CMAKE_SOURCE_DIR}/source/platform/ogl_context.cpp
    ${CMAKE_SOURCE_DIR}/source/platform/ogl_render_device.cpp
    ${CMAKE_SOURCE_DIR}/source/platform/ogl_buffer.cpp
    ${CMAKE_SOURCE_DIR}/source/platform/ogl_framebuffer.cpp
    ${CMAKE_SOURCE_DIR}/source/platform/ogl_texture.cpp
    ${CMAKE_SOURCE_DIR}/source/platform/ogl_shader.cpp
    ${CMAKE_SOURCE_DIR}/source/platform/ogl_query_timer.cpp
    ${CMAKE_SOURCE_DIR}/source/platform/glfw_window.cpp
    ${CMAKE_SOURCE_DIR}/source/platform/glfw_input.cpp
  )

set(SRC_IMGUI
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/imgui/imgui_utils.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/imgui/imgui_build.cpp)

add_definitions(-DW_DEBUG)
add_definitions(-DW_ENABLE_ASSERT)
add_definitions(-DLOGGING_ENABLED=1)

if(DEFINED W_DEBUG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--export-dynamic")
endif()

add_library(erwin SHARED
            ${SRC_CORE}
            ${SRC_FILESYSTEM}
            ${SRC_MATH}
            ${SRC_DEBUG}
            ${SRC_RENDERER}
            ${SRC_INPUT}
            ${SRC_PLATFORM}
            ${SRC_IMGUI}
            ${CMAKE_SOURCE_DIR}/source/vendor/inih/ini.c
            ${CMAKE_SOURCE_DIR}/source/vendor/inih/cpp/INIReader.cpp
            )

target_include_directories(erwin PUBLIC "${CMAKE_SOURCE_DIR}/source")
target_include_directories(erwin PUBLIC "${CMAKE_SOURCE_DIR}/source/Erwin")
target_include_directories(erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor")
target_include_directories(erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/glm")
target_include_directories(erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/imgui")
target_include_directories(erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/glad/include")
target_include_directories(erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/ctti/include")
target_include_directories(erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/freetype/include")
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

target_link_libraries(erwin
                      m
                      z
                      stdc++fs
                      pthread
                      X11
                      glad
                      glfw)
