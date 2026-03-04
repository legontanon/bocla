# Generated CMake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

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
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(bocla C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Shared include paths
set(BOCLA_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR})

# Shared modules
add_library(bocla_audio audio.c)
target_include_directories(bocla_audio PUBLIC ${BOCLA_INCLUDE_DIR})
target_link_libraries(bocla_audio PUBLIC
    pico_stdlib
    hardware_pio
    hardware_dma
    hardware_irq
    hardware_clocks
    hardware_sync
)
pico_generate_pio_header(bocla_audio ${CMAKE_CURRENT_LIST_DIR}/audio.pio)

add_library(bocla_buttons buttons.c)
target_include_directories(bocla_buttons PUBLIC ${BOCLA_INCLUDE_DIR})
target_link_libraries(bocla_buttons PUBLIC
    pico_stdlib
    hardware_pio
    hardware_irq
)
pico_generate_pio_header(bocla_buttons ${CMAKE_CURRENT_LIST_DIR}/buttons.pio)

add_library(bocla_led led.c)
target_include_directories(bocla_led PUBLIC ${BOCLA_INCLUDE_DIR})
target_link_libraries(bocla_led PUBLIC
    pico_stdlib
    hardware_pio
    hardware_clocks
)
pico_generate_pio_header(bocla_led ${CMAKE_CURRENT_LIST_DIR}/led.pio)

add_library(bocla_sequencer sequencer.c)
target_include_directories(bocla_sequencer PUBLIC ${BOCLA_INCLUDE_DIR})

add_library(bocla_usb usb_manager.c usb_descriptors.c)
target_include_directories(bocla_usb PUBLIC ${BOCLA_INCLUDE_DIR})
target_link_libraries(bocla_usb PUBLIC
    bocla_audio
    pico_stdlib
    tinyusb_device
    tinyusb_board
)

# Main firmware
add_executable(bocla
    bocla.c
    ble_manager.c
)

pico_set_program_name(bocla "bocla")
pico_set_program_version(bocla "0.1")
pico_enable_stdio_uart(bocla 0)
pico_enable_stdio_usb(bocla 0)

target_include_directories(bocla PRIVATE ${BOCLA_INCLUDE_DIR})
target_link_libraries(bocla
    bocla_audio
    bocla_buttons
    bocla_led
    bocla_usb
    pico_multicore
)

pico_add_extra_outputs(bocla)

# Test targets
add_executable(test_led test_led.c)
pico_set_program_name(test_led "test_led")
pico_set_program_version(test_led "0.1")
pico_enable_stdio_uart(test_led 1)
pico_enable_stdio_usb(test_led 0)
target_include_directories(test_led PRIVATE ${BOCLA_INCLUDE_DIR})
target_link_libraries(test_led
    bocla_led
)
pico_add_extra_outputs(test_led)

add_executable(test_button test_button.c)
pico_set_program_name(test_button "test_button")
pico_set_program_version(test_button "0.1")
pico_enable_stdio_uart(test_button 1)
pico_enable_stdio_usb(test_button 0)
target_include_directories(test_button PRIVATE ${BOCLA_INCLUDE_DIR})
target_link_libraries(test_button
    bocla_buttons
    bocla_led
)
pico_add_extra_outputs(test_button)

# Note: `test_audio` currently depends on legacy `synth.*` sources that are not
# present in this workspace, so it is intentionally excluded from the default build.

