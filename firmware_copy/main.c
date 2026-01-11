/**
 * @file main.c
 * @brief AI Music Coach / PracticePod - Firmware for T5AI
 *
 * This version uses the Tuya IoT SDK for "Best Practice" Authentication &
 * Pairing.
 *
 * Build with: tos.py build
 */

#include "board_com_api.h"
#include "cJSON.h"
#include "http_client_interface.h" // For HTTP requests
#include "lv_vendor.h"
#include "lvgl.h"
#include "netconn_wifi.h"
#include "netmgr.h"
#include "tal_api.h"
#include "tal_log.h"
#include "tal_memory.h"
#include "tal_network.h"
#include "tal_system.h"
#include "tal_thread.h"
#include "tal_wifi.h"
#include "tkl_audio.h"
#include "tkl_fs.h"
#include "tkl_output.h"
#include "tuya_iot.h"
#include "tuya_ringbuf.h"
#include "wav_encode.h"
#include <string.h>

#include "tkl_gpio.h" // For LED Debug

#define DISPLAY_NAME "display"

#define TAG "practicepod"

// =================================================================
// PRODUCT CONFIGURATION
// You MUST obtain these from the Tuya IoT Platform (iot.tuya.com)
// Create a "Wi-Fi + BLE" Product to get them.
// =================================================================
#define PRODUCT_KEY "qdqsfcjifdin8t1t"              // REPLACE WITH YOUR PID
#define DEVICE_UUID "uuid2395651a4cae9262"          // REPLACE WITH YOUR UUID
#define AUTH_KEY "dzVrgPGISjLsVkzmpjFTNYKWHM3GzIws" // REPLACE WITH YOUR AUTHKEY
// =================================================================

// IMPORTANT: This must be the IP of your computer on the phone's hotspot
// network Connect your computer to the phone hotspot, then find its IP (e.g.,
// 172.20.10.x) and update this value. The device and computer must be on the
// same network.
#define CLOUD_BACKEND_HOST "192.168.34.176"
#define CLOUD_BACKEND_PORT 5001
#define CLOUD_BACKEND_PATH "/api/firmware/upload"

#define SYMBOL_MIC "\xEF\x84\xB0"

#define MIC_SAMPLE_RATE TKL_AUDIO_SAMPLE_16K
#define MIC_SAMPLE_BITS TKL_AUDIO_DATABITS_16
#define MIC_CHANNEL TKL_AUDIO_CHANNEL_MONO
#define MIC_MAX_DURATION_S (60) // 1 minute max for now
#define PCM_BUF_SIZE                                                           \
  (MIC_MAX_DURATION_S * MIC_SAMPLE_RATE * MIC_SAMPLE_BITS * MIC_CHANNEL / 8)

#define SD_MOUNT_POINT "/sdcard"
#define TEMP_PCM_PATH "/sdcard/temp.pcm"
#define FINAL_WAV_PATH "/sdcard/record.wav"

typedef enum {
  SCREEN_MAIN = 0,
  SCREEN_RECORDING,
  SCREEN_ANALYZE,
  SCREEN_SESSIONS,
  SCREEN_UPLOADS,
  SCREEN_VIEW_UPLOAD,
  SCREEN_PLAYBACK,
  SCREEN_SELECT_SESSION, // For analyze - session selection
  SCREEN_SELECT_SHEET    // For analyze - sheet music selection
} screen_state_t;

#define MAX_SESSIONS 5
#define MAX_UPLOADS 5
#define MAX_TITLE_LEN 32

typedef struct {
  char id[48];
  char title[MAX_TITLE_LEN];
  char date[32];       // Formatted: "Jan 10, 1:45 PM"
  char audio_url[256]; // For playback
} session_info_t;

typedef struct {
  char id[48];
  char title[MAX_TITLE_LEN];
  char date[32];
  char file_url[256]; // Image URL
  // Reference data fields
  char time_signature[16]; // e.g., "4/4"
  char key_signature[16];  // e.g., "C", "Bb", "F#"
  int note_count;
  char clef[16];       // e.g., "treble", "bass"
  BOOL_T has_analysis; // True if audiveris_raw_output exists
} upload_info_t;

typedef struct {
  TUYA_RINGBUFF_T pcm_ringbuf;
  BOOL_T is_recording;
  uint32_t start_time;
  uint16_t peak_amplitude;
  screen_state_t current_screen;
  uint32_t recording_count;

  // Session data from backend
  session_info_t sessions[MAX_SESSIONS];
  int session_count;
  BOOL_T sessions_loaded;
  BOOL_T is_analyzing;
  char analyze_feedback[512];

  // Debug: last HTTP status
  int last_http_status;
  int last_http_code;

  // Main screen
  lv_obj_t *main_screen;
  lv_obj_t *btn_record;
  lv_obj_t *btn_analyze;
  lv_obj_t *btn_sessions;
  lv_obj_t *btn_uploads;
  lv_obj_t *greeting_label;
  lv_obj_t *wifi_label;

  // Recording screen
  lv_obj_t *recording_screen;
  lv_obj_t *timer_label;
  lv_obj_t *bars[20];
  lv_obj_t *stop_btn;
  lv_obj_t *status_label;

  // Analyze screen
  lv_obj_t *analyze_screen;
  lv_obj_t *analyze_status;
  lv_obj_t *analyze_btn;
  lv_obj_t *analyze_result_cont;
  lv_obj_t *analyze_result_label;
  lv_obj_t *analyze_session_btn;
  lv_obj_t *analyze_sheet_btn;
  lv_obj_t *analyze_session_label;
  lv_obj_t *analyze_sheet_label;

  // Analyze selection state
  int selected_session_idx;
  int selected_upload_idx;

  // Uploads/sheet music data
  upload_info_t uploads[MAX_UPLOADS];
  int upload_count;
  BOOL_T uploads_loaded;

  // Session selection screen (for analyze)
  lv_obj_t *select_session_screen;
  lv_obj_t *select_session_list_cont;
  lv_obj_t *select_session_status_label;
  lv_obj_t *select_session_labels[MAX_SESSIONS];

  // Sheet music selection screen (for analyze)
  lv_obj_t *select_sheet_screen;
  lv_obj_t *select_sheet_list_cont;
  lv_obj_t *select_sheet_status_label;
  lv_obj_t *select_sheet_labels[MAX_UPLOADS];

  // Sessions screen
  lv_obj_t *sessions_screen;
  lv_obj_t *sessions_list_cont;
  lv_obj_t *sessions_status_label;
  lv_obj_t *session_labels[MAX_SESSIONS];

  // Uploads screen
  lv_obj_t *uploads_screen;
  lv_obj_t *uploads_list_cont;
  lv_obj_t *uploads_status_label;
  lv_obj_t *upload_labels[MAX_UPLOADS];

  // View upload screen (information display)
  lv_obj_t *view_upload_screen;
  lv_obj_t *view_upload_title_label;
  lv_obj_t *view_upload_info_cont; // Scrollable container for info
  lv_obj_t *view_upload_time_sig_chip;
  lv_obj_t *view_upload_key_sig_chip;
  lv_obj_t *view_upload_analyzed_chip;
  lv_obj_t *view_upload_music_data_cont;
  lv_obj_t *view_upload_music_data_label;
  lv_obj_t *view_upload_date_label;
  int current_upload_idx;

  // Playback screen
  lv_obj_t *playback_screen;
  lv_obj_t *playback_title_label;
  lv_obj_t *playback_time_label;
  lv_obj_t *playback_slider;
  lv_obj_t *playback_play_btn;
  lv_obj_t *playback_status_label;

  // Playback state
  BOOL_T is_playing;
  int current_session_idx;
  uint8_t *audio_buffer;
  uint32_t audio_size;
  uint32_t audio_position;
  uint32_t playback_volume;

  // Loading overlay (shown during download)
  lv_obj_t *loading_overlay;
  lv_obj_t *loading_bar;
  lv_obj_t *loading_label;
  lv_obj_t *loading_size_label;
  lv_obj_t *loading_cancel_btn;
  BOOL_T download_cancelled;

  lv_timer_t *ui_timer;
} recorder_mgr_t;

// Hardcoded credentials for development
#define USER_SSID "JJ Lake"
#define USER_PASSWORD "20220315"

static recorder_mgr_t g_recorder;
static BOOL_T g_net_connected = FALSE;

// Forward declarations
static void create_main_ui(void);
static void show_main_screen(void);
static void show_recording_screen(void);
static void show_analyze_screen(void);
static void show_sessions_screen(void);
static void show_uploads_screen(void);
static void show_view_upload_screen(int upload_idx);
static void show_playback_screen(int session_idx);
static void show_select_session_screen(void);
static void show_select_sheet_screen(void);
// static void app_fs_init(void);  // SD card disabled
static void app_audio_init(void);
static void control_recording(BOOL_T start);
static void ui_timer_cb(lv_timer_t *timer);
static void fetch_sessions(void);
static void fetch_uploads(void);
static void analyze_last_recording(void);
static void update_sessions_ui(void);
static void update_uploads_ui(void);
static void create_uploads_screen(void);
static void create_view_upload_screen(void);
static void update_view_upload_ui(int upload_idx);
static void update_select_session_ui(void);
static void update_select_sheet_ui(void);
static void create_select_session_screen(void);
static void create_select_sheet_screen(void);
static void update_sessions_ui(void);
static void update_analyze_ui(void);
static void download_and_play_audio(const char *recording_id);
static void stop_playback(void);
static void update_playback_ui(void);
static void show_loading_overlay(const char *title);
static void hide_loading_overlay(void);
static void update_loading_progress(int progress, uint32_t downloaded,
                                    uint32_t total);

// =================================================================
// NETWORK & IOT CALLBACKS
// =================================================================

/**
 * @brief Tuya IoT Event Handler
 */
static void app_event_handler(tuya_iot_client_t *client,
                              tuya_event_msg_t *event) {
  PR_DEBUG("Received Event: %s (%d)", EVENT_ID2STR(event->id), event->id);

  switch (event->id) {
  case TUYA_EVENT_DP_RECEIVE_OBJ: {
    dp_obj_recv_t *dp_obj = event->value.dpobj;
    PR_DEBUG("DP count: %d", dp_obj->dpscnt);
    for (int i = 0; i < dp_obj->dpscnt; i++) {
      dp_obj_t *dp = &dp_obj->dps[i];
      PR_DEBUG("DP ID: %d, Type: %d", dp->id, dp->type);

      if (dp->id == 101 && dp->type == PROP_BOOL) {
        BOOL_T start = dp->value.dp_bool;
        PR_NOTICE("Remote Record Command: %s", start ? "START" : "STOP");
        control_recording(start);

        // Report back to confirm
        char dps_buf[32];
        snprintf(dps_buf, sizeof(dps_buf), "{\"101\":%s}",
                 start ? "true" : "false");
        tuya_iot_dp_report_json(client, dps_buf);
      }
    }
    break;
  }
  case TUYA_EVENT_MQTT_CONNECTED:
    PR_NOTICE("Cloud Connected!");
    g_net_connected = TRUE;
    break;
  case TUYA_EVENT_MQTT_DISCONNECT:
    PR_NOTICE("Cloud Disconnected!");
    g_net_connected = FALSE;
    break;
  default:
    break;
  }
}

/**
 * @brief Application Initialization (Device Init)
 * Called by SDK after basic system init is done.
 */
static VOID device_init(VOID) {
  PR_NOTICE("[INIT] Device Init - Starting...");

  // Debug LED Init - Turn ON
  PR_DEBUG("[INIT] Setting up debug LED (GPIO 28)");
  TUYA_GPIO_BASE_CFG_T led_cfg = {.mode = TUYA_GPIO_PUSH_PULL,
                                  .direct = TUYA_GPIO_OUTPUT,
                                  .level = TUYA_GPIO_LEVEL_HIGH};
  tkl_gpio_init(TUYA_GPIO_NUM_28, &led_cfg);

  // 1. Hardware Init
  PR_NOTICE("[INIT] Step 1: Registering board hardware...");
  int ret = board_register_hardware();
  if (ret != OPRT_OK) {
    PR_ERR("[INIT] board_register_hardware FAILED: %d", ret);
  } else {
    PR_NOTICE("[INIT] board_register_hardware OK");
  }

  // Debug LED - Turn OFF after hardware init
  tkl_gpio_write(TUYA_GPIO_NUM_28, TUYA_GPIO_LEVEL_LOW);

  // 2. LVGL Init
  PR_NOTICE("[INIT] Step 2: Initializing LVGL...");
#ifdef DISPLAY_NAME
  lv_vendor_init(DISPLAY_NAME);
  PR_DEBUG("[INIT] lv_vendor_init('%s') done", DISPLAY_NAME);
#else
  lv_vendor_init("display");
  PR_DEBUG("[INIT] lv_vendor_init('display') done");
#endif

#ifdef CONFIG_ENABLE_LVGL_LODEPNG
  // Initialize PNG decoder if enabled
  PR_NOTICE("[INIT] Initializing PNG decoder (lodepng)...");
  extern void lv_png_init(void);
  lv_png_init();
  PR_DEBUG("[INIT] PNG decoder initialized");
#endif

  // 3. UI & Logic
  PR_NOTICE("[INIT] Step 3: Creating UI...");
  create_main_ui();
  PR_DEBUG("[INIT] UI created");

  // SD card disabled - uploading directly to cloud
  // PR_DEBUG("[INIT] Initializing filesystem...");
  // app_fs_init();

  PR_DEBUG("[INIT] Initializing audio...");
  app_audio_init();

  // 4. PCM Buffer
  PR_DEBUG("[INIT] Step 4: Creating PCM ring buffer (%d bytes)...",
           PCM_BUF_SIZE);
  tuya_ring_buff_create(PCM_BUF_SIZE, OVERFLOW_PSRAM_STOP_TYPE,
                        &g_recorder.pcm_ringbuf);

  // 5. UI Timer
  PR_DEBUG("[INIT] Step 5: Creating UI timer...");
  g_recorder.ui_timer = lv_timer_create(NULL, 50, NULL);
  if (g_recorder.ui_timer) {
    lv_timer_set_cb(g_recorder.ui_timer, ui_timer_cb);
    PR_DEBUG("[INIT] UI timer created (50ms interval)");
  } else {
    PR_ERR("[INIT] Failed to create UI timer!");
  }

  // 6. Start LVGL Task
  PR_NOTICE("[INIT] Step 6: Starting LVGL task...");
  lv_vendor_start(5, 1024 * 8);
  PR_NOTICE("[INIT] LVGL Task Started (priority=5, stack=8KB)");

  // 7. Force backlight ON (GPIO 9, HIGH = ON)
  PR_DEBUG("[INIT] Step 7: Enabling backlight (GPIO 9)...");
  TUYA_GPIO_BASE_CFG_T bl_cfg = {.mode = TUYA_GPIO_PUSH_PULL,
                                 .direct = TUYA_GPIO_OUTPUT,
                                 .level = TUYA_GPIO_LEVEL_HIGH};
  tkl_gpio_init(TUYA_GPIO_NUM_9, &bl_cfg);
  tkl_gpio_write(TUYA_GPIO_NUM_9, TUYA_GPIO_LEVEL_HIGH);
  PR_NOTICE("[INIT] Backlight ON");

  // Give display time to stabilize
  PR_DEBUG("[INIT] Waiting 200ms for display to stabilize...");
  tal_system_sleep(200);

  // Debug LED - Turn ON after app init
  tkl_gpio_write(TUYA_GPIO_NUM_28, TUYA_GPIO_LEVEL_HIGH);
  PR_NOTICE("[INIT] Device Init COMPLETE");
}

// =================================================================
// APPLICATION LOGIC
// =================================================================

static void manual_upload_recording(const char *filepath, size_t file_size,
                                    uint8_t *file_buf) {
  (void)filepath; // Not used - we upload from memory buffer

  PR_NOTICE("Starting upload: %d bytes to %s:%d%s", file_size,
            CLOUD_BACKEND_HOST, CLOUD_BACKEND_PORT, CLOUD_BACKEND_PATH);

  // Get device ID - ALWAYS use hardcoded UUID (tuya_iot_devid_get returns
  // garbage)
  const char *dev_id = DEVICE_UUID;
  PR_NOTICE("Upload using device ID: %s", dev_id);

  if (g_recorder.status_label)
    lv_label_set_text(g_recorder.status_label, "Uploading...");

  // Use the proper Tuya HTTP client interface
  http_client_response_t http_response = {0};

  // Set up headers - Content-Type and X-User-ID
  http_client_header_t headers[] = {
      {.key = "Content-Type", .value = "audio/wav"},
      {.key = "X-User-ID", .value = dev_id}};

  // Calculate timeout: 30 seconds base + 1 second per 10KB
  uint32_t timeout_ms = 30000 + (file_size / 10240) * 1000;
  if (timeout_ms > 120000)
    timeout_ms = 120000; // Cap at 2 minutes

  PR_NOTICE("Using HTTP client with timeout %d ms", timeout_ms);

  // Make the HTTP POST request using the SDK's HTTP client
  http_client_status_t http_status = http_client_request(
      &(const http_client_request_t){
          .host = CLOUD_BACKEND_HOST,
          .port = CLOUD_BACKEND_PORT,
          .path = CLOUD_BACKEND_PATH,
          .cacert = NULL, // No TLS for local development
          .cacert_len = 0,
          .method = "POST",
          .headers = headers,
          .headers_count = sizeof(headers) / sizeof(http_client_header_t),
          .body = file_buf,
          .body_length = file_size,
          .timeout_ms = timeout_ms},
      &http_response);

  if (http_status != HTTP_CLIENT_SUCCESS) {
    PR_ERR("HTTP request failed with status: %d", http_status);
    if (g_recorder.status_label) {
      if (http_status == HTTP_CLIENT_SEND_FAULT) {
        lv_label_set_text(g_recorder.status_label, "Err: Send Failed");
      } else if (http_status == HTTP_CLIENT_MALLOC_FAULT) {
        lv_label_set_text(g_recorder.status_label, "Err: No Memory");
      } else {
        lv_label_set_text(g_recorder.status_label, "Err: HTTP Failed");
      }
    }
    http_client_free(&http_response);
    return;
  }

  // Check HTTP status code
  PR_NOTICE("HTTP Response: status=%d, body_len=%d", http_response.status_code,
            http_response.body_length);

  if (http_response.body && http_response.body_length > 0) {
    PR_NOTICE("Response body: %.200s", (char *)http_response.body);
  }

  if (http_response.status_code == 200) {
    if (g_recorder.status_label)
      lv_label_set_text(g_recorder.status_label, "Upload Success!");
    PR_NOTICE("Upload successful!");
  } else {
    PR_ERR("Upload rejected with HTTP %d", http_response.status_code);
    if (g_recorder.status_label)
      lv_label_set_text_fmt(g_recorder.status_label, "Err: HTTP %d",
                            http_response.status_code);
  }

  http_client_free(&http_response);
}

// upload_recording() removed - we now upload directly from memory in
// save_recording()

// -----------------------------------------------------------
// Helpers
// -----------------------------------------------------------
// get_next_filename() removed - no longer needed since we skip SD card

static int _audio_frame_put(TKL_AUDIO_FRAME_INFO_T *pframe) {
  if (g_recorder.is_recording && g_recorder.pcm_ringbuf) {
    tuya_ring_buff_write(g_recorder.pcm_ringbuf, pframe->pbuf,
                         pframe->used_size);
    // Visualizer Logic
    int16_t *samples = (int16_t *)pframe->pbuf;
    uint32_t sample_count = pframe->used_size / 2;
    uint16_t max_amp = 0;
    for (uint32_t i = 0; i < sample_count; i++) {
      uint16_t abs_val = (samples[i] < 0) ? -samples[i] : samples[i];
      if (abs_val > max_amp)
        max_amp = abs_val;
    }
    uint32_t boosted_amp = (uint32_t)max_amp * 4;
    if (boosted_amp > 32767)
      boosted_amp = 32767;
    g_recorder.peak_amplitude =
        (g_recorder.peak_amplitude * 2 + boosted_amp) / 3;
  }
  return pframe->used_size;
}

// SD card disabled - uploading directly to cloud
// static void app_fs_init(void) { tkl_fs_mount(SD_MOUNT_POINT, DEV_SDCARD); }

static void app_audio_init(void) {
  TKL_AUDIO_CONFIG_T config = {0};
  config.enable = false;
  config.card = TKL_AUDIO_TYPE_BOARD;
  config.ai_chn = TKL_AI_0;
  config.sample = MIC_SAMPLE_RATE;
  config.datebits = MIC_SAMPLE_BITS;
  config.channel = MIC_CHANNEL;
  config.codectype = TKL_CODEC_AUDIO_PCM;
  config.put_cb = _audio_frame_put;

  tkl_ai_init(&config, 1);
  tkl_ai_set_vol(TKL_AUDIO_TYPE_BOARD, 0, 90);
  tkl_ai_start(TKL_AUDIO_TYPE_BOARD, 0);
}

static void save_recording(void) {
  uint32_t pcm_len = tuya_ring_buff_used_size_get(g_recorder.pcm_ringbuf);
  if (pcm_len == 0) {
    if (g_recorder.status_label)
      lv_label_set_text(g_recorder.status_label, "No audio data");
    return;
  }

  // Check WiFi connection first - wait a bit for connection to stabilize
  WF_STATION_STAT_E stat = WSS_IDLE;
  tal_wifi_station_get_status(&stat);
  if (stat != WSS_GOT_IP) {
    if (g_recorder.status_label)
      lv_label_set_text(g_recorder.status_label, "Err: No WiFi");
    PR_ERR("Cannot upload: WiFi not connected (status: %d)", stat);
    return;
  }

  // Give WiFi a moment to fully stabilize after getting IP
  if (!g_net_connected) {
    PR_NOTICE("WiFi connected, waiting for network to stabilize...");
    tal_system_sleep(1000); // Wait 1 second
    g_net_connected = TRUE;
  }

  if (g_recorder.status_label)
    lv_label_set_text(g_recorder.status_label, "Uploading...");

  // Build WAV file in memory (skip SD card entirely)
  uint32_t wav_size = WAV_HEAD_LEN + pcm_len;

  // Allocate buffer for complete WAV file
  uint8_t *wav_buf = tal_malloc(wav_size);
#if defined(ENABLE_EXT_RAM) && (ENABLE_EXT_RAM == 1)
  void *psram_buf = tal_psram_malloc(wav_size);
  if (psram_buf)
    wav_buf = psram_buf;
#endif

  if (!wav_buf) {
    if (g_recorder.status_label)
      lv_label_set_text(g_recorder.status_label, "Err: No Memory");
    PR_ERR("Failed to allocate WAV buffer (%d bytes)", wav_size);
    return;
  }

  // 1. Write WAV header
  uint8_t wav_head[WAV_HEAD_LEN];
  app_get_wav_head(pcm_len, 1, MIC_SAMPLE_RATE, MIC_SAMPLE_BITS, MIC_CHANNEL,
                   wav_head);
  memcpy(wav_buf, wav_head, WAV_HEAD_LEN);

  // 2. Copy PCM data from ring buffer
  tuya_ring_buff_read(g_recorder.pcm_ringbuf, wav_buf + WAV_HEAD_LEN, pcm_len);

  PR_NOTICE("WAV file built in memory: %d bytes (PCM: %d)", wav_size, pcm_len);

  // 3. Upload directly to backend (which saves to Supabase)
  manual_upload_recording(NULL, wav_size, wav_buf);

  // 4. Cleanup
#if defined(ENABLE_EXT_RAM) && (ENABLE_EXT_RAM == 1)
  tal_psram_free(wav_buf);
#else
  tal_free(wav_buf);
#endif

  // 5. Increment recording count and return to main after delay
  g_recorder.recording_count++;

  // Wait longer to ensure backend has processed the upload
  // This helps prevent "Download send/recv error" when immediately going to
  // sessions
  tal_system_sleep(2000); // Show upload status for 2 sec, then return to main

  // Invalidate sessions cache so next fetch gets fresh data with new recording
  g_recorder.sessions_loaded = FALSE;

  show_main_screen();
}

// UI Timer Callback
static void ui_timer_cb(lv_timer_t *timer) {
  (void)timer;

  // Poll WiFi Status periodically (every ~1 sec = 20 ticks)
  static uint32_t poll_cnt = 0;
  static uint32_t retry_delay = 0;
  static BOOL_T needs_retry = FALSE;

  if (++poll_cnt % 20 == 0) {
    if (g_recorder.wifi_label && g_recorder.current_screen == SCREEN_MAIN) {
      WF_STATION_STAT_E stat = WSS_IDLE;
      tal_wifi_station_get_status(&stat);

      if (stat == WSS_GOT_IP) {
        lv_label_set_text(g_recorder.wifi_label, LV_SYMBOL_WIFI " Connected");
        lv_obj_set_style_text_color(g_recorder.wifi_label,
                                    lv_color_hex(0x00FF00),
                                    LV_PART_MAIN); // Green

        // Auto-fetch sessions when WiFi first connects
        if (!g_net_connected && !g_recorder.sessions_loaded) {
          PR_NOTICE("[WIFI] Just connected! Fetching sessions...");
          g_net_connected = TRUE;
          fetch_sessions();
        }
        g_net_connected = TRUE;
        needs_retry = FALSE;
        retry_delay = 0;
      } else if (stat == WSS_CONNECTING) {
        lv_label_set_text(g_recorder.wifi_label,
                          LV_SYMBOL_WIFI " Connecting...");
        lv_obj_set_style_text_color(g_recorder.wifi_label,
                                    lv_color_hex(0xFFFF00),
                                    LV_PART_MAIN); // Yellow
        g_net_connected = FALSE;
      } else if (stat == WSS_PASSWD_WRONG) {
        lv_label_set_text(g_recorder.wifi_label,
                          LV_SYMBOL_WARNING " Wrong Password");
        lv_obj_set_style_text_color(g_recorder.wifi_label,
                                    lv_color_hex(0xFF0000), LV_PART_MAIN);
        g_net_connected = FALSE;
      } else if (stat == WSS_NO_AP_FOUND) {
        lv_label_set_text(g_recorder.wifi_label,
                          LV_SYMBOL_WARNING " No WiFi Found");
        lv_obj_set_style_text_color(g_recorder.wifi_label,
                                    lv_color_hex(0xFF0000), LV_PART_MAIN);
        g_net_connected = FALSE;
        needs_retry = TRUE;
      } else if (stat == WSS_CONN_FAIL || stat == WSS_IDLE) {
        // Auto-retry on connection failure or idle state
        if (needs_retry || stat == WSS_CONN_FAIL) {
          if (retry_delay == 0) {
            // Immediately try to reconnect
            PR_NOTICE("[WIFI] Connection failed/idle, retrying...");
            lv_label_set_text(g_recorder.wifi_label,
                              LV_SYMBOL_WIFI " Reconnecting...");
            lv_obj_set_style_text_color(g_recorder.wifi_label,
                                        lv_color_hex(0xFFFF00), LV_PART_MAIN);

            // Trigger reconnection
            netconn_wifi_info_t wifi_info = {0};
            strcpy(wifi_info.ssid, USER_SSID);
            strcpy(wifi_info.pswd, USER_PASSWORD);
            netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_SSID_PSWD, &wifi_info);

            retry_delay = 5; // Wait 5 seconds before next retry
            needs_retry = FALSE;
          } else {
            retry_delay--;
            char buf[32];
            snprintf(buf, sizeof(buf), LV_SYMBOL_WIFI " Retry in %ds...",
                     retry_delay);
            lv_label_set_text(g_recorder.wifi_label, buf);
            lv_obj_set_style_text_color(g_recorder.wifi_label,
                                        lv_color_hex(0xFFAA00), LV_PART_MAIN);
          }
        } else {
          lv_label_set_text(g_recorder.wifi_label,
                            LV_SYMBOL_WARNING " Connection Failed");
          lv_obj_set_style_text_color(g_recorder.wifi_label,
                                      lv_color_hex(0xFF0000), LV_PART_MAIN);
          needs_retry = TRUE;
        }
        g_net_connected = FALSE;
      } else {
        lv_label_set_text(g_recorder.wifi_label,
                          LV_SYMBOL_WIFI " Initializing...");
        lv_obj_set_style_text_color(g_recorder.wifi_label,
                                    lv_color_hex(0xFFFF00), LV_PART_MAIN);
        g_net_connected = FALSE;
      }
    }
  }

  // Handle audio playback
  if (g_recorder.current_screen == SCREEN_PLAYBACK && g_recorder.is_playing) {
    if (g_recorder.audio_buffer &&
        g_recorder.audio_position < g_recorder.audio_size) {
      // Play a chunk of audio (4096 bytes at a time)
      uint32_t chunk_size = 4096;
      uint32_t remaining = g_recorder.audio_size - g_recorder.audio_position;
      if (chunk_size > remaining)
        chunk_size = remaining;

      TKL_AUDIO_FRAME_INFO_T frame;
      frame.pbuf =
          (char *)(g_recorder.audio_buffer + g_recorder.audio_position);
      frame.used_size = chunk_size;
      tkl_ao_put_frame(0, 0, NULL, &frame);

      g_recorder.audio_position += chunk_size;

      // Update UI every 10 chunks
      static int playback_ui_cnt = 0;
      if (++playback_ui_cnt % 10 == 0) {
        update_playback_ui();
      }

      // Check if finished
      if (g_recorder.audio_position >= g_recorder.audio_size) {
        g_recorder.is_playing = FALSE;
        g_recorder.audio_position = 0;
        lv_label_set_text(lv_obj_get_child(g_recorder.playback_play_btn, 0),
                          LV_SYMBOL_PLAY);
        lv_label_set_text(g_recorder.playback_status_label, "Finished");
        update_playback_ui();
      }
    }
  }

  // Only update recording UI when on recording screen and recording
  if (g_recorder.current_screen != SCREEN_RECORDING || !g_recorder.is_recording)
    return;

  uint32_t elapsed = tal_system_get_tick_count() - g_recorder.start_time;
  uint32_t ms = elapsed % 1000;
  uint32_t sec = elapsed / 1000;
  uint32_t min = sec / 60;
  sec %= 60;

  if (g_recorder.timer_label)
    lv_label_set_text_fmt(g_recorder.timer_label, "%02d:%02d.%03d", min, sec,
                          ms);

  uint32_t amp = (g_recorder.peak_amplitude * 45) / 32768 + 2;
  for (int i = 0; i < 19; i++) {
    int32_t val = lv_bar_get_value(g_recorder.bars[i + 1]);
    lv_bar_set_value(g_recorder.bars[i], val, LV_ANIM_OFF);
    lv_bar_set_start_value(g_recorder.bars[i], -val, LV_ANIM_OFF);
  }
  lv_bar_set_value(g_recorder.bars[19], amp, LV_ANIM_OFF);
  lv_bar_set_start_value(g_recorder.bars[19], -amp, LV_ANIM_OFF);
}

static void control_recording(BOOL_T start) {
  if (start && !g_recorder.is_recording) {
    // START
    tuya_ring_buff_reset(g_recorder.pcm_ringbuf);
    g_recorder.start_time = tal_system_get_tick_count();
    g_recorder.is_recording = TRUE;
    g_recorder.peak_amplitude = 0;

    if (g_recorder.status_label)
      lv_label_set_text(g_recorder.status_label, "Recording...");

    // Report to cloud
    char dps_buf[32];
    snprintf(dps_buf, sizeof(dps_buf), "{\"101\":true}");
    tuya_iot_dp_report_json(tuya_iot_client_get(), dps_buf);

  } else if (!start && g_recorder.is_recording) {
    // STOP
    g_recorder.is_recording = FALSE;

    if (g_recorder.status_label)
      lv_label_set_text(g_recorder.status_label, "Saving...");

    // Report to cloud
    char dps_buf[32];
    snprintf(dps_buf, sizeof(dps_buf), "{\"101\":false}");
    tuya_iot_dp_report_json(tuya_iot_client_get(), dps_buf);

    save_recording();
    // Note: save_recording() already invalidates sessions cache
  }
}

// =================================================================
// DATA FETCHING FUNCTIONS
// =================================================================

// Format ISO date "2026-01-10T13:45:00.000Z" to "Jan 10, 1:45 PM"
static void format_date_time(const char *iso_date, char *out, size_t out_size) {
  if (!iso_date || strlen(iso_date) < 16) {
    strncpy(out, "Unknown", out_size);
    return;
  }

  // Parse ISO format: YYYY-MM-DDTHH:MM:SS
  int year, month, day, hour, minute;
  if (sscanf(iso_date, "%d-%d-%dT%d:%d", &year, &month, &day, &hour, &minute) !=
      5) {
    strncpy(out, iso_date, out_size > 10 ? 10 : out_size);
    return;
  }

  // Month names
  const char *months[] = {"",    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  const char *month_name = (month >= 1 && month <= 12) ? months[month] : "???";

  // Convert 24h to 12h
  const char *ampm = (hour >= 12) ? "PM" : "AM";
  int hour12 = hour % 12;
  if (hour12 == 0)
    hour12 = 12;

  snprintf(out, out_size, "%s %d, %d:%02d %s", month_name, day, hour12, minute,
           ampm);
}

static void fetch_sessions(void) {
  // Show debug on screen
  if (g_recorder.sessions_status_label) {
    char buf[64];
    snprintf(buf, sizeof(buf), "WiFi: %s", g_net_connected ? "YES" : "NO");
    lv_label_set_text(g_recorder.sessions_status_label, buf);
  }

  if (!g_net_connected) {
    if (g_recorder.sessions_status_label) {
      lv_label_set_text(g_recorder.sessions_status_label,
                        "ERROR: No WiFi connection");
    }
    g_recorder.sessions_loaded = TRUE;
    g_recorder.session_count = 0;
    return;
  }

  if (g_recorder.sessions_status_label) {
    lv_label_set_text(g_recorder.sessions_status_label, "Calling HTTP...");
  }

  // Get device ID - ALWAYS use hardcoded UUID (tuya_iot_devid_get returns
  // garbage)
  const char *dev_id = DEVICE_UUID;

  http_client_response_t response = {0};
  http_client_header_t headers[] = {
      {.key = "Content-Type", .value = "application/json"},
      {.key = "X-User-ID", .value = dev_id}};

  http_client_status_t status = http_client_request(
      &(const http_client_request_t){.host = CLOUD_BACKEND_HOST,
                                     .port = CLOUD_BACKEND_PORT,
                                     .path = "/api/device/recordings",
                                     .cacert = NULL,
                                     .cacert_len = 0,
                                     .method = "GET",
                                     .headers = headers,
                                     .headers_count = 2,
                                     .body = NULL,
                                     .body_length = 0,
                                     .timeout_ms = 10000},
      &response);

  // Store HTTP result for debugging
  g_recorder.last_http_status = status;
  g_recorder.last_http_code = response.status_code;

  if (status != HTTP_CLIENT_SUCCESS) {
    g_recorder.session_count = 0;
    g_recorder.sessions_loaded = TRUE;
    http_client_free(&response);
    return;
  }

  if (response.status_code != 200) {
    g_recorder.session_count = 0;
    g_recorder.sessions_loaded = TRUE;
    http_client_free(&response);
    return;
  }

  // Parse JSON response - with debug tracking
  g_recorder.session_count = 0;
  int parse_step = 0; // Track where we are for debug

  if (response.body && response.body_length > 0) {
    parse_step = 1; // Got body
    cJSON *root = cJSON_Parse((char *)response.body);
    if (root) {
      parse_step = 2; // Parsed JSON
      cJSON *recordings = cJSON_GetObjectItem(root, "recordings");
      if (recordings && cJSON_IsArray(recordings)) {
        parse_step = 3; // Found recordings array
        int count = cJSON_GetArraySize(recordings);
        parse_step = 10 + count; // 10 + count = found N items
        if (count > MAX_SESSIONS)
          count = MAX_SESSIONS;

        for (int i = 0; i < count; i++) {
          cJSON *rec = cJSON_GetArrayItem(recordings, i);
          if (rec) {
            cJSON *id = cJSON_GetObjectItem(rec, "id");
            cJSON *title = cJSON_GetObjectItem(rec, "title");
            cJSON *created = cJSON_GetObjectItem(rec, "created_at");
            cJSON *audio_url = cJSON_GetObjectItem(rec, "audio_url");

            if (id && title) {
              strncpy(g_recorder.sessions[i].id,
                      cJSON_GetStringValue(id) ? cJSON_GetStringValue(id) : "",
                      sizeof(g_recorder.sessions[i].id) - 1);
              strncpy(g_recorder.sessions[i].title,
                      cJSON_GetStringValue(title) ? cJSON_GetStringValue(title)
                                                  : "Untitled",
                      sizeof(g_recorder.sessions[i].title) - 1);
              // Format date nicely: "Jan 10, 1:45 PM"
              if (created && cJSON_GetStringValue(created)) {
                format_date_time(cJSON_GetStringValue(created),
                                 g_recorder.sessions[i].date,
                                 sizeof(g_recorder.sessions[i].date));
              } else {
                strncpy(g_recorder.sessions[i].date, "No date",
                        sizeof(g_recorder.sessions[i].date));
              }
              // Store audio URL for playback
              if (audio_url && cJSON_GetStringValue(audio_url)) {
                strncpy(g_recorder.sessions[i].audio_url,
                        cJSON_GetStringValue(audio_url),
                        sizeof(g_recorder.sessions[i].audio_url) - 1);
              } else {
                g_recorder.sessions[i].audio_url[0] = '\0';
              }
              g_recorder.session_count++;
            }
          }
        }
      }
      cJSON_Delete(root);
    }
  }

  // Store parse step for debug display
  g_recorder.last_http_code = parse_step; // Reuse this field for debug

  g_recorder.sessions_loaded = TRUE;
  http_client_free(&response);
}

static void fetch_uploads(void) {
  if (!g_net_connected) {
    g_recorder.uploads_loaded = TRUE;
    g_recorder.upload_count = 0;
    return;
  }

  const char *dev_id = DEVICE_UUID;

  http_client_response_t response = {0};
  http_client_header_t headers[] = {
      {.key = "Content-Type", .value = "application/json"},
      {.key = "X-User-ID", .value = dev_id}};

  http_client_status_t status = http_client_request(
      &(const http_client_request_t){.host = CLOUD_BACKEND_HOST,
                                     .port = CLOUD_BACKEND_PORT,
                                     .path = "/api/device/sheet-music",
                                     .cacert = NULL,
                                     .cacert_len = 0,
                                     .method = "GET",
                                     .headers = headers,
                                     .headers_count = 2,
                                     .body = NULL,
                                     .body_length = 0,
                                     .timeout_ms = 10000},
      &response);

  if (status != HTTP_CLIENT_SUCCESS || response.status_code != 200) {
    g_recorder.upload_count = 0;
    g_recorder.uploads_loaded = TRUE;
    http_client_free(&response);
    return;
  }

  // Parse JSON response
  g_recorder.upload_count = 0;

  if (response.body && response.body_length > 0) {
    cJSON *root = cJSON_Parse((char *)response.body);
    if (root) {
      cJSON *uploads = cJSON_GetObjectItem(root, "sheet_music");
      if (uploads && cJSON_IsArray(uploads)) {
        int count = cJSON_GetArraySize(uploads);
        if (count > MAX_UPLOADS)
          count = MAX_UPLOADS;

        for (int i = 0; i < count; i++) {
          cJSON *upload = cJSON_GetArrayItem(uploads, i);
          if (upload) {
            cJSON *id = cJSON_GetObjectItem(upload, "id");
            cJSON *title = cJSON_GetObjectItem(upload, "title");
            cJSON *created = cJSON_GetObjectItem(upload, "created_at");
            cJSON *file_url = cJSON_GetObjectItem(upload, "file_url");
            cJSON *ref_data = cJSON_GetObjectItem(upload, "reference_data");
            cJSON *audiveris =
                cJSON_GetObjectItem(upload, "audiveris_raw_output");

            if (id && title) {
              strncpy(g_recorder.uploads[i].id,
                      cJSON_GetStringValue(id) ? cJSON_GetStringValue(id) : "",
                      sizeof(g_recorder.uploads[i].id) - 1);
              strncpy(g_recorder.uploads[i].title,
                      cJSON_GetStringValue(title) ? cJSON_GetStringValue(title)
                                                  : "Untitled",
                      sizeof(g_recorder.uploads[i].title) - 1);
              if (created && cJSON_GetStringValue(created)) {
                format_date_time(cJSON_GetStringValue(created),
                                 g_recorder.uploads[i].date,
                                 sizeof(g_recorder.uploads[i].date));
              } else {
                strncpy(g_recorder.uploads[i].date, "No date",
                        sizeof(g_recorder.uploads[i].date));
              }
              // Store file URL
              if (file_url && cJSON_GetStringValue(file_url)) {
                strncpy(g_recorder.uploads[i].file_url,
                        cJSON_GetStringValue(file_url),
                        sizeof(g_recorder.uploads[i].file_url) - 1);
              } else {
                g_recorder.uploads[i].file_url[0] = '\0';
              }

              // Parse reference_data
              g_recorder.uploads[i].note_count = 0;
              strncpy(g_recorder.uploads[i].time_signature, "4/4",
                      sizeof(g_recorder.uploads[i].time_signature) - 1);
              strncpy(g_recorder.uploads[i].key_signature, "C",
                      sizeof(g_recorder.uploads[i].key_signature) - 1);
              g_recorder.uploads[i].clef[0] = '\0';
              g_recorder.uploads[i].has_analysis = FALSE;

              if (ref_data && cJSON_IsObject(ref_data)) {
                // Parse time signature
                cJSON *time_sig =
                    cJSON_GetObjectItem(ref_data, "timeSignature");
                if (!time_sig)
                  time_sig = cJSON_GetObjectItem(ref_data, "time_signature");
                if (time_sig && cJSON_GetStringValue(time_sig)) {
                  strncpy(g_recorder.uploads[i].time_signature,
                          cJSON_GetStringValue(time_sig),
                          sizeof(g_recorder.uploads[i].time_signature) - 1);
                }

                // Parse key signature
                cJSON *key_sig = cJSON_GetObjectItem(ref_data, "key_signature");
                if (!key_sig)
                  key_sig = cJSON_GetObjectItem(ref_data, "key");
                if (key_sig && cJSON_GetStringValue(key_sig)) {
                  const char *key = cJSON_GetStringValue(key_sig);
                  char friendly_key[16] = {0};
                  // Replace - with b (flat), + with # (sharp)
                  int j = 0;
                  for (int k = 0; key[k] && j < 15; k++) {
                    if (key[k] == '-')
                      friendly_key[j++] = 'b';
                    else if (key[k] == '+')
                      friendly_key[j++] = '#';
                    else
                      friendly_key[j++] = key[k];
                  }
                  friendly_key[j] = '\0';
                  strncpy(g_recorder.uploads[i].key_signature, friendly_key,
                          sizeof(g_recorder.uploads[i].key_signature) - 1);
                }

                // Parse note count
                cJSON *notes = cJSON_GetObjectItem(ref_data, "notes");
                if (notes && cJSON_IsArray(notes)) {
                  g_recorder.uploads[i].note_count = cJSON_GetArraySize(notes);
                }

                // Parse clef
                cJSON *clef = cJSON_GetObjectItem(ref_data, "clef");
                if (clef && cJSON_GetStringValue(clef)) {
                  strncpy(g_recorder.uploads[i].clef,
                          cJSON_GetStringValue(clef),
                          sizeof(g_recorder.uploads[i].clef) - 1);
                }
              }

              // Check if analyzed
              if (audiveris && cJSON_IsString(audiveris) &&
                  cJSON_GetStringValue(audiveris) &&
                  strlen(cJSON_GetStringValue(audiveris)) > 0) {
                g_recorder.uploads[i].has_analysis = TRUE;
              }

              g_recorder.upload_count++;
            }
          }
        }
      }
      cJSON_Delete(root);
    }
  }

  g_recorder.uploads_loaded = TRUE;
  http_client_free(&response);
}

static void analyze_last_recording(void) {
  if (!g_net_connected) {
    strncpy(g_recorder.analyze_feedback, "No network connection",
            sizeof(g_recorder.analyze_feedback));
    g_recorder.is_analyzing = FALSE;
    return;
  }

  if (g_recorder.session_count == 0) {
    strncpy(g_recorder.analyze_feedback,
            "No recordings to analyze.\nRecord something first!",
            sizeof(g_recorder.analyze_feedback));
    g_recorder.is_analyzing = FALSE;
    return;
  }

  PR_NOTICE("Analyzing last recording...");
  g_recorder.is_analyzing = TRUE;
  strncpy(g_recorder.analyze_feedback, "Analyzing...",
          sizeof(g_recorder.analyze_feedback));

  // Get device ID - ALWAYS use hardcoded UUID (tuya_iot_devid_get returns
  // garbage)
  const char *dev_id = DEVICE_UUID;

  // Build request body with last recording ID
  char body[256];
  snprintf(body, sizeof(body),
           "{\"recording_id\":\"%s\",\"use_latest_upload\":true}",
           g_recorder.sessions[0].id);

  http_client_response_t response = {0};
  http_client_header_t headers[] = {
      {.key = "Content-Type", .value = "application/json"},
      {.key = "X-User-ID", .value = dev_id}};

  http_client_status_t status = http_client_request(
      &(const http_client_request_t){
          .host = CLOUD_BACKEND_HOST,
          .port = CLOUD_BACKEND_PORT,
          .path = "/api/device/analyze",
          .cacert = NULL,
          .cacert_len = 0,
          .method = "POST",
          .headers = headers,
          .headers_count = 2,
          .body = (uint8_t *)body,
          .body_length = strlen(body),
          .timeout_ms = 60000 // 60 sec for AI analysis
      },
      &response);

  g_recorder.is_analyzing = FALSE;

  if (status != HTTP_CLIENT_SUCCESS) {
    snprintf(g_recorder.analyze_feedback, sizeof(g_recorder.analyze_feedback),
             "Request failed: %d", status);
    http_client_free(&response);
    return;
  }

  if (response.status_code != 200) {
    snprintf(g_recorder.analyze_feedback, sizeof(g_recorder.analyze_feedback),
             "Server error: %d", response.status_code);
    http_client_free(&response);
    return;
  }

  // Parse response
  if (response.body && response.body_length > 0) {
    cJSON *root = cJSON_Parse((char *)response.body);
    if (root) {
      cJSON *feedback = cJSON_GetObjectItem(root, "feedback");
      if (feedback && cJSON_GetStringValue(feedback)) {
        strncpy(g_recorder.analyze_feedback, cJSON_GetStringValue(feedback),
                sizeof(g_recorder.analyze_feedback) - 1);
      } else {
        strncpy(g_recorder.analyze_feedback, "Analysis complete!",
                sizeof(g_recorder.analyze_feedback));
      }
      cJSON_Delete(root);
    } else {
      strncpy(g_recorder.analyze_feedback, "Failed to parse response",
              sizeof(g_recorder.analyze_feedback));
    }
  }

  PR_NOTICE("Analysis complete");
  http_client_free(&response);
}

// =================================================================
// BUTTON CALLBACKS
// =================================================================

static void btn_record_cb(lv_event_t *e) {
  (void)e;
  show_recording_screen();
  control_recording(TRUE);
}

static void btn_analyze_cb(lv_event_t *e) {
  (void)e;
  PR_NOTICE(">>> Analyze button pressed! <<<");
  // Fetch sessions first if needed (to know what to analyze)
  if (!g_recorder.sessions_loaded) {
    PR_NOTICE("Sessions not loaded, fetching...");
    fetch_sessions();
  }
  show_analyze_screen();
}

static void btn_sessions_cb(lv_event_t *e) {
  (void)e;
  show_sessions_screen();

  // Show debug status ON SCREEN
  if (g_recorder.sessions_status_label) {
    if (!g_net_connected) {
      lv_label_set_text(g_recorder.sessions_status_label, "DEBUG: No WiFi!");
    } else {
      lv_label_set_text(g_recorder.sessions_status_label, "DEBUG: Fetching...");
    }
  }

  fetch_sessions();
  update_sessions_ui();
}

static void btn_uploads_cb(lv_event_t *e) {
  (void)e;
  show_uploads_screen();

  // Show debug status ON SCREEN
  if (g_recorder.uploads_status_label) {
    if (!g_net_connected) {
      lv_label_set_text(g_recorder.uploads_status_label, "DEBUG: No WiFi!");
    } else {
      lv_label_set_text(g_recorder.uploads_status_label, "DEBUG: Fetching...");
    }
  }

  fetch_uploads();
  update_uploads_ui();
}

static void btn_refresh_uploads_cb(lv_event_t *e) {
  (void)e;
  lv_label_set_text(g_recorder.uploads_status_label, "Loading...");
  g_recorder.uploads_loaded = FALSE;
  fetch_uploads();
  update_uploads_ui();
}

static void upload_item_clicked_cb(lv_event_t *e) {
  int index = (int)(intptr_t)lv_event_get_user_data(e);
  if (index < 0 || index >= g_recorder.upload_count)
    return;

  PR_NOTICE("Upload %d clicked: %s", index, g_recorder.uploads[index].title);
  PR_NOTICE("File URL: %s", g_recorder.uploads[index].file_url);

  show_view_upload_screen(index);
}

static void btn_do_analyze_cb(lv_event_t *e) {
  (void)e;
  if (g_recorder.is_analyzing)
    return;

  // Check if both selections are made
  if (g_recorder.selected_session_idx < 0 ||
      g_recorder.selected_upload_idx < 0) {
    strncpy(g_recorder.analyze_feedback,
            "Please select both a recording and sheet music first",
            sizeof(g_recorder.analyze_feedback));
    update_analyze_ui();
    return;
  }

  lv_label_set_text(g_recorder.analyze_status, "Analyzing with AI...");
  g_recorder.is_analyzing = TRUE;

  // TODO: Call analyze with selected session and sheet music IDs
  // For now, use the existing analyze function
  analyze_last_recording();
  update_analyze_ui();
}

static void btn_refresh_sessions_cb(lv_event_t *e) {
  (void)e;
  lv_label_set_text(g_recorder.sessions_status_label, "Loading...");
  g_recorder.sessions_loaded = FALSE;
  fetch_sessions();
  update_sessions_ui();
}

static void session_item_clicked_cb(lv_event_t *e) {
  int index = (int)(intptr_t)lv_event_get_user_data(e);
  if (index < 0 || index >= g_recorder.session_count)
    return;

  PR_NOTICE("Session %d clicked: %s", index, g_recorder.sessions[index].title);
  PR_NOTICE("Audio URL: %s", g_recorder.sessions[index].audio_url);

  // Show loading overlay immediately (before switching screens)
  show_loading_overlay("Loading Recording...");

  // Open playback screen
  show_playback_screen(index);
}

static void btn_back_cb(lv_event_t *e) {
  (void)e;
  if (g_recorder.is_recording) {
    control_recording(FALSE);
  }
  show_main_screen();
}

static void btn_stop_cb(lv_event_t *e) {
  (void)e;
  control_recording(FALSE);
  // After upload completes, go back to main
  // (status_label will show upload progress)
}

// =================================================================
// SCREEN CREATION HELPERS
// =================================================================

static lv_obj_t *create_menu_button(lv_obj_t *parent, const char *icon,
                                    const char *label, lv_color_t color,
                                    lv_event_cb_t callback) {
  lv_obj_t *btn = lv_button_create(parent);
  lv_obj_set_size(btn, LV_PCT(92), 58);
  lv_obj_set_style_bg_color(btn, color, LV_PART_MAIN);
  lv_obj_set_style_radius(btn, 16, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 15, LV_PART_MAIN);
  lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_shadow_opa(btn, LV_OPA_40, LV_PART_MAIN);
  lv_obj_set_style_shadow_offset_y(btn, 5, LV_PART_MAIN);
  lv_obj_set_style_shadow_spread(btn, 0, LV_PART_MAIN);
  // Pressed state - darker
  lv_obj_set_style_bg_opa(btn, LV_OPA_80, LV_STATE_PRESSED);
  lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, NULL);

  // Icon + Label in row
  lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_left(btn, 18, LV_PART_MAIN);
  lv_obj_set_style_pad_right(btn, 18, LV_PART_MAIN);
  lv_obj_set_style_pad_top(btn, 14, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(btn, 14, LV_PART_MAIN);
  lv_obj_set_style_pad_gap(btn, 14, LV_PART_MAIN);

  lv_obj_t *icon_lbl = lv_label_create(btn);
  lv_label_set_text(icon_lbl, icon);
  lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(icon_lbl, lv_color_hex(0xffffff), LV_PART_MAIN);

  lv_obj_t *text_lbl = lv_label_create(btn);
  lv_label_set_text(text_lbl, label);
  lv_obj_set_style_text_font(text_lbl, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(text_lbl, lv_color_hex(0xffffff), LV_PART_MAIN);

  return btn;
}

static lv_obj_t *create_back_button(lv_obj_t *parent) {
  lv_obj_t *btn = lv_button_create(parent);
  lv_obj_set_size(btn, 44, 44);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0x2d2d4a), LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0x3d3d5a), LV_STATE_PRESSED);
  lv_obj_set_style_radius(btn, 12, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 8, LV_PART_MAIN);
  lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_shadow_opa(btn, LV_OPA_30, LV_PART_MAIN);
  lv_obj_set_style_shadow_offset_y(btn, 3, LV_PART_MAIN);
  lv_obj_add_event_cb(btn, btn_back_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl = lv_label_create(btn);
  lv_label_set_text(lbl, LV_SYMBOL_LEFT);
  lv_obj_center(lbl);
  lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, LV_PART_MAIN);

  return btn;
}

// =================================================================
// MAIN SCREEN (3 Buttons)
// =================================================================

static void create_main_screen(void) {
  g_recorder.main_screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(g_recorder.main_screen, lv_color_hex(0x1a1a2e),
                            LV_PART_MAIN);

  lv_obj_t *cont = lv_obj_create(g_recorder.main_screen);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_opa(cont, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(cont, 20, LV_PART_MAIN);
  lv_obj_set_style_pad_top(cont, 30, LV_PART_MAIN);
  lv_obj_set_style_pad_gap(cont, 18, LV_PART_MAIN);

  // Greeting
  g_recorder.greeting_label = lv_label_create(cont);
  lv_label_set_text(g_recorder.greeting_label, "PracticePod");
  lv_obj_set_style_text_font(g_recorder.greeting_label, &lv_font_montserrat_24,
                             LV_PART_MAIN);
  lv_obj_set_style_text_color(g_recorder.greeting_label, lv_color_hex(0xffffff),
                              LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(g_recorder.greeting_label, 8, LV_PART_MAIN);

  // WiFi Status
  g_recorder.wifi_label = lv_label_create(cont);
  lv_label_set_text(g_recorder.wifi_label, "Connecting...");
  lv_obj_set_style_text_font(g_recorder.wifi_label, &lv_font_montserrat_14,
                             LV_PART_MAIN);
  lv_obj_set_style_text_color(g_recorder.wifi_label, lv_color_hex(0xffff00),
                              LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(g_recorder.wifi_label, 25, LV_PART_MAIN);

  // Button 1: Start Recording (Purple gradient feel)
  g_recorder.btn_record =
      create_menu_button(cont, SYMBOL_MIC, "Start Recording",
                         lv_color_hex(0x667eea), btn_record_cb);

  // Button 2: Analyze Performance (Green)
  g_recorder.btn_analyze =
      create_menu_button(cont, LV_SYMBOL_REFRESH, "Analyze",
                         lv_color_hex(0x11998e), btn_analyze_cb);

  // Button 3: Open Sessions (Pink)
  g_recorder.btn_sessions =
      create_menu_button(cont, LV_SYMBOL_LIST, "Sessions",
                         lv_color_hex(0xf5576c), btn_sessions_cb);

  // Button 4: View Uploads (Orange)
  g_recorder.btn_uploads = create_menu_button(
      cont, LV_SYMBOL_FILE, "Uploads", lv_color_hex(0xff8e53), btn_uploads_cb);
}

// =================================================================
// RECORDING SCREEN
// =================================================================

static void create_recording_screen(void) {
  g_recorder.recording_screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(g_recorder.recording_screen, lv_color_hex(0x000000),
                            LV_PART_MAIN);

  lv_obj_t *cont = lv_obj_create(g_recorder.recording_screen);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_opa(cont, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(cont, 15, LV_PART_MAIN);

  // Back button (top-left)
  lv_obj_t *back_btn = create_back_button(g_recorder.recording_screen);
  lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);

  // Timer
  g_recorder.timer_label = lv_label_create(cont);
  lv_label_set_text(g_recorder.timer_label, "00:00.000");
  lv_obj_set_style_text_font(g_recorder.timer_label, &lv_font_montserrat_24,
                             LV_PART_MAIN);
  lv_obj_set_style_text_color(g_recorder.timer_label, lv_color_hex(0xffffff),
                              LV_PART_MAIN);

  // Waveform visualizer
  lv_obj_t *bar_cont = lv_obj_create(cont);
  lv_obj_set_size(bar_cont, LV_PCT(95), 80);
  lv_obj_set_style_bg_opa(bar_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(bar_cont, 0, LV_PART_MAIN);
  lv_obj_set_flex_flow(bar_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bar_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(bar_cont, 3, LV_PART_MAIN);
  lv_obj_remove_flag(bar_cont, LV_OBJ_FLAG_SCROLLABLE);

  for (int i = 0; i < 20; i++) {
    g_recorder.bars[i] = lv_bar_create(bar_cont);
    lv_obj_set_size(g_recorder.bars[i], 5, 80);
    lv_bar_set_mode(g_recorder.bars[i], LV_BAR_MODE_RANGE);
    lv_bar_set_range(g_recorder.bars[i], -50, 50);
    lv_obj_set_style_bg_opa(g_recorder.bars[i], 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_recorder.bars[i], lv_color_hex(0xff3b30),
                              LV_PART_INDICATOR);
    lv_bar_set_value(g_recorder.bars[i], 2, LV_ANIM_OFF);
    lv_bar_set_start_value(g_recorder.bars[i], -2, LV_ANIM_OFF);
  }

  // Status label
  g_recorder.status_label = lv_label_create(cont);
  lv_label_set_text(g_recorder.status_label, "Recording...");
  lv_obj_set_style_text_color(g_recorder.status_label, lv_color_hex(0xff3b30),
                              LV_PART_MAIN);

  // Stop button
  g_recorder.stop_btn = lv_button_create(cont);
  lv_obj_set_size(g_recorder.stop_btn, 70, 70);
  lv_obj_set_style_radius(g_recorder.stop_btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);
  lv_obj_set_style_bg_color(g_recorder.stop_btn, lv_color_hex(0xff3b30),
                            LV_PART_MAIN);
  lv_obj_set_style_shadow_width(g_recorder.stop_btn, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(g_recorder.stop_btn, btn_stop_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *stop_lbl = lv_label_create(g_recorder.stop_btn);
  lv_label_set_text(stop_lbl, LV_SYMBOL_STOP);
  lv_obj_center(stop_lbl);
  lv_obj_set_style_text_font(stop_lbl, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(stop_lbl, lv_color_hex(0xffffff), LV_PART_MAIN);
}

// =================================================================
// ANALYZE SCREEN
// =================================================================

static void btn_select_session_cb(lv_event_t *e);
static void btn_select_sheet_cb(lv_event_t *e);

static void create_analyze_screen(void) {
  // Initialize selection state
  g_recorder.selected_session_idx = -1;
  g_recorder.selected_upload_idx = -1;

  g_recorder.analyze_screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(g_recorder.analyze_screen, lv_color_hex(0x1a1a2e),
                            LV_PART_MAIN);

  // Back button
  lv_obj_t *back_btn = create_back_button(g_recorder.analyze_screen);
  lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 12, 12);

  // Title
  lv_obj_t *title = lv_label_create(g_recorder.analyze_screen);
  lv_label_set_text(title, "Analyze");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN);

  // Main container
  lv_obj_t *main_cont = lv_obj_create(g_recorder.analyze_screen);
  lv_obj_set_size(main_cont, LV_PCT(92), LV_PCT(80));
  lv_obj_align(main_cont, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_style_bg_opa(main_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(main_cont, 0, LV_PART_MAIN);
  lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(main_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(main_cont, 12, LV_PART_MAIN);

  // Status label
  g_recorder.analyze_status = lv_label_create(main_cont);
  lv_label_set_text(g_recorder.analyze_status,
                    "Select a recording and sheet music to compare");
  lv_obj_set_style_text_align(g_recorder.analyze_status, LV_TEXT_ALIGN_CENTER,
                              0);
  lv_obj_set_style_text_color(g_recorder.analyze_status, lv_color_hex(0xaaaaaa),
                              LV_PART_MAIN);
  lv_obj_set_width(g_recorder.analyze_status, LV_PCT(100));
  lv_obj_set_style_text_font(g_recorder.analyze_status, &lv_font_montserrat_14,
                             LV_PART_MAIN);

  // ===== Button 1: Select Session =====
  g_recorder.analyze_session_btn = lv_button_create(main_cont);
  lv_obj_set_size(g_recorder.analyze_session_btn, LV_PCT(100), 65);
  lv_obj_set_style_bg_color(g_recorder.analyze_session_btn,
                            lv_color_hex(0x2d2d4a), LV_PART_MAIN);
  lv_obj_set_style_bg_color(g_recorder.analyze_session_btn,
                            lv_color_hex(0x3d3d5a), LV_STATE_PRESSED);
  lv_obj_set_style_radius(g_recorder.analyze_session_btn, 16, LV_PART_MAIN);
  lv_obj_set_style_border_width(g_recorder.analyze_session_btn, 2,
                                LV_PART_MAIN);
  lv_obj_set_style_border_color(g_recorder.analyze_session_btn,
                                lv_color_hex(0x667eea), LV_PART_MAIN);
  lv_obj_set_style_shadow_width(g_recorder.analyze_session_btn, 10,
                                LV_PART_MAIN);
  lv_obj_set_style_shadow_color(g_recorder.analyze_session_btn,
                                lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_shadow_opa(g_recorder.analyze_session_btn, LV_OPA_20,
                              LV_PART_MAIN);
  lv_obj_set_style_shadow_offset_y(g_recorder.analyze_session_btn, 3,
                                   LV_PART_MAIN);
  lv_obj_add_event_cb(g_recorder.analyze_session_btn, btn_select_session_cb,
                      LV_EVENT_CLICKED, NULL);

  // Session button content
  lv_obj_set_flex_flow(g_recorder.analyze_session_btn, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(g_recorder.analyze_session_btn, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_left(g_recorder.analyze_session_btn, 16, LV_PART_MAIN);
  lv_obj_set_style_pad_gap(g_recorder.analyze_session_btn, 12, LV_PART_MAIN);

  lv_obj_t *session_icon_cont = lv_obj_create(g_recorder.analyze_session_btn);
  lv_obj_set_size(session_icon_cont, 40, 40);
  lv_obj_set_style_bg_color(session_icon_cont, lv_color_hex(0x667eea),
                            LV_PART_MAIN);
  lv_obj_set_style_radius(session_icon_cont, 10, LV_PART_MAIN);
  lv_obj_set_style_border_width(session_icon_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(session_icon_cont, 0, LV_PART_MAIN);
  lv_obj_remove_flag(session_icon_cont, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *session_icon = lv_label_create(session_icon_cont);
  lv_label_set_text(session_icon, LV_SYMBOL_AUDIO);
  lv_obj_set_style_text_color(session_icon, lv_color_hex(0xffffff),
                              LV_PART_MAIN);
  lv_obj_center(session_icon);

  lv_obj_t *session_text_cont = lv_obj_create(g_recorder.analyze_session_btn);
  lv_obj_set_size(session_text_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(session_text_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(session_text_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(session_text_cont, 0, LV_PART_MAIN);
  lv_obj_set_flex_flow(session_text_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_remove_flag(session_text_cont, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *session_title = lv_label_create(session_text_cont);
  lv_label_set_text(session_title, "Select Session");
  lv_obj_set_style_text_color(session_title, lv_color_hex(0xffffff),
                              LV_PART_MAIN);
  lv_obj_set_style_text_font(session_title, &lv_font_montserrat_16,
                             LV_PART_MAIN);

  g_recorder.analyze_session_label = lv_label_create(session_text_cont);
  lv_label_set_text(g_recorder.analyze_session_label,
                    "Tap to choose a recording");
  lv_obj_set_style_text_color(g_recorder.analyze_session_label,
                              lv_color_hex(0x888888), LV_PART_MAIN);
  lv_obj_set_style_text_font(g_recorder.analyze_session_label,
                             &lv_font_montserrat_14, LV_PART_MAIN);

  // ===== Button 2: Select Sheet Music =====
  g_recorder.analyze_sheet_btn = lv_button_create(main_cont);
  lv_obj_set_size(g_recorder.analyze_sheet_btn, LV_PCT(100), 65);
  lv_obj_set_style_bg_color(g_recorder.analyze_sheet_btn,
                            lv_color_hex(0x2d2d4a), LV_PART_MAIN);
  lv_obj_set_style_bg_color(g_recorder.analyze_sheet_btn,
                            lv_color_hex(0x3d3d5a), LV_STATE_PRESSED);
  lv_obj_set_style_radius(g_recorder.analyze_sheet_btn, 16, LV_PART_MAIN);
  lv_obj_set_style_border_width(g_recorder.analyze_sheet_btn, 2, LV_PART_MAIN);
  lv_obj_set_style_border_color(g_recorder.analyze_sheet_btn,
                                lv_color_hex(0xf5576c), LV_PART_MAIN);
  lv_obj_set_style_shadow_width(g_recorder.analyze_sheet_btn, 10, LV_PART_MAIN);
  lv_obj_set_style_shadow_color(g_recorder.analyze_sheet_btn,
                                lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_shadow_opa(g_recorder.analyze_sheet_btn, LV_OPA_20,
                              LV_PART_MAIN);
  lv_obj_set_style_shadow_offset_y(g_recorder.analyze_sheet_btn, 3,
                                   LV_PART_MAIN);
  lv_obj_add_event_cb(g_recorder.analyze_sheet_btn, btn_select_sheet_cb,
                      LV_EVENT_CLICKED, NULL);

  // Sheet button content
  lv_obj_set_flex_flow(g_recorder.analyze_sheet_btn, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(g_recorder.analyze_sheet_btn, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_left(g_recorder.analyze_sheet_btn, 16, LV_PART_MAIN);
  lv_obj_set_style_pad_gap(g_recorder.analyze_sheet_btn, 12, LV_PART_MAIN);

  lv_obj_t *sheet_icon_cont = lv_obj_create(g_recorder.analyze_sheet_btn);
  lv_obj_set_size(sheet_icon_cont, 40, 40);
  lv_obj_set_style_bg_color(sheet_icon_cont, lv_color_hex(0xf5576c),
                            LV_PART_MAIN);
  lv_obj_set_style_radius(sheet_icon_cont, 10, LV_PART_MAIN);
  lv_obj_set_style_border_width(sheet_icon_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(sheet_icon_cont, 0, LV_PART_MAIN);
  lv_obj_remove_flag(sheet_icon_cont, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *sheet_icon = lv_label_create(sheet_icon_cont);
  lv_label_set_text(sheet_icon, LV_SYMBOL_FILE);
  lv_obj_set_style_text_color(sheet_icon, lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_center(sheet_icon);

  lv_obj_t *sheet_text_cont = lv_obj_create(g_recorder.analyze_sheet_btn);
  lv_obj_set_size(sheet_text_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(sheet_text_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(sheet_text_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(sheet_text_cont, 0, LV_PART_MAIN);
  lv_obj_set_flex_flow(sheet_text_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_remove_flag(sheet_text_cont, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *sheet_title = lv_label_create(sheet_text_cont);
  lv_label_set_text(sheet_title, "Select Sheet Music");
  lv_obj_set_style_text_color(sheet_title, lv_color_hex(0xffffff),
                              LV_PART_MAIN);
  lv_obj_set_style_text_font(sheet_title, &lv_font_montserrat_16, LV_PART_MAIN);

  g_recorder.analyze_sheet_label = lv_label_create(sheet_text_cont);
  lv_label_set_text(g_recorder.analyze_sheet_label,
                    "Tap to choose sheet music");
  lv_obj_set_style_text_color(g_recorder.analyze_sheet_label,
                              lv_color_hex(0x888888), LV_PART_MAIN);
  lv_obj_set_style_text_font(g_recorder.analyze_sheet_label,
                             &lv_font_montserrat_14, LV_PART_MAIN);

  // ===== Analyze Button (disabled until both selected) =====
  g_recorder.analyze_btn = lv_button_create(main_cont);
  lv_obj_set_size(g_recorder.analyze_btn, LV_PCT(100), 55);
  lv_obj_set_style_bg_color(g_recorder.analyze_btn, lv_color_hex(0x444466),
                            LV_PART_MAIN);
  lv_obj_set_style_bg_color(g_recorder.analyze_btn, lv_color_hex(0x11998e),
                            LV_STATE_USER_1);
  lv_obj_set_style_radius(g_recorder.analyze_btn, 16, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(g_recorder.analyze_btn, 12, LV_PART_MAIN);
  lv_obj_set_style_shadow_color(g_recorder.analyze_btn, lv_color_hex(0x000000),
                                LV_PART_MAIN);
  lv_obj_set_style_shadow_opa(g_recorder.analyze_btn, LV_OPA_30, LV_PART_MAIN);
  lv_obj_set_style_shadow_offset_y(g_recorder.analyze_btn, 4, LV_PART_MAIN);
  lv_obj_add_event_cb(g_recorder.analyze_btn, btn_do_analyze_cb,
                      LV_EVENT_CLICKED, NULL);

  lv_obj_t *btn_cont = lv_obj_create(g_recorder.analyze_btn);
  lv_obj_set_size(btn_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_opa(btn_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn_cont, 0, LV_PART_MAIN);
  lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(btn_cont, 10, LV_PART_MAIN);
  lv_obj_remove_flag(btn_cont, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *btn_icon = lv_label_create(btn_cont);
  lv_label_set_text(btn_icon, LV_SYMBOL_PLAY);
  lv_obj_set_style_text_color(btn_icon, lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_style_text_font(btn_icon, &lv_font_montserrat_16, LV_PART_MAIN);

  lv_obj_t *btn_text = lv_label_create(btn_cont);
  lv_label_set_text(btn_text, "Start Analysis");
  lv_obj_set_style_text_color(btn_text, lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_style_text_font(btn_text, &lv_font_montserrat_16, LV_PART_MAIN);

  // ===== Result container (scrollable) =====
  g_recorder.analyze_result_cont = lv_obj_create(main_cont);
  lv_obj_set_size(g_recorder.analyze_result_cont, LV_PCT(100), 80);
  lv_obj_set_style_bg_color(g_recorder.analyze_result_cont,
                            lv_color_hex(0x222244), LV_PART_MAIN);
  lv_obj_set_style_radius(g_recorder.analyze_result_cont, 12, LV_PART_MAIN);
  lv_obj_set_style_border_width(g_recorder.analyze_result_cont, 0,
                                LV_PART_MAIN);
  lv_obj_set_style_pad_all(g_recorder.analyze_result_cont, 12, LV_PART_MAIN);
  lv_obj_add_flag(g_recorder.analyze_result_cont, LV_OBJ_FLAG_HIDDEN);

  g_recorder.analyze_result_label =
      lv_label_create(g_recorder.analyze_result_cont);
  lv_label_set_text(g_recorder.analyze_result_label, "");
  lv_obj_set_width(g_recorder.analyze_result_label, LV_PCT(100));
  lv_label_set_long_mode(g_recorder.analyze_result_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(g_recorder.analyze_result_label,
                              lv_color_hex(0xcccccc), LV_PART_MAIN);
  lv_obj_set_style_text_font(g_recorder.analyze_result_label,
                             &lv_font_montserrat_14, LV_PART_MAIN);
}

static void update_analyze_ui(void) {
  // Update session selection label
  if (g_recorder.analyze_session_label) {
    if (g_recorder.selected_session_idx >= 0 &&
        g_recorder.selected_session_idx < g_recorder.session_count) {
      lv_label_set_text(
          g_recorder.analyze_session_label,
          g_recorder.sessions[g_recorder.selected_session_idx].title);
      lv_obj_set_style_text_color(g_recorder.analyze_session_label,
                                  lv_color_hex(0x38EF7D), LV_PART_MAIN);
    } else {
      lv_label_set_text(g_recorder.analyze_session_label,
                        "Tap to choose a recording");
      lv_obj_set_style_text_color(g_recorder.analyze_session_label,
                                  lv_color_hex(0x888888), LV_PART_MAIN);
    }
  }

  // Update sheet selection label
  if (g_recorder.analyze_sheet_label) {
    if (g_recorder.selected_upload_idx >= 0 &&
        g_recorder.selected_upload_idx < g_recorder.upload_count) {
      lv_label_set_text(
          g_recorder.analyze_sheet_label,
          g_recorder.uploads[g_recorder.selected_upload_idx].title);
      lv_obj_set_style_text_color(g_recorder.analyze_sheet_label,
                                  lv_color_hex(0x38EF7D), LV_PART_MAIN);
    } else {
      lv_label_set_text(g_recorder.analyze_sheet_label,
                        "Tap to choose sheet music");
      lv_obj_set_style_text_color(g_recorder.analyze_sheet_label,
                                  lv_color_hex(0x888888), LV_PART_MAIN);
    }
  }

  // Enable/disable analyze button based on selections
  if (g_recorder.analyze_btn) {
    BOOL_T both_selected = (g_recorder.selected_session_idx >= 0 &&
                            g_recorder.selected_upload_idx >= 0);
    if (both_selected) {
      lv_obj_set_style_bg_color(g_recorder.analyze_btn, lv_color_hex(0x11998e),
                                LV_PART_MAIN);
    } else {
      lv_obj_set_style_bg_color(g_recorder.analyze_btn, lv_color_hex(0x444466),
                                LV_PART_MAIN);
    }
  }

  // Update result display
  if (g_recorder.analyze_result_label && g_recorder.analyze_result_cont) {
    if (strlen(g_recorder.analyze_feedback) > 0) {
      lv_label_set_text(g_recorder.analyze_result_label,
                        g_recorder.analyze_feedback);
      lv_obj_remove_flag(g_recorder.analyze_result_cont, LV_OBJ_FLAG_HIDDEN);
    }
  }

  // Update status
  if (g_recorder.analyze_status) {
    if (g_recorder.is_analyzing) {
      lv_label_set_text(g_recorder.analyze_status, "Analyzing with AI...");
    } else if (strlen(g_recorder.analyze_feedback) > 20) {
      lv_label_set_text(g_recorder.analyze_status, "Analysis complete!");
    } else {
      lv_label_set_text(g_recorder.analyze_status,
                        "Select a recording and sheet music to compare");
    }
  }
}

// =================================================================
// SESSIONS SCREEN
// =================================================================

static void create_sessions_screen(void) {
  g_recorder.sessions_screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(g_recorder.sessions_screen, lv_color_hex(0x1a1a2e),
                            LV_PART_MAIN);

  // Back button
  lv_obj_t *back_btn = create_back_button(g_recorder.sessions_screen);
  lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 12, 12);

  // Title
  lv_obj_t *title = lv_label_create(g_recorder.sessions_screen);
  lv_label_set_text(title, "Sessions");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN);

  // Refresh button
  lv_obj_t *refresh_btn = lv_button_create(g_recorder.sessions_screen);
  lv_obj_set_size(refresh_btn, 44, 44);
  lv_obj_align(refresh_btn, LV_ALIGN_TOP_RIGHT, -12, 12);
  lv_obj_set_style_bg_color(refresh_btn, lv_color_hex(0x2d2d4a), LV_PART_MAIN);
  lv_obj_set_style_bg_color(refresh_btn, lv_color_hex(0x3d3d5a),
                            LV_STATE_PRESSED);
  lv_obj_set_style_radius(refresh_btn, 12, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(refresh_btn, 8, LV_PART_MAIN);
  lv_obj_set_style_shadow_color(refresh_btn, lv_color_hex(0x000000),
                                LV_PART_MAIN);
  lv_obj_set_style_shadow_opa(refresh_btn, LV_OPA_30, LV_PART_MAIN);
  lv_obj_set_style_shadow_offset_y(refresh_btn, 3, LV_PART_MAIN);
  lv_obj_add_event_cb(refresh_btn, btn_refresh_sessions_cb, LV_EVENT_CLICKED,
                      NULL);

  lv_obj_t *refresh_icon = lv_label_create(refresh_btn);
  lv_label_set_text(refresh_icon, LV_SYMBOL_REFRESH);
  lv_obj_center(refresh_icon);
  lv_obj_set_style_text_color(refresh_icon, lv_color_hex(0xffffff),
                              LV_PART_MAIN);
  lv_obj_set_style_text_font(refresh_icon, &lv_font_montserrat_16,
                             LV_PART_MAIN);

  // Status label
  g_recorder.sessions_status_label =
      lv_label_create(g_recorder.sessions_screen);
  lv_label_set_text(g_recorder.sessions_status_label, "Loading...");
  lv_obj_align(g_recorder.sessions_status_label, LV_ALIGN_TOP_MID, 0, 65);
  lv_obj_set_style_text_color(g_recorder.sessions_status_label,
                              lv_color_hex(0xaaaaaa), LV_PART_MAIN);
  lv_obj_set_style_text_font(g_recorder.sessions_status_label,
                             &lv_font_montserrat_14, LV_PART_MAIN);

  // List container
  g_recorder.sessions_list_cont = lv_obj_create(g_recorder.sessions_screen);
  lv_obj_set_size(g_recorder.sessions_list_cont, LV_PCT(92), LV_PCT(70));
  lv_obj_align(g_recorder.sessions_list_cont, LV_ALIGN_BOTTOM_MID, 0, -15);
  lv_obj_set_style_bg_opa(g_recorder.sessions_list_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(g_recorder.sessions_list_cont, 0, LV_PART_MAIN);
  lv_obj_set_flex_flow(g_recorder.sessions_list_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(g_recorder.sessions_list_cont, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(g_recorder.sessions_list_cont, 12, LV_PART_MAIN);
  lv_obj_set_style_pad_all(g_recorder.sessions_list_cont, 4, LV_PART_MAIN);

  // Create session item placeholders - each is a clickable card
  for (int i = 0; i < MAX_SESSIONS; i++) {
    lv_obj_t *item = lv_obj_create(g_recorder.sessions_list_cont);
    lv_obj_set_size(item, LV_PCT(100), 72);
    lv_obj_set_style_bg_color(item, lv_color_hex(0x2d2d4a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(item, lv_color_hex(0x3d3d6a), LV_STATE_PRESSED);
    lv_obj_set_style_radius(item, 16, LV_PART_MAIN);
    lv_obj_set_style_border_width(item, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(item, lv_color_hex(0x444466), LV_PART_MAIN);
    lv_obj_set_style_pad_all(item, 16, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(item, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(item, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(item, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_shadow_offset_y(item, 3, LV_PART_MAIN);
    lv_obj_add_flag(item, LV_OBJ_FLAG_HIDDEN); // Hidden until loaded
    lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(item, session_item_clicked_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)i);

    // Play icon container on the left
    lv_obj_t *icon_cont = lv_obj_create(item);
    lv_obj_set_size(icon_cont, 40, 40);
    lv_obj_set_style_bg_color(icon_cont, lv_color_hex(0x667eea), LV_PART_MAIN);
    lv_obj_set_style_radius(icon_cont, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(icon_cont, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_border_width(icon_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(icon_cont, 0, LV_PART_MAIN);
    lv_obj_align(icon_cont, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *play_icon = lv_label_create(icon_cont);
    lv_label_set_text(play_icon, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_color(play_icon, lv_color_hex(0xffffff),
                                LV_PART_MAIN);
    lv_obj_set_style_text_font(play_icon, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_center(play_icon);

    // Session info (title + date) on the right of icon
    g_recorder.session_labels[i] = lv_label_create(item);
    lv_label_set_text(g_recorder.session_labels[i], "");
    lv_obj_set_style_text_color(g_recorder.session_labels[i],
                                lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(g_recorder.session_labels[i],
                               &lv_font_montserrat_16, LV_PART_MAIN);
    lv_label_set_long_mode(g_recorder.session_labels[i], LV_LABEL_LONG_DOT);
    lv_obj_align(g_recorder.session_labels[i], LV_ALIGN_LEFT_MID, 52, 0);
    lv_obj_set_width(g_recorder.session_labels[i], LV_PCT(75));
  }
}

static void update_sessions_ui(void) {
  if (!g_recorder.sessions_loaded) {
    lv_label_set_text(g_recorder.sessions_status_label, "Loading...");
    return;
  }

  if (g_recorder.session_count == 0) {
    // DEBUG: Show full status
    char debug_msg[128];
    snprintf(debug_msg, sizeof(debug_msg), "0 recs WiFi:%s HTTP:%d/%d",
             g_net_connected ? "Y" : "N", g_recorder.last_http_status,
             g_recorder.last_http_code);
    lv_label_set_text(g_recorder.sessions_status_label, debug_msg);
    // Hide all items
    for (int i = 0; i < MAX_SESSIONS; i++) {
      lv_obj_t *parent = lv_obj_get_parent(g_recorder.session_labels[i]);
      lv_obj_add_flag(parent, LV_OBJ_FLAG_HIDDEN);
    }
    return;
  }

  lv_label_set_text_fmt(g_recorder.sessions_status_label, "%d Recording%s",
                        g_recorder.session_count,
                        g_recorder.session_count == 1 ? "" : "s");

  // Update visible items
  for (int i = 0; i < MAX_SESSIONS; i++) {
    lv_obj_t *parent = lv_obj_get_parent(g_recorder.session_labels[i]);

    if (i < g_recorder.session_count) {
      // Title already contains date info, just display it
      lv_label_set_text(g_recorder.session_labels[i],
                        g_recorder.sessions[i].title);
      lv_obj_remove_flag(parent, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(parent, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

// =================================================================
// UPLOADS SCREEN
// =================================================================

static void create_uploads_screen(void) {
  g_recorder.uploads_screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(g_recorder.uploads_screen, lv_color_hex(0x1a1a2e),
                            LV_PART_MAIN);

  // Back button
  lv_obj_t *back_btn = create_back_button(g_recorder.uploads_screen);
  lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 12, 12);

  // Title
  lv_obj_t *title = lv_label_create(g_recorder.uploads_screen);
  lv_label_set_text(title, "Uploads");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN);

  // Refresh button
  lv_obj_t *refresh_btn = lv_button_create(g_recorder.uploads_screen);
  lv_obj_set_size(refresh_btn, 44, 44);
  lv_obj_align(refresh_btn, LV_ALIGN_TOP_RIGHT, -12, 12);
  lv_obj_set_style_bg_color(refresh_btn, lv_color_hex(0x2d2d4a), LV_PART_MAIN);
  lv_obj_set_style_bg_color(refresh_btn, lv_color_hex(0x3d3d5a),
                            LV_STATE_PRESSED);
  lv_obj_set_style_radius(refresh_btn, 12, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(refresh_btn, 8, LV_PART_MAIN);
  lv_obj_set_style_shadow_color(refresh_btn, lv_color_hex(0x000000),
                                LV_PART_MAIN);
  lv_obj_set_style_shadow_opa(refresh_btn, LV_OPA_30, LV_PART_MAIN);
  lv_obj_set_style_shadow_offset_y(refresh_btn, 3, LV_PART_MAIN);
  lv_obj_add_event_cb(refresh_btn, btn_refresh_uploads_cb, LV_EVENT_CLICKED,
                      NULL);

  lv_obj_t *refresh_icon = lv_label_create(refresh_btn);
  lv_label_set_text(refresh_icon, LV_SYMBOL_REFRESH);
  lv_obj_center(refresh_icon);
  lv_obj_set_style_text_color(refresh_icon, lv_color_hex(0xffffff),
                              LV_PART_MAIN);
  lv_obj_set_style_text_font(refresh_icon, &lv_font_montserrat_16,
                             LV_PART_MAIN);

  // Status label
  g_recorder.uploads_status_label = lv_label_create(g_recorder.uploads_screen);
  lv_label_set_text(g_recorder.uploads_status_label, "Loading...");
  lv_obj_align(g_recorder.uploads_status_label, LV_ALIGN_TOP_MID, 0, 65);
  lv_obj_set_style_text_color(g_recorder.uploads_status_label,
                              lv_color_hex(0xaaaaaa), LV_PART_MAIN);
  lv_obj_set_style_text_font(g_recorder.uploads_status_label,
                             &lv_font_montserrat_14, LV_PART_MAIN);

  // List container
  g_recorder.uploads_list_cont = lv_obj_create(g_recorder.uploads_screen);
  lv_obj_set_size(g_recorder.uploads_list_cont, LV_PCT(92), LV_PCT(70));
  lv_obj_align(g_recorder.uploads_list_cont, LV_ALIGN_BOTTOM_MID, 0, -15);
  lv_obj_set_style_bg_opa(g_recorder.uploads_list_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(g_recorder.uploads_list_cont, 0, LV_PART_MAIN);
  lv_obj_set_flex_flow(g_recorder.uploads_list_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(g_recorder.uploads_list_cont, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(g_recorder.uploads_list_cont, 12, LV_PART_MAIN);
  lv_obj_set_style_pad_all(g_recorder.uploads_list_cont, 4, LV_PART_MAIN);

  // Create upload item placeholders - each is a card
  for (int i = 0; i < MAX_UPLOADS; i++) {
    lv_obj_t *item = lv_obj_create(g_recorder.uploads_list_cont);
    lv_obj_set_size(item, LV_PCT(100), 72);
    lv_obj_set_style_bg_color(item, lv_color_hex(0x2d2d4a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(item, lv_color_hex(0x3d3d6a), LV_STATE_PRESSED);
    lv_obj_set_style_radius(item, 16, LV_PART_MAIN);
    lv_obj_set_style_border_width(item, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(item, lv_color_hex(0x444466), LV_PART_MAIN);
    lv_obj_set_style_pad_all(item, 16, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(item, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(item, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(item, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_shadow_offset_y(item, 3, LV_PART_MAIN);
    lv_obj_add_flag(item, LV_OBJ_FLAG_HIDDEN); // Hidden until loaded
    lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(item, upload_item_clicked_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)i);

    // File icon container on the left
    lv_obj_t *icon_cont = lv_obj_create(item);
    lv_obj_set_size(icon_cont, 40, 40);
    lv_obj_set_style_bg_color(icon_cont, lv_color_hex(0xff8e53), LV_PART_MAIN);
    lv_obj_set_style_radius(icon_cont, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(icon_cont, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_border_width(icon_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(icon_cont, 0, LV_PART_MAIN);
    lv_obj_align(icon_cont, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *file_icon = lv_label_create(icon_cont);
    lv_label_set_text(file_icon, LV_SYMBOL_FILE);
    lv_obj_set_style_text_color(file_icon, lv_color_hex(0xffffff),
                                LV_PART_MAIN);
    lv_obj_set_style_text_font(file_icon, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_center(file_icon);

    // Upload info (title + date) on the right of icon
    g_recorder.upload_labels[i] = lv_label_create(item);
    lv_label_set_text(g_recorder.upload_labels[i], "");
    lv_obj_set_style_text_color(g_recorder.upload_labels[i],
                                lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(g_recorder.upload_labels[i],
                               &lv_font_montserrat_16, LV_PART_MAIN);
    lv_label_set_long_mode(g_recorder.upload_labels[i], LV_LABEL_LONG_DOT);
    lv_obj_align(g_recorder.upload_labels[i], LV_ALIGN_LEFT_MID, 52, 0);
    lv_obj_set_width(g_recorder.upload_labels[i], LV_PCT(75));
  }
}

static void update_uploads_ui(void) {
  if (!g_recorder.uploads_loaded) {
    lv_label_set_text(g_recorder.uploads_status_label, "Loading...");
    return;
  }

  if (g_recorder.upload_count == 0) {
    char debug_msg[128];
    snprintf(debug_msg, sizeof(debug_msg), "0 uploads WiFi:%s",
             g_net_connected ? "Y" : "N");
    lv_label_set_text(g_recorder.uploads_status_label, debug_msg);
    // Hide all items
    for (int i = 0; i < MAX_UPLOADS; i++) {
      lv_obj_t *parent = lv_obj_get_parent(g_recorder.upload_labels[i]);
      lv_obj_add_flag(parent, LV_OBJ_FLAG_HIDDEN);
    }
    return;
  }

  lv_label_set_text_fmt(g_recorder.uploads_status_label, "%d Upload%s",
                        g_recorder.upload_count,
                        g_recorder.upload_count == 1 ? "" : "s");

  // Update visible items
  for (int i = 0; i < MAX_UPLOADS; i++) {
    lv_obj_t *parent = lv_obj_get_parent(g_recorder.upload_labels[i]);

    if (i < g_recorder.upload_count) {
      // Display title
      lv_label_set_text(g_recorder.upload_labels[i],
                        g_recorder.uploads[i].title);
      lv_obj_remove_flag(parent, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(parent, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

// =================================================================
// SELECTION SCREENS (for Analyze)
// =================================================================

static void btn_select_session_back_cb(lv_event_t *e) {
  (void)e;
  show_analyze_screen();
}

static void btn_select_sheet_back_cb(lv_event_t *e) {
  (void)e;
  show_analyze_screen();
}

static void select_session_item_cb(lv_event_t *e) {
  int index = (int)(intptr_t)lv_event_get_user_data(e);
  if (index < 0 || index >= g_recorder.session_count)
    return;

  g_recorder.selected_session_idx = index;
  PR_NOTICE("Selected session %d: %s", index, g_recorder.sessions[index].title);
  show_analyze_screen();
  update_analyze_ui();
}

static void select_sheet_item_cb(lv_event_t *e) {
  int index = (int)(intptr_t)lv_event_get_user_data(e);
  if (index < 0 || index >= g_recorder.upload_count)
    return;

  g_recorder.selected_upload_idx = index;
  PR_NOTICE("Selected sheet music %d: %s", index,
            g_recorder.uploads[index].title);
  show_analyze_screen();
  update_analyze_ui();
}

static void btn_select_session_cb(lv_event_t *e) {
  (void)e;
  if (!g_recorder.sessions_loaded) {
    fetch_sessions();
  }
  show_select_session_screen();
}

static void btn_select_sheet_cb(lv_event_t *e) {
  (void)e;
  if (!g_recorder.uploads_loaded) {
    fetch_uploads();
  }
  show_select_sheet_screen();
}

static void create_select_session_screen(void) {
  g_recorder.select_session_screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(g_recorder.select_session_screen,
                            lv_color_hex(0x1a1a2e), LV_PART_MAIN);

  // Back button
  lv_obj_t *back_btn = lv_button_create(g_recorder.select_session_screen);
  lv_obj_set_size(back_btn, 44, 44);
  lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 12, 12);
  lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x2d2d4a), LV_PART_MAIN);
  lv_obj_set_style_radius(back_btn, 12, LV_PART_MAIN);
  lv_obj_add_event_cb(back_btn, btn_select_session_back_cb, LV_EVENT_CLICKED,
                      NULL);
  lv_obj_t *back_lbl = lv_label_create(back_btn);
  lv_label_set_text(back_lbl, LV_SYMBOL_LEFT);
  lv_obj_center(back_lbl);
  lv_obj_set_style_text_color(back_lbl, lv_color_hex(0xffffff), LV_PART_MAIN);

  // Title
  lv_obj_t *title = lv_label_create(g_recorder.select_session_screen);
  lv_label_set_text(title, "Select Recording");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN);

  // Status label
  g_recorder.select_session_status_label =
      lv_label_create(g_recorder.select_session_screen);
  lv_label_set_text(g_recorder.select_session_status_label, "Loading...");
  lv_obj_align(g_recorder.select_session_status_label, LV_ALIGN_TOP_MID, 0, 60);
  lv_obj_set_style_text_color(g_recorder.select_session_status_label,
                              lv_color_hex(0xaaaaaa), LV_PART_MAIN);

  // List container
  g_recorder.select_session_list_cont =
      lv_obj_create(g_recorder.select_session_screen);
  lv_obj_set_size(g_recorder.select_session_list_cont, LV_PCT(92), LV_PCT(70));
  lv_obj_align(g_recorder.select_session_list_cont, LV_ALIGN_BOTTOM_MID, 0,
               -15);
  lv_obj_set_style_bg_opa(g_recorder.select_session_list_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(g_recorder.select_session_list_cont, 0,
                                LV_PART_MAIN);
  lv_obj_set_flex_flow(g_recorder.select_session_list_cont,
                       LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(g_recorder.select_session_list_cont,
                        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(g_recorder.select_session_list_cont, 10,
                           LV_PART_MAIN);

  // Create items
  for (int i = 0; i < MAX_SESSIONS; i++) {
    lv_obj_t *item = lv_obj_create(g_recorder.select_session_list_cont);
    lv_obj_set_size(item, LV_PCT(100), 60);
    lv_obj_set_style_bg_color(item, lv_color_hex(0x2d2d4a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(item, lv_color_hex(0x667eea), LV_STATE_PRESSED);
    lv_obj_set_style_radius(item, 14, LV_PART_MAIN);
    lv_obj_set_style_border_width(item, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(item, lv_color_hex(0x667eea), LV_PART_MAIN);
    lv_obj_set_style_pad_all(item, 12, LV_PART_MAIN);
    lv_obj_add_flag(item, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(item, select_session_item_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)i);

    lv_obj_t *icon = lv_label_create(item);
    lv_label_set_text(icon, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_color(icon, lv_color_hex(0x667eea), LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 0, 0);

    g_recorder.select_session_labels[i] = lv_label_create(item);
    lv_label_set_text(g_recorder.select_session_labels[i], "");
    lv_obj_set_style_text_color(g_recorder.select_session_labels[i],
                                lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_label_set_long_mode(g_recorder.select_session_labels[i],
                           LV_LABEL_LONG_DOT);
    lv_obj_align(g_recorder.select_session_labels[i], LV_ALIGN_LEFT_MID, 30, 0);
    lv_obj_set_width(g_recorder.select_session_labels[i], LV_PCT(80));
  }
}

static void update_select_session_ui(void) {
  if (!g_recorder.sessions_loaded) {
    lv_label_set_text(g_recorder.select_session_status_label, "Loading...");
    return;
  }

  if (g_recorder.session_count == 0) {
    lv_label_set_text(g_recorder.select_session_status_label,
                      "No recordings found");
    for (int i = 0; i < MAX_SESSIONS; i++) {
      lv_obj_t *parent = lv_obj_get_parent(g_recorder.select_session_labels[i]);
      lv_obj_add_flag(parent, LV_OBJ_FLAG_HIDDEN);
    }
    return;
  }

  lv_label_set_text_fmt(g_recorder.select_session_status_label,
                        "Choose a recording (%d available)",
                        g_recorder.session_count);

  for (int i = 0; i < MAX_SESSIONS; i++) {
    lv_obj_t *parent = lv_obj_get_parent(g_recorder.select_session_labels[i]);
    if (i < g_recorder.session_count) {
      lv_label_set_text(g_recorder.select_session_labels[i],
                        g_recorder.sessions[i].title);
      lv_obj_remove_flag(parent, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(parent, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

static void create_select_sheet_screen(void) {
  g_recorder.select_sheet_screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(g_recorder.select_sheet_screen,
                            lv_color_hex(0x1a1a2e), LV_PART_MAIN);

  // Back button
  lv_obj_t *back_btn = lv_button_create(g_recorder.select_sheet_screen);
  lv_obj_set_size(back_btn, 44, 44);
  lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 12, 12);
  lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x2d2d4a), LV_PART_MAIN);
  lv_obj_set_style_radius(back_btn, 12, LV_PART_MAIN);
  lv_obj_add_event_cb(back_btn, btn_select_sheet_back_cb, LV_EVENT_CLICKED,
                      NULL);
  lv_obj_t *back_lbl = lv_label_create(back_btn);
  lv_label_set_text(back_lbl, LV_SYMBOL_LEFT);
  lv_obj_center(back_lbl);
  lv_obj_set_style_text_color(back_lbl, lv_color_hex(0xffffff), LV_PART_MAIN);

  // Title
  lv_obj_t *title = lv_label_create(g_recorder.select_sheet_screen);
  lv_label_set_text(title, "Select Sheet Music");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN);

  // Status label
  g_recorder.select_sheet_status_label =
      lv_label_create(g_recorder.select_sheet_screen);
  lv_label_set_text(g_recorder.select_sheet_status_label, "Loading...");
  lv_obj_align(g_recorder.select_sheet_status_label, LV_ALIGN_TOP_MID, 0, 60);
  lv_obj_set_style_text_color(g_recorder.select_sheet_status_label,
                              lv_color_hex(0xaaaaaa), LV_PART_MAIN);

  // List container
  g_recorder.select_sheet_list_cont =
      lv_obj_create(g_recorder.select_sheet_screen);
  lv_obj_set_size(g_recorder.select_sheet_list_cont, LV_PCT(92), LV_PCT(70));
  lv_obj_align(g_recorder.select_sheet_list_cont, LV_ALIGN_BOTTOM_MID, 0, -15);
  lv_obj_set_style_bg_opa(g_recorder.select_sheet_list_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(g_recorder.select_sheet_list_cont, 0,
                                LV_PART_MAIN);
  lv_obj_set_flex_flow(g_recorder.select_sheet_list_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(g_recorder.select_sheet_list_cont, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(g_recorder.select_sheet_list_cont, 10, LV_PART_MAIN);

  // Create items
  for (int i = 0; i < MAX_UPLOADS; i++) {
    lv_obj_t *item = lv_obj_create(g_recorder.select_sheet_list_cont);
    lv_obj_set_size(item, LV_PCT(100), 60);
    lv_obj_set_style_bg_color(item, lv_color_hex(0x2d2d4a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(item, lv_color_hex(0xf5576c), LV_STATE_PRESSED);
    lv_obj_set_style_radius(item, 14, LV_PART_MAIN);
    lv_obj_set_style_border_width(item, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(item, lv_color_hex(0xf5576c), LV_PART_MAIN);
    lv_obj_set_style_pad_all(item, 12, LV_PART_MAIN);
    lv_obj_add_flag(item, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(item, select_sheet_item_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)i);

    lv_obj_t *icon = lv_label_create(item);
    lv_label_set_text(icon, LV_SYMBOL_FILE);
    lv_obj_set_style_text_color(icon, lv_color_hex(0xf5576c), LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 0, 0);

    g_recorder.select_sheet_labels[i] = lv_label_create(item);
    lv_label_set_text(g_recorder.select_sheet_labels[i], "");
    lv_obj_set_style_text_color(g_recorder.select_sheet_labels[i],
                                lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_label_set_long_mode(g_recorder.select_sheet_labels[i],
                           LV_LABEL_LONG_DOT);
    lv_obj_align(g_recorder.select_sheet_labels[i], LV_ALIGN_LEFT_MID, 30, 0);
    lv_obj_set_width(g_recorder.select_sheet_labels[i], LV_PCT(80));
  }
}

static void update_select_sheet_ui(void) {
  if (!g_recorder.uploads_loaded) {
    lv_label_set_text(g_recorder.select_sheet_status_label, "Loading...");
    return;
  }

  if (g_recorder.upload_count == 0) {
    lv_label_set_text(g_recorder.select_sheet_status_label,
                      "No sheet music found");
    for (int i = 0; i < MAX_UPLOADS; i++) {
      lv_obj_t *parent = lv_obj_get_parent(g_recorder.select_sheet_labels[i]);
      lv_obj_add_flag(parent, LV_OBJ_FLAG_HIDDEN);
    }
    return;
  }

  lv_label_set_text_fmt(g_recorder.select_sheet_status_label,
                        "Choose sheet music (%d available)",
                        g_recorder.upload_count);

  for (int i = 0; i < MAX_UPLOADS; i++) {
    lv_obj_t *parent = lv_obj_get_parent(g_recorder.select_sheet_labels[i]);
    if (i < g_recorder.upload_count) {
      lv_label_set_text(g_recorder.select_sheet_labels[i],
                        g_recorder.uploads[i].title);
      lv_obj_remove_flag(parent, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(parent, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

// =================================================================
// PLAYBACK SCREEN
// =================================================================

static void btn_playback_back_cb(lv_event_t *e) {
  (void)e;
  stop_playback();
  show_sessions_screen();
}

static void btn_view_upload_back_cb(lv_event_t *e) {
  (void)e;
  show_uploads_screen();
}

static void btn_play_pause_cb(lv_event_t *e) {
  (void)e;
  if (g_recorder.is_playing) {
    // Pause
    g_recorder.is_playing = FALSE;
    lv_label_set_text(lv_obj_get_child(g_recorder.playback_play_btn, 0),
                      LV_SYMBOL_PLAY);
  } else {
    // Play/Resume
    if (g_recorder.audio_buffer && g_recorder.audio_size > 0) {
      g_recorder.is_playing = TRUE;
      lv_label_set_text(lv_obj_get_child(g_recorder.playback_play_btn, 0),
                        LV_SYMBOL_PAUSE);
    }
  }
}

static void btn_skip_back_cb(lv_event_t *e) {
  (void)e;
  // Skip back 5 seconds (5 * 16000 * 2 bytes = 160000 bytes for 16kHz mono
  // 16-bit)
  if (g_recorder.audio_position > 160000) {
    g_recorder.audio_position -= 160000;
  } else {
    g_recorder.audio_position = 0;
  }
  update_playback_ui();
}

static void btn_skip_forward_cb(lv_event_t *e) {
  (void)e;
  // Skip forward 5 seconds
  g_recorder.audio_position += 160000;
  if (g_recorder.audio_position >= g_recorder.audio_size) {
    g_recorder.audio_position =
        g_recorder.audio_size > 160000 ? g_recorder.audio_size - 160000 : 0;
  }
  update_playback_ui();
}

static void slider_volume_cb(lv_event_t *e) {
  lv_obj_t *slider = lv_event_get_target(e);
  int32_t val = lv_slider_get_value(slider);
  g_recorder.playback_volume = val;
  tkl_ao_set_vol(TKL_AUDIO_TYPE_BOARD, 0, NULL, val);
}

// =================================================================
// VIEW UPLOAD SCREEN (Information Display)
// =================================================================

static void create_view_upload_screen(void) {
  g_recorder.view_upload_screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(g_recorder.view_upload_screen,
                            lv_color_hex(0x1a1a2e), LV_PART_MAIN);

  // Back button
  lv_obj_t *back_btn = create_back_button(g_recorder.view_upload_screen);
  lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 12, 12);
  lv_obj_add_event_cb(back_btn, btn_view_upload_back_cb, LV_EVENT_CLICKED,
                      NULL);

  // Title
  g_recorder.view_upload_title_label =
      lv_label_create(g_recorder.view_upload_screen);
  lv_label_set_text(g_recorder.view_upload_title_label, "Sheet Music");
  lv_obj_align(g_recorder.view_upload_title_label, LV_ALIGN_TOP_MID, 0, 18);
  lv_obj_set_style_text_font(g_recorder.view_upload_title_label,
                             &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(g_recorder.view_upload_title_label,
                              lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_width(g_recorder.view_upload_title_label, LV_PCT(80));
  lv_label_set_long_mode(g_recorder.view_upload_title_label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(g_recorder.view_upload_title_label,
                              LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

  // Main scrollable info container
  g_recorder.view_upload_info_cont =
      lv_obj_create(g_recorder.view_upload_screen);
  lv_obj_set_size(g_recorder.view_upload_info_cont, LV_PCT(92), LV_PCT(75));
  lv_obj_align(g_recorder.view_upload_info_cont, LV_ALIGN_BOTTOM_MID, 0, -15);
  lv_obj_set_style_bg_opa(g_recorder.view_upload_info_cont, LV_OPA_0,
                          LV_PART_MAIN);
  lv_obj_set_style_border_width(g_recorder.view_upload_info_cont, 0,
                                LV_PART_MAIN);
  lv_obj_set_style_pad_all(g_recorder.view_upload_info_cont, 0, LV_PART_MAIN);

  // Info chips container (horizontal)
  lv_obj_t *chips_cont = lv_obj_create(g_recorder.view_upload_info_cont);
  lv_obj_set_size(chips_cont, LV_PCT(100), 40);
  lv_obj_align(chips_cont, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_bg_opa(chips_cont, LV_OPA_0, LV_PART_MAIN);
  lv_obj_set_style_border_width(chips_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(chips_cont, 0, LV_PART_MAIN);
  lv_obj_set_flex_flow(chips_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(chips_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(chips_cont, 10, LV_PART_MAIN);

  // Time signature chip
  g_recorder.view_upload_time_sig_chip = lv_obj_create(chips_cont);
  lv_obj_set_size(g_recorder.view_upload_time_sig_chip, 70, 36);
  lv_obj_set_style_bg_color(g_recorder.view_upload_time_sig_chip,
                            lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(g_recorder.view_upload_time_sig_chip, LV_OPA_10,
                          LV_PART_MAIN);
  lv_obj_set_style_radius(g_recorder.view_upload_time_sig_chip, 8,
                          LV_PART_MAIN);
  lv_obj_set_style_border_width(g_recorder.view_upload_time_sig_chip, 0,
                                LV_PART_MAIN);
  lv_obj_set_style_pad_all(g_recorder.view_upload_time_sig_chip, 8,
                           LV_PART_MAIN);
  lv_obj_set_flex_flow(g_recorder.view_upload_time_sig_chip, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(g_recorder.view_upload_time_sig_chip,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(g_recorder.view_upload_time_sig_chip, 6,
                           LV_PART_MAIN);

  lv_obj_t *time_icon = lv_label_create(g_recorder.view_upload_time_sig_chip);
  lv_label_set_text(time_icon, LV_SYMBOL_REFRESH);
  lv_obj_set_style_text_color(time_icon, lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_style_text_font(time_icon, &lv_font_montserrat_14, LV_PART_MAIN);

  lv_obj_t *time_label = lv_label_create(g_recorder.view_upload_time_sig_chip);
  lv_label_set_text(time_label, "4/4");
  lv_obj_set_style_text_color(time_label, lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_style_text_font(time_label, &lv_font_montserrat_14, LV_PART_MAIN);

  // Key signature chip
  g_recorder.view_upload_key_sig_chip = lv_obj_create(chips_cont);
  lv_obj_set_size(g_recorder.view_upload_key_sig_chip, 60, 36);
  lv_obj_set_style_bg_color(g_recorder.view_upload_key_sig_chip,
                            lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(g_recorder.view_upload_key_sig_chip, LV_OPA_10,
                          LV_PART_MAIN);
  lv_obj_set_style_radius(g_recorder.view_upload_key_sig_chip, 8, LV_PART_MAIN);
  lv_obj_set_style_border_width(g_recorder.view_upload_key_sig_chip, 0,
                                LV_PART_MAIN);
  lv_obj_set_style_pad_all(g_recorder.view_upload_key_sig_chip, 8,
                           LV_PART_MAIN);
  lv_obj_set_flex_flow(g_recorder.view_upload_key_sig_chip, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(g_recorder.view_upload_key_sig_chip,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(g_recorder.view_upload_key_sig_chip, 6,
                           LV_PART_MAIN);

  lv_obj_t *key_icon = lv_label_create(g_recorder.view_upload_key_sig_chip);
  lv_label_set_text(key_icon, "#"); // Key signature symbol
  lv_obj_set_style_text_color(key_icon, lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_style_text_font(key_icon, &lv_font_montserrat_14, LV_PART_MAIN);

  lv_obj_t *key_label = lv_label_create(g_recorder.view_upload_key_sig_chip);
  lv_label_set_text(key_label, "C");
  lv_obj_set_style_text_color(key_label, lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_style_text_font(key_label, &lv_font_montserrat_14, LV_PART_MAIN);

  // Analyzed chip (created but hidden until needed)
  g_recorder.view_upload_analyzed_chip = lv_obj_create(chips_cont);
  lv_obj_set_size(g_recorder.view_upload_analyzed_chip, 90, 36);
  lv_obj_set_style_bg_color(g_recorder.view_upload_analyzed_chip,
                            lv_color_hex(0x38EF7D), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(g_recorder.view_upload_analyzed_chip, LV_OPA_20,
                          LV_PART_MAIN);
  lv_obj_set_style_radius(g_recorder.view_upload_analyzed_chip, 8,
                          LV_PART_MAIN);
  lv_obj_set_style_border_width(g_recorder.view_upload_analyzed_chip, 0,
                                LV_PART_MAIN);
  lv_obj_set_style_pad_all(g_recorder.view_upload_analyzed_chip, 8,
                           LV_PART_MAIN);
  lv_obj_set_flex_flow(g_recorder.view_upload_analyzed_chip, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(g_recorder.view_upload_analyzed_chip,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(g_recorder.view_upload_analyzed_chip, 6,
                           LV_PART_MAIN);
  lv_obj_add_flag(g_recorder.view_upload_analyzed_chip, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *check_icon = lv_label_create(g_recorder.view_upload_analyzed_chip);
  lv_label_set_text(check_icon, LV_SYMBOL_OK);
  lv_obj_set_style_text_color(check_icon, lv_color_hex(0x38EF7D), LV_PART_MAIN);
  lv_obj_set_style_text_font(check_icon, &lv_font_montserrat_14, LV_PART_MAIN);

  lv_obj_t *analyzed_label =
      lv_label_create(g_recorder.view_upload_analyzed_chip);
  lv_label_set_text(analyzed_label, "Analyzed");
  lv_obj_set_style_text_color(analyzed_label, lv_color_hex(0x38EF7D),
                              LV_PART_MAIN);
  lv_obj_set_style_text_font(analyzed_label, &lv_font_montserrat_14,
                             LV_PART_MAIN);

  // Music Data container
  g_recorder.view_upload_music_data_cont =
      lv_obj_create(g_recorder.view_upload_info_cont);
  lv_obj_set_size(g_recorder.view_upload_music_data_cont, LV_PCT(100), 100);
  lv_obj_align(g_recorder.view_upload_music_data_cont, LV_ALIGN_TOP_LEFT, 0,
               50);
  lv_obj_set_style_bg_color(g_recorder.view_upload_music_data_cont,
                            lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(g_recorder.view_upload_music_data_cont, LV_OPA_10,
                          LV_PART_MAIN);
  lv_obj_set_style_radius(g_recorder.view_upload_music_data_cont, 12,
                          LV_PART_MAIN);
  lv_obj_set_style_border_width(g_recorder.view_upload_music_data_cont, 0,
                                LV_PART_MAIN);
  lv_obj_set_style_pad_all(g_recorder.view_upload_music_data_cont, 16,
                           LV_PART_MAIN);

  // Music Data section title
  lv_obj_t *music_title =
      lv_label_create(g_recorder.view_upload_music_data_cont);
  lv_label_set_text(music_title, "Music Data");
  lv_obj_align(music_title, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_text_color(music_title, lv_color_hex(0xaaaaaa),
                              LV_PART_MAIN);
  lv_obj_set_style_text_font(music_title, &lv_font_montserrat_14, LV_PART_MAIN);

  // Music Data content label
  g_recorder.view_upload_music_data_label =
      lv_label_create(g_recorder.view_upload_music_data_cont);
  lv_label_set_text(g_recorder.view_upload_music_data_label, "");
  lv_obj_align(g_recorder.view_upload_music_data_label, LV_ALIGN_TOP_LEFT, 0,
               24);
  lv_obj_set_width(g_recorder.view_upload_music_data_label, LV_PCT(100));
  lv_label_set_long_mode(g_recorder.view_upload_music_data_label,
                         LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(g_recorder.view_upload_music_data_label,
                              lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_style_text_font(g_recorder.view_upload_music_data_label,
                             &lv_font_montserrat_16, LV_PART_MAIN);

  // Upload date label
  g_recorder.view_upload_date_label =
      lv_label_create(g_recorder.view_upload_info_cont);
  lv_label_set_text(g_recorder.view_upload_date_label, "");
  lv_obj_align(g_recorder.view_upload_date_label, LV_ALIGN_TOP_LEFT, 0, 160);
  lv_obj_set_style_text_color(g_recorder.view_upload_date_label,
                              lv_color_hex(0x666666), LV_PART_MAIN);
  lv_obj_set_style_text_font(g_recorder.view_upload_date_label,
                             &lv_font_montserrat_14, LV_PART_MAIN);

  // Initialize state
  g_recorder.current_upload_idx = -1;
}

static void update_view_upload_ui(int upload_idx) {
  if (upload_idx < 0 || upload_idx >= g_recorder.upload_count)
    return;

  const upload_info_t *upload = &g_recorder.uploads[upload_idx];

  // Update time signature chip
  if (g_recorder.view_upload_time_sig_chip) {
    lv_obj_t *time_label = lv_obj_get_child(
        g_recorder.view_upload_time_sig_chip, 1); // Second child is label
    if (time_label) {
      lv_label_set_text(time_label, upload->time_signature);
    }
  }

  // Update key signature chip
  if (g_recorder.view_upload_key_sig_chip) {
    lv_obj_t *key_label = lv_obj_get_child(g_recorder.view_upload_key_sig_chip,
                                           1); // Second child is label
    if (key_label) {
      lv_label_set_text(key_label, upload->key_signature);
    }
  }

  // Show/hide analyzed chip
  if (g_recorder.view_upload_analyzed_chip) {
    if (upload->has_analysis) {
      lv_obj_clear_flag(g_recorder.view_upload_analyzed_chip,
                        LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(g_recorder.view_upload_analyzed_chip, LV_OBJ_FLAG_HIDDEN);
    }
  }

  // Update Music Data section
  if (g_recorder.view_upload_music_data_label) {
    char music_text[128] = {0};
    int pos = 0;

    // Note count
    pos += snprintf(music_text + pos, sizeof(music_text) - pos,
                    "%d notes detected", upload->note_count);

    // Clef (if available)
    if (upload->clef[0] != '\0') {
      pos += snprintf(music_text + pos, sizeof(music_text) - pos, "\nClef: %s",
                      upload->clef);
    }

    lv_label_set_text(g_recorder.view_upload_music_data_label, music_text);
  }

  // Update upload date
  if (g_recorder.view_upload_date_label) {
    char date_text[64];
    snprintf(date_text, sizeof(date_text), "Uploaded %s", upload->date);
    lv_label_set_text(g_recorder.view_upload_date_label, date_text);
  }
}

// =================================================================
// PLAYBACK SCREEN
// =================================================================

static void create_playback_screen(void) {
  g_recorder.playback_screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(g_recorder.playback_screen, lv_color_hex(0x0f0f1a),
                            LV_PART_MAIN);

  // Back button
  lv_obj_t *back_btn = lv_button_create(g_recorder.playback_screen);
  lv_obj_set_size(back_btn, 50, 40);
  lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x333355), LV_PART_MAIN);
  lv_obj_set_style_radius(back_btn, 10, LV_PART_MAIN);
  lv_obj_add_event_cb(back_btn, btn_playback_back_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_center(back_label);
  lv_obj_set_style_text_color(back_label, lv_color_hex(0xffffff), LV_PART_MAIN);

  // "Now Playing" header
  lv_obj_t *header = lv_label_create(g_recorder.playback_screen);
  lv_label_set_text(header, "Now Playing");
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 15);
  lv_obj_set_style_text_font(header, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(header, lv_color_hex(0x888888), LV_PART_MAIN);

  // Title
  g_recorder.playback_title_label = lv_label_create(g_recorder.playback_screen);
  lv_label_set_text(g_recorder.playback_title_label, "Loading...");
  lv_obj_align(g_recorder.playback_title_label, LV_ALIGN_TOP_MID, 0, 60);
  lv_obj_set_style_text_font(g_recorder.playback_title_label,
                             &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(g_recorder.playback_title_label,
                              lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_width(g_recorder.playback_title_label, LV_PCT(80));
  lv_obj_set_style_text_align(g_recorder.playback_title_label,
                              LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_label_set_long_mode(g_recorder.playback_title_label,
                         LV_LABEL_LONG_SCROLL_CIRCULAR);

  // Status/time label
  g_recorder.playback_status_label =
      lv_label_create(g_recorder.playback_screen);
  lv_label_set_text(g_recorder.playback_status_label, "");
  lv_obj_align(g_recorder.playback_status_label, LV_ALIGN_TOP_MID, 0, 100);
  lv_obj_set_style_text_color(g_recorder.playback_status_label,
                              lv_color_hex(0x667eea), LV_PART_MAIN);

  // Progress slider (visual only for now)
  g_recorder.playback_slider = lv_slider_create(g_recorder.playback_screen);
  lv_obj_set_size(g_recorder.playback_slider, LV_PCT(80), 8);
  lv_obj_align(g_recorder.playback_slider, LV_ALIGN_CENTER, 0, -20);
  lv_slider_set_range(g_recorder.playback_slider, 0, 100);
  lv_slider_set_value(g_recorder.playback_slider, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(g_recorder.playback_slider, lv_color_hex(0x333355),
                            LV_PART_MAIN);
  lv_obj_set_style_bg_color(g_recorder.playback_slider, lv_color_hex(0x667eea),
                            LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(g_recorder.playback_slider, lv_color_hex(0x667eea),
                            LV_PART_KNOB);
  lv_obj_remove_flag(g_recorder.playback_slider,
                     LV_OBJ_FLAG_CLICKABLE); // Read-only progress

  // Time label
  g_recorder.playback_time_label = lv_label_create(g_recorder.playback_screen);
  lv_label_set_text(g_recorder.playback_time_label, "0:00 / 0:00");
  lv_obj_align(g_recorder.playback_time_label, LV_ALIGN_CENTER, 0, 10);
  lv_obj_set_style_text_color(g_recorder.playback_time_label,
                              lv_color_hex(0x888888), LV_PART_MAIN);

  // Control buttons container
  lv_obj_t *controls = lv_obj_create(g_recorder.playback_screen);
  lv_obj_set_size(controls, LV_PCT(80), 70);
  lv_obj_align(controls, LV_ALIGN_CENTER, 0, 70);
  lv_obj_set_style_bg_opa(controls, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(controls, 0, LV_PART_MAIN);
  lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(controls, LV_FLEX_ALIGN_SPACE_EVENLY,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // Skip back button
  lv_obj_t *skip_back_btn = lv_button_create(controls);
  lv_obj_set_size(skip_back_btn, 50, 50);
  lv_obj_set_style_bg_color(skip_back_btn, lv_color_hex(0x333355),
                            LV_PART_MAIN);
  lv_obj_set_style_radius(skip_back_btn, 25, LV_PART_MAIN);
  lv_obj_add_event_cb(skip_back_btn, btn_skip_back_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *skip_back_lbl = lv_label_create(skip_back_btn);
  lv_label_set_text(skip_back_lbl, LV_SYMBOL_PREV);
  lv_obj_center(skip_back_lbl);
  lv_obj_set_style_text_color(skip_back_lbl, lv_color_hex(0xffffff),
                              LV_PART_MAIN);

  // Play/Pause button (large)
  g_recorder.playback_play_btn = lv_button_create(controls);
  lv_obj_set_size(g_recorder.playback_play_btn, 70, 70);
  lv_obj_set_style_bg_color(g_recorder.playback_play_btn,
                            lv_color_hex(0x667eea), LV_PART_MAIN);
  lv_obj_set_style_radius(g_recorder.playback_play_btn, 35, LV_PART_MAIN);
  lv_obj_add_event_cb(g_recorder.playback_play_btn, btn_play_pause_cb,
                      LV_EVENT_CLICKED, NULL);
  lv_obj_t *play_lbl = lv_label_create(g_recorder.playback_play_btn);
  lv_label_set_text(play_lbl, LV_SYMBOL_PLAY);
  lv_obj_center(play_lbl);
  lv_obj_set_style_text_color(play_lbl, lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_style_text_font(play_lbl, &lv_font_montserrat_24, LV_PART_MAIN);

  // Skip forward button
  lv_obj_t *skip_fwd_btn = lv_button_create(controls);
  lv_obj_set_size(skip_fwd_btn, 50, 50);
  lv_obj_set_style_bg_color(skip_fwd_btn, lv_color_hex(0x333355), LV_PART_MAIN);
  lv_obj_set_style_radius(skip_fwd_btn, 25, LV_PART_MAIN);
  lv_obj_add_event_cb(skip_fwd_btn, btn_skip_forward_cb, LV_EVENT_CLICKED,
                      NULL);
  lv_obj_t *skip_fwd_lbl = lv_label_create(skip_fwd_btn);
  lv_label_set_text(skip_fwd_lbl, LV_SYMBOL_NEXT);
  lv_obj_center(skip_fwd_lbl);
  lv_obj_set_style_text_color(skip_fwd_lbl, lv_color_hex(0xffffff),
                              LV_PART_MAIN);

  // Volume section
  lv_obj_t *vol_cont = lv_obj_create(g_recorder.playback_screen);
  lv_obj_set_size(vol_cont, LV_PCT(80), 40);
  lv_obj_align(vol_cont, LV_ALIGN_BOTTOM_MID, 0, -30);
  lv_obj_set_style_bg_opa(vol_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(vol_cont, 0, LV_PART_MAIN);
  lv_obj_set_flex_flow(vol_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(vol_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(vol_cont, 10, LV_PART_MAIN);

  lv_obj_t *vol_icon = lv_label_create(vol_cont);
  lv_label_set_text(vol_icon, LV_SYMBOL_VOLUME_MAX);
  lv_obj_set_style_text_color(vol_icon, lv_color_hex(0x888888), LV_PART_MAIN);

  lv_obj_t *vol_slider = lv_slider_create(vol_cont);
  lv_obj_set_size(vol_slider, 150, 8);
  lv_slider_set_range(vol_slider, 0, 100);
  lv_slider_set_value(vol_slider, 70, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(vol_slider, lv_color_hex(0x333355), LV_PART_MAIN);
  lv_obj_set_style_bg_color(vol_slider, lv_color_hex(0x38EF7D),
                            LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(vol_slider, lv_color_hex(0x38EF7D), LV_PART_KNOB);
  lv_obj_add_event_cb(vol_slider, slider_volume_cb, LV_EVENT_VALUE_CHANGED,
                      NULL);

  g_recorder.playback_volume = 70;
}

// =================================================================
// CHUNKED DOWNLOAD - for large audio files
// Downloads in small chunks using HTTP Range requests to work around
// the firmware HTTP client's buffer size limitations
// =================================================================

#define CHUNK_SIZE 131072 // 128KB chunks

static void download_and_play_audio(const char *recording_id) {
  if (!recording_id || strlen(recording_id) == 0) {
    lv_label_set_text(g_recorder.playback_status_label, "No recording ID");
    return;
  }

  // Check network connectivity first
  WF_STATION_STAT_E wifi_stat = WSS_IDLE;
  tal_wifi_station_get_status(&wifi_stat);
  if (wifi_stat != WSS_GOT_IP) {
    PR_ERR("Cannot download: WiFi not connected (status: %d)", wifi_stat);
    lv_label_set_text(g_recorder.playback_status_label, "Err: No WiFi");
    return;
  }

  // Ensure network is stable
  if (!g_net_connected) {
    PR_NOTICE("Network not marked connected, waiting for stabilization...");
    tal_system_sleep(500);
    g_net_connected = TRUE;
  }

  PR_NOTICE("Recording ID: '%s'", recording_id);

  // Show loading overlay
  show_loading_overlay("Loading Recording...");

  // Free previous buffer if any
  if (g_recorder.audio_buffer) {
    tal_psram_free(g_recorder.audio_buffer);
    g_recorder.audio_buffer = NULL;
  }
  g_recorder.audio_size = 0;
  g_recorder.audio_position = 0;

  // Clean the recording ID
  char clean_id[48] = {0};
  int j = 0;
  for (int i = 0; recording_id[i] && j < 47; i++) {
    char c = recording_id[i];
    if (c > 32 && c < 127) {
      clean_id[j++] = c;
    }
  }
  clean_id[j] = '\0';

  if (strlen(clean_id) == 0) {
    hide_loading_overlay();
    lv_label_set_text(g_recorder.playback_status_label, "Invalid recording ID");
    return;
  }

  // Build paths
  char info_path[128];
  char audio_path[128];
  snprintf(info_path, sizeof(info_path), "/api/device/audio-info/%s", clean_id);
  snprintf(audio_path, sizeof(audio_path), "/api/device/audio/%s", clean_id);

  const char *dev_id = DEVICE_UUID;

  // =================================================================
  // STEP 1: Get file size from backend
  // =================================================================
  PR_NOTICE("Fetching audio info from %s:%d%s", CLOUD_BACKEND_HOST,
            CLOUD_BACKEND_PORT, info_path);
  if (g_recorder.loading_size_label) {
    lv_label_set_text(g_recorder.loading_size_label, "Getting info...");
  }

  http_client_header_t headers[] = {{.key = "X-User-ID", .value = dev_id},
                                    {.key = "Connection", .value = "close"}};

  http_client_response_t response = {0};
  http_client_status_t status = http_client_request(
      &(const http_client_request_t){.host = CLOUD_BACKEND_HOST,
                                     .port = CLOUD_BACKEND_PORT,
                                     .path = info_path,
                                     .cacert = NULL,
                                     .cacert_len = 0,
                                     .method = "GET",
                                     .headers = headers,
                                     .headers_count = 2,
                                     .body = NULL,
                                     .body_length = 0,
                                     .timeout_ms = 10000},
      &response);

  if (status != HTTP_CLIENT_SUCCESS || response.status_code != 200) {
    PR_ERR("Failed to get audio info: status=%d, http=%d", status,
           response.status_code);
    hide_loading_overlay();
    lv_label_set_text(g_recorder.playback_status_label, "Err: Can't get info");
    http_client_free(&response);
    return;
  }

  // Parse JSON response to get file size
  uint32_t total_size = 0;
  if (response.body && response.body_length > 0) {
    cJSON *root = cJSON_Parse((char *)response.body);
    if (root) {
      cJSON *size_json = cJSON_GetObjectItem(root, "size");
      if (size_json && cJSON_IsNumber(size_json)) {
        total_size = (uint32_t)size_json->valuedouble;
      }
      cJSON_Delete(root);
    }
  }
  http_client_free(&response);

  if (total_size < 100) {
    PR_ERR("Invalid file size: %d", total_size);
    hide_loading_overlay();
    lv_label_set_text(g_recorder.playback_status_label, "Err: Invalid size");
    return;
  }

  PR_NOTICE("Audio file size: %d bytes", total_size);

  // =================================================================
  // STEP 2: Allocate PSRAM buffer for PCM data (skip 44-byte WAV header)
  // =================================================================
  uint32_t pcm_size = total_size - 44; // Skip WAV header
  g_recorder.audio_buffer = tal_psram_malloc(pcm_size);
  if (!g_recorder.audio_buffer) {
    PR_ERR("Failed to allocate %d bytes in PSRAM", pcm_size);
    hide_loading_overlay();
    lv_label_set_text(g_recorder.playback_status_label, "Err: No memory");
    return;
  }
  PR_NOTICE("Allocated %d bytes in PSRAM for audio", pcm_size);

  // =================================================================
  // STEP 3: Download in chunks using HTTP Range requests
  // =================================================================
  uint32_t bytes_downloaded = 0;
  uint32_t chunk_count = 0;
  BOOL_T download_failed = FALSE;

  while (bytes_downloaded < total_size && !download_failed) {
    // Process UI events (so cancel button can be clicked)
    lv_timer_handler();

    // Check if user cancelled
    if (g_recorder.download_cancelled) {
      PR_NOTICE("Download cancelled by user");
      download_failed = TRUE;
      break;
    }

    // Calculate range for this chunk
    uint32_t range_start = bytes_downloaded;
    uint32_t range_end = bytes_downloaded + CHUNK_SIZE - 1;
    if (range_end >= total_size) {
      range_end = total_size - 1;
    }

    // Build Range header value
    char range_value[64];
    snprintf(range_value, sizeof(range_value), "bytes=%d-%d", range_start,
             range_end);

    http_client_header_t chunk_headers[] = {
        {.key = "X-User-ID", .value = dev_id},
        {.key = "Range", .value = range_value},
        {.key = "Connection", .value = "close"}};

    // Update loading overlay progress
    int progress = (bytes_downloaded * 100) / total_size;
    update_loading_progress(progress, bytes_downloaded, total_size);

    // Make request for this chunk
    memset(&response, 0, sizeof(response));
    status = http_client_request(
        &(const http_client_request_t){
            .host = CLOUD_BACKEND_HOST,
            .port = CLOUD_BACKEND_PORT,
            .path = audio_path,
            .cacert = NULL,
            .cacert_len = 0,
            .method = "GET",
            .headers = chunk_headers,
            .headers_count = 3,
            .body = NULL,
            .body_length = 0,
            .timeout_ms = 10000 // 10 seconds per chunk
        },
        &response);

    // Check result - expect 206 Partial Content or 200 OK
    if (status != HTTP_CLIENT_SUCCESS) {
      PR_ERR("Chunk %d failed: status=%d", chunk_count, status);
      // Retry this chunk once
      tal_system_sleep(500);
      memset(&response, 0, sizeof(response));
      status = http_client_request(
          &(const http_client_request_t){.host = CLOUD_BACKEND_HOST,
                                         .port = CLOUD_BACKEND_PORT,
                                         .path = audio_path,
                                         .cacert = NULL,
                                         .cacert_len = 0,
                                         .method = "GET",
                                         .headers = chunk_headers,
                                         .headers_count = 3,
                                         .body = NULL,
                                         .body_length = 0,
                                         .timeout_ms = 15000},
          &response);

      if (status != HTTP_CLIENT_SUCCESS) {
        PR_ERR("Chunk %d retry failed: status=%d", chunk_count, status);
        download_failed = TRUE;
        http_client_free(&response);
        break;
      }
    }

    if (response.status_code != 206 && response.status_code != 200) {
      PR_ERR("Chunk %d: unexpected HTTP %d", chunk_count, response.status_code);
      download_failed = TRUE;
      http_client_free(&response);
      break;
    }

    if (response.body_length == 0) {
      PR_ERR("Chunk %d: empty body", chunk_count);
      download_failed = TRUE;
      http_client_free(&response);
      break;
    }

    // Copy chunk data to PSRAM buffer (skipping WAV header bytes)
    uint32_t chunk_size = response.body_length;
    uint8_t *src = (uint8_t *)response.body;
    uint32_t src_offset = 0;

    // Handle WAV header in first chunk(s)
    if (range_start < 44) {
      // Some or all of this chunk is WAV header
      uint32_t header_bytes_in_chunk = 44 - range_start;
      if (header_bytes_in_chunk > chunk_size) {
        header_bytes_in_chunk = chunk_size;
      }
      src_offset = header_bytes_in_chunk; // Skip header bytes
    }

    // Calculate where to write in PCM buffer
    uint32_t pcm_offset = (range_start > 44) ? (range_start - 44) : 0;
    uint32_t bytes_to_copy = chunk_size - src_offset;

    if (bytes_to_copy > 0 && pcm_offset + bytes_to_copy <= pcm_size) {
      memcpy(g_recorder.audio_buffer + pcm_offset, src + src_offset,
             bytes_to_copy);
    }

    bytes_downloaded += chunk_size;
    chunk_count++;

    // Log progress every 10 chunks
    if (chunk_count % 10 == 0) {
      PR_NOTICE("Downloaded %d/%d bytes (%d chunks)", bytes_downloaded,
                total_size, chunk_count);
    }

    http_client_free(&response);

    // No delay needed - keep downloading as fast as possible
  }

  // Hide loading overlay
  hide_loading_overlay();

  if (download_failed) {
    if (g_recorder.download_cancelled) {
      PR_NOTICE("Download cancelled by user");
      // Buffer already freed by cancel callback, just make sure it's NULL
      if (g_recorder.audio_buffer) {
        tal_psram_free(g_recorder.audio_buffer);
        g_recorder.audio_buffer = NULL;
      }
      // Already navigated back to sessions by cancel callback, just return
      return;
    } else {
      PR_ERR("Download failed after %d chunks", chunk_count);
      lv_label_set_text(g_recorder.playback_status_label, "Download failed");
      if (g_recorder.audio_buffer) {
        tal_psram_free(g_recorder.audio_buffer);
        g_recorder.audio_buffer = NULL;
      }
      return;
    }
  }

  PR_NOTICE("Download complete: %d bytes in %d chunks", bytes_downloaded,
            chunk_count);

  g_recorder.audio_size = pcm_size;
  g_recorder.audio_position = 0;

  // Initialize audio output
  tkl_ao_set_vol(TKL_AUDIO_TYPE_BOARD, 0, NULL, g_recorder.playback_volume);

  lv_label_set_text(g_recorder.playback_status_label, "Ready to play");
  update_playback_ui();

  // Auto-play
  g_recorder.is_playing = TRUE;
  lv_label_set_text(lv_obj_get_child(g_recorder.playback_play_btn, 0),
                    LV_SYMBOL_PAUSE);
}

static void stop_playback(void) {
  g_recorder.is_playing = FALSE;
  g_recorder.audio_position = 0;

  if (g_recorder.audio_buffer) {
    tal_psram_free(g_recorder.audio_buffer); // Allocated from PSRAM
    g_recorder.audio_buffer = NULL;
  }
  g_recorder.audio_size = 0;
}

static void update_playback_ui(void) {
  if (!g_recorder.playback_screen)
    return;

  // Update progress
  if (g_recorder.audio_size > 0) {
    int progress = (g_recorder.audio_position * 100) / g_recorder.audio_size;
    lv_slider_set_value(g_recorder.playback_slider, progress, LV_ANIM_OFF);

    // Calculate time (16kHz, 16-bit mono = 32000 bytes per second)
    uint32_t current_sec = g_recorder.audio_position / 32000;
    uint32_t total_sec = g_recorder.audio_size / 32000;

    char time_str[32];
    snprintf(time_str, sizeof(time_str), "%d:%02d / %d:%02d", current_sec / 60,
             current_sec % 60, total_sec / 60, total_sec % 60);
    lv_label_set_text(g_recorder.playback_time_label, time_str);
  }
}

// =================================================================
// SCREEN NAVIGATION
// =================================================================

static void show_main_screen(void) {
  g_recorder.current_screen = SCREEN_MAIN;
  lv_screen_load(g_recorder.main_screen);
}

static void show_recording_screen(void) {
  g_recorder.current_screen = SCREEN_RECORDING;
  lv_label_set_text(g_recorder.timer_label, "00:00.000");
  lv_label_set_text(g_recorder.status_label, "Recording...");
  lv_screen_load(g_recorder.recording_screen);
}

static void show_analyze_screen(void) {
  g_recorder.current_screen = SCREEN_ANALYZE;
  update_analyze_ui();
  lv_screen_load(g_recorder.analyze_screen);
}

static void show_sessions_screen(void) {
  g_recorder.current_screen = SCREEN_SESSIONS;
  lv_screen_load(g_recorder.sessions_screen);
}

static void show_uploads_screen(void) {
  g_recorder.current_screen = SCREEN_UPLOADS;
  update_uploads_ui();
  lv_screen_load(g_recorder.uploads_screen);
}

static void show_view_upload_screen(int upload_idx) {
  if (upload_idx < 0 || upload_idx >= g_recorder.upload_count)
    return;

  g_recorder.current_upload_idx = upload_idx;
  g_recorder.current_screen = SCREEN_VIEW_UPLOAD;

  // Set title
  if (g_recorder.view_upload_title_label) {
    lv_label_set_text(g_recorder.view_upload_title_label,
                      g_recorder.uploads[upload_idx].title);
  }

  lv_screen_load(g_recorder.view_upload_screen);

  // Display information
  update_view_upload_ui(upload_idx);
}

static void show_select_session_screen(void) {
  g_recorder.current_screen = SCREEN_SELECT_SESSION;
  update_select_session_ui();
  lv_screen_load(g_recorder.select_session_screen);
}

static void show_select_sheet_screen(void) {
  g_recorder.current_screen = SCREEN_SELECT_SHEET;
  update_select_sheet_ui();
  lv_screen_load(g_recorder.select_sheet_screen);
}

static void show_playback_screen(int session_idx) {
  if (session_idx < 0 || session_idx >= g_recorder.session_count)
    return;

  g_recorder.current_screen = SCREEN_PLAYBACK;
  g_recorder.current_session_idx = session_idx;

  // Set title
  lv_label_set_text(g_recorder.playback_title_label,
                    g_recorder.sessions[session_idx].title);
  lv_label_set_text(g_recorder.playback_status_label, "Getting Ready...");
  lv_slider_set_value(g_recorder.playback_slider, 0, LV_ANIM_OFF);
  lv_label_set_text(g_recorder.playback_time_label, "0:00 / 0:00");
  lv_label_set_text(lv_obj_get_child(g_recorder.playback_play_btn, 0),
                    LV_SYMBOL_PLAY);

  lv_screen_load(g_recorder.playback_screen);

  // Check if recording has valid ID
  if (strlen(g_recorder.sessions[session_idx].id) == 0) {
    PR_ERR("Session has no valid ID");
    lv_label_set_text(g_recorder.playback_status_label,
                      "Error: No recording ID");
    return;
  }

  // Check if audio_url exists (optional - we can still download by ID)
  if (g_recorder.sessions[session_idx].audio_url[0] != '\0') {
    PR_NOTICE("Session has audio_url: %s",
              g_recorder.sessions[session_idx].audio_url);
  } else {
    PR_NOTICE("Session has no audio_url yet, will wait for it");
  }

  // Give UI time to render before starting download
  tal_system_sleep(200);

  // Start download via backend proxy (uses recording ID)
  download_and_play_audio(g_recorder.sessions[session_idx].id);
}

// =================================================================
// MAIN UI INITIALIZATION
// =================================================================

// Cancel button callback for loading overlay
// Create loading overlay (modal dialog over any screen)
static void create_loading_overlay(void) {
  // Create overlay on the default display's layer_top so it appears over
  // everything
  g_recorder.loading_overlay = lv_obj_create(lv_layer_top());
  lv_obj_set_size(g_recorder.loading_overlay, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(g_recorder.loading_overlay, lv_color_hex(0x000000),
                            LV_PART_MAIN);
  lv_obj_set_style_bg_opa(g_recorder.loading_overlay, LV_OPA_80, LV_PART_MAIN);
  lv_obj_set_style_border_width(g_recorder.loading_overlay, 0, LV_PART_MAIN);
  lv_obj_add_flag(g_recorder.loading_overlay,
                  LV_OBJ_FLAG_HIDDEN); // Hidden by default

  // Center container for the loading content
  lv_obj_t *cont = lv_obj_create(g_recorder.loading_overlay);
  lv_obj_set_size(cont, LV_PCT(80), 120);
  lv_obj_center(cont);
  lv_obj_set_style_bg_color(cont, lv_color_hex(0x1a1a2e), LV_PART_MAIN);
  lv_obj_set_style_radius(cont, 16, LV_PART_MAIN);
  lv_obj_set_style_border_width(cont, 2, LV_PART_MAIN);
  lv_obj_set_style_border_color(cont, lv_color_hex(0x667eea), LV_PART_MAIN);
  lv_obj_set_style_pad_all(cont, 15, LV_PART_MAIN);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  // "Loading..." label
  g_recorder.loading_label = lv_label_create(cont);
  lv_label_set_text(g_recorder.loading_label, "Loading...");
  lv_obj_set_style_text_color(g_recorder.loading_label, lv_color_hex(0xffffff),
                              LV_PART_MAIN);
  lv_obj_set_style_text_font(g_recorder.loading_label, &lv_font_montserrat_16,
                             LV_PART_MAIN);

  // Progress bar
  g_recorder.loading_bar = lv_bar_create(cont);
  lv_obj_set_size(g_recorder.loading_bar, LV_PCT(100), 12);
  lv_bar_set_range(g_recorder.loading_bar, 0, 100);
  lv_bar_set_value(g_recorder.loading_bar, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(g_recorder.loading_bar, lv_color_hex(0x333355),
                            LV_PART_MAIN);
  lv_obj_set_style_bg_color(g_recorder.loading_bar, lv_color_hex(0x667eea),
                            LV_PART_INDICATOR);
  lv_obj_set_style_radius(g_recorder.loading_bar, 6, LV_PART_MAIN);
  lv_obj_set_style_radius(g_recorder.loading_bar, 6, LV_PART_INDICATOR);

  // Size label (e.g., "450 / 1700 KB")
  g_recorder.loading_size_label = lv_label_create(cont);
  lv_label_set_text(g_recorder.loading_size_label, "0 / 0 KB");
  lv_obj_set_style_text_color(g_recorder.loading_size_label,
                              lv_color_hex(0x888888), LV_PART_MAIN);
  lv_obj_set_style_text_font(g_recorder.loading_size_label,
                             &lv_font_montserrat_14, LV_PART_MAIN);
}

static void show_loading_overlay(const char *title) {
  if (g_recorder.loading_overlay) {
    lv_label_set_text(g_recorder.loading_label, title ? title : "Loading...");
    lv_label_set_text(g_recorder.loading_size_label, "Connecting...");
    lv_bar_set_value(g_recorder.loading_bar, 0, LV_ANIM_OFF);
    g_recorder.download_cancelled = FALSE;
    lv_obj_remove_flag(g_recorder.loading_overlay, LV_OBJ_FLAG_HIDDEN);
    // Force immediate UI refresh so overlay is visible
    lv_refr_now(NULL);
  }
}

static void hide_loading_overlay(void) {
  if (g_recorder.loading_overlay) {
    lv_obj_add_flag(g_recorder.loading_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_refr_now(NULL);
  }
}

static void update_loading_progress(int progress, uint32_t downloaded,
                                    uint32_t total) {
  if (g_recorder.loading_bar) {
    lv_bar_set_value(g_recorder.loading_bar, progress, LV_ANIM_OFF);
  }
  if (g_recorder.loading_size_label) {
    char size_str[48];
    snprintf(size_str, sizeof(size_str), "%d / %d KB", downloaded / 1024,
             total / 1024);
    lv_label_set_text(g_recorder.loading_size_label, size_str);
  }
  // Force UI refresh to show progress updates
  lv_refr_now(NULL);
}

static void create_main_ui(void) {
  g_recorder.recording_count = 0;
  g_recorder.current_screen = SCREEN_MAIN;
  g_recorder.download_cancelled = FALSE;

  // Create all screens
  create_main_screen();
  create_recording_screen();
  create_analyze_screen();
  create_sessions_screen();
  create_uploads_screen();
  create_view_upload_screen();
  create_select_session_screen();
  create_select_sheet_screen();
  create_playback_screen();

  // Create loading overlay (appears on top of all screens)
  create_loading_overlay();

  // Show main screen
  show_main_screen();
}

// =================================================================
// MAIN ENTRY
// =================================================================

/**
 * @brief User Main - Called by Tuya Thread
 */
void user_main(void) {
  tal_log_init(TAL_LOG_LEVEL_DEBUG, 4096, (TAL_LOG_OUTPUT_CB)tkl_log_output);

  PR_NOTICE("========================================");
  PR_NOTICE("=== PracticePod Starting ===");
  PR_NOTICE("=== Log Level: DEBUG ===");
  PR_NOTICE("========================================");

  // 0. Initialize Hardware & UI FIRST
  PR_NOTICE("[MAIN] Calling device_init()...");
  device_init();
  PR_NOTICE("[MAIN] device_init() returned");

  // Initialize IoT Config
  PR_DEBUG("[MAIN] Setting up Tuya IoT config...");
  PR_DEBUG("[MAIN]   PRODUCT_KEY: %s", PRODUCT_KEY);
  PR_DEBUG("[MAIN]   DEVICE_UUID: %s", DEVICE_UUID);
  tuya_iot_config_t config = {
      .productkey = PRODUCT_KEY,
      .uuid = DEVICE_UUID,
      .authkey = AUTH_KEY,
      .software_ver = "1.0.0",
      .event_handler = app_event_handler,
  };

  PR_NOTICE("[MAIN] Project: %s, Version: 1.0.0", PROJECT_NAME);

  // Required SDK initializations (from examples)
  PR_NOTICE("[INIT] Initializing TAL subsystems...");
  tal_kv_init(&(tal_kv_cfg_t){
      .seed = "vmlkasdh93dlvlcy",
      .key = "dflfuap134ddlduq",
  });
  PR_DEBUG("[INIT] tal_kv_init OK");

  tal_sw_timer_init();
  PR_DEBUG("[INIT] tal_sw_timer_init OK");

  tal_workq_init();
  PR_DEBUG("[INIT] tal_workq_init OK");

  // Init Network Manager
  PR_NOTICE("[WIFI] Initializing network manager...");
  OPERATE_RET ret = netmgr_init(NETCONN_WIFI);
  if (ret != OPRT_OK) {
    PR_ERR("[WIFI] netmgr_init FAILED: %d", ret);
  } else {
    PR_NOTICE("[WIFI] netmgr_init OK");
  }

  // Connect to WiFi using network manager
  PR_NOTICE("[WIFI] Connecting to WiFi...");
  PR_NOTICE("[WIFI]   SSID: '%s'", USER_SSID);
  PR_NOTICE("[WIFI]   Password: '%s'", USER_PASSWORD);
  netconn_wifi_info_t wifi_info = {0};
  strcpy(wifi_info.ssid, USER_SSID);
  strcpy(wifi_info.pswd, USER_PASSWORD);
  netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_SSID_PSWD, &wifi_info);
  PR_NOTICE("[WIFI] WiFi connection initiated (async)");

  // Wait for WiFi connection (up to 30 seconds)
  PR_NOTICE("[WIFI] Waiting for connection (max 30 sec)...");
  WF_STATION_STAT_E wifi_stat = WSS_IDLE;
  int wifi_wait = 0;
  while (wifi_wait < 300) { // 30 seconds max (300 * 100ms)
    tal_wifi_station_get_status(&wifi_stat);
    if (wifi_stat == WSS_GOT_IP) {
      PR_NOTICE("[WIFI] *** CONNECTED! Got IP address ***");
      g_net_connected = TRUE;
      break;
    }
    if (wifi_wait % 10 == 0) { // Log every second
      const char *stat_str = "UNKNOWN";
      switch (wifi_stat) {
      case WSS_IDLE:
        stat_str = "IDLE";
        break;
      case WSS_CONNECTING:
        stat_str = "CONNECTING";
        break;
      case WSS_PASSWD_WRONG:
        stat_str = "PASSWD_WRONG";
        break;
      case WSS_NO_AP_FOUND:
        stat_str = "NO_AP_FOUND";
        break;
      case WSS_CONN_FAIL:
        stat_str = "CONN_FAIL";
        break;
      case WSS_GOT_IP:
        stat_str = "GOT_IP";
        break;
      default:
        break;
      }
      PR_DEBUG("[WIFI] Status: %s (%d) - waiting %d sec...", stat_str,
               wifi_stat, wifi_wait / 10);
    }
    tal_system_sleep(100);
    wifi_wait++;
  }

  if (wifi_stat != WSS_GOT_IP) {
    PR_ERR("[WIFI] *** CONNECTION FAILED after 30 seconds ***");
    PR_ERR("[WIFI] Final status: %d", wifi_stat);
    PR_ERR("[WIFI] Check: Is hotspot ON? Is SSID '%s' correct? Is password "
           "'%s' correct?",
           USER_SSID, USER_PASSWORD);
  }

  // Initialize Tuya IoT SDK
  PR_NOTICE("[TUYA] Initializing Tuya IoT SDK...");
  tuya_iot_client_t *client = tuya_iot_client_get();
  ret = tuya_iot_init(client, &config);
  if (ret != OPRT_OK) {
    PR_ERR("[TUYA] tuya_iot_init FAILED: %d", ret);
  } else {
    PR_NOTICE("[TUYA] tuya_iot_init OK");
  }

  PR_NOTICE("[TUYA] Starting Tuya IoT (connecting to cloud)...");
  ret = tuya_iot_start(client);
  if (ret != OPRT_OK) {
    PR_ERR("[TUYA] tuya_iot_start FAILED: %d", ret);
  } else {
    PR_NOTICE("[TUYA] tuya_iot_start OK");
  }

  PR_NOTICE("[MAIN] ========================================");
  PR_NOTICE("[MAIN] === INITIALIZATION COMPLETE ===");
  PR_NOTICE("[MAIN] === Device UUID: %s ===", DEVICE_UUID);
  PR_NOTICE("[MAIN] ========================================");
  PR_NOTICE("[MAIN] Entering main loop...");

  int loop_count = 0;
  while (1) {
    tuya_iot_yield(client); // Process Tuya Cloud messages
    tal_system_sleep(100);

    // Log every 10 seconds to show we're alive
    loop_count++;
    if (loop_count % 100 == 0) {
      PR_DEBUG("[MAIN] Heartbeat: loop %d, WiFi=%s", loop_count / 100,
               g_net_connected ? "OK" : "NO");
    }
  }
}

/**
 * @brief Tuya App Thread Helper
 */
static THREAD_HANDLE ty_app_thread = NULL;
static void tuya_app_thread(void *arg) {
  (void)arg;
  user_main();
  tal_thread_delete(ty_app_thread);
  ty_app_thread = NULL;
}

void tuya_app_main(void) {
  THREAD_CFG_T thrd_param = {1024 * 4, 4, "tuya_app_main"};
  tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL,
                              &thrd_param);
}
