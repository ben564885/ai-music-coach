/**
 * BLE Manager Implementation for T5AI Music Coach
 * Uses TuyaOpen TKL BLE APIs
 */

#include "ble_manager.h"
#include "tuya_config.h"

#include "tal_log.h"
#include "tal_memory.h"
#include "tal_system.h"
#include "tkl_bluetooth.h"

#include <string.h>

#define TAG "BLE_MGR"

// BLE GATT handles
static uint16_t s_conn_handle = 0xFFFF;
static uint16_t s_tx_char_handle = 0;
static uint16_t s_rx_char_handle = 0;

// Status and callbacks
static ble_status_t s_ble_status = BLE_STATUS_IDLE;
static ble_data_cb_t s_data_callback = NULL;
static ble_status_cb_t s_status_callback = NULL;

// Service and characteristic UUIDs
static const uint8_t s_service_uuid[] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00,
                                         0x00, 0x80, 0x00, 0x10, 0x00, 0x00,
                                         0xE0, 0xFF, 0x00, 0x00};

static const uint8_t s_tx_char_uuid[] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00,
                                         0x00, 0x80, 0x00, 0x10, 0x00, 0x00,
                                         0xE1, 0xFF, 0x00, 0x00};

static const uint8_t s_rx_char_uuid[] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00,
                                         0x00, 0x80, 0x00, 0x10, 0x00, 0x00,
                                         0xE2, 0xFF, 0x00, 0x00};

/**
 * BLE GAP event handler
 */
static void ble_gap_event_handler(TKL_BLE_GAP_EVT_T *p_event) {
  if (!p_event)
    return;

  switch (p_event->type) {
  case TKL_BLE_GAP_EVT_CONNECT:
    TAL_LOGI(TAG, "BLE connected, handle: %d", p_event->conn_handle);
    s_conn_handle = p_event->conn_handle;
    s_ble_status = BLE_STATUS_CONNECTED;
    if (s_status_callback) {
      s_status_callback(s_ble_status);
    }
    break;

  case TKL_BLE_GAP_EVT_DISCONNECT:
    TAL_LOGI(TAG, "BLE disconnected");
    s_conn_handle = 0xFFFF;
    s_ble_status = BLE_STATUS_IDLE;
    if (s_status_callback) {
      s_status_callback(s_ble_status);
    }
    // Restart advertising
    ble_manager_start_advertising();
    break;

  case TKL_BLE_GAP_EVT_ADV_REPORT:
    // Not used in peripheral mode
    break;

  default:
    break;
  }
}

/**
 * BLE GATT event handler
 */
static void ble_gatt_event_handler(TKL_BLE_GATT_EVT_T *p_event) {
  if (!p_event)
    return;

  switch (p_event->type) {
  case TKL_BLE_GATT_EVT_WRITE_REQ:
    TAL_LOGI(TAG, "GATT write request, len: %d",
             p_event->gatt_event.write_req.length);

    // Pass data to application callback
    if (s_data_callback && p_event->gatt_event.write_req.length > 0) {
      s_data_callback(p_event->gatt_event.write_req.data,
                      p_event->gatt_event.write_req.length);
    }
    break;

  case TKL_BLE_GATT_EVT_NOTIFY_COMPLETE:
    TAL_LOGD(TAG, "Notification sent");
    break;

  case TKL_BLE_GATT_EVT_MTU_EXCHANGE:
    TAL_LOGI(TAG, "MTU exchanged: %d", p_event->gatt_event.mtu);
    break;

  default:
    break;
  }
}

int ble_manager_init(void) {
  TAL_LOGI(TAG, "Initializing BLE manager");

  // Initialize BLE stack
  TKL_BLE_STACK_INIT_PARAMS_T init_params = {0};
  init_params.role = TKL_BLE_ROLE_SERVER;
  init_params.gap_event_cb = ble_gap_event_handler;
  init_params.gatt_event_cb = ble_gatt_event_handler;

  OPERATE_RET ret = tkl_ble_stack_init(&init_params);
  if (ret != OPRT_OK) {
    TAL_LOGE(TAG, "tkl_ble_stack_init failed: %d", ret);
    return -1;
  }

  // Register GATT service
  TKL_BLE_GATTS_PARAMS_T service_params = {0};
  service_params.svc_uuid.uuid_type = TKL_BLE_UUID_TYPE_128;
  memcpy(service_params.svc_uuid.uuid.uuid128, s_service_uuid, 16);

  // TX Characteristic (notify)
  TKL_BLE_CHAR_PARAMS_T tx_char = {0};
  tx_char.char_uuid.uuid_type = TKL_BLE_UUID_TYPE_128;
  memcpy(tx_char.char_uuid.uuid.uuid128, s_tx_char_uuid, 16);
  tx_char.property = TKL_BLE_GATT_CHAR_PROP_NOTIFY;
  tx_char.permission = TKL_BLE_GATT_PERM_READ;

  // RX Characteristic (write)
  TKL_BLE_CHAR_PARAMS_T rx_char = {0};
  rx_char.char_uuid.uuid_type = TKL_BLE_UUID_TYPE_128;
  memcpy(rx_char.char_uuid.uuid.uuid128, s_rx_char_uuid, 16);
  rx_char.property =
      TKL_BLE_GATT_CHAR_PROP_WRITE | TKL_BLE_GATT_CHAR_PROP_WRITE_NO_RSP;
  rx_char.permission = TKL_BLE_GATT_PERM_WRITE;

  service_params.char_num = 2;
  TKL_BLE_CHAR_PARAMS_T chars[2] = {tx_char, rx_char};
  service_params.p_char = chars;

  ret = tkl_ble_gatts_service_add(&service_params);
  if (ret != OPRT_OK) {
    TAL_LOGE(TAG, "tkl_ble_gatts_service_add failed: %d", ret);
    return -1;
  }

  s_tx_char_handle = service_params.p_char[0].char_handle;
  s_rx_char_handle = service_params.p_char[1].char_handle;

  TAL_LOGI(TAG, "BLE manager initialized, TX handle: %d, RX handle: %d",
           s_tx_char_handle, s_rx_char_handle);
  return 0;
}

int ble_manager_start_advertising(void) {
  TAL_LOGI(TAG, "Starting BLE advertising");

  // Set advertising data
  TKL_BLE_GAP_ADV_PARAMS_T adv_params = {0};
  adv_params.adv_type = TKL_BLE_GAP_ADV_TYPE_CONN_SCANNABLE_UNDIRECTED;
  adv_params.adv_interval_min = 160; // 100ms
  adv_params.adv_interval_max = 320; // 200ms
  adv_params.adv_channel_map = 0x07; // All channels

  // Advertising data
  uint8_t adv_data[31] = {0};
  uint8_t adv_len = 0;

  // Flags
  adv_data[adv_len++] = 0x02;
  adv_data[adv_len++] = 0x01;
  adv_data[adv_len++] = 0x06; // General discoverable + BR/EDR not supported

  // Complete local name
  const char *name = BLE_DEVICE_NAME;
  uint8_t name_len = strlen(name);
  adv_data[adv_len++] = name_len + 1;
  adv_data[adv_len++] = 0x09; // Complete local name
  memcpy(&adv_data[adv_len], name, name_len);
  adv_len += name_len;

  adv_params.p_adv_data = adv_data;
  adv_params.adv_data_len = adv_len;

  OPERATE_RET ret = tkl_ble_gap_adv_start(&adv_params);
  if (ret != OPRT_OK) {
    TAL_LOGE(TAG, "tkl_ble_gap_adv_start failed: %d", ret);
    s_ble_status = BLE_STATUS_ERROR;
    return -1;
  }

  s_ble_status = BLE_STATUS_ADVERTISING;
  if (s_status_callback) {
    s_status_callback(s_ble_status);
  }

  TAL_LOGI(TAG, "BLE advertising started");
  return 0;
}

void ble_manager_stop_advertising(void) {
  tkl_ble_gap_adv_stop();
  if (s_ble_status == BLE_STATUS_ADVERTISING) {
    s_ble_status = BLE_STATUS_IDLE;
  }
}

int ble_manager_send_data(const uint8_t *data, uint16_t len) {
  if (!data || len == 0) {
    return -1;
  }

  if (s_conn_handle == 0xFFFF) {
    TAL_LOGW(TAG, "No BLE connection");
    return -1;
  }

  TKL_BLE_NOTIFY_PARAMS_T notify_params = {0};
  notify_params.conn_handle = s_conn_handle;
  notify_params.char_handle = s_tx_char_handle;
  notify_params.p_data = (uint8_t *)data;
  notify_params.data_len = len;
  notify_params.type = TKL_BLE_GATT_NOTIFY;

  OPERATE_RET ret = tkl_ble_gatts_value_notify(&notify_params);
  if (ret != OPRT_OK) {
    TAL_LOGE(TAG, "tkl_ble_gatts_value_notify failed: %d", ret);
    return -1;
  }

  return 0;
}

int ble_manager_send_message(ble_msg_type_t type, const uint8_t *data,
                             uint16_t len) {
  // Build message packet: [type:1][len:2][data:len]
  uint16_t packet_len = 3 + len;
  uint8_t *packet = (uint8_t *)tal_malloc(packet_len);
  if (!packet) {
    return -1;
  }

  packet[0] = (uint8_t)type;
  packet[1] = (uint8_t)(len & 0xFF);
  packet[2] = (uint8_t)((len >> 8) & 0xFF);
  if (data && len > 0) {
    memcpy(&packet[3], data, len);
  }

  int ret = ble_manager_send_data(packet, packet_len);
  tal_free(packet);

  return ret;
}

int ble_manager_send_analysis_results(uint32_t mistake_count,
                                      const char *feedback_text) {
  // Build analysis result message
  uint16_t text_len = feedback_text ? strlen(feedback_text) : 0;
  uint16_t data_len = 4 + text_len; // 4 bytes for mistake_count

  uint8_t *data = (uint8_t *)tal_malloc(data_len);
  if (!data) {
    return -1;
  }

  // Mistake count (4 bytes, little endian)
  data[0] = (uint8_t)(mistake_count & 0xFF);
  data[1] = (uint8_t)((mistake_count >> 8) & 0xFF);
  data[2] = (uint8_t)((mistake_count >> 16) & 0xFF);
  data[3] = (uint8_t)((mistake_count >> 24) & 0xFF);

  // Feedback text
  if (text_len > 0) {
    memcpy(&data[4], feedback_text, text_len);
  }

  int ret = ble_manager_send_message(BLE_MSG_ANALYSIS_RESULT, data, data_len);
  tal_free(data);

  return ret;
}

ble_status_t ble_manager_get_status(void) { return s_ble_status; }

bool ble_manager_is_connected(void) {
  return (s_ble_status == BLE_STATUS_CONNECTED);
}

void ble_manager_set_data_callback(ble_data_cb_t callback) {
  s_data_callback = callback;
}

void ble_manager_set_status_callback(ble_status_cb_t callback) {
  s_status_callback = callback;
}

void ble_manager_disconnect(void) {
  if (s_conn_handle != 0xFFFF) {
    tkl_ble_gap_disconnect(s_conn_handle, 0x13); // Remote user terminated
    s_conn_handle = 0xFFFF;
  }
}
