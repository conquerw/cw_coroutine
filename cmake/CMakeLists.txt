cmake_minimum_required(VERSION 3.5)

project(cw_coroutine)

include_directories(../src
					../sample)
					
aux_source_directory(../src SRC)
aux_source_directory(../sample DEMO)

set(CMAKE_C_COMPILER ${CC})
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-g -std=gnu11 -Wall")
add_definitions("-D_GNU_SOURCE")
message(STATUS "CMAKE_C_COMPILER = " ${CMAKE_C_COMPILER})
message(STATUS "CMAKE_C_FLAGS = " ${CMAKE_C_FLAGS})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../output)

add_executable(cw_coroutine ${SRC} ${DEMO})
target_link_libraries(cw_coroutine dl pthread)
