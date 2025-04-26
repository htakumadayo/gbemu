cmake_minimum_required(VERSION 3.15)

project(GBEMU VERSION 0.0 DESCRIPTION "A GameBoy emulator" LANGUAGES C)

include_directories(include)

add_library(queue STATIC src/misc/queue.c)

set(CMAKE_C_FLAGS "-g")
add_executable(emu src/main.c src/chead.c)
target_link_libraries(emu PUBLIC queue)
