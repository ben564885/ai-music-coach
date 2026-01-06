/**
 * Audio Service for T5AI Music Coach
 * Uses TuyaOpen TDL Audio APIs for recording and playback
 */

#ifndef AUDIO_SERVICE_H
#define AUDIO_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * Audio service state
 */
typedef enum {
  AUDIO_STATE_IDLE = 0,
  AUDIO_STATE_RECORDING,
  AUDIO_STATE_PLAYING,
  AUDIO_STATE_ERROR
} audio_state_t;

/**
 * Audio chunk callback (called during recording)
 */
typedef void (*audio_chunk_cb_t)(const int16_t *samples, uint32_t sample_count);

/**
 * Audio playback complete callback
 */
typedef void (*audio_play_complete_cb_t)(void);

/**
 * Initialize audio subsystem
 * @return 0 on success, negative on error
 */
int audio_service_init(void);

/**
 * Deinitialize audio subsystem
 */
void audio_service_deinit(void);

/**
 * Start audio recording
 * @param chunk_callback Called for each audio chunk (optional)
 * @return 0 on success, negative on error
 */
int audio_service_start_recording(audio_chunk_cb_t chunk_callback);

/**
 * Stop audio recording
 * @param out_buffer Output buffer to receive recorded audio (optional)
 * @param buffer_size Size of output buffer in bytes
 * @return Number of bytes written, or negative on error
 */
int audio_service_stop_recording(uint8_t *out_buffer, uint32_t buffer_size);

/**
 * Get recording buffer
 * Returns pointer to internal recording buffer after stop_recording
 * @param out_size Size of the buffer in bytes
 * @return Pointer to buffer, or NULL
 */
const uint8_t *audio_service_get_recording(uint32_t *out_size);

/**
 * Play audio data
 * @param data Audio data (PCM 16-bit)
 * @param size Size in bytes
 * @param callback Called when playback completes (optional)
 * @return 0 on success, negative on error
 */
int audio_service_play(const uint8_t *data, uint32_t size,
                       audio_play_complete_cb_t callback);

/**
 * Play Text-to-Speech
 * Uses Tuya cloud TTS or local TTS engine
 * @param text Text to speak
 * @param callback Called when playback completes (optional)
 * @return 0 on success, negative on error
 */
int audio_service_speak(const char *text, audio_play_complete_cb_t callback);

/**
 * Stop audio playback
 */
void audio_service_stop_playback(void);

/**
 * Set playback volume
 * @param volume 0.0 to 1.0
 */
void audio_service_set_volume(float volume);

/**
 * Get playback volume
 * @return Current volume 0.0 to 1.0
 */
float audio_service_get_volume(void);

/**
 * Get current audio state
 * @return Current state
 */
audio_state_t audio_service_get_state(void);

/**
 * Check if recording
 * @return true if recording
 */
bool audio_service_is_recording(void);

/**
 * Check if playing
 * @return true if playing
 */
bool audio_service_is_playing(void);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_SERVICE_H
