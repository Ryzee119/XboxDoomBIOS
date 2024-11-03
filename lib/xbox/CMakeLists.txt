cmake_minimum_required(VERSION 3.13)

add_library(xbox INTERFACE)

file(GLOB C_SOURCES "*.c")
file(GLOB ASM_SOURCES "*.nasm")
target_sources(xbox INTERFACE ${C_SOURCES} ${ASM_SOURCES})

target_include_directories(xbox INTERFACE
 ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(xbox INTERFACE c)
