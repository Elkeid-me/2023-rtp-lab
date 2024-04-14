cmake_minimum_required(VERSION 3.22)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(rtp-lab)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE "Release")
add_compile_options("-Wall")

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
