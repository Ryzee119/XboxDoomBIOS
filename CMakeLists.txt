set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/toolchain-xboxbios.cmake)
cmake_minimum_required (VERSION 3.13)
cmake_policy(SET CMP0079 NEW)
cmake_policy(SET CMP0077 NEW)

find_package (Python3 REQUIRED COMPONENTS Interpreter)

set(PROJECT_NAME "visor")
set(OUTPUT_NAME "rom.elf")
set(CMAKE_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

project(${PROJECT_NAME} C ASM ASM_NASM)
add_executable(${OUTPUT_NAME})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -ffunction-sections -fdata-sections -fomit-frame-pointer")

# picolibc
#set(_FORMAT_DEFAULT_INTEGER ON) #Smaller, no float support
set(_FORMAT_DEFAULT_DOUBLE ON)
add_compile_definitions(_WANT_FAST_BUFIO)
#add_compile_definitions(PREFER_SIZE_OVER_SPEED) # Doesnt make a big difference
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/picolibc)

#arith64 - prevents having to link libgcc for a couple functions
add_library(arith64 INTERFACE)
set(ARITH64_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/arith64)
target_sources(arith64 INTERFACE ${ARITH64_DIR}/arith64.c)

# FreeRTOS
set(FREERTOS_PORT "GCC_IA32_FLAT")
set(FREERTOS_HEAP "3") # 3=Wraps malloc/free
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM INTERFACE config_freertos)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/freertos_kernel freertos_kernel)

# TinyUSB
add_library(tinyusb_config INTERFACE)
target_compile_definitions(tinyusb_config INTERFACE TUH_OPT_RHPORT=0 CFG_TUSB_MCU=0)
target_include_directories(tinyusb_config SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/config_tinyusb)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb/src tinyusb)
target_compile_options(tinyusb PRIVATE -Wno-unreachable-code)
target_sources(tinyusb PRIVATE ${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb/src/portable/ohci/ohci.c)
target_link_libraries(tinyusb PRIVATE c freertos_kernel)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/tusb_xinput xinput_host)

# FATFS
set(FATFS_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/fatfs)
add_library(fatfs INTERFACE)
target_include_directories(fatfs PRIVATE INTERFACE ${CMAKE_CURRENT_LIST_DIR}/config_fatfs)
target_sources(fatfs INTERFACE ${FATFS_DIR}/ff.c ${FATFS_DIR}/ffunicode.c ${CMAKE_CURRENT_LIST_DIR}/config_fatfs/ffsystem.c)

# FATX
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/libfatx/)
add_library(fatfx INTERFACE)
list(REMOVE_ITEM FATX_SOURCES ${CMAKE_CURRENT_LIST_DIR}/lib/libfatx/fatx_dev.c)
target_sources(fatfx INTERFACE ${FATX_SOURCES})

#ISO9660
set(ISO9660_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/lib9660)
add_library(iso9660 INTERFACE)
target_sources(iso9660 INTERFACE ${ISO9660_DIR}/lib9660.c)

#Midi
add_library(midi INTERFACE)
target_sources(midi INTERFACE ${CMAKE_CURRENT_LIST_DIR}/lib/midi/midiplay.c ${CMAKE_CURRENT_LIST_DIR}/lib/midi/synth.c)

# Build our xbox specific code
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/xbox xbox)

# Generic PC code
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/pc pc)

# Link the libraries
target_link_libraries(${OUTPUT_NAME} PRIVATE c arith64 freertos_kernel pc xbox tinyusb xinput_host fatfs fatfx iso9660 midi)

# Create a binary from the elf
add_custom_command(TARGET ${OUTPUT_NAME} POST_BUILD
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/calculate_usage.py ${CMAKE_BINARY_DIR}/${OUTPUT_NAME}

    # Copy the original elf for debugging
    COMMAND cp ${CMAKE_BINARY_DIR}/${OUTPUT_NAME} ${CMAKE_BINARY_DIR}/original.elf

    COMMAND echo "Compressing ROM..."
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/compress_rom.py
                                 ${CMAKE_BINARY_DIR}/${OUTPUT_NAME} ${CMAKE_BINARY_DIR}/compressed.bin

    # Push the compressed blob back into the elf
    COMMAND ${CMAKE_OBJCOPY} --update-section .compressed=${CMAKE_BINARY_DIR}/compressed.bin ${OUTPUT_NAME}

    # These sections are now compressed, so we can remove them from the final binary as they are decompressed at runtime
    COMMAND ${CMAKE_OBJCOPY} --remove-section=.text --remove-section=.data --remove-section=.rodata ${OUTPUT_NAME}

    # Calculate the final size ROM usage
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/calculate_usage.py ${CMAKE_BINARY_DIR}/${OUTPUT_NAME}

    # Convert the elf to a binary 0x100000000 is 256kB from ROM base which makes 256Kb output size
    COMMAND ${CMAKE_OBJCOPY} -O binary --pad-to 0x100000000 --gap-fill 0xFF ${OUTPUT_NAME} ${CMAKE_BINARY_DIR}/rom.bin
)

#add_custom_target(apply_patch ALL
#    COMMAND git apply ${CMAKE_SOURCE_DIR}/config_freertos/pic_ack.patch
#    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/lib/freertos_kernel
#)
#add_dependencies(${OUTPUT_NAME} apply_patch)

# Include directories
target_include_directories(${OUTPUT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/lib
)

file(GLOB C_SOURCES ${CMAKE_SOURCE_DIR}/*.c ${CMAKE_SOURCE_DIR}/fs_io/*.c)

# Source files
target_sources(${OUTPUT_NAME} PRIVATE
    config_freertos/irq.S
    config_freertos/irq.c
    ${C_SOURCES}
)
