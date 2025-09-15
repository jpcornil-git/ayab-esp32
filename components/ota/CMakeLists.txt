set(COMPONENT_SRCS
    ota_handler.c    
    ota_app.c
    ota_littlefs.c
)

set(COMPONENT_PUBLIC_INCLUDE_DIRS
    include
)

set(COMPONENT_PRIV_REQUIRES 
    app_update
    esp_http_server
    ra4m1
    services
)

idf_component_register(
    SRCS "${COMPONENT_SRCS}"
    INCLUDE_DIRS "${COMPONENT_PUBLIC_INCLUDE_DIRS}"
    PRIV_REQUIRES "${COMPONENT_PRIV_REQUIRES}"
)