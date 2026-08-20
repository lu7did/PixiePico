# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(CMAKE_C_COMPILER "/opt/homebrew/bin/arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "/opt/homebrew/bin/arm-none-eabi-gcc")


cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(testUSB C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(testUSB
    src/main.c
    src/usb_descriptors.c
)

target_include_directories(testUSB PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# TinyUSB device stack (incluye USB audio)
target_link_libraries(testUSB
    pico_stdlib
    tinyusb_device
    tinyusb_board
    m
)

# Importante: NO uses stdio por USB si querés que el USB sea "placa de sonido".
# Usá UART para logs.
pico_enable_stdio_usb(testUSB 0)
pico_enable_stdio_uart(testUSB 1)

pico_add_extra_outputs(testUSB)

