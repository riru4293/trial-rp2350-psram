cmake_minimum_required( VERSION 3.12 )

# C/C++ standard to use
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Make a compile_commands.json on cmake
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Variables
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(PICO_SDK_PATH ${LIB_DIR}/pico-sdk)

#set(PICO_BOARD pico CACHE STRING
set(PICO_BOARD pimoroni_pico_plus2_w_rp2350 CACHE STRING
    "Target board definition")

set(FREERTOS_KERNEL_PATH ${LIB_DIR}/FreeRTOS-Kernel CACHE STRING
    "FreeRTOS kernel")
set(FREERTOS_PORT_DIR ${LIB_DIR}/FreeRTOS-Port)

include(${FREERTOS_PORT_DIR}/pico_sdk_import.cmake)
include(${FREERTOS_PORT_DIR}/FreeRTOS_Kernel_import.cmake)

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config
    INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

project(TrialPsram VERSION 1.0.0 LANGUAGES C CXX ASM
    DESCRIPTION "Trial using PSRAM")

pico_sdk_init()

add_executable(trial
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pal_critical_section.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pal_reset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pal_psram_arena.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pal_heap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mw_heap_repo.cpp
)

target_include_directories(trial
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(trial
PRIVATE
    pico_stdlib
    hardware_psram
    pico_cyw43_arch_none
    freertos_config
    pico_async_context_freertos
    FreeRTOS-Kernel-Heap4
)

pico_enable_stdio_uart(trial TRUE)
pico_enable_stdio_usb(trial FALSE)

pico_add_extra_outputs(trial)

### ---------- ---------- ---------- ----------
### Compile options

add_compile_options(
    -Wall
    -Wextra
    -O2
    -Wno-format
    -Wno-unused-function
)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-maybe-uninitialized)
endif()
