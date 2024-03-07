cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(hwclock CXX ASM C)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_executable(hwclock
    main.cpp
)

pico_generate_pio_header(hwclock ${CMAKE_CURRENT_LIST_DIR}/counter.pio)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(hwclock pico_stdlib hardware_pio)

pico_enable_stdio_usb(hwclock ENABLED)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(hwclock)