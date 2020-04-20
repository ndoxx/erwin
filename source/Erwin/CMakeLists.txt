
file(GLOB_RECURSE SRC_ERWIN    "${CMAKE_SOURCE_DIR}/source/Erwin/*.cpp")
file(GLOB_RECURSE SRC_PLATFORM "${CMAKE_SOURCE_DIR}/source/platform/*.cpp")

set(SRC_IMGUI
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui_widgets.cpp
    )

set(INC_EASTL
    ${CMAKE_SOURCE_DIR}/source/vendor/eastl/include
    ${CMAKE_SOURCE_DIR}/source/vendor/eastl
    ${CMAKE_SOURCE_DIR}/source/vendor/eastl/test/packages/EAAssert/include
    ${CMAKE_SOURCE_DIR}/source/vendor/eastl/test/packages/EABase/include/Common
    ${CMAKE_SOURCE_DIR}/source/vendor/eastl/test/packages/EAMain/include
    ${CMAKE_SOURCE_DIR}/source/vendor/eastl/test/packages/EAStdC/include
    ${CMAKE_SOURCE_DIR}/source/vendor/eastl/test/packages/EATest/include
    ${CMAKE_SOURCE_DIR}/source/vendor/eastl/test/packages/EAThread/include
    )

# Disable all warnings for third party stuff
set_source_files_properties("${CMAKE_SOURCE_DIR}/source/Erwin/asset/stb_build.cpp" PROPERTIES COMPILE_FLAGS -w)
set_source_files_properties("${CMAKE_SOURCE_DIR}/source/Erwin/render/stb_build.cpp" PROPERTIES COMPILE_FLAGS -w)
set_source_files_properties("${CMAKE_SOURCE_DIR}/source/Erwin/core/z_wrapper.cpp" PROPERTIES COMPILE_FLAGS -w)
set_source_files_properties(${SRC_IMGUI} PROPERTIES COMPILE_FLAGS -w)

if(DEFINED W_DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--export-dynamic")
endif()

add_library(erwin 
    SHARED
        ${SRC_ERWIN}
        ${SRC_PLATFORM}
        ${SRC_IMGUI}
    )

target_include_directories(erwin
    PRIVATE
        "${CMAKE_SOURCE_DIR}/source"
        "${CMAKE_SOURCE_DIR}/source/Erwin"
    )
target_include_directories(erwin
    SYSTEM PRIVATE
        "${CMAKE_SOURCE_DIR}/source/vendor"
        "${CMAKE_SOURCE_DIR}/source/vendor/glm"
        "${CMAKE_SOURCE_DIR}/source/vendor/imgui"
        "${CMAKE_SOURCE_DIR}/source/vendor/taskflow"
        "${CMAKE_SOURCE_DIR}/source/vendor/glad/include"
        "${CMAKE_SOURCE_DIR}/source/vendor/glfw/include"
        "${CMAKE_SOURCE_DIR}/source/vendor/ctti/include"
        "${CMAKE_SOURCE_DIR}/source/vendor/freetype/include"
        "${CMAKE_SOURCE_DIR}/source/vendor/entt/src"
    )
target_include_directories(erwin SYSTEM PUBLIC "${INC_EASTL}")

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

set_target_properties(eastl
    PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libEASTL.a"
        INTERFACE_INCLUDE_DIRECTORIES "${INC_EASTL}"
        POSITION_INDEPENDENT_CODE ON
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

cotire(erwin)
