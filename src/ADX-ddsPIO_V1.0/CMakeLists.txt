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
option(PICOW "Support Raspberry Pi Pico Wireless" OFF)

set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.5)
include(/Users/PCOLLA/Documents/GitHub/pico/pico-sdk/lib/tinyusb/hw/bsp/family_support.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROJECT ADX-ddsPIO)
set(TARGET ADX-ddsPIO)

cmake_minimum_required(VERSION 3.12)

#*---- Habilitado como recomendación para compilar picoW
#include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
#pico_sdk_init()
#*---- Habilitado para recomendación para compilar picoW



include(${PICO_SDK_PATH}/lib/tinyusb/hw/bsp/family_support.cmake)

project(${PROJECT} C CXX ASM)
#pico_sdk_init()
add_executable(${PROJECT} src/main.c)

# gets PROJECT name for the example (e.g. <BOARD>-<DIR_NAME>)
family_get_project_name(${PROJECT} ${CMAKE_CURRENT_LIST_DIR})

project(${PROJECT})

# Checks this example is valid for the family and initializes the project
family_initialize_project(${PROJECT} ${CMAKE_CURRENT_LIST_DIR})

target_compile_options(${PROJECT} PRIVATE
    -Wall -Wextra
    -Werror
    -Wsign-conversion
    -Wconversion
)

# No tratar warnings de lwIP como errores (terceros)
if (TARGET pico_lwip)
    target_compile_options(pico_lwip INTERFACE -Wno-error -Wno-sign-conversion -Wno-conversion)
endif()
if (TARGET pico_lwip_nosys)
    target_compile_options(pico_lwip_nosys INTERFACE -Wno-error -Wno-sign-conversion -Wno-conversion)
endif()



# Example source
target_sources(${PROJECT} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_descriptors.c
        ${CMAKE_CURRENT_LIST_DIR}/src/piodco.c
        ${CMAKE_CURRENT_LIST_DIR}/src/lib/assert.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_audio.c 
        )

# Example include
#target_include_directories(${PROJECT} PUBLIC
#         ${CMAKE_CURRENT_SOURCE_DIR}/src
#         ${PICO_SDK_PATH}/lib/tinyusb/hw
#        )

#target_include_directories(ADX-ddsPIO PRIVATE
target_include_directories(ADX-ddsPIO PUBLIC
        ${PICO_SDK_PATH}/lib/tinyusb/hw
        ${PICO_SDK_PATH}/lib/tinyusb/src
        ${CMAKE_CURRENT_LIST_DIR}/src
)

if (NOT EXISTS "${PICO_SDK_PATH}/lib/tinyusb/hw/bsp/rp2040/board.h")
  message(FATAL_ERROR "No se encuentra ${PICO_SDK_PATH}/lib/tinyusb/hw/bsp/rp2040/board.h")
endif()



if (PICOW)
target_link_libraries(${PROJECT} PUBLIC
        pico_stdlib
        tinyusb_device
        tinyusb_board
        pico_multicore
        hardware_pio
        hardware_adc
        hardware_i2c
        #pico_cyw43_arch_lwip_threadsafe_background
        pico_cyw43_arch_lwip_poll
        )
endif() #PICOW

if (NOT PICOW)
target_link_libraries(${PROJECT} PUBLIC
        pico_stdlib
        tinyusb_device
        tinyusb_board
        pico_multicore
        hardware_pio
        hardware_adc
        hardware_i2c
        )
endif()  #!PICOW

#==============================[Específico para PicoW]=================================================
# Para que lwIP encuentre lwipopts.h cuando compila lwIP (no solo tu app)
if (PICOW)

if (TARGET pico_lwip)
    target_include_directories(pico_lwip INTERFACE ${CMAKE_CURRENT_LIST_DIR})
endif()

if (TARGET pico_lwip_nosys)
    target_include_directories(pico_lwip_nosys INTERFACE ${CMAKE_CURRENT_LIST_DIR})
endif()

if (TARGET pico_lwip_freertos)
    target_include_directories(pico_lwip_freertos INTERFACE ${CMAKE_CURRENT_LIST_DIR})
endif()
endif() #PICOW
#==============================[Específico para PicoW]=================================================

pico_add_extra_outputs(${PROJECT})

# Configure compilation flags and libraries for the example... see the corresponding function
# in hw/bsp/FAMILY/family.cmake for details.
family_configure_device_example(${PROJECT} noos)


