add_executable(internstr)
target_sources(internstr
    PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/source/internstr.cpp"
    )
set_target_properties(internstr
    PROPERTIES
    	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
target_include_directories(internstr
	PUBLIC
		"${CMAKE_SOURCE_DIR}/source/Erwin"
	)

target_link_libraries(internstr
	PRIVATE
		stdc++fs
	)

cotire(internstr)