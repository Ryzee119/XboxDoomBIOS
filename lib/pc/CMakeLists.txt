cmake_minimum_required(VERSION 3.13)

add_library(pc INTERFACE)

file(GLOB C_SOURCES "*.c")
target_sources(pc INTERFACE ${C_SOURCES})

target_link_libraries(pc INTERFACE c)
