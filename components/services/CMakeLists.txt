set(COMPONENT_SRCS
    srv_file.c
    srv_http.c
    srv_mdns.c
    srv_littlefs.c
    srv_websocket.c
    srv_wifi.c
)

set(COMPONENT_PUBLIC_INCLUDE_DIRS
    include
)

set(COMPONENT_REQUIRES
    esp_wifi
    esp_http_server
    espressif__mdns
    json
)

set(COMPONENT_PRIV_REQUIRES
    esp_app_format
    esp_partition
    littlefs
    app_config
    ota
    ra4m1
    vfs
)

idf_component_register(
    SRCS "${COMPONENT_SRCS}"
    INCLUDE_DIRS "${COMPONENT_PUBLIC_INCLUDE_DIRS}"
    REQUIRES "${COMPONENT_REQUIRES}"
    PRIV_REQUIRES "${COMPONENT_PRIV_REQUIRES}" main
)