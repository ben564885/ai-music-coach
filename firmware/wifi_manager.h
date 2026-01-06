/**
 * WiFi Manager for T5AI Music Coach
 * Uses TuyaOpen TAL WiFi APIs for connectivity
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * WiFi connection status
 */
typedef enum {
  WIFI_STATUS_DISCONNECTED = 0,
  WIFI_STATUS_CONNECTING,
  WIFI_STATUS_CONNECTED,
  WIFI_STATUS_ERROR
} wifi_status_t;

/**
 * WiFi event callback type
 */
typedef void (*wifi_event_cb_t)(wifi_status_t status);

/**
 * Initialize WiFi subsystem
 * @return 0 on success, negative on error
 */
int wifi_manager_init(void);

/**
 * Connect to configured WiFi network
 * Uses SSID/password from tuya_config.h
 * @return 0 on success, negative on error
 */
int wifi_manager_connect(void);

/**
 * Connect to specified WiFi network
 * @param ssid Network SSID
 * @param password Network password
 * @return 0 on success, negative on error
 */
int wifi_manager_connect_to(const char *ssid, const char *password);

/**
 * Disconnect from current network
 */
void wifi_manager_disconnect(void);

/**
 * Get current WiFi status
 * @return Current connection status
 */
wifi_status_t wifi_manager_get_status(void);

/**
 * Check if WiFi is connected
 * @return true if connected
 */
bool wifi_manager_is_connected(void);

/**
 * Get current IP address
 * @param ip_buf Buffer to store IP string (min 16 bytes)
 * @return 0 on success, negative on error
 */
int wifi_manager_get_ip(char *ip_buf);

/**
 * Register WiFi event callback
 * @param callback Callback function
 */
void wifi_manager_set_callback(wifi_event_cb_t callback);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H
