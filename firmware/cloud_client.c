/**
 * Cloud Client Implementation for T5AI Music Coach
 * Uses TuyaOpen HTTP client for backend communication
 */

#include "cloud_client.h"
#include "tuya_config.h"
#include "wifi_manager.h"

#include "cJSON.h"
#include "http_client_interface.h"
#include "tal_log.h"
#include "tal_memory.h"
#include "tal_system.h"

#include <stdio.h>
#include <string.h>

#define TAG "CLOUD_CLI"

// HTTP response buffer
#define HTTP_RESPONSE_BUFFER_SIZE 8192

static bool s_initialized = false;

int cloud_client_init(void) {
  TAL_LOGI(TAG, "Initializing cloud client");
  TAL_LOGI(TAG, "Backend URL: %s", CLOUD_BACKEND_URL);

  s_initialized = true;
  return 0;
}

void cloud_client_deinit(void) { s_initialized = false; }

/**
 * Build full URL for endpoint
 */
static void build_url(char *url_buf, size_t buf_size, const char *endpoint) {
  snprintf(url_buf, buf_size, "%s%s", CLOUD_BACKEND_URL, endpoint);
}

int cloud_client_upload_audio(const uint8_t *audio_data, uint32_t audio_size,
                              const char *session_id,
                              cloud_result_cb_t callback) {
  // For async operation, we would use a task
  // For now, use synchronous with callback
  cloud_analysis_result_t result = {0};
  int ret = cloud_client_upload_audio_sync(audio_data, audio_size, session_id,
                                           &result);

  if (callback) {
    callback(ret == 0, &result);
    cloud_client_free_result(&result);
  }

  return ret;
}

int cloud_client_upload_audio_sync(const uint8_t *audio_data,
                                   uint32_t audio_size, const char *session_id,
                                   cloud_analysis_result_t *result) {
  if (!s_initialized) {
    TAL_LOGE(TAG, "Cloud client not initialized");
    return -1;
  }

  if (!wifi_manager_is_connected()) {
    TAL_LOGE(TAG, "WiFi not connected");
    return -1;
  }

  if (!audio_data || audio_size == 0) {
    TAL_LOGE(TAG, "Invalid audio data");
    return -1;
  }

  TAL_LOGI(TAG, "Uploading audio: %lu bytes, session: %s",
           (unsigned long)audio_size, session_id ? session_id : "unknown");

  // Build URL
  char url[256];
  build_url(url, sizeof(url), CLOUD_UPLOAD_ENDPOINT);

  // Create HTTP client
  http_client_handle_t client = http_client_init();
  if (!client) {
    TAL_LOGE(TAG, "Failed to create HTTP client");
    return -1;
  }

  // Set request parameters
  http_client_set_url(client, url);
  http_client_set_method(client, HTTP_METHOD_POST);
  http_client_set_header(client, "Content-Type", "application/octet-stream");

  if (session_id) {
    http_client_set_header(client, "X-Session-ID", session_id);
  }

  // Set audio format headers
  char sample_rate_str[16];
  snprintf(sample_rate_str, sizeof(sample_rate_str), "%d", AUDIO_SAMPLE_RATE);
  http_client_set_header(client, "X-Audio-Sample-Rate", sample_rate_str);

  char channels_str[8];
  snprintf(channels_str, sizeof(channels_str), "%d", AUDIO_CHANNELS);
  http_client_set_header(client, "X-Audio-Channels", channels_str);

  // Set body
  http_client_set_post_field(client, (const char *)audio_data, audio_size);

  // Allocate response buffer
  char *response_buf = (char *)tal_malloc(HTTP_RESPONSE_BUFFER_SIZE);
  if (!response_buf) {
    http_client_cleanup(client);
    return -1;
  }

  // Perform request
  OPERATE_RET ret = http_client_perform(client);
  int http_status = http_client_get_status_code(client);

  if (ret != OPRT_OK || http_status != 200) {
    TAL_LOGE(TAG, "HTTP request failed: ret=%d, status=%d", ret, http_status);
    tal_free(response_buf);
    http_client_cleanup(client);
    return -1;
  }

  // Read response
  int response_len =
      http_client_read(client, response_buf, HTTP_RESPONSE_BUFFER_SIZE - 1);
  if (response_len > 0) {
    response_buf[response_len] = '\0';

    // Parse JSON response
    cJSON *json = cJSON_Parse(response_buf);
    if (json) {
      if (result) {
        cJSON *mistake_count = cJSON_GetObjectItem(json, "mistake_count");
        if (cJSON_IsNumber(mistake_count)) {
          result->mistake_count = (uint32_t)mistake_count->valueint;
        }

        cJSON *feedback = cJSON_GetObjectItem(json, "feedback");
        if (cJSON_IsString(feedback) && feedback->valuestring) {
          result->feedback_text = tal_strdup(feedback->valuestring);
        }

        cJSON *score = cJSON_GetObjectItem(json, "score");
        if (cJSON_IsNumber(score)) {
          result->overall_score = (float)score->valuedouble;
        }
      }
      cJSON_Delete(json);
      TAL_LOGI(TAG, "Analysis result: %lu mistakes, score: %.1f",
               (unsigned long)(result ? result->mistake_count : 0),
               result ? result->overall_score : 0.0f);
    }
  }

  tal_free(response_buf);
  http_client_cleanup(client);

  return 0;
}

int cloud_client_get_analysis(const char *session_id,
                              cloud_analysis_result_t *result) {
  if (!s_initialized || !wifi_manager_is_connected()) {
    return -1;
  }

  if (!session_id || !result) {
    return -1;
  }

  // Build URL with session ID
  char url[256];
  snprintf(url, sizeof(url), "%s%s?session_id=%s", CLOUD_BACKEND_URL,
           CLOUD_ANALYSIS_ENDPOINT, session_id);

  // Create HTTP client
  http_client_handle_t client = http_client_init();
  if (!client) {
    return -1;
  }

  http_client_set_url(client, url);
  http_client_set_method(client, HTTP_METHOD_GET);

  // Allocate response buffer
  char *response_buf = (char *)tal_malloc(HTTP_RESPONSE_BUFFER_SIZE);
  if (!response_buf) {
    http_client_cleanup(client);
    return -1;
  }

  // Perform request
  OPERATE_RET ret = http_client_perform(client);
  int http_status = http_client_get_status_code(client);

  if (ret != OPRT_OK || http_status != 200) {
    tal_free(response_buf);
    http_client_cleanup(client);
    return -1;
  }

  // Read and parse response
  int response_len =
      http_client_read(client, response_buf, HTTP_RESPONSE_BUFFER_SIZE - 1);
  if (response_len > 0) {
    response_buf[response_len] = '\0';

    cJSON *json = cJSON_Parse(response_buf);
    if (json) {
      cJSON *mistake_count = cJSON_GetObjectItem(json, "mistake_count");
      if (cJSON_IsNumber(mistake_count)) {
        result->mistake_count = (uint32_t)mistake_count->valueint;
      }

      cJSON *feedback = cJSON_GetObjectItem(json, "feedback");
      if (cJSON_IsString(feedback) && feedback->valuestring) {
        result->feedback_text = tal_strdup(feedback->valuestring);
      }

      cJSON *score = cJSON_GetObjectItem(json, "score");
      if (cJSON_IsNumber(score)) {
        result->overall_score = (float)score->valuedouble;
      }

      cJSON_Delete(json);
    }
  }

  tal_free(response_buf);
  http_client_cleanup(client);

  return 0;
}

int cloud_client_set_reference(const char *reference_json,
                               const char *session_id) {
  if (!s_initialized || !wifi_manager_is_connected()) {
    return -1;
  }

  if (!reference_json) {
    return -1;
  }

  TAL_LOGI(TAG, "Setting reference data for session: %s",
           session_id ? session_id : "unknown");

  // Build URL
  char url[256];
  build_url(url, sizeof(url), "/api/set_reference");

  // Create HTTP client
  http_client_handle_t client = http_client_init();
  if (!client) {
    return -1;
  }

  http_client_set_url(client, url);
  http_client_set_method(client, HTTP_METHOD_POST);
  http_client_set_header(client, "Content-Type", "application/json");

  if (session_id) {
    http_client_set_header(client, "X-Session-ID", session_id);
  }

  http_client_set_post_field(client, reference_json, strlen(reference_json));

  // Perform request
  OPERATE_RET ret = http_client_perform(client);
  int http_status = http_client_get_status_code(client);

  http_client_cleanup(client);

  if (ret != OPRT_OK || http_status != 200) {
    TAL_LOGE(TAG, "Failed to set reference: ret=%d, status=%d", ret,
             http_status);
    return -1;
  }

  return 0;
}

bool cloud_client_is_available(void) {
  if (!s_initialized || !wifi_manager_is_connected()) {
    return false;
  }

  // Simple health check
  char url[256];
  build_url(url, sizeof(url), "/health");

  http_client_handle_t client = http_client_init();
  if (!client) {
    return false;
  }

  http_client_set_url(client, url);
  http_client_set_method(client, HTTP_METHOD_GET);
  http_client_set_timeout_ms(client, 5000); // 5 second timeout

  OPERATE_RET ret = http_client_perform(client);
  int http_status = http_client_get_status_code(client);

  http_client_cleanup(client);

  return (ret == OPRT_OK && http_status == 200);
}

void cloud_client_free_result(cloud_analysis_result_t *result) {
  if (result && result->feedback_text) {
    tal_free(result->feedback_text);
    result->feedback_text = NULL;
  }
}
