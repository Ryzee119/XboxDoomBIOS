set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR i686)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_ASM_COMPILER gcc) # Ugh FreeRTOS asm needs gcc
set(CMAKE_ASM_NASM_COMPILER nasm)
set(CMAKE_OBJCOPY objcopy)

set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES ${PROJECT_BINARY_DIR}/lib/picolibc/picolibc/include)

set(CMAKE_C_FLAGS_INIT "\
    -target i386-elf -m32 -march=pentium \
    -mfpmath=sse -msse -mmmx \
    -ffreestanding -nostdlib -static -fno-stack-protector \
" CACHE STRING "" FORCE)

set(CMAKE_CXX_FLAGS_INIT ${CMAKE_C_FLAGS_INIT})

set(CMAKE_ASM_FLAGS_INIT "-m32 -march=pentium")

set(CMAKE_C_BYTE_ORDER LITTLE_ENDIAN)
set(CMAKE_CXX_BYTE_ORDER LITTLE_ENDIAN)

# NASM elf32 i386. Note we set CMAKE_ASM_NASM_COMPILE_OBJECT otherwise it doesnt compile elf32 properly. FIXME?
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_FLAGS "-I${CMAKE_SOURCE_DIR}")
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <DEFINES> \
    <INCLUDES> <FLAGS> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

set(CMAKE_EXE_LINKER_FLAGS "-T${CMAKE_CURRENT_LIST_DIR}/rom.ld -Wl,--gc-sections -Wl,--build-id=none -target i386-elf")
