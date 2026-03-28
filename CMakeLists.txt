cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico_hid)
pico_sdk_init()

add_executable(pico_hid
  main.c
  usb_descriptors.c
)

set_target_properties(pico_hid PROPERTIES OUTPUT_NAME pico-hid)

target_include_directories(pico_hid PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(pico_hid
  pico_stdlib
  tinyusb_device
  tinyusb_board
)

pico_enable_stdio_usb(pico_hid 1)
pico_enable_stdio_uart(pico_hid 0)
pico_add_extra_outputs(pico_hid)
