#ifndef _OTA_HANDLER_H
#define _OTA_HANDLER_H

#include "esp_err.h"
#include "esp_http_server.h"

/**
 * @brief Handle POST requests for OTA updates.
 *
 * This function processes incoming HTTP POST requests for OTA updates.
 * It reads the request body and writes the data to the OTA partition.
 *
 * @param req Pointer to the HTTP request structure.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ota_post_handler(httpd_req_t *req);

#endif