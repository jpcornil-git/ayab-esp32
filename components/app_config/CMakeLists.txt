set(COMPONENT_SRCS
    app_config.c
)

set(COMPONENT_PUBLIC_INCLUDE_DIRS
    include
)

set(COMPONENT_REQUIRES 
    nvs_flash
)

idf_component_register(
    SRCS "${COMPONENT_SRCS}"
    INCLUDE_DIRS "${COMPONENT_PUBLIC_INCLUDE_DIRS}"
    REQUIRES "${COMPONENT_REQUIRES}"
)