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

project(PixiePico C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

add_executable(PixiePico
    src/main.c
    src/ddsvco.c
    src/rotary_encoder.c
    src/ssd1306.c
    src/usb_audio.c
    src/usb_descriptors.c
)

pico_generate_pio_header(PixiePico
    ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio
)

pico_generate_pio_header(PixiePico
    ${CMAKE_CURRENT_LIST_DIR}/src/ddsvco_square.pio
)

target_include_directories(PixiePico PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(PixiePico
    pico_stdlib
    hardware_clocks
    hardware_i2c
    hardware_irq
    hardware_pio
    hardware_pll
    hardware_sync
    hardware_vreg
    tinyusb_device
    tinyusb_board
    hardware_adc

    hardware_watchdog
)

pico_enable_stdio_usb(PixiePico 0)
pico_enable_stdio_uart(PixiePico 0)

pico_add_extra_outputs(PixiePico)
