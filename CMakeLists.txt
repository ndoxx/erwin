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
    ${CMAKE_SOURCE_DIR}/source/Erwin/core/core.cpp
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
    ${CMAKE_SOURCE_DIR}/source/Erwin/core/job_system.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/memory/linear_allocator.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/memory/pool_allocator.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/memory/free_list.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/memory/memory_utils.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/memory/handle_pool.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/memory/heap_area.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/utils/random.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/imgui/imgui_layer.cpp
  )

set(SRC_EVENT
    ${CMAKE_SOURCE_DIR}/source/Erwin/event/event_bus.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/event/window_events.cpp
  )

set(SRC_ASSET
    ${CMAKE_SOURCE_DIR}/source/Erwin/asset/stb_build.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/asset/asset_manager.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/asset/handles.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/asset/texture_atlas.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/asset/material.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/asset/procedural_geometry.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/asset/bounding.cpp
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
    ${CMAKE_SOURCE_DIR}/source/Erwin/math/convolution.cpp
    # ${CMAKE_SOURCE_DIR}/source/Erwin/math/math3d.cpp
    # ${CMAKE_SOURCE_DIR}/source/Erwin/math/quaternion.cpp
  )

set(SRC_ENTITY
    ${CMAKE_SOURCE_DIR}/source/Erwin/entity/component_transform.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/entity/component_bounding_box.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/entity/light.cpp
  )

set(SRC_DEBUG
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/stack_trace.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/logger_common.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/logger_thread.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/logger_sink.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/logger.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/debug/instrumentor.cpp
  )

set(SRC_RENDERER
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/stb_build.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/buffer_layout.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/buffer.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/framebuffer.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/framebuffer_layout.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/framebuffer_pool.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/texture.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/render_device.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/render_state.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/renderer_2d.cpp
    # ${CMAKE_SOURCE_DIR}/source/Erwin/render/renderer_2d_alt.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/renderer_pp.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/renderer_3d.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/renderer.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/camera_2d.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/camera_3d.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/shader_lang.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/shader.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/query_timer.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/common_geometry.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/render/handles.cpp
    # ${CMAKE_SOURCE_DIR}/source/Erwin/render/mesh.cpp
    # ${CMAKE_SOURCE_DIR}/source/Erwin/render/wmesh.cpp
  )

set(SRC_INPUT
    ${CMAKE_SOURCE_DIR}/source/Erwin/input/input.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/input/camera_2d_controller.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/input/camera_3d_controller.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/input/freefly_camera_controller.cpp
  )

set(SRC_EDITOR
    ${CMAKE_SOURCE_DIR}/source/Erwin/editor/layer_editor.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/editor/scene.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/editor/widget.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/editor/widget_console.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/editor/widget_scene_view.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/editor/widget_scene_hierarchy.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/editor/widget_inspector.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/editor/widget_rt_peek.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/editor/widget_hex_dump.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/editor/widget_keybindings.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/editor/overlay_stats.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/editor/overlay_camera_tracker.cpp
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
    ${CMAKE_SOURCE_DIR}/source/Erwin/imgui/imgui_build.cpp
    ${CMAKE_SOURCE_DIR}/source/Erwin/imgui/theme.cpp
  )

add_definitions(-DW_DEBUG)
add_definitions(-DW_ENABLE_ASSERT)
add_definitions(-DLOGGING_ENABLED=1)

if(DEFINED W_DEBUG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--export-dynamic")
endif()

add_library(erwin SHARED
            ${SRC_CORE}
            ${SRC_EVENT}
            ${SRC_ASSET}
            ${SRC_FILESYSTEM}
            ${SRC_MATH}
            ${SRC_ENTITY}
            ${SRC_DEBUG}
            ${SRC_RENDERER}
            ${SRC_INPUT}
            ${SRC_PLATFORM}
            ${SRC_IMGUI}
            ${SRC_EDITOR}
            ${CMAKE_SOURCE_DIR}/source/vendor/inih/ini.c
            ${CMAKE_SOURCE_DIR}/source/vendor/inih/cpp/INIReader.cpp
            )

target_include_directories(erwin PUBLIC "${CMAKE_SOURCE_DIR}/source")
target_include_directories(erwin PUBLIC "${CMAKE_SOURCE_DIR}/source/Erwin")
target_include_directories(erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor")
target_include_directories(erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/glm")
target_include_directories(erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/imgui")
target_include_directories(erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/taskflow")
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
target_include_directories(erwin PRIVATE "${INC_EASTL}")
add_library(eastl STATIC IMPORTED)
set_target_properties(eastl PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libEASTL.a"
  INTERFACE_INCLUDE_DIRECTORIES "${INC_EASTL}"
)

target_link_libraries(erwin
                      m
                      z
                      stdc++fs
                      pthread
                      atomic
                      X11
                      glad
                      glfw
                      )
