cmake_minimum_required(VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico2_w)
include(pico_sdk_import.cmake)
project(main C CXX ASM)
pico_sdk_init()

add_executable(main main.c ssd1306.c ir_receiver.c ir_emitter.c ecmr142.c aht20.c bmp280.c ac_state.c ac_strategy_economic.c humidex.c)
target_include_directories(main PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(main pico_stdlib pico_cyw43_arch_none hardware_i2c hardware_pwm)
pico_add_extra_outputs(main)

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

option(VERBOSE "Enable verbose boot logging in main.c" OFF)
if(VERBOSE)
    target_compile_definitions(main PRIVATE VERBOSE=1)
endif()

# Add custom flash target
add_custom_target(flash
    COMMAND ${CMAKE_COMMAND} -E echo "Flashing main.uf2..."
    COMMAND picotool load -f -x ${CMAKE_BINARY_DIR}/main.uf2
    DEPENDS main
    VERBATIM
)
