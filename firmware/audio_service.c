/**
 * Audio Service Implementation for T5AI Music Coach
 * Uses TuyaOpen TDL Audio APIs
 */

#include "audio_service.h"
#include "tuya_config.h"

#include "tal_log.h"
#include "tal_memory.h"
#include "tal_mutex.h"
#include "tal_system.h"
#include "tdl_audio_manage.h"
#include "tuya_cloud_com_defs.h"

#include <string.h>

#define TAG "AUDIO_SVC"

// Maximum recording buffer size (5 minutes at 16kHz stereo 16-bit)
#define MAX_RECORDING_SIZE                                                     \
  (AUDIO_SAMPLE_RATE * AUDIO_CHANNELS * 2 * MAX_RECORDING_SECONDS)

// Audio state
static audio_state_t s_audio_state = AUDIO_STATE_IDLE;
static MUTEX_HANDLE s_mutex = NULL;

// Recording buffer
static uint8_t *s_recording_buffer = NULL;
static uint32_t s_recording_size = 0;
static uint32_t s_recording_pos = 0;
static audio_chunk_cb_t s_chunk_callback = NULL;

// Playback
static audio_play_complete_cb_t s_play_complete_cb = NULL;
static float s_volume = 0.8f;

// Audio device handle
static TDL_AUDIO_HANDLE_T s_audio_handle = NULL;

/**
 * Audio input callback (recording)
 */
static void audio_input_callback(TDL_AUDIO_HANDLE_T handle, uint8_t *data,
                                 uint32_t len) {
  if (s_audio_state != AUDIO_STATE_RECORDING) {
    return;
  }

  // Store in recording buffer
  if (s_recording_buffer && s_recording_pos + len <= MAX_RECORDING_SIZE) {
    memcpy(s_recording_buffer + s_recording_pos, data, len);
    s_recording_pos += len;
    s_recording_size = s_recording_pos;
  }

  // Call chunk callback for real-time processing
  if (s_chunk_callback) {
    uint32_t sample_count = len / (AUDIO_CHANNELS * sizeof(int16_t));
    s_chunk_callback((const int16_t *)data, sample_count);
  }
}

/**
 * Audio output callback (playback complete)
 */
static void audio_output_callback(TDL_AUDIO_HANDLE_T handle,
                                  TDL_AUDIO_EVENT_E event) {
  if (event == TDL_AUDIO_EVENT_PLAY_COMPLETE) {
    TAL_LOGI(TAG, "Playback complete");
    s_audio_state = AUDIO_STATE_IDLE;

    if (s_play_complete_cb) {
      s_play_complete_cb();
      s_play_complete_cb = NULL;
    }
  }
}

int audio_service_init(void) {
  TAL_LOGI(TAG, "Initializing audio service");

  // Create mutex
  OPERATE_RET ret = tal_mutex_create_init(&s_mutex);
  if (ret != OPRT_OK) {
    TAL_LOGE(TAG, "Failed to create mutex: %d", ret);
    return -1;
  }

  // Allocate recording buffer
  s_recording_buffer = (uint8_t *)tal_malloc(MAX_RECORDING_SIZE);
  if (!s_recording_buffer) {
    TAL_LOGE(TAG, "Failed to allocate recording buffer");
    return -1;
  }

  // Initialize audio device
  TDL_AUDIO_CFG_T audio_cfg = {0};
  audio_cfg.sample_rate = AUDIO_SAMPLE_RATE;
  audio_cfg.bit_depth = AUDIO_BIT_DEPTH;
  audio_cfg.channels = AUDIO_CHANNELS;
  audio_cfg.buffer_size = AUDIO_BUFFER_SIZE;
  audio_cfg.input_cb = audio_input_callback;
  audio_cfg.output_cb = audio_output_callback;

  ret = tdl_audio_open(&audio_cfg, &s_audio_handle);
  if (ret != OPRT_OK) {
    TAL_LOGE(TAG, "tdl_audio_open failed: %d", ret);
    tal_free(s_recording_buffer);
    s_recording_buffer = NULL;
    return -1;
  }

  // Set initial volume
  tdl_audio_set_volume(s_audio_handle, (int)(s_volume * 100));

  TAL_LOGI(TAG, "Audio service initialized");
  return 0;
}

void audio_service_deinit(void) {
  if (s_audio_handle) {
    tdl_audio_close(s_audio_handle);
    s_audio_handle = NULL;
  }

  if (s_recording_buffer) {
    tal_free(s_recording_buffer);
    s_recording_buffer = NULL;
  }

  if (s_mutex) {
    tal_mutex_release(s_mutex);
    s_mutex = NULL;
  }

  s_audio_state = AUDIO_STATE_IDLE;
}

int audio_service_start_recording(audio_chunk_cb_t chunk_callback) {
  if (s_audio_state != AUDIO_STATE_IDLE) {
    TAL_LOGW(TAG, "Audio service busy");
    return -1;
  }

  tal_mutex_lock(s_mutex);

  // Reset recording buffer
  s_recording_pos = 0;
  s_recording_size = 0;
  s_chunk_callback = chunk_callback;

  // Start audio input
  OPERATE_RET ret = tdl_audio_start_record(s_audio_handle);
  if (ret != OPRT_OK) {
    TAL_LOGE(TAG, "tdl_audio_start_record failed: %d", ret);
    tal_mutex_unlock(s_mutex);
    return -1;
  }

  s_audio_state = AUDIO_STATE_RECORDING;
  tal_mutex_unlock(s_mutex);

  TAL_LOGI(TAG, "Recording started");
  return 0;
}

int audio_service_stop_recording(uint8_t *out_buffer, uint32_t buffer_size) {
  if (s_audio_state != AUDIO_STATE_RECORDING) {
    return -1;
  }

  tal_mutex_lock(s_mutex);

  // Stop audio input
  tdl_audio_stop_record(s_audio_handle);
  s_audio_state = AUDIO_STATE_IDLE;
  s_chunk_callback = NULL;

  TAL_LOGI(TAG, "Recording stopped, size: %lu bytes",
           (unsigned long)s_recording_size);

  // Copy to output buffer if provided
  int copied = 0;
  if (out_buffer && buffer_size > 0) {
    uint32_t copy_size =
        (s_recording_size < buffer_size) ? s_recording_size : buffer_size;
    memcpy(out_buffer, s_recording_buffer, copy_size);
    copied = copy_size;
  }

  tal_mutex_unlock(s_mutex);
  return copied;
}

const uint8_t *audio_service_get_recording(uint32_t *out_size) {
  if (out_size) {
    *out_size = s_recording_size;
  }
  return s_recording_buffer;
}

int audio_service_play(const uint8_t *data, uint32_t size,
                       audio_play_complete_cb_t callback) {
  if (!data || size == 0) {
    return -1;
  }

  if (s_audio_state == AUDIO_STATE_PLAYING) {
    audio_service_stop_playback();
  }

  tal_mutex_lock(s_mutex);

  s_play_complete_cb = callback;

  OPERATE_RET ret = tdl_audio_play(s_audio_handle, (uint8_t *)data, size);
  if (ret != OPRT_OK) {
    TAL_LOGE(TAG, "tdl_audio_play failed: %d", ret);
    tal_mutex_unlock(s_mutex);
    return -1;
  }

  s_audio_state = AUDIO_STATE_PLAYING;
  tal_mutex_unlock(s_mutex);

  TAL_LOGI(TAG, "Playing audio, size: %lu bytes", (unsigned long)size);
  return 0;
}

int audio_service_speak(const char *text, audio_play_complete_cb_t callback) {
  if (!text || strlen(text) == 0) {
    return -1;
  }

  TAL_LOGI(TAG, "TTS: %s", text);

  // Use Tuya cloud TTS service
  // The TTS API will synthesize speech and play through the audio output
  s_play_complete_cb = callback;

  // Call Tuya AI TTS service
  OPERATE_RET ret = tdl_audio_tts_speak(s_audio_handle, text, strlen(text));
  if (ret != OPRT_OK) {
    TAL_LOGE(TAG, "TTS failed: %d", ret);
    // Fallback: just log the message
    if (callback) {
      callback();
    }
    return -1;
  }

  s_audio_state = AUDIO_STATE_PLAYING;
  return 0;
}

void audio_service_stop_playback(void) {
  if (s_audio_state == AUDIO_STATE_PLAYING) {
    tdl_audio_stop_play(s_audio_handle);
    s_audio_state = AUDIO_STATE_IDLE;
    s_play_complete_cb = NULL;
  }
}

void audio_service_set_volume(float volume) {
  if (volume < 0.0f)
    volume = 0.0f;
  if (volume > 1.0f)
    volume = 1.0f;

  s_volume = volume;

  if (s_audio_handle) {
    tdl_audio_set_volume(s_audio_handle, (int)(volume * 100));
  }
}

float audio_service_get_volume(void) { return s_volume; }

audio_state_t audio_service_get_state(void) { return s_audio_state; }

bool audio_service_is_recording(void) {
  return (s_audio_state == AUDIO_STATE_RECORDING);
}

bool audio_service_is_playing(void) {
  return (s_audio_state == AUDIO_STATE_PLAYING);
}
