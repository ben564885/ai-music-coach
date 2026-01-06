/**
 * Cloud Client for T5AI Music Coach
 * HTTP client for backend communication
 */

#ifndef CLOUD_CLIENT_H
#define CLOUD_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * Analysis result structure
 */
typedef struct {
  uint32_t mistake_count;
  char *feedback_text; // Allocated, caller must free
  float overall_score; // 0-100
} cloud_analysis_result_t;

/**
 * Cloud request callback
 */
typedef void (*cloud_result_cb_t)(bool success,
                                  cloud_analysis_result_t *result);

/**
 * Initialize cloud client
 * @return 0 on success, negative on error
 */
int cloud_client_init(void);

/**
 * Deinitialize cloud client
 */
void cloud_client_deinit(void);

/**
 * Upload audio recording to backend for analysis
 * @param audio_data PCM audio data
 * @param audio_size Size in bytes
 * @param session_id Recording session ID
 * @param callback Result callback (optional, called async)
 * @return 0 on success (request sent), negative on error
 */
int cloud_client_upload_audio(const uint8_t *audio_data, uint32_t audio_size,
                              const char *session_id,
                              cloud_result_cb_t callback);

/**
 * Upload audio recording (synchronous version)
 * @param audio_data PCM audio data
 * @param audio_size Size in bytes
 * @param session_id Recording session ID
 * @param result Output analysis result
 * @return 0 on success, negative on error
 */
int cloud_client_upload_audio_sync(const uint8_t *audio_data,
                                   uint32_t audio_size, const char *session_id,
                                   cloud_analysis_result_t *result);

/**
 * Get analysis results for a session
 * @param session_id Recording session ID
 * @param result Output analysis result
 * @return 0 on success, negative on error
 */
int cloud_client_get_analysis(const char *session_id,
                              cloud_analysis_result_t *result);

/**
 * Send sheet music reference data to backend
 * @param reference_json JSON string with reference note data
 * @param session_id Recording session ID
 * @return 0 on success, negative on error
 */
int cloud_client_set_reference(const char *reference_json,
                               const char *session_id);

/**
 * Check backend connectivity
 * @return true if backend is reachable
 */
bool cloud_client_is_available(void);

/**
 * Free analysis result memory
 * @param result Result to free
 */
void cloud_client_free_result(cloud_analysis_result_t *result);

#ifdef __cplusplus
}
#endif

#endif // CLOUD_CLIENT_H
