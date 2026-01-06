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
#include "tkl_output.h"
#include "tuya_cloud_types.h"
#include <string.h>

#define TAG "practicepod"

// Forward declaration
static void create_welcome_screen(void);

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

  // 3. Create our UI (no locking needed before lv_vendor_start)
  create_welcome_screen();
  PR_NOTICE("UI Created");

  // 4. Start the LVGL background task
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
 * @brief Create the PracticePod welcome screen with LVGL
 */
static void create_welcome_screen(void) {
  // Get the active screen
  lv_obj_t *scr = lv_screen_active();

  // Set background color - let's use a visible color first (dark blue)
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1a2e), LV_PART_MAIN);

  // Create a container for centering content
  lv_obj_t *container = lv_obj_create(scr);
  lv_obj_set_size(container, LV_PCT(90), LV_PCT(90));
  lv_obj_center(container);
  lv_obj_set_style_bg_color(container, lv_color_hex(0x16213e), LV_PART_MAIN);
  lv_obj_set_style_border_color(container, lv_color_hex(0x0f3460),
                                LV_PART_MAIN);
  lv_obj_set_style_border_width(container, 2, LV_PART_MAIN);
  lv_obj_set_style_radius(container, 10, LV_PART_MAIN);
  lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  // Title: "PracticePod"
  lv_obj_t *title = lv_label_create(container);
  lv_label_set_text(title, "PracticePod");
  lv_obj_set_style_text_color(title, lv_color_hex(0xe94560), LV_PART_MAIN);

  // Main message
  lv_obj_t *msg = lv_label_create(container);
  lv_label_set_text(msg, "Hello! I'm PracticePod.\nReady to practice?");
  lv_obj_set_style_text_color(msg, lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
}
