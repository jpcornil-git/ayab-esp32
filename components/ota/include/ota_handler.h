#ifndef _OTA_HANDLER_H
#define _OTA_HANDLER_H

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t ota_post_handler(httpd_req_t *req);

#endif