cmake_minimum_required(VERSION 3.13)

# Exporta compile_commands.json (para IntelliSense)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- SDK path (igual a lo tuyo, pero sin duplicados) ----
if(NOT DEFINED PICO_SDK_PATH)
  if(DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}" CACHE PATH "Path to Pico SDK")
  else()
    set(PICO_SDK_PATH "/Users/PCOLLA/Documents/GitHub/pico/pico-sdk" CACHE PATH "Path to Pico SDK")
  endif()
endif()

# ---- Board ----
set(PICO_BOARD pico CACHE STRING "Board type")  # pico_w si corresponde

# ---- TinyUSB family (solo si realmente lo necesitás) ----
if(NOT DEFINED FAMILY)
  set(FAMILY "rp2040" CACHE STRING "TinyUSB family" FORCE)
endif()

# Pull in Pico SDK
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# TinyUSB family support (una sola vez)
include(${PICO_SDK_PATH}/lib/tinyusb/hw/bsp/family_support.cmake)

project(ADX-ddsPIO C CXX ASM)

# Importante: NO llamás pico_sdk_init() porque family_initialize_project lo hace
family_initialize_project(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR})

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
  ${CMAKE_CURRENT_LIST_DIR}/src/usb_descriptors.c
  ${CMAKE_CURRENT_LIST_DIR}/src/piodco.c
  ${CMAKE_CURRENT_LIST_DIR}/src/si4732.c
  ${CMAKE_CURRENT_LIST_DIR}/src/usb_audio.c
  ${CMAKE_CURRENT_LIST_DIR}/src/lib/assert.c
  ${CMAKE_CURRENT_LIST_DIR}/src/EEPROM.c
  ${CMAKE_CURRENT_LIST_DIR}/src/quad.c
  ${CMAKE_CURRENT_LIST_DIR}/src/usb_msc_ramdisk.c
  ${CMAKE_CURRENT_LIST_DIR}/src/si4732_patch_ssb.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/src
  ${PICO_SDK_PATH}/lib/tinyusb/src
  ${PICO_SDK_PATH}/lib/tinyusb/hw
)

target_compile_options(${PROJECT_NAME} PRIVATE
  -Wall -Wextra -Werror -Wsign-conversion -Wconversion
)

if (PICOW)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    pico_stdlib
    tinyusb_device
    tinyusb_board
    pico_multicore
    hardware_pio
    hardware_adc
    hardware_i2c
    hardware_gpio
    hardware_rtc
    hardware_watchdog
    hardware_flash
    hardware_sync
    pico_cyw43_arch_lwip_poll
  )
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE
    pico_stdlib
    tinyusb_device
    tinyusb_board
    pico_multicore
    hardware_pio
    hardware_adc
    hardware_i2c
    hardware_gpio
    hardware_rtc
    hardware_watchdog
    hardware_flash
    hardware_sync
    hardware_clocks
    hardware_pll
  )
endif()

pico_add_extra_outputs(${PROJECT_NAME})

# Configuración TinyUSB ejemplo
family_configure_device_example(${PROJECT_NAME} noos)

# PIO headers
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/BFO.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/dco2.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/quad.pio)
