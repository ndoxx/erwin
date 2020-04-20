file(GLOB_RECURSE SRC_ERWIN    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE SRC_PLATFORM "${CMAKE_CURRENT_SOURCE_DIR}/../platform/*.cpp")

set(INC_EASTL
    "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/eastl/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/eastl"
    "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/eastl/test/packages/EAAssert/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/eastl/test/packages/EABase/include/Common"
    "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/eastl/test/packages/EAMain/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/eastl/test/packages/EAStdC/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/eastl/test/packages/EATest/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/eastl/test/packages/EAThread/include"
    )

# Disable all warnings for third party stuff
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/asset/stb_build.cpp" PROPERTIES COMPILE_FLAGS -w)
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/render/stb_build.cpp" PROPERTIES COMPILE_FLAGS -w)
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/core/z_wrapper.cpp" PROPERTIES COMPILE_FLAGS -w)

if(DEFINED W_DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--export-dynamic")
endif()

add_library(erwin SHARED)
target_sources(erwin
    PRIVATE
        ${SRC_ERWIN}
        ${SRC_PLATFORM}
    )

target_include_directories(erwin
    PRIVATE
        "${CMAKE_SOURCE_DIR}/source"
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )
target_include_directories(erwin
    SYSTEM PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../vendor"
        "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/taskflow"
        "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/glad/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/glfw/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/freetype/include"
    SYSTEM PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/glm"
        "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui"
        "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/ctti/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/entt/src"
        "${INC_EASTL}"
    )

set_target_properties(erwin
    PROPERTIES
        VERSION                  ${PROJECT_VERSION}
        SOVERSION                1
        PUBLIC_HEADER            "${CMAKE_SOURCE_DIR}/source/erwin.h"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    )
target_compile_definitions(erwin PRIVATE W_BUILD_LIB=1)

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

# GLFW library as a static import
add_library(glfw STATIC IMPORTED)
set_target_properties(glfw
    PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libglfw3.a"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/source/vendor/glfw"
    )

# ImGui: Compile as an object library (with relocatable code)
add_library(imgui OBJECT)
target_sources(imgui
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imgui.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imgui_demo.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imgui_draw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imgui_widgets.cpp"
    )
set_target_properties(imgui
    PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
target_link_libraries(imgui)

# EASTL
set_target_properties(eastl
    PROPERTIES
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
        imgui
    )

cotire(erwin)
