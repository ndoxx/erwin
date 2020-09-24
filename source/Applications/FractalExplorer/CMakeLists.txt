add_executable(fractal)
target_sources(fractal
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/source/fractal_explorer.cpp"
    )
target_include_directories(fractal
    PRIVATE
        "${CMAKE_SOURCE_DIR}/source/Erwin"
        "${CMAKE_SOURCE_DIR}/source/vendor"
    )
set_target_properties(fractal
    PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

target_link_libraries(fractal
    PRIVATE
		erwin
        pthread
        stdc++fs
    )

cotire(fractal)