file(GLOB_RECURSE SRC_EDITOR "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

# ImGuizmo: Compile as an object library (with relocatable code)
add_library(imguizmo OBJECT)
target_sources(imguizmo
    PRIVATE
        "${CMAKE_SOURCE_DIR}/source/vendor/imguizmo/ImGuizmo.cpp"
    )
target_include_directories(imguizmo 
    PRIVATE 
        "${CMAKE_SOURCE_DIR}/source/vendor/imguizmo"
        "${CMAKE_SOURCE_DIR}/source/vendor/imgui"
    )
set_target_properties(imguizmo
    PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
target_link_libraries(imguizmo)

add_executable(editor)
target_sources(editor
    PRIVATE
        ${SRC_EDITOR}
    )
target_include_directories(editor
    SYSTEM PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/source"
        "${CMAKE_SOURCE_DIR}/source/vendor"
        "${CMAKE_SOURCE_DIR}/source/Erwin"
    )

set_target_properties(editor
    PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

target_link_libraries(editor
    PRIVATE
        project_warnings
        project_options
        erwin
        imguizmo
        pthread
        stdc++fs
)

cotire(editor)