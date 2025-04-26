cmake_minimum_required(VERSION 3.15)
project(GBEMU VERSION 0.0 DESCRIPTION "Test" LANGUAGES C)
include(CTest)
include_directories("../include")
set(CMAKE_C_FLAGS "-g")
add_library(queue STATIC ../src/misc/queue.c)


add_executable(queuetest queuetest.c)
target_link_libraries(queuetest PUBLIC queue)

add_test(NAME QueueTest COMMAND queuetest)

