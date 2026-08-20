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

cmake_minimum_required(VERSION 3.12)

# ----------------------------------------------------------------------------
# Pico SDK path
# ----------------------------------------------------------------------------
if(NOT DEFINED PICO_SDK_PATH)
  if(DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}" CACHE PATH "Path to Pico SDK")
  else()
    set(PICO_SDK_PATH "/Users/PCOLLA/Documents/GitHub/pico/pico-sdk" CACHE PATH "Path to Pico SDK")
  endif()
endif()

# ----------------------------------------------------------------------------
# TinyUSB family (requerido por family_support.cmake)
# ----------------------------------------------------------------------------
if(NOT DEFINED FAMILY)
  set(FAMILY "rp2040" CACHE STRING "TinyUSB family" FORCE)
endif()

# ----------------------------------------------------------------------------
# Import TinyUSB family flow
# ----------------------------------------------------------------------------
include(${PICO_SDK_PATH}/lib/tinyusb/hw/bsp/family_support.cmake)

project(quad_osc C CXX ASM)

family_get_project_name(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR})
family_initialize_project(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR})

# ----------------------------------------------------------------------------
# Ejecutable
# ----------------------------------------------------------------------------
add_executable(${PROJECT_NAME}
  main.c
  quad.c
  usb_descriptors.c
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  CFG_TUSB_RHPORT0_MODE=OPT_MODE_DEVICE
  CFG_TUSB_RHPORT1_MODE=0
)


# ----------------------------------------------------------------------------
# Construir pioasm desde el SDK (robusto, sin depender de PATH)
# ----------------------------------------------------------------------------
# El SDK trae el código fuente en: ${PICO_SDK_PATH}/tools/pioasm
# Esto genera un target ejecutable llamado "pioasm".
#add_subdirectory(${PICO_SDK_PATH}/tools/pioasm ${CMAKE_BINARY_DIR}/pioasm_build)

# ----------------------------------------------------------------------------
# Generación de quad.pio.h usando pioasm (c-sdk usa prefijo)
# ----------------------------------------------------------------------------
set(PIO_FILE   ${CMAKE_CURRENT_LIST_DIR}/quad.pio)
set(PIO_OUT    ${CMAKE_CURRENT_BINARY_DIR}/quad)        # pioasm genera "quad"
set(PIO_HEADER ${CMAKE_CURRENT_BINARY_DIR}/quad.pio.h)  # nombre final deseado

add_custom_command(
  OUTPUT  ${PIO_HEADER}
  COMMAND $<TARGET_FILE:pioasm> -o c-sdk ${PIO_FILE} ${PIO_OUT}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PIO_OUT} ${PIO_HEADER}
  DEPENDS ${PIO_FILE} pioasm
  VERBATIM
)

target_sources(${PROJECT_NAME} PRIVATE ${PIO_HEADER})
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_LIST_DIR}
)

# --------------------------------------------------------------------
# Generación de quad.pio.h usando pioasm provisto por family_support
# --------------------------------------------------------------------
#set(PIO_FILE   ${CMAKE_CURRENT_LIST_DIR}/quad.pio)
#set(PIO_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/quad)
#set(PIO_HEADER ${PIO_PREFIX}.pio.h)
#
# Verificación defensiva: el target pioasm debe existir
#if (NOT TARGET pioasm)
#    message(FATAL_ERROR "Target 'pioasm' no existe. family_support no lo creó correctamente.")
#endif()
#
#add_custom_command(
#    OUTPUT  ${PIO_HEADER}
#    COMMAND $<TARGET_FILE:pioasm> -o c-sdk ${PIO_FILE} ${PIO_PREFIX}
#    DEPENDS ${PIO_FILE} pioasm
#    VERBATIM
#)

# Forzar dependencia del ejecutable al header generado
#target_sources(${PROJECT_NAME} PRIVATE ${PIO_HEADER})

# Include donde queda quad.pio.h
#target_include_directories(${PROJECT_NAME} PRIVATE
#    ${CMAKE_CURRENT_BINARY_DIR}
#    ${CMAKE_CURRENT_LIST_DIR}
#)

# ----------------------------------------------------------------------------
# Librerías necesarias
# ----------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PUBLIC
  pico_stdlib
  tinyusb_device
  tinyusb_board
  hardware_pio
  hardware_clocks
  hardware_pll
  pico_multicore
  hardware_adc
  hardware_rtc
  hardware_flash 
  hardware_sync
  hardware_i2c
  hardware_watchdog
)


target_compile_options(${PROJECT_NAME} PRIVATE
  -Wall -Wextra
  -Werror
  -Wsign-conversion
  -Wconversion
)

pico_add_extra_outputs(${PROJECT_NAME})
family_configure_device_example(${PROJECT_NAME} noos)

message(STATUS "PICO_SDK_PATH=${PICO_SDK_PATH}")
#pico_generate_pio_header(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/quad.pio)