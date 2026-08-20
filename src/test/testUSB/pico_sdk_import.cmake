if (DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif ()

if (NOT PICO_SDK_PATH)
    message(FATAL_ERROR "PICO_SDK_PATH no está definido. Exportá PICO_SDK_PATH apuntando a tu pico-sdk.")
endif ()

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

