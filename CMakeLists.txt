cmake_minimum_required(VERSION 3.16)
project(ErikKernel VERSION 0.1)
enable_language(C ASM)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Generic")
message(FATAL_ERROR "Please specify a compatible toolchain file. 
For example: \"cmake -DCMAKE_TOOLCHAIN_FILE=../x64.cmake ..\"")
endif()

option(DEBUG_PRINTK "Print additional debug output" CACHE)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

add_compile_options(-Wall -Wextra -pedantic -Werror)
add_compile_options(-ffreestanding)
add_compile_options(-fshort-wchar)
add_compile_options(-mno-red-zone)
add_compile_options(-Wno-unused-variable)
add_compile_options(-Wno-unused-command-line-argument)
add_compile_options(-Wno-c11-extensions)
add_compile_options(-Wno-c99-extensions)
add_compile_options(-Wno-c2x-extensions)
add_compile_options(-Wno-language-extension-token)
add_compile_options(-Wno-writable-strings)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${CMAKE_SOURCE_DIR}/src/linker.ld")

add_executable(KERNEL.ERIK
    src/acpi.c
    src/fs.c
    src/heap.c
    src/main.c
    src/memory.c
    src/serial.c
    ${ARCH_SOURCES}
)

if(DEBUG_PRINTK)
    target_sources(KERNEL.ERIK PRIVATE src/debug.c)
    target_compile_definitions(KERNEL.ERIK PRIVATE DEBUG_PRINTK)
endif()

if(AARCH64_QEMU_UART)
    target_sources(KERNEL.ERIK PRIVATE src/arch/aarch64/pl011.c)
    target_compile_definitions(KERNEL.ERIK PRIVATE AARCH64_QEMU_UART)
endif()

if(X64_UART)
    target_sources(KERNEL.ERIK PRIVATE src/arch/x86_64/serial.c)
    target_compile_definitions(KERNEL.ERIK PRIVATE X64_UART)
endif()

target_include_directories(KERNEL.ERIK PUBLIC 
    include)
