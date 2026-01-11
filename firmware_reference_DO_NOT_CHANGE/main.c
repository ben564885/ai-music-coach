/**
 * @file main.c
 * @brief AI Music Coach / PracticePod - Firmware for T5AI
 *
 * Build with: tos.py build
 */

#include "board_com_api.h"
#include "lv_vendor.h"
#include "lvgl.h"
#include "tal_api.h"
#include "tal_log.h"
#include "tal_system.h"
#include "tkl_audio.h"
#include "tkl_fs.h"
#include "tkl_output.h"
#include "tuya_cloud_types.h"
#include "tuya_ringbuf.h"
#include "wav_encode.h"
#include <string.h>

#define TAG "practicepod"

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

typedef struct {
  TUYA_RINGBUFF_T pcm_ringbuf;
  BOOL_T is_recording;
  uint32_t start_time;
  uint16_t peak_amplitude;

  lv_obj_t *timer_label;
  lv_obj_t *bars[20];
  lv_obj_t *record_btn;
  lv_obj_t *record_btn_lbl;
  lv_obj_t *status_label;
  lv_timer_t *ui_timer;
} recorder_mgr_t;

static recorder_mgr_t g_recorder;

// Helper to find next available filename
static void get_next_filename(char *path_buf, size_t buf_len) {
  int i = 1;
  while (i < 1000) {
    snprintf(path_buf, buf_len, "/sdcard/record_%d.wav", i);
    BOOL_T exists = FALSE;
    if (tkl_fs_is_exist(path_buf, &exists) != OPRT_OK || !exists) {
      return;
    }
    i++;
  }
}

// Forward declarations
static void create_recording_ui(void);

// File system and Audio capture logic
static int _audio_frame_put(TKL_AUDIO_FRAME_INFO_T *pframe) {
  if (g_recorder.is_recording && g_recorder.pcm_ringbuf) {
    tuya_ring_buff_write(g_recorder.pcm_ringbuf, pframe->pbuf,
                         pframe->used_size);

    // Simple peak amplitude calculation for visualizer
    int16_t *samples = (int16_t *)pframe->pbuf;
    uint32_t sample_count = pframe->used_size / 2;
    uint16_t max_amp = 0;
    for (uint32_t i = 0; i < sample_count; i++) {
      uint16_t abs_val = (samples[i] < 0) ? -samples[i] : samples[i];
      if (abs_val > max_amp)
        max_amp = abs_val;
    }
    // Higher sensitivity and better smoothing
    uint32_t boosted_amp = (uint32_t)max_amp * 4; // 4x boost
    if (boosted_amp > 32767)
      boosted_amp = 32767;
    g_recorder.peak_amplitude =
        (g_recorder.peak_amplitude * 2 + boosted_amp) / 3;
  }
  return pframe->used_size;
}

static OPERATE_RET app_fs_init(void) {
  OPERATE_RET rt = tkl_fs_mount(SD_MOUNT_POINT, DEV_SDCARD);
  if (g_recorder.status_label) {
    if (rt != OPRT_OK) {
      lv_label_set_text_fmt(g_recorder.status_label, "Status: SD Error (%d)",
                            rt);
      PR_ERR("SD mount failed: %d", rt);
    } else {
      lv_label_set_text(g_recorder.status_label, "Status: SD Ready");
      PR_NOTICE("SD mount success");
    }
  }
  return rt;
}

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

static void save_recording(void);

// Periodic UI Update
static void ui_timer_cb(lv_timer_t *timer) {
  (void)timer;
  if (!g_recorder.is_recording)
    return;

  // Update Timer (MM:SS.mmm)
  uint32_t elapsed = tal_system_get_tick_count() - g_recorder.start_time;
  uint32_t ms = elapsed % 1000;
  uint32_t sec = elapsed / 1000;
  uint32_t min = sec / 60;
  sec %= 60;
  lv_label_set_text_fmt(g_recorder.timer_label, "%02d:%02d.%03d", min, sec, ms);

  // Update Waveform (Mirrored bars)
  // Scale amplitude (0-32767) to spectrum range (0-50)
  uint32_t amp = (g_recorder.peak_amplitude * 45) / 32768 + 2; // Min 2px line

  // Shift bars
  for (int i = 0; i < 19; i++) {
    int32_t val = lv_bar_get_value(g_recorder.bars[i + 1]);
    lv_bar_set_value(g_recorder.bars[i], val, LV_ANIM_OFF);
    lv_bar_set_start_value(g_recorder.bars[i], -val, LV_ANIM_OFF);
  }
  lv_bar_set_value(g_recorder.bars[19], amp, LV_ANIM_OFF);
  lv_bar_set_start_value(g_recorder.bars[19], -amp, LV_ANIM_OFF);
}

// Button callback (Toggle Record/Stop)
static void record_toggle_cb(lv_event_t *e) {
  (void)e;
  if (!g_recorder.is_recording) {
    // START RECORDING
    tuya_ring_buff_reset(g_recorder.pcm_ringbuf);
    g_recorder.start_time = tal_system_get_tick_count();
    g_recorder.is_recording = TRUE;
    g_recorder.peak_amplitude = 0;

    // UI Feedback
    lv_label_set_text(g_recorder.record_btn_lbl, LV_SYMBOL_STOP);
    lv_obj_set_style_bg_color(g_recorder.record_btn, lv_color_hex(0xffffff),
                              LV_PART_MAIN);
    lv_obj_set_style_text_color(g_recorder.record_btn_lbl,
                                lv_color_hex(0xff3b30), LV_PART_MAIN);
    lv_label_set_text(g_recorder.status_label, "Recording...");

    // Clear bars
    for (int i = 0; i < 20; i++) {
      lv_bar_set_value(g_recorder.bars[i], 2, LV_ANIM_OFF);
      lv_bar_set_start_value(g_recorder.bars[i], -2, LV_ANIM_OFF);
    }

    PR_NOTICE("Recording started");
  } else {
    // STOP RECORDING
    g_recorder.is_recording = FALSE;

    // UI Feedback
    lv_label_set_text(g_recorder.record_btn_lbl, LV_SYMBOL_STOP);
    lv_obj_set_style_bg_color(g_recorder.record_btn, lv_color_hex(0xff3b30),
                              LV_PART_MAIN);
    lv_obj_set_style_text_color(g_recorder.record_btn_lbl,
                                lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_label_set_text(g_recorder.status_label, "Stopping & Saving...");
    lv_label_set_text(g_recorder.timer_label, "00:00.000"); // Reset immediately

    // Reset bars immediately to flat
    for (int i = 0; i < 20; i++) {
      lv_bar_set_value(g_recorder.bars[i], 2, LV_ANIM_OFF);
      lv_bar_set_start_value(g_recorder.bars[i], -2, LV_ANIM_OFF);
    }

    PR_NOTICE("Recording stopped");

    // Auto Save
    save_recording();
  }
}

static void save_recording(void) {
  uint32_t pcm_len = tuya_ring_buff_used_size_get(g_recorder.pcm_ringbuf);
  if (pcm_len == 0) {
    lv_label_set_text(g_recorder.status_label, "No audio data");
    return;
  }

  char filename[64];
  get_next_filename(filename, sizeof(filename));

  lv_label_set_text_fmt(g_recorder.status_label, "Saving...");
  PR_NOTICE("Auto-saving %d bytes to %s...", pcm_len, filename);

  uint8_t wav_head[WAV_HEAD_LEN];
  app_get_wav_head(pcm_len, 1, MIC_SAMPLE_RATE, MIC_SAMPLE_BITS, MIC_CHANNEL,
                   wav_head);

  TUYA_FILE f = tkl_fopen(filename, "w");
  if (f == NULL) {
    PR_ERR("Failed to open file for saving: %s", filename);

    // Check if directory exists
    BOOL_T exists = FALSE;
    tkl_fs_is_exist(SD_MOUNT_POINT, &exists);
    if (!exists) {
      lv_label_set_text(g_recorder.status_label, "Status: SD Path Missing");
    } else {
      lv_label_set_text(g_recorder.status_label, "Status: File Open Failed");
    }
    return;
  }

  tkl_fwrite(wav_head, WAV_HEAD_LEN, f);

// Write in chunks to avoid large memory allocation
#define CHUNK_SIZE 4096
  uint8_t *chunk = tal_malloc(CHUNK_SIZE);
  if (chunk) {
    uint32_t remaining = pcm_len;
    while (remaining > 0) {
      uint32_t to_read = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
      tuya_ring_buff_read(g_recorder.pcm_ringbuf, chunk, to_read);
      tkl_fwrite(chunk, to_read, f);
      remaining -= to_read;
    }
    tal_free(chunk);
  }

  tkl_fclose(f);
  // Extract filename from path for status message
  char *base = strrchr(filename, '/');
  lv_label_set_text_fmt(g_recorder.status_label, "Saved %s",
                        base ? base + 1 : filename);
  PR_NOTICE("Save complete: %s", filename);
}

/**
 * @brief user_main - Application logic entry point
 */
void user_main(void) {
  tal_log_init(TAL_LOG_LEVEL_DEBUG, 4096, (TAL_LOG_OUTPUT_CB)tkl_log_output);

  PR_NOTICE("========================================");
  PR_NOTICE("=== PracticePod Starting ===");
  PR_NOTICE("Firmware version: 1.1.0");
  PR_NOTICE("========================================");

  // 1. Initialize Board Hardware (Pinmux, Display Power, etc.)
  PR_NOTICE("Registering Board Hardware...");
  int ret = board_register_hardware();
  if (ret != OPRT_OK) {
    PR_ERR("board_register_hardware failed: %d", ret);
  } else {
    PR_NOTICE("board_register_hardware success");
  }

  // 2. Initialize LVGL via Vendor API
  // DISPLAY_NAME is usually defined via Kconfig/build system
#ifdef DISPLAY_NAME
  lv_vendor_init(DISPLAY_NAME);
#else
  lv_vendor_init("display");
#endif
  PR_NOTICE("LVGL Vendor Init complete");

  // 3. Create our UI
  create_recording_ui();
  PR_NOTICE("UI Created");

  // 4. Initialize SD card and Audio
  app_fs_init();
  app_audio_init();

  // Create PCM ring buffer
  tuya_ring_buff_create(PCM_BUF_SIZE, OVERFLOW_PSRAM_STOP_TYPE,
                        &g_recorder.pcm_ringbuf);

  // Setup UI Timer (50ms for smooth waveform)
  g_recorder.ui_timer = lv_timer_create(ui_timer_cb, 50, NULL);

  // 5. Start the LVGL background task
  // Priority 5, 8KB stack
  lv_vendor_start(5, 1024 * 8);
  PR_NOTICE("LVGL Task Started");

  // Main application loop
  int counter = 0;
  while (1) {
    tal_system_sleep(1000);
    counter++;
    if (counter % 10 == 0) {
      PR_DEBUG("PracticePod alive, uptime=%ds", counter);
    }
  }
}

/**
 * @brief Tuya App Thread
 */
static THREAD_HANDLE ty_app_thread = NULL;

static void tuya_app_thread(void *arg) {
  (void)arg;
  user_main();
  tal_thread_delete(ty_app_thread);
  ty_app_thread = NULL;
}

/**
 * @brief Main entry point called by the SDK
 */
void tuya_app_main(void) {
  THREAD_CFG_T thrd_param = {1024 * 4, 4, "tuya_app_main"};
  tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL,
                              &thrd_param);
}

/**
 * @brief Create the Audio Recording UI
 */
static void create_recording_ui(void) {
  lv_obj_t *scr = lv_screen_active();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);

  lv_obj_t *main_cont = lv_obj_create(scr);
  lv_obj_set_size(main_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_opa(main_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(main_cont, 0, LV_PART_MAIN);
  lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(main_cont, LV_FLEX_ALIGN_SPACE_AROUND,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(main_cont, 20, LV_PART_MAIN);

  // 1. Timer Display
  g_recorder.timer_label = lv_label_create(main_cont);
  lv_label_set_text(g_recorder.timer_label, "00:00.000");
  lv_obj_set_width(g_recorder.timer_label,
                   160); // Fixed width to prevent jitter
  lv_obj_set_style_text_align(g_recorder.timer_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(g_recorder.timer_label, &lv_font_montserrat_24,
                             LV_PART_MAIN);
  lv_obj_set_style_text_color(g_recorder.timer_label, lv_color_hex(0xffffff),
                              LV_PART_MAIN);

  // 2. Waveform Visualizer (Bar Row) - Mirrored from middle
  lv_obj_t *bar_area = lv_obj_create(main_cont);
  lv_obj_set_size(bar_area, LV_PCT(95), 100);
  lv_obj_set_style_bg_opa(bar_area, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(bar_area, 0, LV_PART_MAIN);
  lv_obj_remove_flag(bar_area, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *bar_cont = lv_obj_create(bar_area);
  lv_obj_set_size(bar_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_opa(bar_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(bar_cont, 0, LV_PART_MAIN);
  lv_obj_set_flex_flow(bar_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bar_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(bar_cont, 4, LV_PART_MAIN);
  lv_obj_set_style_pad_all(bar_cont, 0, LV_PART_MAIN);
  lv_obj_remove_flag(bar_cont, LV_OBJ_FLAG_SCROLLABLE);

  for (int i = 0; i < 20; i++) {
    g_recorder.bars[i] = lv_bar_create(bar_cont);
    lv_obj_set_size(g_recorder.bars[i], 6, 100);
    lv_bar_set_mode(g_recorder.bars[i], LV_BAR_MODE_RANGE);
    lv_bar_set_range(g_recorder.bars[i], -50, 50);
    lv_obj_set_style_bg_opa(g_recorder.bars[i], 0, LV_PART_MAIN); // Hide track
    lv_obj_set_style_bg_color(g_recorder.bars[i], lv_color_hex(0xff3b30),
                              LV_PART_INDICATOR);
    lv_bar_set_value(g_recorder.bars[i], 2, LV_ANIM_OFF);
    lv_bar_set_start_value(g_recorder.bars[i], -2, LV_ANIM_OFF);
  }

  // 3. Status/Format info
  g_recorder.status_label = lv_label_create(main_cont);
  lv_label_set_text(g_recorder.status_label, "Ready to Record");
  lv_obj_set_style_text_color(g_recorder.status_label, lv_color_hex(0x888888),
                              LV_PART_MAIN);

  // 4. Record Button (Prominent Red Toggle)
  g_recorder.record_btn = lv_button_create(main_cont);
  lv_obj_set_size(g_recorder.record_btn, 80, 80);
  lv_obj_set_style_radius(g_recorder.record_btn, LV_RADIUS_CIRCLE,
                          LV_PART_MAIN);
  lv_obj_set_style_bg_color(g_recorder.record_btn, lv_color_hex(0xff3b30),
                            LV_PART_MAIN);
  lv_obj_add_event_cb(g_recorder.record_btn, record_toggle_cb, LV_EVENT_CLICKED,
                      NULL);
  lv_obj_set_style_shadow_width(g_recorder.record_btn, 0, LV_PART_MAIN);

  g_recorder.record_btn_lbl = lv_label_create(g_recorder.record_btn);
  lv_label_set_text(g_recorder.record_btn_lbl, SYMBOL_MIC);
  lv_obj_center(g_recorder.record_btn_lbl);
  lv_obj_set_style_text_font(g_recorder.record_btn_lbl, &lv_font_montserrat_24,
                             LV_PART_MAIN);
}
