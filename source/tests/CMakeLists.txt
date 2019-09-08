# Defines a logging test application


set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE MinSizeRel)
# set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_definitions(-D__DEBUG__)

if(DEFINED __DEBUG__)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--export-dynamic")
endif()

# -------- LOGGER TEST -------- #

set(SRC_TEST_LOGGER
    ${SRC_LOG}
    ${SRC_MATH}
    ${SRC_CORE}
    test_logger.cpp
   )

add_executable(test_logger ${SRC_TEST_LOGGER})

target_include_directories(test_logger PRIVATE "${CMAKE_SOURCE_DIR}/source/Erwin")
target_include_directories(test_logger PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/ctti/include")

set_target_properties(test_logger
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/test/"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/test/"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/test/"
)

target_link_libraries(test_logger
                      pthread)
cotire(test_logger)


# -------- SERIALIZATION TEST -------- #


set(SRC_TEST_SERIAL
    ${SRC_LOG}
    ${SRC_MATH}
    ${SRC_CORE}
    test_serial.cpp
   )

add_executable(test_serial ${SRC_TEST_SERIAL})

target_include_directories(test_serial PRIVATE "${CMAKE_SOURCE_DIR}/source/Erwin")
target_include_directories(test_serial PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/ctti/include")
target_include_directories(test_serial PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/cereal/include")

set_target_properties(test_serial
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/test"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/test"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/test"
)

target_link_libraries(test_serial
                      pthread
                      stdc++fs
                      )
cotire(test_serial)



# -------- CONFIGURATION TEST -------- #

set(SRC_TEST_CONFIG
    ${SRC_LOG}
    ${SRC_MATH}
    ${SRC_CORE}
    test_config.cpp
   )

add_executable(test_config ${SRC_TEST_CONFIG})

target_include_directories(test_config PRIVATE "${CMAKE_SOURCE_DIR}/source/Erwin")
target_include_directories(test_config PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/ctti/include")
target_include_directories(test_config PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/cereal/include")

set_target_properties(test_config
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/test"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/test"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/test"
)

target_link_libraries(test_config
                      pthread
                      stdc++fs
                      )
cotire(test_config)