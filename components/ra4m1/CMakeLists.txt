set(COMPONENT_SRCS
    ra4m1_ctrl.c
    ra4m1_flash.c    
    ra4m1_samba.c
    ra4m1_uart.c
)

set(COMPONENT_PUBLIC_INCLUDE_DIRS
    include
)

set(COMPONENT_REQUIRES 
    esp_driver_gpio
    esp_driver_uart
    esp_partition
)

idf_component_register(
    SRCS "${COMPONENT_SRCS}"
    INCLUDE_DIRS "${COMPONENT_PUBLIC_INCLUDE_DIRS}"
    REQUIRES "${COMPONENT_REQUIRES}"
)