cmake_minimum_required(VERSION 3.18)
project(assignment2-rtp)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE "Release")
add_compile_options("-Wall")

find_package(Threads REQUIRED)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

add_library(rtp_lib
    src/error_process.cxx
    src/file_process.cxx
    src/rtp_header.cxx
    src/tools.cxx
    src/socket_process.cxx)

add_executable(sender src/sender.cxx)
add_executable(receiver src/receiver.cxx)

target_link_libraries(sender PUBLIC rtp_lib)
target_link_libraries(receiver PUBLIC rtp_lib)
