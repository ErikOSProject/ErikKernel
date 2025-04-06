cmake_minimum_required(VERSION 3.16)
project(ErikKernel VERSION 0.1)
enable_language(C ASM)

# You can pass -DTARGET_ARCH=<arch> to select an architecture
if(NOT DEFINED TARGET_ARCH)
    set(TARGET_ARCH "x86_64") # Default architecture
endif()

include(${TARGET_ARCH}.cmake OPTIONAL)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Generic")
message(FATAL_ERROR "Please specify a compatible toolchain file. 
For example: \"cmake -DCMAKE_TOOLCHAIN_FILE=../x64.cmake ..\"")
endif()

option(DEBUG_PRINTK "Print additional debug output" CACHE)

add_executable(KERNEL.ERIK
    src/acpi.c
    src/elf.c
    src/fs.c
    src/heap.c
    src/list.c
    src/main.c
    src/memory.c
    src/serial.c
    ${ARCH_SOURCES}
)

target_compile_options(KERNEL.ERIK PRIVATE 
    -Wall -Wextra -pedantic -Werror
    -ffreestanding
    -fshort-wchar
    -mno-red-zone
    -Wno-unused-variable
    -Wno-unused-command-line-argument
    -Wno-c11-extensions
    -Wno-c99-extensions
    -Wno-c2x-extensions
    -Wno-language-extension-token
    -Wno-writable-strings)

target_link_options(KERNEL.ERIK PRIVATE
    -nostdlib
    -T ${CMAKE_CURRENT_SOURCE_DIR}/src/linker.ld)

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

install(TARGETS KERNEL.ERIK)
