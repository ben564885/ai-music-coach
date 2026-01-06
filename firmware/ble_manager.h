/**
 * BLE Manager for T5AI Music Coach
 * Uses TuyaOpen TKL BLE APIs for mobile app communication
 */

#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * BLE connection status
 */
typedef enum {
  BLE_STATUS_IDLE = 0,
  BLE_STATUS_ADVERTISING,
  BLE_STATUS_CONNECTED,
  BLE_STATUS_ERROR
} ble_status_t;

/**
 * BLE message types (matching mobile app protocol)
 */
typedef enum {
  BLE_MSG_SHEET_MUSIC = 0x01,
  BLE_MSG_START_RECORDING = 0x02,
  BLE_MSG_STOP_RECORDING = 0x03,
  BLE_MSG_SET_INSTRUMENT = 0x04,
  BLE_MSG_SET_SCALE = 0x05,
  BLE_MSG_ANALYSIS_RESULT = 0x10,
  BLE_MSG_STATUS_UPDATE = 0x11,
  BLE_MSG_ERROR = 0xFF
} ble_msg_type_t;

/**
 * BLE message structure
 */
typedef struct {
  ble_msg_type_t type;
  uint16_t length;
  uint8_t *data;
} ble_message_t;

/**
 * BLE data received callback
 */
typedef void (*ble_data_cb_t)(const uint8_t *data, uint16_t len);

/**
 * BLE connection status callback
 */
typedef void (*ble_status_cb_t)(ble_status_t status);

/**
 * Initialize BLE subsystem
 * @return 0 on success, negative on error
 */
int ble_manager_init(void);

/**
 * Start BLE advertising
 * @return 0 on success, negative on error
 */
int ble_manager_start_advertising(void);

/**
 * Stop BLE advertising
 */
void ble_manager_stop_advertising(void);

/**
 * Send data to connected mobile app
 * @param data Data buffer
 * @param len Data length
 * @return 0 on success, negative on error
 */
int ble_manager_send_data(const uint8_t *data, uint16_t len);

/**
 * Send typed message to mobile app
 * @param type Message type
 * @param data Payload data
 * @param len Payload length
 * @return 0 on success, negative on error
 */
int ble_manager_send_message(ble_msg_type_t type, const uint8_t *data,
                             uint16_t len);

/**
 * Send analysis results to mobile app
 * @param mistake_count Number of mistakes
 * @param feedback_text Feedback string
 * @return 0 on success, negative on error
 */
int ble_manager_send_analysis_results(uint32_t mistake_count,
                                      const char *feedback_text);

/**
 * Get current BLE status
 * @return Current status
 */
ble_status_t ble_manager_get_status(void);

/**
 * Check if a device is connected
 * @return true if connected
 */
bool ble_manager_is_connected(void);

/**
 * Register data received callback
 * @param callback Callback function
 */
void ble_manager_set_data_callback(ble_data_cb_t callback);

/**
 * Register status change callback
 * @param callback Callback function
 */
void ble_manager_set_status_callback(ble_status_cb_t callback);

/**
 * Disconnect current connection
 */
void ble_manager_disconnect(void);

#ifdef __cplusplus
}
#endif

#endif // BLE_MANAGER_H
