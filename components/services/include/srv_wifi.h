#ifndef _SRV_WIFI_H_
#define _SRV_WIFI_H_

/**
 * @brief Start WiFi in Station (STA) mode and connect to an access point
 * 
 * Initializes the WiFi subsystem (if not already initialized) and connects to
 * the specified access point. The function handles automatic reconnection up to
 * a maximum number of retries if the connection is lost.
 * 
 * @param event_group Handle to the event group used for WiFi state notifications.
 *                    The following event bits will be set:
 *                    - WIFI_STA_CONNECTED_EVENT: When successfully connected and IP obtained
 *                    - WIFI_STA_CANT_CONNECT_EVENT: When connection fails after max retries
 * @param ssid        Null-terminated string containing the SSID of the access point.
 *                    Must not exceed 32 characters (including null terminator).
 * @param password    Null-terminated string containing the password for the access point.
 *                    Must not exceed 64 characters (including null terminator).
 * 
 * @note The function performs ESP_ERROR_CHECK on critical operations and will abort on failure.
 * @note WiFi initialization is performed only once and reused for subsequent calls.
 * 
 * @warning Ensure the event_group handle remains valid for the lifetime of the WiFi connection.
 */
void srv_wifi_start_STA(EventGroupHandle_t event_group, const char* ssid, const char* password);

/**
 * @brief Start WiFi in Access Point (AP) mode
 * 
 * Initializes the WiFi subsystem (if not already initialized) and creates a
 * software access point with default configuration. The AP will be open (no password)
 * and support up to 4 concurrent connections.
 * 
 * @param event_group Handle to the event group used for WiFi state notifications.
 *                    The following event bit will be set:
 *                    - WIFI_AP_CONNECTED_EVENT: When the AP is successfully started
 * 
 * @note The function performs ESP_ERROR_CHECK on critical operations and will abort on failure.
 * @note WiFi initialization is performed only once and reused for subsequent calls.
 * 
 * @warning Ensure the event_group handle remains valid for the lifetime of the WiFi AP.
 */
void srv_wifi_start_AP(EventGroupHandle_t event_group);

/**
 * @brief Stop WiFi operation and clean up all resources
 * 
 * Stops the WiFi subsystem, unregisters event handlers, deinitializes WiFi,
 * and destroys the network interface. This function performs a complete cleanup
 * of WiFi resources and resets the internal state.
 * 
 * @note This function continues cleanup even if esp_wifi_stop() fails, logging a warning.
 * @note After calling this function, srv_wifi_start_STA() or srv_wifi_start_AP() 
 *       can be called again to restart WiFi operation.
 * @note The function performs ESP_ERROR_CHECK on event handler unregistration.
 * 
 * @warning This function should be called before system shutdown or when switching
 *          to a different WiFi implementation.
 */
void srv_wifi_stop(void);

#endif