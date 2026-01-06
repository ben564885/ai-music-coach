/**
 * T5AI Music Coach Firmware - Main Application
 *
 * Entry point for TuyaOpen SDK based firmware.
 * Handles initialization of all subsystems and main application loop.
 */

#include <stdio.h>
#include <string.h>

// TuyaOpen SDK headers
#include "tal_api.h"
#include "tal_log.h"
#include "tal_memory.h"
#include "tal_system.h"
#include "tal_thread.h"
#include "tkl_gpio.h"
#include "tuya_cloud_types.h"

// Application modules
#include "audio_service.h"
#include "ble_manager.h"
#include "cloud_client.h"
#include "display_service.h"
#include "real_time_analyzer.h"
#include "tuya_config.h"
#include "types.h"
#include "wifi_manager.h"

#define TAG "MUSIC_COACH"

/* =============================================================================
 * Application State
 * =============================================================================
 */

typedef struct {
  bool is_recording;
  bool wifi_connected;
  bool ble_connected;
  bool reference_loaded;
  instrument_type_t current_instrument;
  uint32_t recording_start_time;
  char session_id[64];
  char *reference_json;
} app_state_t;

static app_state_t s_app_state = {.is_recording = false,
                                  .wifi_connected = false,
                                  .ble_connected = false,
                                  .reference_loaded = false,
                                  .current_instrument = INSTRUMENT_PIANO,
                                  .recording_start_time = 0,
                                  .session_id = "",
                                  .reference_json = NULL};

// Real-time analyzer instance
static RealTimeAnalyzer *s_rt_analyzer = NULL;

// Task handles
static THREAD_HANDLE s_main_task = NULL;
static THREAD_HANDLE s_audio_task = NULL;
static THREAD_HANDLE s_display_task = NULL;

/* =============================================================================
 * Callbacks
 * =============================================================================
 */

/**
 * WiFi status callback
 */
static void on_wifi_status(wifi_status_t status) {
  TAL_LOGI(TAG, "WiFi status: %d", status);
  s_app_state.wifi_connected = (status == WIFI_STATUS_CONNECTED);
  display_service_show_connection(s_app_state.wifi_connected,
                                  s_app_state.ble_connected);

  if (s_app_state.wifi_connected) {
    audio_service_speak("WiFi connected", NULL);
  }
}

/**
 * BLE status callback
 */
static void on_ble_status(ble_status_t status) {
  TAL_LOGI(TAG, "BLE status: %d", status);
  s_app_state.ble_connected = (status == BLE_STATUS_CONNECTED);
  display_service_show_connection(s_app_state.wifi_connected,
                                  s_app_state.ble_connected);

  if (s_app_state.ble_connected) {
    audio_service_speak("Mobile app connected", NULL);
  }
}

/**
 * BLE data received callback
 */
static void on_ble_data_received(const uint8_t *data, uint16_t len) {
  if (len < 3)
    return; // Minimum: type + len (2 bytes)

  ble_msg_type_t msg_type = (ble_msg_type_t)data[0];
  uint16_t payload_len = data[1] | (data[2] << 8);
  const uint8_t *payload = &data[3];

  TAL_LOGI(TAG, "BLE message received: type=%d, len=%d", msg_type, payload_len);

  switch (msg_type) {
  case BLE_MSG_SHEET_MUSIC:
    // Store reference JSON
    if (s_app_state.reference_json) {
      tal_free(s_app_state.reference_json);
    }
    s_app_state.reference_json = (char *)tal_malloc(payload_len + 1);
    if (s_app_state.reference_json) {
      memcpy(s_app_state.reference_json, payload, payload_len);
      s_app_state.reference_json[payload_len] = '\0';
      s_app_state.reference_loaded = true;
      audio_service_speak("Sheet music received. Ready to record.", NULL);
      display_service_show_message(
          "Ready", "Sheet music loaded.\nPress button to record.");
    }
    break;

  case BLE_MSG_START_RECORDING:
    if (!s_app_state.is_recording) {
      start_recording();
    }
    break;

  case BLE_MSG_STOP_RECORDING:
    if (s_app_state.is_recording) {
      stop_recording();
    }
    break;

  case BLE_MSG_SET_INSTRUMENT:
    if (payload_len >= 1) {
      s_app_state.current_instrument = (instrument_type_t)payload[0];
      display_service_set_instrument(s_app_state.current_instrument);
      TAL_LOGI(TAG, "Instrument set to: %d", s_app_state.current_instrument);
    }
    break;

  default:
    TAL_LOGW(TAG, "Unknown BLE message type: %d", msg_type);
    break;
  }
}

/**
 * Audio chunk callback (real-time analysis)
 */
static void on_audio_chunk(const int16_t *samples, uint32_t sample_count) {
  if (!s_rt_analyzer || !s_app_state.is_recording)
    return;

  // Process audio chunk for real-time note detection
  NoteDetection detected =
      s_rt_analyzer->process_audio_chunk(samples, sample_count);

  if (detected.is_valid && detected.confidence > RT_CONFIDENCE_THRESHOLD) {
    // Display detected note
    display_note_info_t note_info = {
        .note_name = detected.note_name,
        .octave = detected.octave,
        .frequency = detected.frequency,
        .is_correct = true // Will be updated if we have expected note
    };

    // Check against expected note if reference is loaded
    ExpectedNote expected = s_rt_analyzer->get_current_expected_note();
    if (expected.frequency > 0) {
      bool matches = s_rt_analyzer->check_note_match(detected, expected);
      note_info.is_correct = matches;

      if (!matches) {
        // Show wrong note and correct fingering
        display_note_info_t expected_info = {.note_name = expected.note_name,
                                             .octave = expected.octave,
                                             .frequency = expected.frequency,
                                             .is_correct = true};
        display_service_show_wrong_note(&note_info, &expected_info);

        // Speak correction
        char feedback[128];
        snprintf(feedback, sizeof(feedback),
                 "You played %s, but the correct note is %s%d",
                 detected.note_name, expected.note_name, expected.octave);
        audio_service_speak(feedback, NULL);
      }
    }

    display_service_show_note(&note_info);
  }
}

/* =============================================================================
 * Recording Control
 * =============================================================================
 */

static void start_recording(void) {
  if (s_app_state.is_recording)
    return;

  if (!s_app_state.ble_connected) {
    audio_service_speak("Please connect the mobile app first", NULL);
    return;
  }

  TAL_LOGI(TAG, "Starting recording");

  // Generate session ID
  snprintf(s_app_state.session_id, sizeof(s_app_state.session_id),
           "session_%lu", (unsigned long)tal_system_get_millisecond());

  // Send reference data to backend if available
  if (s_app_state.reference_json && s_app_state.wifi_connected) {
    cloud_client_set_reference(s_app_state.reference_json,
                               s_app_state.session_id);
  }

  // Start audio recording with real-time callback
  int ret = audio_service_start_recording(on_audio_chunk);
  if (ret != 0) {
    audio_service_speak("Failed to start recording", NULL);
    return;
  }

  s_app_state.is_recording = true;
  s_app_state.recording_start_time = tal_system_get_millisecond();

  audio_service_speak("Recording started. Begin playing when ready.", NULL);
  display_service_show_recording(true, 0);

  // Notify mobile app
  ble_manager_send_message(BLE_MSG_STATUS_UPDATE, (uint8_t *)"recording", 9);
}

static void stop_recording(void) {
  if (!s_app_state.is_recording)
    return;

  TAL_LOGI(TAG, "Stopping recording");

  // Stop recording
  audio_service_stop_recording(NULL, 0);
  s_app_state.is_recording = false;

  display_service_show_recording(false, 0);
  audio_service_speak("Recording complete. Analyzing your performance.", NULL);

  // Get recorded audio
  uint32_t audio_size;
  const uint8_t *audio_data = audio_service_get_recording(&audio_size);

  if (audio_data && audio_size > 0 && s_app_state.wifi_connected) {
    display_service_show_message("Analyzing", "Please wait...");

    // Upload to cloud for analysis
    cloud_analysis_result_t result = {0};
    int ret = cloud_client_upload_audio_sync(audio_data, audio_size,
                                             s_app_state.session_id, &result);

    if (ret == 0) {
      TAL_LOGI(TAG, "Analysis complete: %lu mistakes, score: %.1f",
               (unsigned long)result.mistake_count, result.overall_score);

      // Speak feedback
      if (result.feedback_text) {
        audio_service_speak(result.feedback_text, NULL);
      }

      // Send results to mobile app
      ble_manager_send_analysis_results(result.mistake_count,
                                        result.feedback_text);

      // Display summary
      char summary[128];
      snprintf(summary, sizeof(summary), "Mistakes: %lu\nScore: %.0f%%",
               (unsigned long)result.mistake_count, result.overall_score);
      display_service_show_message("Results", summary);

      cloud_client_free_result(&result);
    } else {
      audio_service_speak("Failed to analyze recording. Please try again.",
                          NULL);
      display_service_show_message("Error", "Analysis failed");
    }
  } else {
    display_service_show_message("Done", "Recording saved locally");
  }

  // Notify mobile app
  ble_manager_send_message(BLE_MSG_STATUS_UPDATE, (uint8_t *)"idle", 4);
}

/* =============================================================================
 * Button Handler
 * =============================================================================
 */

static volatile bool s_button_pressed = false;

static void gpio_button_callback(void *arg) { s_button_pressed = true; }

static void init_button(void) {
  // Configure record button
  TUYA_GPIO_BASE_CFG_T gpio_cfg = {.mode = TUYA_GPIO_PULLUP,
                                   .direct = TUYA_GPIO_INPUT,
                                   .level = TUYA_GPIO_LEVEL_HIGH};

  tkl_gpio_init(GPIO_RECORD_BUTTON, &gpio_cfg);
  tkl_gpio_irq_init(GPIO_RECORD_BUTTON, TUYA_GPIO_IRQ_FALL,
                    gpio_button_callback, NULL);
}

static void handle_button_press(void) {
  if (!s_button_pressed)
    return;
  s_button_pressed = false;

  // Debounce
  static uint32_t last_press = 0;
  uint32_t now = tal_system_get_millisecond();
  if (now - last_press < 500)
    return;
  last_press = now;

  TAL_LOGI(TAG, "Button pressed");

  // Toggle recording
  if (s_app_state.is_recording) {
    stop_recording();
  } else {
    start_recording();
  }
}

/* =============================================================================
 * Tasks
 * =============================================================================
 */

/**
 * Main application task
 */
static void main_task(void *arg) {
  TAL_LOGI(TAG, "Main task started");

  while (1) {
    // Handle button press
    handle_button_press();

    // Update recording duration display
    if (s_app_state.is_recording) {
      uint32_t duration_ms =
          tal_system_get_millisecond() - s_app_state.recording_start_time;
      display_service_show_recording(true, duration_ms / 1000);
    }

    tal_system_sleep(100);
  }
}

/**
 * Display update task
 */
static void display_task(void *arg) {
  TAL_LOGI(TAG, "Display task started");

  while (1) {
    // Update LVGL
    display_service_update();

    // LVGL recommends 5-10ms tick
    tal_system_sleep(10);
  }
}

/* =============================================================================
 * Initialization
 * =============================================================================
 */

/**
 * Initialize all subsystems
 */
static int app_init(void) {
  TAL_LOGI(TAG, "=== AI Music Coach Initializing ===");

  // Initialize display first for feedback
  if (display_service_init() == 0) {
    display_service_show_message("Initializing", "Please wait...");
  }

  // Initialize WiFi
  TAL_LOGI(TAG, "Initializing WiFi...");
  if (wifi_manager_init() != 0) {
    TAL_LOGE(TAG, "WiFi init failed");
  } else {
    wifi_manager_set_callback(on_wifi_status);
    wifi_manager_connect();
  }

  // Initialize BLE
  TAL_LOGI(TAG, "Initializing BLE...");
  if (ble_manager_init() != 0) {
    TAL_LOGE(TAG, "BLE init failed");
  } else {
    ble_manager_set_status_callback(on_ble_status);
    ble_manager_set_data_callback(on_ble_data_received);
    ble_manager_start_advertising();
  }

  // Initialize audio service
  TAL_LOGI(TAG, "Initializing audio...");
  if (audio_service_init() != 0) {
    TAL_LOGE(TAG, "Audio init failed");
  }
  audio_service_set_volume(TTS_VOICE_VOLUME);

  // Initialize cloud client
  TAL_LOGI(TAG, "Initializing cloud client...");
  cloud_client_init();

  // Initialize real-time analyzer
  TAL_LOGI(TAG, "Initializing real-time analyzer...");
  s_rt_analyzer = new RealTimeAnalyzer();
  if (s_rt_analyzer) {
    s_rt_analyzer->init();
  }

  // Initialize button
  init_button();

  TAL_LOGI(TAG, "=== Initialization Complete ===");
  display_service_show_idle();
  audio_service_speak("Music Coach ready. Connect mobile app to begin.", NULL);

  return 0;
}

/* =============================================================================
 * Entry Point
 * =============================================================================
 */

/**
 * Application entry point
 * Called by TuyaOpen SDK after system initialization
 */
extern "C" void tuya_app_main(void) {
  TAL_LOGI(TAG, "T5AI Music Coach - Starting");
  TAL_LOGI(TAG, "Firmware Version: 1.0.0");

  // Initialize application
  if (app_init() != 0) {
    TAL_LOGE(TAG, "Application init failed!");
    return;
  }

  // Create main task
  THREAD_CFG_T main_cfg = {
      .thrdname = "main_task",
      .stackDepth = TASK_STACK_MAIN,
      .priority = TASK_PRIORITY_MAIN,
  };
  tal_thread_create_and_start(&s_main_task, NULL, NULL, main_task, NULL,
                              &main_cfg);

  // Create display task
  THREAD_CFG_T display_cfg = {
      .thrdname = "display_task",
      .stackDepth = TASK_STACK_DISPLAY,
      .priority = TASK_PRIORITY_DISPLAY,
  };
  tal_thread_create_and_start(&s_display_task, NULL, NULL, display_task, NULL,
                              &display_cfg);

  TAL_LOGI(TAG, "All tasks started");
}
