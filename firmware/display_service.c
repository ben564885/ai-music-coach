/**
 * Display Service Implementation for T5AI Music Coach
 * Uses LVGL for graphical rendering
 */

#include "display_service.h"
#include "fingering_charts.h"
#include "tuya_config.h"

#include "lvgl.h"
#include "tal_log.h"
#include "tal_memory.h"
#include "tal_system.h"
#include "tdd_display.h"
#include "tkl_gpio.h"

#include <stdio.h>
#include <string.h>

#define TAG "DISPLAY"

// LVGL display buffer
#define LVGL_BUFFER_SIZE (DISPLAY_WIDTH * 40)

// Display state
static bool s_initialized = false;
static instrument_type_t s_current_instrument = INSTRUMENT_PIANO;
static lv_disp_drv_t s_disp_drv;
static lv_disp_draw_buf_t s_draw_buf;
static lv_color_t *s_buf1 = NULL;
static lv_color_t *s_buf2 = NULL;

// LVGL objects
static lv_obj_t *s_screen = NULL;
static lv_obj_t *s_note_label = NULL;
static lv_obj_t *s_freq_label = NULL;
static lv_obj_t *s_status_label = NULL;
static lv_obj_t *s_fingering_area = NULL;
static lv_obj_t *s_message_box = NULL;

// Colors
static const lv_color_t COLOR_CORRECT =
    LV_COLOR_MAKE(0x00, 0xC8, 0x53);                                   // Green
static const lv_color_t COLOR_WRONG = LV_COLOR_MAKE(0xFF, 0x3B, 0x30); // Red
static const lv_color_t COLOR_NEUTRAL = LV_COLOR_MAKE(0x00, 0x7A, 0xFF); // Blue
static const lv_color_t COLOR_BG = LV_COLOR_MAKE(0x1A, 0x1A, 0x2E); // Dark blue
static const lv_color_t COLOR_TEXT = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF); // White

/**
 * LVGL display flush callback
 */
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area,
                          lv_color_t *color_p) {
  int32_t x, y;
  int32_t w = area->x2 - area->x1 + 1;
  int32_t h = area->y2 - area->y1 + 1;

  // Flush to display driver
  tdd_display_set_window(area->x1, area->y1, area->x2, area->y2);
  tdd_display_write_data((uint8_t *)color_p, w * h * sizeof(lv_color_t));

  lv_disp_flush_ready(drv);
}

/**
 * Create the main UI
 */
static void create_ui(void) {
  // Create main screen with dark background
  s_screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(s_screen, COLOR_BG, 0);
  lv_scr_load(s_screen);

  // Note name label (large, centered)
  s_note_label = lv_label_create(s_screen);
  lv_obj_set_style_text_font(s_note_label, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(s_note_label, COLOR_NEUTRAL, 0);
  lv_obj_align(s_note_label, LV_ALIGN_TOP_MID, 0, 30);
  lv_label_set_text(s_note_label, "--");

  // Frequency label
  s_freq_label = lv_label_create(s_screen);
  lv_obj_set_style_text_font(s_freq_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(s_freq_label, COLOR_TEXT, 0);
  lv_obj_align(s_freq_label, LV_ALIGN_TOP_MID, 0, 90);
  lv_label_set_text(s_freq_label, "-- Hz");

  // Status label (connection, recording)
  s_status_label = lv_label_create(s_screen);
  lv_obj_set_style_text_font(s_status_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_status_label, COLOR_TEXT, 0);
  lv_obj_align(s_status_label, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_label_set_text(s_status_label, "Ready");

  // Fingering area (middle of screen)
  s_fingering_area = lv_obj_create(s_screen);
  lv_obj_set_size(s_fingering_area, DISPLAY_WIDTH - 20, 140);
  lv_obj_align(s_fingering_area, LV_ALIGN_CENTER, 0, 20);
  lv_obj_set_style_bg_color(s_fingering_area, lv_color_darken(COLOR_BG, 30), 0);
  lv_obj_set_style_border_width(s_fingering_area, 1, 0);
  lv_obj_set_style_border_color(s_fingering_area, COLOR_NEUTRAL, 0);
  lv_obj_set_style_radius(s_fingering_area, 10, 0);
}

int display_service_init(void) {
#if !DISPLAY_ENABLE
  TAL_LOGI(TAG, "Display disabled in config");
  return 0;
#endif

  TAL_LOGI(TAG, "Initializing display service");

  // Initialize display hardware
  tdd_display_config_t disp_cfg = {0};
  disp_cfg.type = TDD_DISPLAY_TYPE_SPI;
  disp_cfg.width = DISPLAY_WIDTH;
  disp_cfg.height = DISPLAY_HEIGHT;
  disp_cfg.spi_cfg.mosi_pin = DISPLAY_SPI_MOSI;
  disp_cfg.spi_cfg.clk_pin = DISPLAY_SPI_CLK;
  disp_cfg.spi_cfg.cs_pin = DISPLAY_SPI_CS;
  disp_cfg.spi_cfg.dc_pin = DISPLAY_DC_PIN;
  disp_cfg.spi_cfg.rst_pin = DISPLAY_RST_PIN;
  disp_cfg.spi_cfg.freq_hz = DISPLAY_SPI_FREQ_HZ;

  OPERATE_RET ret = tdd_display_init(&disp_cfg);
  if (ret != OPRT_OK) {
    TAL_LOGE(TAG, "tdd_display_init failed: %d", ret);
    return -1;
  }

  // Initialize backlight
  tkl_gpio_init(DISPLAY_BL_PIN,
                &(TUYA_GPIO_BASE_CFG_T){.mode = TUYA_GPIO_PUSH_PULL,
                                        .direct = TUYA_GPIO_OUTPUT,
                                        .level = TUYA_GPIO_LEVEL_HIGH});

  // Initialize LVGL
  lv_init();

  // Allocate display buffers
  s_buf1 = (lv_color_t *)tal_malloc(LVGL_BUFFER_SIZE * sizeof(lv_color_t));
  s_buf2 = (lv_color_t *)tal_malloc(LVGL_BUFFER_SIZE * sizeof(lv_color_t));
  if (!s_buf1 || !s_buf2) {
    TAL_LOGE(TAG, "Failed to allocate LVGL buffers");
    return -1;
  }

  // Initialize draw buffer
  lv_disp_draw_buf_init(&s_draw_buf, s_buf1, s_buf2, LVGL_BUFFER_SIZE);

  // Initialize display driver
  lv_disp_drv_init(&s_disp_drv);
  s_disp_drv.hor_res = DISPLAY_WIDTH;
  s_disp_drv.ver_res = DISPLAY_HEIGHT;
  s_disp_drv.flush_cb = lvgl_flush_cb;
  s_disp_drv.draw_buf = &s_draw_buf;
  lv_disp_drv_register(&s_disp_drv);

  // Create UI
  create_ui();

  s_initialized = true;
  TAL_LOGI(TAG, "Display service initialized");
  return 0;
}

void display_service_deinit(void) {
  if (s_buf1) {
    tal_free(s_buf1);
    s_buf1 = NULL;
  }
  if (s_buf2) {
    tal_free(s_buf2);
    s_buf2 = NULL;
  }

  tdd_display_deinit();
  s_initialized = false;
}

void display_service_set_instrument(instrument_type_t instrument) {
  if (instrument < INSTRUMENT_COUNT) {
    s_current_instrument = instrument;
    TAL_LOGI(TAG, "Instrument set to: %d", instrument);
  }
}

void display_service_show_note(const display_note_info_t *note) {
  if (!s_initialized || !note)
    return;

  // Update note label
  char note_str[16];
  snprintf(note_str, sizeof(note_str), "%s%d", note->note_name, note->octave);
  lv_label_set_text(s_note_label, note_str);

  // Set color based on correctness
  lv_color_t color = note->is_correct ? COLOR_CORRECT : COLOR_WRONG;
  lv_obj_set_style_text_color(s_note_label, color, 0);

  // Update frequency label
  char freq_str[32];
  snprintf(freq_str, sizeof(freq_str), "%.1f Hz", note->frequency);
  lv_label_set_text(s_freq_label, freq_str);
}

void display_service_show_wrong_note(const display_note_info_t *played,
                                     const display_note_info_t *expected) {
  if (!s_initialized || !played || !expected)
    return;

  // Show played note in red
  char note_str[32];
  snprintf(note_str, sizeof(note_str), "%s%d -> %s%d", played->note_name,
           played->octave, expected->note_name, expected->octave);
  lv_label_set_text(s_note_label, note_str);
  lv_obj_set_style_text_color(s_note_label, COLOR_WRONG, 0);

  // Clear fingering area and show expected fingering
  lv_obj_clean(s_fingering_area);

  lv_obj_t *title = lv_label_create(s_fingering_area);
  lv_label_set_text(title, "Correct fingering:");
  lv_obj_set_style_text_color(title, COLOR_TEXT, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

  // Get fingering chart
  const FingeringChart *chart =
      FingeringCharts_get_fingering(expected->note_name, expected->octave,
                                    (InstrumentType)s_current_instrument);

  if (chart && chart->fingering_diagram) {
    lv_obj_t *fingering = lv_label_create(s_fingering_area);
    lv_label_set_text(fingering, chart->fingering_diagram);
    lv_obj_set_style_text_font(fingering, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(fingering, COLOR_TEXT, 0);
    lv_obj_align(fingering, LV_ALIGN_CENTER, 0, 10);
  }
}

void display_service_show_fingering(const char *note_name, uint8_t octave) {
  if (!s_initialized || !note_name)
    return;

  lv_obj_clean(s_fingering_area);

  const FingeringChart *chart = FingeringCharts_get_fingering(
      note_name, octave, (InstrumentType)s_current_instrument);

  if (chart) {
    lv_obj_t *title = lv_label_create(s_fingering_area);
    char title_str[32];
    snprintf(title_str, sizeof(title_str), "%s%d", note_name, octave);
    lv_label_set_text(title, title_str);
    lv_obj_set_style_text_color(title, COLOR_NEUTRAL, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    if (chart->fingering_diagram) {
      lv_obj_t *diagram = lv_label_create(s_fingering_area);
      lv_label_set_text(diagram, chart->fingering_diagram);
      lv_obj_set_style_text_font(diagram, &lv_font_montserrat_12, 0);
      lv_obj_set_style_text_color(diagram, COLOR_TEXT, 0);
      lv_obj_align(diagram, LV_ALIGN_CENTER, 0, 0);
    }

    if (chart->fingering_text) {
      lv_obj_t *hint = lv_label_create(s_fingering_area);
      lv_label_set_text(hint, chart->fingering_text);
      lv_obj_set_style_text_font(hint, &lv_font_montserrat_10, 0);
      lv_obj_set_style_text_color(hint, lv_color_lighten(COLOR_BG, 100), 0);
      lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -5);
    }
  }
}

void display_service_show_recording(bool is_recording, uint32_t duration_sec) {
  if (!s_initialized)
    return;

  char status_str[64];
  if (is_recording) {
    uint32_t min = duration_sec / 60;
    uint32_t sec = duration_sec % 60;
    snprintf(status_str, sizeof(status_str), "Recording %02lu:%02lu",
             (unsigned long)min, (unsigned long)sec);
    lv_obj_set_style_text_color(s_status_label, COLOR_WRONG, 0);
  } else {
    snprintf(status_str, sizeof(status_str), "Ready");
    lv_obj_set_style_text_color(s_status_label, COLOR_CORRECT, 0);
  }

  lv_label_set_text(s_status_label, status_str);
}

void display_service_show_connection(bool wifi_connected, bool ble_connected) {
  if (!s_initialized)
    return;

  char status_str[64];
  if (wifi_connected && ble_connected) {
    snprintf(status_str, sizeof(status_str), "WiFi ✓  BLE ✓");
    lv_obj_set_style_text_color(s_status_label, COLOR_CORRECT, 0);
  } else if (wifi_connected) {
    snprintf(status_str, sizeof(status_str), "WiFi ✓  BLE ✗");
    lv_obj_set_style_text_color(
        s_status_label, lv_color_mix(COLOR_CORRECT, COLOR_WRONG, 128), 0);
  } else if (ble_connected) {
    snprintf(status_str, sizeof(status_str), "WiFi ✗  BLE ✓");
    lv_obj_set_style_text_color(
        s_status_label, lv_color_mix(COLOR_CORRECT, COLOR_WRONG, 128), 0);
  } else {
    snprintf(status_str, sizeof(status_str), "Disconnected");
    lv_obj_set_style_text_color(s_status_label, COLOR_WRONG, 0);
  }

  lv_label_set_text(s_status_label, status_str);
}

void display_service_show_idle(void) {
  if (!s_initialized)
    return;

  lv_label_set_text(s_note_label, "--");
  lv_obj_set_style_text_color(s_note_label, COLOR_NEUTRAL, 0);
  lv_label_set_text(s_freq_label, "-- Hz");
  lv_obj_clean(s_fingering_area);

  lv_obj_t *hint = lv_label_create(s_fingering_area);
  lv_label_set_text(hint, "Press button to record\nor connect mobile app");
  lv_obj_set_style_text_color(hint, COLOR_TEXT, 0);
  lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(hint, LV_ALIGN_CENTER, 0, 0);
}

void display_service_show_message(const char *title, const char *message) {
  if (!s_initialized)
    return;

  lv_label_set_text(s_note_label, title ? title : "");
  lv_obj_set_style_text_color(s_note_label, COLOR_NEUTRAL, 0);
  lv_label_set_text(s_freq_label, "");

  lv_obj_clean(s_fingering_area);
  if (message) {
    lv_obj_t *msg = lv_label_create(s_fingering_area);
    lv_label_set_text(msg, message);
    lv_obj_set_style_text_color(msg, COLOR_TEXT, 0);
    lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(msg, lv_obj_get_width(s_fingering_area) - 20);
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, 0);
  }
}

void display_service_clear(void) {
  if (!s_initialized)
    return;

  lv_label_set_text(s_note_label, "");
  lv_label_set_text(s_freq_label, "");
  lv_label_set_text(s_status_label, "");
  lv_obj_clean(s_fingering_area);
}

void display_service_update(void) {
  if (!s_initialized)
    return;

  // Handle LVGL timer
  lv_timer_handler();
}

void display_service_set_brightness(uint8_t brightness) {
  // Simple on/off via GPIO for now
  // For PWM brightness, use tkl_pwm APIs
  if (brightness > 0) {
    tkl_gpio_write(DISPLAY_BL_PIN, TUYA_GPIO_LEVEL_HIGH);
  } else {
    tkl_gpio_write(DISPLAY_BL_PIN, TUYA_GPIO_LEVEL_LOW);
  }
}
