# Generated Cmake Pico project file

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
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(picoloader C CXX ASM)

# Shrink flash region to 128KB
set(PICO_FLASH_SIZE_BYTES 131072)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable
add_executable(picoloader picoloader.c)
pico_set_program_name(picoloader "picoloader")

# Program version
set(PROGRAM_VERSION_MAJOR "1")
set(PROGRAM_VERSION_MINOR "1")
set(PROGRAM_VERSION "${PROGRAM_VERSION_MAJOR}.${PROGRAM_VERSION_MINOR}")

pico_set_program_version(picoloader "${PROGRAM_VERSION}")

target_compile_definitions(picoloader PRIVATE
    PROGRAM_VERSION_MAJOR=${PROGRAM_VERSION_MAJOR}
    PROGRAM_VERSION_MINOR=${PROGRAM_VERSION_MINOR}
    PROGRAM_VERSION="${PROGRAM_VERSION}"
)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(picoloader 0)
pico_enable_stdio_usb(picoloader 1)

# Include additional files
target_sources(picoloader PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/dvd.h
        ${CMAKE_CURRENT_LIST_DIR}/dvd.c
        ${CMAKE_CURRENT_LIST_DIR}/dvd_drv.h
        ${CMAKE_CURRENT_LIST_DIR}/dvd_drv.c
)

# Add the standard library to the build
target_link_libraries(picoloader
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(picoloader PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add additional libraries
target_link_libraries(picoloader 
        hardware_dma
        hardware_pio
)

# Generate PIO header
pico_generate_pio_header(picoloader ${CMAKE_CURRENT_LIST_DIR}/drive_interface.pio)

# Add data memory regions
target_link_options(picoloader PRIVATE -T${CMAKE_CURRENT_LIST_DIR}/memmap_data.ld)

pico_add_extra_outputs(picoloader)

