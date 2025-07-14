#ifndef _SRV_FILE_H_
#define _SRV_FILE_H_

#include "esp_http_server.h"
#include "cJSON.h"


/**
 * @brief Populate a cJSON object with a list of all files (name, size, url).
 *
 * @param list_files Pointer to a cJSON object to be filled with file information.
 */
void srv_file_json_list_files(cJSON *list_files);

/**
 * @brief Delete files listed in a cJSON array.
 *
 * @param list_files cJSON array containing file URIs to delete.
 * @return ESP_OK on success, ESP_FAIL on error.
 */
esp_err_t srv_file_json_delete_files(cJSON *list_files);

/**
 * @brief Start the file server and register URI handlers for upload/download.
 *
 * @param server     HTTP server handle.
 * @param base_path  Root path for file storage.
 * @param www_path   Path for static web content.
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t srv_file_start(httpd_handle_t server, const char *base_path, const char *www_path);

/**
 * @brief Stop the file server and release resources.
 */
void srv_file_stop();

#endif