cmake_minimum_required(VERSION 3.12)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT PICO_SDK_PATH)
  set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif()

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(adx_usb_composite_fatfs C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(adx_usb_composite_fatfs
  main.c
  fs.c
  flash_bd.c
  usb_descriptors.c
  usb_msc.c
  fatfs/fattime.c
  fatfs/ff.c
  fatfs/diskio.c
  lib/jsmn/jsmn.c
)

target_include_directories(adx_usb_composite_fatfs PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}          # <-- donde está tusb_config.h
  ${CMAKE_CURRENT_LIST_DIR}/fatfs
  ${CMAKE_CURRENT_LIST_DIR}/lib/jsmn
)

# Fuerza a TinyUSB a usar este archivo (sin redefinir macros)
target_compile_definitions(adx_usb_composite_fatfs PRIVATE
  CFG_TUSB_CONFIG_FILE="tusb_config.h"
)

target_link_libraries(adx_usb_composite_fatfs
  pico_stdlib
  hardware_flash
  hardware_sync
  hardware_clocks
  tinyusb_device
  tinyusb_board
  hardware_rtc
  hardware_watchdog

)

pico_enable_stdio_usb(adx_usb_composite_fatfs 0)
pico_enable_stdio_uart(adx_usb_composite_fatfs 1)

pico_add_extra_outputs(adx_usb_composite_fatfs)
