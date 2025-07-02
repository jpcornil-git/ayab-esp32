/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* HTTP File Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "esp_err.h"
#include "esp_log.h"

#include "esp_vfs.h"
#include "esp_spiffs.h"

#include "srv_file.h"

/* Max length a file path can have on storage */
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)
#define UPLOAD_SCRIPT_HTM "upload_script.htm"
#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

/* Max size of an individual file. Make sure this
 * value is same as that set in upload_script.html */
#define MAX_FILE_SIZE   (200*1024) // 200 KB
#define MAX_FILE_SIZE_STR "200KB"

/* Scratch buffer size */
#define SCRATCH_BUFSIZE  8192

typedef struct  {
    bool is_running;
    /* SPIFFS root path*/
    char root_path[ESP_VFS_PATH_MAX + 1];
    size_t root_path_len;
    /* Base path for www files */
    char base_path[ESP_VFS_PATH_MAX + 1];
    size_t base_path_len;
    /* Scratch buffer for temporary storage during file transfer */
    char scratch[SCRATCH_BUFSIZE];
} srv_file_data_t;

static const char *TAG = "srv_file";

static srv_file_data_t _self = {.is_running=false};

/* Set HTTP response content type according to file extension */
static esp_err_t _set_content_type_from_file(httpd_req_t *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".pdf")) {
        return httpd_resp_set_type(req, "application/pdf");
    } else if (IS_FILE_EXT(filename, ".html") || IS_FILE_EXT(filename, ".htm")) {
        return httpd_resp_set_type(req, "text/html");
    } else if (IS_FILE_EXT(filename, ".css")) {
        return httpd_resp_set_type(req, "text/css");
    } else if (IS_FILE_EXT(filename, ".js")) {
        return httpd_resp_set_type(req, "text/javascript");
    } else if (IS_FILE_EXT(filename, ".jpeg") || IS_FILE_EXT(filename, ".jpg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    } else if (IS_FILE_EXT(filename, ".ico")) {
        return httpd_resp_set_type(req, "image/x-icon");
    }
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}

/* Copies the full path into destination buffer and return a pointer to path (without the base path) */
static const char* _get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{
    if (!strcmp(uri, "/")) {
        uri = "/index.htm";
    }

    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest) {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash) {
        pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize) {
        /* Full path string won't fit into destination buffer */
        return NULL;
    }

    /* Construct full path (base + path) */
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* Return pointer to path, skipping the base */
    return dest + base_pathlen;
}

/* Send all file chunks to the request's client. */
static esp_err_t _send_all_file_chunks(httpd_req_t *req, const char* filepath) {
    FILE *fd = NULL;

    fd = fopen(filepath, "r");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to read existing file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *chunk = _self.scratch;
    size_t chunksize;
    do {
        /* Read file in chunks into the scratch buffer */
        chunksize = fread(chunk, 1, SCRATCH_BUFSIZE, fd);

        if (chunksize > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
                fclose(fd);
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
               return ESP_FAIL;
           }
        }
        /* Keep looping till the whole file is sent */
    } while (chunksize != 0);

    /* Close file after sending complete */
    fclose(fd);
    return ESP_OK;
}

/* Handler to download a file from the file system. */
static esp_err_t _download_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
    struct stat file_stat;

    const char *filename = _get_path_from_uri(filepath, _self.base_path, req->uri, sizeof(filepath));
    if (!filename) {
        ESP_LOGE(TAG, "Filename is too long");
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
        return ESP_FAIL;
    }

    if (stat(filepath, &file_stat) == -1) {
        // Try from root (uri from flash_utils)
        filename = _get_path_from_uri(filepath, _self.root_path, req->uri, sizeof(filepath));
        if (stat(filepath, &file_stat) == -1) {
            ESP_LOGE(TAG, "Failed to stat file : %s", filepath);
            /* Respond with 404 Not Found */
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "Sending file : %s (%ld bytes)...", filename, file_stat.st_size);
    _set_content_type_from_file(req, filename);
    _send_all_file_chunks(req, filepath);

    /* Respond with an empty chunk to signal HTTP response completion */
#ifdef CONFIG_EXAMPLE_HTTPD_CONN_CLOSE_HEADER
    httpd_resp_set_hdr(req, "Connection", "close");
#endif
    httpd_resp_send_chunk(req, NULL, 0);
    ESP_LOGI(TAG, "File sending complete");
    return ESP_OK;
}

/* Handler to upload a file onto the file system. */
static esp_err_t _upload_post_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
    FILE *fd = NULL;
    struct stat file_stat;

    /* Skip leading "/upload" from URI to get filename */
    /* Note sizeof() counts NULL termination hence the -1 */
    const char *filename = _get_path_from_uri(filepath, _self.root_path,
                                             req->uri + sizeof("/upload") - 1, sizeof(filepath));
    if (!filename) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
        return ESP_FAIL;
    }

    /* Filename cannot have a trailing '/' */
    if (filename[strlen(filename) - 1] == '/') {
        ESP_LOGE(TAG, "Invalid filename : %s", filename);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid filename");
        return ESP_FAIL;
    }

    if (stat(filepath, &file_stat) == 0) {
        ESP_LOGW(TAG, "Deleting already existing file : %s", filepath);
        unlink(filepath);
    }

    /* File cannot be larger than a limit */
    if (req->content_len > MAX_FILE_SIZE) {
        ESP_LOGE(TAG, "File too large : %d bytes", req->content_len);
        /* Respond with 400 Bad Request */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "File size must be less than "
                            MAX_FILE_SIZE_STR "!");
        /* Return failure to close underlying connection else the
         * incoming file content will keep the socket busy */
        return ESP_FAIL;
    }

    fd = fopen(filepath, "w");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to create file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create file");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Receiving file : %s...", filename);

    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *buf = _self.scratch;
    int received;

    /* Content length of the request gives
     * the size of the file being uploaded */
    int remaining = req->content_len;

    while (remaining > 0) {

        ESP_LOGI(TAG, "Remaining size : %d", remaining);
        /* Receive the file part by part into a buffer */
        if ((received = httpd_req_recv(req, buf, MIN(remaining, SCRATCH_BUFSIZE))) <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry if timeout occurred */
                continue;
            }

            /* In case of unrecoverable error,
             * close and delete the unfinished file*/
            fclose(fd);
            unlink(filepath);

            ESP_LOGE(TAG, "File reception failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
            return ESP_FAIL;
        }

        /* Write buffer content to file on storage */
        if (received && (received != fwrite(buf, 1, received, fd))) {
            /* Couldn't write everything to file!
             * Storage may be full? */
            fclose(fd);
            unlink(filepath);

            ESP_LOGE(TAG, "File write failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to write file to storage");
            return ESP_FAIL;
        }

        /* Keep track of remaining size of
         * the file left to be uploaded */
        remaining -= received;
    }

    /* Close file upon upload completion */
    fclose(fd);
    ESP_LOGI(TAG, "File reception complete");

    /* Redirect onto flash_utils to see the updated file list */
    //httpd_resp_set_status(req, "303 See Other");
    //httpd_resp_set_hdr(req, "Location", "/flash_utils   ");
    //httpd_resp_sendstr(req, "File uploaded successfully");
    httpd_resp_send(req, "File uploaded successfully", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Populate JSON data with list of all files (name, sie, [url])
void srv_file_json_list_files(cJSON *data) {
    char entrypath[FILE_PATH_MAX];
    struct dirent *entry;
    struct stat entry_stat;

    /* Open root directory */
    snprintf(entrypath, FILE_PATH_MAX, "%s/", _self.root_path);
    DIR *dir = opendir(entrypath);
    if (!dir) {
        ESP_LOGE(TAG, "%s", "Failed to open root directory");
        return;
    }

    // FIXME: Add error handling ...
    cJSON *json_entries = cJSON_CreateArray();
    cJSON_AddItemToObject(data, "list_files", json_entries);
    /* Iterate over all files / folders and fetch their names and sizes */
    while ((entry = readdir(dir)) != NULL) {
        strlcpy(entrypath + _self.root_path_len + 1, entry->d_name, sizeof(entrypath) - _self.root_path_len -1);
        if (stat(entrypath, &entry_stat) == -1) {
            ESP_LOGE(TAG, "Failed to stat : %s", entry->d_name);
            continue;
        }
        // Build JSON dictionnary for this entry
        cJSON *json_entry = cJSON_CreateObject();
        cJSON_AddItemToArray(json_entries, json_entry);
        cJSON_AddStringToObject(json_entry, "name", entry->d_name);
        cJSON_AddNumberToObject(json_entry, "size", entry_stat.st_size);
        if (!strncmp(_self.base_path, entrypath, _self.base_path_len)) {
            cJSON_AddStringToObject(json_entry, "url" , entrypath+_self.base_path_len);
        }
    }
    closedir(dir);
}

/* Delete file listed in JSON data*/
esp_err_t srv_file_json_delete_files(cJSON *list_files) {
    char filepath[FILE_PATH_MAX];
    struct stat file_stat;

    for(int i=0; i<cJSON_GetArraySize(list_files); i++) {
        cJSON *file_uri = cJSON_GetArrayItem(list_files, i);
        /* Add basepath to file uri*/
        const char *filename = _get_path_from_uri(filepath, _self.root_path,
                                file_uri->valuestring, sizeof(filepath));        
        /* Filename cannot have a trailing '/' */
        if (filename[strlen(filename) - 1] == '/') {
            ESP_LOGE(TAG, "Invalid filename : %s", filename);
            return ESP_FAIL;
        }
        /* Check file existence*/
        if (stat(filepath, &file_stat) == -1) {
            ESP_LOGE(TAG, "File does not exist : %s", filename);
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "Deleting file : %s", filename);
        /* Delete file */
        unlink(filepath);
    }
    return ESP_OK;
}

/* Start the file server. */
esp_err_t srv_file_start(httpd_handle_t server, const char *base_path, const char *www_path)
{
    ESP_LOGI(TAG, "Starting file service");
    if (_self.is_running) {
        ESP_LOGE(TAG, "File server already started");
        return ESP_ERR_INVALID_STATE;
    }
    _self.is_running = true;

    strncpy(_self.root_path, base_path, ESP_VFS_PATH_MAX);
    snprintf(_self.base_path, ESP_VFS_PATH_MAX, "%s%s", base_path, www_path);
    _self.base_path_len = strlen(_self.base_path);
    _self.root_path_len = strlen(_self.root_path);

    /* URI handler for uploading files to server */
    httpd_uri_t file_upload = {
        .uri       = "/upload/*",     // Match all URIs of type /upload/path/to/file
        .method    = HTTP_POST,
        .handler   = _upload_post_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &file_upload);

    /* URI handler for getting uploaded files */
    httpd_uri_t file_download = {
        .uri       = "/*",            // Match all URIs of type /path/to/file
        .method    = HTTP_GET,
        .handler   = _download_get_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &file_download);

    return ESP_OK;
}

/* Stop the file server. */
void srv_file_stop() {
    ESP_LOGI(TAG, "Stopping file service");
    _self.is_running = false;
}
