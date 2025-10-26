cmake_minimum_required(VERSION 3.15)

project(GBEMU VERSION 0.0 DESCRIPTION "A GameBoy emulator" LANGUAGES C)

# CHOOSE CPU BACKEND (either "naive" or "mi")
set(CPU_BACKEND "naive")
# COMPILE FLAGS
set(CMAKE_C_FLAGS "-g")
# debug mode: print register and memory
set(DEBUG_MODE 0)


# ===================
# +++++++++++++++++++
# ===================


include_directories(include)
find_package(SDL3 REQUIRED)

#include_directories(${SDL3_INCLUDE_DIRS})

add_executable(emu src/main.c src/chead.c src/SM83.c src/PPUbackend/sdl.c src/serial.c src/timer.c src/joypadbackend/sdl.c)

if(DEBUG_MODE)
    add_compile_definitions(CPU_DEBUG)
endif(DEBUG_MODE)

if(CPU_BACKEND MATCHES "mi")
    message("Compiling with microinstruction version of cpu")
    add_library(queue STATIC src/misc/queue.c)
    target_link_libraries(emu PUBLIC queue)
    target_sources(emu PUBLIC src/SM83backends/MI.c)
else(CPU_BACKEND MATCHES "naive")
    message("Compiling with naive version of cpu")   
    target_sources(emu PUBLIC src/SM83backends/naive.c)
endif(CPU_BACKEND MATCHES "mi")

target_link_libraries(emu PUBLIC SDL3::SDL3)
