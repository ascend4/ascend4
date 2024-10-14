cmake_minimum_required(VERSION 3.5.0)
project(mem-test3 VERSION 0.1.0 LANGUAGES C)

add_library(memptr lib-1.c)
add_executable(mem-test3 main.c)
target_link_libraries(mem-test3 memptr)

