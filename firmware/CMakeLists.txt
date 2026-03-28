cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico_vsp)
pico_sdk_init()

add_executable(pico_vsp
  main.c
  usb_descriptors.c
)

set_target_properties(pico_vsp PROPERTIES OUTPUT_NAME pico-vsp)

target_include_directories(pico_vsp PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(pico_vsp
  pico_stdlib
  tinyusb_device
  tinyusb_board
)

pico_enable_stdio_usb(pico_vsp 0)
pico_enable_stdio_uart(pico_vsp 0)
pico_add_extra_outputs(pico_vsp)
