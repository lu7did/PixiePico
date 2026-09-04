# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD waveshare_rp2040_zero CACHE STRING "Target board")

include(pico_sdk_import.cmake)

project(testDDS C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

add_executable(testDDS
    src/main.c
    src/dds.c
    src/rotary_encoder.c
    src/ssd1306.c
)

pico_generate_pio_header(testDDS
    ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio
)

pico_generate_pio_header(testDDS
    ${CMAKE_CURRENT_LIST_DIR}/src/dds_square.pio
)

target_include_directories(testDDS PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(testDDS
    pico_stdlib
    hardware_clocks
    hardware_dma
    hardware_i2c
    hardware_irq
    hardware_pio
    hardware_sync
    hardware_vreg
    hardware_watchdog
)

pico_enable_stdio_usb(testDDS 1)
pico_enable_stdio_uart(testDDS 0)

pico_add_extra_outputs(testDDS)
