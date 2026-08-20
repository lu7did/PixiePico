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
set(PROJECT testDDS)
cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(${PROJECT} C CXX ASM)
pico_sdk_init()
add_executable(${PROJECT} testDDS.c)

target_sources(testDDS PUBLIC
	    ${CMAKE_CURRENT_LIST_DIR}/lib/assert.c
        ${CMAKE_CURRENT_LIST_DIR}/piodco/piodco.c
        ${CMAKE_CURRENT_LIST_DIR}/gpstime/GPStime.c
        ${CMAKE_CURRENT_LIST_DIR}/debug/logutils.c
        ${CMAKE_CURRENT_LIST_DIR}/testDDS.c 
        ${CMAKE_CURRENT_LIST_DIR}/conswrapper.c
        ${CMAKE_CURRENT_LIST_DIR}/hfconsole/hfconsole.c
        )


#target_link_libraries(${PROJECT} pico_stdlib)

add_compile_options(-Wall)

# Add any user requested libraries
target_link_libraries(
        testDDS
        pico_stdlib
        pico_sync
        pico_multicore
        hardware_timer
        hardware_clocks
        hardware_pio
        hardware_uart
        )

pico_add_extra_outputs(${PROJECT})
pico_enable_stdio_usb(${PROJECT} 1)
pico_enable_stdio_uart(${PROJECT} 0)

target_include_directories(testDDS PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/gpstime
  ${CMAKE_CURRENT_LIST_DIR}/hfconsole
  ${CMAKE_CURRENT_LIST_DIR}/..
)