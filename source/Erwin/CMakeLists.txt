
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

target_include_directories(erwin SYSTEM PUBLIC "${INC_EASTL}")

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
