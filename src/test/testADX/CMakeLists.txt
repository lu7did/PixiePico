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

set(PICO_BOARD pico CACHE STRING "Board type")
set(PROJECT testADX)
cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(${PROJECT} C CXX ASM)
pico_sdk_init()
add_executable(${PROJECT} testADX.c)

target_sources(testADX PUBLIC
	${CMAKE_CURRENT_LIST_DIR}/lib/assert.c
        ${CMAKE_CURRENT_LIST_DIR}/piodco/piodco.c
        ${CMAKE_CURRENT_LIST_DIR}/gpstime/GPStime.c
        ${CMAKE_CURRENT_LIST_DIR}/debug/logutils.c
        ${CMAKE_CURRENT_LIST_DIR}/ft8_lib/ft8/constants.c
        ${CMAKE_CURRENT_LIST_DIR}/ft8_lib/ft8/text.c
        ${CMAKE_CURRENT_LIST_DIR}/ft8_lib/ft8/pack.c
        ${CMAKE_CURRENT_LIST_DIR}/ft8_lib/ft8/encode.c
        ${CMAKE_CURRENT_LIST_DIR}/ft8_lib/ft8/crc.c
        ${CMAKE_CURRENT_LIST_DIR}/ft8_lib/common/wave.c
        ${CMAKE_CURRENT_LIST_DIR}/testADX.c 
        ${CMAKE_CURRENT_LIST_DIR}/conswrapper.c
        ${CMAKE_CURRENT_LIST_DIR}/hfconsole/hfconsole.c
        )


#target_link_libraries(${PROJECT} pico_stdlib)

add_compile_options(-Wall)

# Add any user requested libraries
target_link_libraries(
        testADX
        pico_stdlib
        pico_sync
        pico_multicore
        hardware_timer
        hardware_clocks
        hardware_pio
        hardware_uart
        hardware_rtc
                )

pico_add_extra_outputs(${PROJECT})
pico_enable_stdio_usb(${PROJECT} 1)
pico_enable_stdio_uart(${PROJECT} 0)

target_include_directories(testADX PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/gpstime
  ${CMAKE_CURRENT_LIST_DIR}/hfconsole
  ${CMAKE_CURRENT_LIST_DIR}/..
)