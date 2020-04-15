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


# Interface library to enable us to pass on options to multiple projects
add_library(project_warnings INTERFACE)
include(compiler_warnings) # Hardcore warnings
set_project_warnings(project_warnings)

subdirs(source)

file(GLOB_RECURSE SRC_ERWIN    "${CMAKE_SOURCE_DIR}/source/Erwin/*.cpp")
file(GLOB_RECURSE SRC_PLATFORM "${CMAKE_SOURCE_DIR}/source/platform/*.cpp")

set(SRC_IMGUI
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui_widgets.cpp
  )

# Disable all warnings for third party stuff
set_source_files_properties("${CMAKE_SOURCE_DIR}/source/Erwin/asset/stb_build.cpp" PROPERTIES COMPILE_FLAGS -w)
set_source_files_properties("${CMAKE_SOURCE_DIR}/source/Erwin/render/stb_build.cpp" PROPERTIES COMPILE_FLAGS -w)
set_source_files_properties("${CMAKE_SOURCE_DIR}/source/Erwin/core/z_wrapper.cpp" PROPERTIES COMPILE_FLAGS -w)
set_source_files_properties(${SRC_IMGUI} PROPERTIES COMPILE_FLAGS -w)

add_definitions(-DW_DEBUG)
add_definitions(-DW_ENABLE_ASSERT)
add_definitions(-DLOGGING_ENABLED=1)

if(DEFINED W_DEBUG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--export-dynamic")
endif()

add_library(erwin SHARED
            ${SRC_ERWIN}
            ${SRC_PLATFORM}
            ${SRC_IMGUI}
            ${CMAKE_SOURCE_DIR}/source/vendor/inih/ini.c
            ${CMAKE_SOURCE_DIR}/source/vendor/inih/cpp/INIReader.cpp
            )

target_include_directories(erwin PUBLIC "${CMAKE_SOURCE_DIR}/source")
target_include_directories(erwin PUBLIC "${CMAKE_SOURCE_DIR}/source/Erwin")
target_include_directories(erwin SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor")
target_include_directories(erwin SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/glm")
target_include_directories(erwin SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/imgui")
target_include_directories(erwin SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/taskflow")
target_include_directories(erwin SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/glad/include")
target_include_directories(erwin SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/ctti/include")
target_include_directories(erwin SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/freetype/include")
target_include_directories(erwin SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/entt/src")

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
target_include_directories(erwin SYSTEM PRIVATE "${INC_EASTL}")
add_library(eastl STATIC IMPORTED)
set_target_properties(eastl PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libEASTL.a"
  INTERFACE_INCLUDE_DIRECTORIES "${INC_EASTL}"
)

target_link_libraries(erwin
                      PRIVATE
                        project_warnings
                        m
                        z
                        stdc++fs
                        pthread
                        atomic
                        X11
                        glad
                        glfw
                      )
