#ifndef _SRV_FILE_H_
#define _SRV_FILE_H_

#include "esp_http_server.h"
#include "cJSON.h"

void srv_file_json_list_files(cJSON *data);
esp_err_t srv_file_json_delete_files(cJSON *list_files);
esp_err_t srv_file_start(httpd_handle_t server, const char *base_path, const char *www_path);
void srv_file_stop();

#endif