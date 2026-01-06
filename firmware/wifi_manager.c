/**
 * WiFi Manager Implementation for T5AI Music Coach
 * Uses TuyaOpen TAL WiFi APIs
 */

#include "wifi_manager.h"
#include "tuya_config.h"

#include "tal_log.h"
#include "tal_system.h"
#include "tal_wifi.h"
#include "tkl_wifi.h"

#define TAG "WIFI_MGR"

// Current WiFi status
static wifi_status_t s_wifi_status = WIFI_STATUS_DISCONNECTED;
static wifi_event_cb_t s_event_callback = NULL;
static char s_current_ip[16] = {0};

/**
 * WiFi event handler from TAL
 */
static void wifi_event_handler(WF_EVENT_E event, void *arg) {
  switch (event) {
  case WFE_CONNECTED:
    TAL_LOGI(TAG, "WiFi connected to AP");
    s_wifi_status = WIFI_STATUS_CONNECTING;
    break;

  case WFE_CONNECT_FAILED:
    TAL_LOGE(TAG, "WiFi connection failed");
    s_wifi_status = WIFI_STATUS_ERROR;
    if (s_event_callback) {
      s_event_callback(s_wifi_status);
    }
    break;

  case WFE_GOT_IP:
    TAL_LOGI(TAG, "WiFi got IP address");
    s_wifi_status = WIFI_STATUS_CONNECTED;
    // Get the IP address
    NW_IP_S ip_info;
    if (tal_wifi_get_ip(WF_STATION, &ip_info) == OPRT_OK) {
      snprintf(s_current_ip, sizeof(s_current_ip), "%d.%d.%d.%d",
               (ip_info.ip >> 0) & 0xFF, (ip_info.ip >> 8) & 0xFF,
               (ip_info.ip >> 16) & 0xFF, (ip_info.ip >> 24) & 0xFF);
      TAL_LOGI(TAG, "IP: %s", s_current_ip);
    }
    if (s_event_callback) {
      s_event_callback(s_wifi_status);
    }
    break;

  case WFE_DISCONNECTED:
    TAL_LOGW(TAG, "WiFi disconnected");
    s_wifi_status = WIFI_STATUS_DISCONNECTED;
    s_current_ip[0] = '\0';
    if (s_event_callback) {
      s_event_callback(s_wifi_status);
    }
    break;

  default:
    break;
  }
}

int wifi_manager_init(void) {
  TAL_LOGI(TAG, "Initializing WiFi manager");

  // Initialize WiFi hardware
  OPERATE_RET ret = tal_wifi_init(wifi_event_handler);
  if (ret != OPRT_OK) {
    TAL_LOGE(TAG, "tal_wifi_init failed: %d", ret);
    return -1;
  }

  // Set WiFi to station mode
  ret = tal_wifi_set_work_mode(WWM_STATION);
  if (ret != OPRT_OK) {
    TAL_LOGE(TAG, "Failed to set station mode: %d", ret);
    return -1;
  }

  TAL_LOGI(TAG, "WiFi manager initialized");
  return 0;
}

int wifi_manager_connect(void) {
  return wifi_manager_connect_to(WIFI_SSID, WIFI_PASSWORD);
}

int wifi_manager_connect_to(const char *ssid, const char *password) {
  if (!ssid || !password) {
    TAL_LOGE(TAG, "Invalid SSID or password");
    return -1;
  }

  TAL_LOGI(TAG, "Connecting to WiFi: %s", ssid);
  s_wifi_status = WIFI_STATUS_CONNECTING;

  // Connect to the specified network
  OPERATE_RET ret = tal_wifi_station_connect(ssid, password);
  if (ret != OPRT_OK) {
    TAL_LOGE(TAG, "tal_wifi_station_connect failed: %d", ret);
    s_wifi_status = WIFI_STATUS_ERROR;
    return -1;
  }

  return 0;
}

void wifi_manager_disconnect(void) {
  TAL_LOGI(TAG, "Disconnecting WiFi");
  tal_wifi_station_disconnect();
  s_wifi_status = WIFI_STATUS_DISCONNECTED;
  s_current_ip[0] = '\0';
}

wifi_status_t wifi_manager_get_status(void) { return s_wifi_status; }

bool wifi_manager_is_connected(void) {
  return (s_wifi_status == WIFI_STATUS_CONNECTED);
}

int wifi_manager_get_ip(char *ip_buf) {
  if (!ip_buf) {
    return -1;
  }

  if (s_wifi_status != WIFI_STATUS_CONNECTED) {
    return -1;
  }

  strcpy(ip_buf, s_current_ip);
  return 0;
}

void wifi_manager_set_callback(wifi_event_cb_t callback) {
  s_event_callback = callback;
}
