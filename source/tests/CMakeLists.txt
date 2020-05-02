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
                      erwin
                      pthread
                      eastl)
cotire(test_logger)


# -------- SERIALIZATION TEST -------- #


set(SRC_TEST_SERIAL
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
                      erwin
                      pthread
                      stdc++fs
                      eastl
                      )
cotire(test_serial)


# -------- ENGINE TEST -------- #

set(SRC_ENGINE_TEST
    catch_app.cpp
    test_memory.cpp
    test_async.cpp
    # test_ecs.cpp
    test_rng.cpp
    test_event.cpp
    # test_jobs.cpp
   )

add_executable(test_erwin ${SRC_ENGINE_TEST})

target_include_directories(test_erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/Erwin")
target_include_directories(test_erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor")
target_include_directories(test_erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/glm")
target_include_directories(test_erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/ctti/include")
target_include_directories(test_erwin PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/Catch2/single_include")

set_target_properties(test_erwin
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/test"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/test"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/test"
)

target_link_libraries(test_erwin
                      erwin
                      pthread
                      stdc++fs
                      eastl
                      )
cotire(test_erwin)

# -------- NUCLEAR TEST -------- #

set(SRC_NUCLEAR
    nuclear.cpp
   )

add_executable(nuclear ${SRC_NUCLEAR})

target_include_directories(nuclear PRIVATE "${CMAKE_SOURCE_DIR}/source/Erwin")
target_include_directories(nuclear SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor")
target_include_directories(nuclear SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/glm")
target_include_directories(nuclear SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/ctti/include")
target_include_directories(nuclear SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/cereal/include")
target_include_directories(nuclear SYSTEM PRIVATE "${INC_EASTL}")

set_target_properties(nuclear
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

target_link_libraries(nuclear
                      erwin
                      pthread
                      stdc++fs
                      eastl
                      )
cotire(nuclear)