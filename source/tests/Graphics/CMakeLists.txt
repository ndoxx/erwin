function(make_gfx_test_app app_name)
    set(NAME "test_${app_name}")

    add_executable(${NAME} 
        "${CMAKE_CURRENT_SOURCE_DIR}/common/test_application.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/${app_name}.cpp"
        )
    target_include_directories(${NAME}
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_SOURCE_DIR}/source/Graphics"
        )
    target_compile_definitions(${NAME} PRIVATE -DLOGGING_ENABLED=1)

    set_target_properties(${NAME}
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/test/gfx"
            OUTPUT_NAME "${app_name}"
            EXCLUDE_FROM_ALL 1
        )

    target_link_libraries(${NAME}
        PRIVATE
            project_options
            project_warnings
            pthread
            stdc++fs
            dl
            kibble
            erwin_graphics
        )

    cotire(${NAME})
endfunction()

add_custom_target(gfx_tests)
# Loop over all cpp files in the graphics tests directory
file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
foreach(FILE ${TEST_SOURCES})
    # Extract file name without extension and use this to declare a new target
    get_filename_component(TEST_NAME ${FILE} NAME_WE)
    make_gfx_test_app(${TEST_NAME})
    # Add dependency to the group target gfx_tests,
    # then we can do "make gfx_tests" to build all of them
    add_dependencies(gfx_tests "test_${TEST_NAME}")
    message(STATUS "Added graphics test: ${TEST_NAME}")
endforeach()
message(STATUS "All graphics tests can be built at once with 'make gfx_tests'")
