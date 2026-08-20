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
# Default FAMILY si no se pasó por -D
if(NOT DEFINED FAMILY)
  set(FAMILY "rp2040" CACHE STRING "TinyUSB family" FORCE)
endif()

# Default PICO_SDK_PATH si no se pasó por -D ni por env
if(NOT DEFINED PICO_SDK_PATH)
  if(DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}" CACHE PATH "Path to Pico SDK")
  else()
    set(PICO_SDK_PATH "/Users/PCOLLA/Documents/GitHub/pico/pico-sdk" CACHE PATH "Path to Pico SDK")
  endif()
endif()

# ============================================================================
# Si no viene definido desde afuera, usar el SDK "de la empresa" (tu ruta)
if (NOT DEFINED PICO_SDK_PATH)
  set(PICO_SDK_PATH "/Users/PCOLLA/Documents/GitHub/pico/pico-sdk" CACHE PATH "Pico SDK Path")
endif()
# ============================================================================

set(CMAKE_C_COMPILER "/opt/homebrew/bin/arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "/opt/homebrew/bin/arm-none-eabi-gcc")

set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.5)
include(/Users/PCOLLA/Documents/GitHub/pico/pico-sdk/lib/tinyusb/hw/bsp/family_support.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROJECT testUSB2)
set(TARGET testUSB2)

cmake_minimum_required(VERSION 3.12)

#include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
#pico_sdk_init()
include(${PICO_SDK_PATH}/lib/tinyusb/hw/bsp/family_support.cmake)

project(${PROJECT} C CXX ASM)
#pico_sdk_init()
add_executable(${PROJECT} src/main.c)

# gets PROJECT name for the example (e.g. <BOARD>-<DIR_NAME>)
family_get_project_name(${PROJECT} ${CMAKE_CURRENT_LIST_DIR})

project(${PROJECT})

# Checks this example is valid for the family and initializes the project
family_initialize_project(${PROJECT} ${CMAKE_CURRENT_LIST_DIR})

# Make ws2812.pio.h headter file for Neopixel
#pico_generate_pio_header(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

# Example source
target_sources(${PROJECT} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_descriptors.c
        #${CMAKE_CURRENT_SOURCE_DIR}/src/si5351.c 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_audio.c 
        )

# Example include
#target_include_directories(${PROJECT} PUBLIC
#         ${CMAKE_CURRENT_SOURCE_DIR}/src
#         ${PICO_SDK_PATH}/lib/tinyusb/hw
#        )

target_include_directories(testUSB2 PRIVATE
        ${PICO_SDK_PATH}/lib/tinyusb/hw
        ${PICO_SDK_PATH}/lib/tinyusb/src
        ${CMAKE_CURRENT_LIST_DIR}/src
)

if (NOT EXISTS "${PICO_SDK_PATH}/lib/tinyusb/hw/bsp/rp2040/board.h")
  message(FATAL_ERROR "No se encuentra ${PICO_SDK_PATH}/lib/tinyusb/hw/bsp/rp2040/board.h")
endif()




target_link_libraries(${PROJECT} PUBLIC
        pico_stdlib
        tinyusb_device
        tinyusb_board
        hardware_pio
        hardware_adc
        hardware_i2c
        )



pico_add_extra_outputs(${PROJECT})

# Configure compilation flags and libraries for the example... see the corresponding function
# in hw/bsp/FAMILY/family.cmake for details.
family_configure_device_example(${PROJECT} noos)


