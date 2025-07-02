set(COMPONENT_SRCS
    main.c
    ifce_wifi.c
)

set(COMPONENT_INCLUDE_DIRS
    .
    include
)

set(COMPONENT_PRIV_REQUIRES
    esp_driver_gpio
    esp_driver_uart
    esp_wifi
    app_config
    ota
    ra4m1
    services
)

idf_component_register(
    SRCS "${COMPONENT_SRCS}"
    INCLUDE_DIRS "${COMPONENT_INCLUDE_DIRS}"
    PRIV_REQUIRES "${COMPONENT_PRIV_REQUIRES}"
)