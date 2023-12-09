cmake_minimum_required(VERSION 3.18)
project(assignment2-rtp)

enable_testing()
add_subdirectory(third_party/googletest-release-1.12.1)
include_directories(third_party/googletest-release-1.12.1/googletest/include)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE "Debug")
add_compile_options("-Wall")

find_package(Threads REQUIRED)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

add_compile_definitions(SOURCE_DIR="${CMAKE_SOURCE_DIR}")

option(DL "debug logging" ON)

if(DL)
    message("Debug logging is on")
    add_compile_definitions(LDEBUG)
endif()

unset(DL CACHE)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)
include(GoogleTest)

add_library(rtp_all src/util.c)

add_library(rtp_lib
    src/error_process.cxx
    src/file_process.cxx
    src/rtp_header.cxx
    src/tools.cxx)

add_executable(sender src/sender.cxx)

add_executable(receiver src/receiver.cxx)

add_executable(rtp_test_all src/test.cpp)
target_link_libraries(rtp_test_all PUBLIC Threads::Threads GTest::gtest_main)
target_link_libraries(rtp_test_all PUBLIC rtp_all)

target_link_libraries(sender PUBLIC rtp_lib)
target_link_libraries(receiver PUBLIC rtp_lib)

gtest_discover_tests(rtp_test_all)
