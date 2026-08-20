cmake_minimum_required(VERSION 3.12)

# == Pico VSCode Extension (solo si NO se definió PICO_SDK_PATH) ==
if(NOT DEFINED PICO_SDK_PATH)
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
endif()
# ============================================

# ----------------------------------------------------------------------------
# Pico SDK path
# ----------------------------------------------------------------------------
if(NOT DEFINED PICO_SDK_PATH)
  if(DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}" CACHE PATH "Path to Pico SDK")
  else()
    message(FATAL_ERROR "PICO_SDK_PATH no está definido (ni por -D ni por entorno).")
  endif()
endif()

# ----------------------------------------------------------------------------
# TinyUSB family
# ----------------------------------------------------------------------------
if(NOT DEFINED FAMILY)
  set(FAMILY "rp2040" CACHE STRING "TinyUSB family" FORCE)
endif()

include(${PICO_SDK_PATH}/lib/tinyusb/hw/bsp/family_support.cmake)

project(si4732_rp2040 C CXX ASM)

family_get_project_name(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR})
family_initialize_project(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR})

add_executable(${PROJECT_NAME}
  src/main.c
  src/si4732.c
  src/si4732_patch_ssb.c
  src/usb_descriptors.c
  src/si4732_patch_ssb.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/src
)


target_link_libraries(${PROJECT_NAME} PUBLIC
  pico_stdlib
  tinyusb_device
  tinyusb_board
  hardware_i2c
  hardware_gpio
  hardware_watchdog
  hardware_sync
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
