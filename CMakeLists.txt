# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# set(PROJECT_VER "0.0.0")
project(ayab-esp32)

# Create littlefs partition image
set(LITTLEFS_PARTITION_NAME "littlefs")
set(LITTLEFS_DATA_DIR "${PROJECT_DIR}/data")
littlefs_create_partition_image(
    "${LITTLEFS_PARTITION_NAME}"
    "${LITTLEFS_DATA_DIR}"
    FLASH_IN_PROJECT
)

# Build the complete flash image
set(FLASH_BIN "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_flash.bin")
add_custom_target(
    create_flash_image
    ALL
    COMMAND
        cd "${CMAKE_BINARY_DIR}" && esptool.py --chip "${CONFIG_IDF_TARGET}" merge_bin --output "${FLASH_BIN}" @flash_args
    COMMENT "Generating flash binary (${FLASH_BIN})"
    VERBATIM
)
# Add target dependencies for create_flash_image
add_dependencies(create_flash_image app)
add_dependencies(create_flash_image bootloader)
add_dependencies(create_flash_image partition-table)
add_dependencies(create_flash_image blank_ota_data)
add_dependencies(create_flash_image littlefs_${LITTLEFS_PARTITION_NAME}_bin)