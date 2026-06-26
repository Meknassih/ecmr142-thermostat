cmake_minimum_required(VERSION 3.13)
set(PICO_BOARD pico2_w)
include(pico_sdk_import.cmake)
project(main C CXX ASM)
pico_sdk_init()

add_executable(main main.c)
target_include_directories(main PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(main pico_stdlib pico_cyw43_arch_none hardware_i2c)
pico_add_extra_outputs(main)

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

# Add custom flash target
add_custom_target(flash
    COMMAND ${CMAKE_COMMAND} -E echo "Flashing main.uf2..."
    COMMAND picotool load -f -x ${CMAKE_BINARY_DIR}/main.uf2
    DEPENDS main
    VERBATIM
)
