cmake_minimum_required(VERSION 3.13)
project(time_at_enklave)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

### Add googletest
# TODO cmake provides a module for google test. However, it works also without for now.

# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
        ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
        EXCLUDE_FROM_ALL)


### Test that filesystem header actually is there and works.
try_compile(HAS_FS "${CMAKE_BINARY_DIR}/temp"
        "${CMAKE_SOURCE_DIR}/tests/has_filesystem.cc"
        CMAKE_FLAGS -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON)
if(HAS_FS)
    message(STATUS "Compiler has filesystem support")
else()
    message(FATAL_ERROR "Compiler is missing filesystem capabilities")
endif(HAS_FS)


### Finish
add_executable(time_at_enklave_tests tests/enklave_tests.cpp)
target_link_libraries(time_at_enklave_tests gtest_main)
add_test(time_at_enklave_tests time_at_enklave_tests)

add_executable(time_at_enklave main.cpp enklave.hpp config.hpp)
target_link_libraries(time_at_enklave)
