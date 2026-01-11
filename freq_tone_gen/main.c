/**
 * @file main.c
 * @brief Tone Generator - Uses onboard speaker to play frequency and display it
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
#include "tkl_output.h"
#include "tuya_cloud_types.h"
#include <math.h>
#include <string.h>

#define TAG "tonegen"

#define SAMPLE_RATE 16000
#define DEFAULT_FREQUENCY 440  // A4 note
#define PI 3.14159265358979323846

// Audio frame size (16ms at 16kHz = 256 samples) - smaller for better timing
#define AUDIO_FRAME_SIZE (SAMPLE_RATE / 62)  // ~256 samples

typedef struct {
  uint32_t frequency;
  BOOL_T is_playing;
  float phase;
  uint32_t volume;  // Volume level (0-100)

  lv_obj_t *freq_label;
  lv_obj_t *status_label;
  lv_obj_t *volume_slider;
  lv_obj_t *volume_label;
  lv_timer_t *ui_timer;

  THREAD_HANDLE audio_thread;
  VOID *audio_handle;  // Handle from tkl_ao_init
} tone_gen_mgr_t;

static tone_gen_mgr_t g_tone_gen;
static int16_t pcm_buffer[AUDIO_FRAME_SIZE];

// Forward declarations
static void create_tone_ui(void);
static void audio_output_task(void *arg);

/**
 * @brief Generate sine wave samples
 */
static void generate_sine_wave(int16_t *buffer, int samples, uint32_t freq,
                                float *phase_ptr) {
  float phase = *phase_ptr;
  float phase_inc = 2.0f * PI * freq / SAMPLE_RATE;
  const float amplitude = 30000.0f; // Max amplitude for 16-bit audio (leave headroom)

  // Verify parameters
  if (freq == 0 || samples == 0) {
    memset(buffer, 0, samples * sizeof(int16_t));
    return;
  }

  for (int i = 0; i < samples; i++) {
    buffer[i] = (int16_t)(amplitude * sinf(phase));
    phase += phase_inc;
    // Keep phase in [0, 2*PI) range
    if (phase >= 2.0f * PI) {
      phase -= 2.0f * PI;
    }
  }

  *phase_ptr = phase;
}

/**
 * @brief Initialize audio output
 */
static OPERATE_RET app_audio_output_init(void) {
  TKL_AUDIO_CONFIG_T config = {0};

  // Configure audio system (based on working speaker example)
  config.enable = true;
  config.card = TKL_AUDIO_TYPE_BOARD;
  config.ai_chn = TKL_AI_1;  // Use AI_1 like in the working example
  config.sample = SAMPLE_RATE;
  config.datebits = TKL_AUDIO_DATABITS_16;
  config.channel = TKL_AUDIO_CHANNEL_MONO;
  config.codectype = TKL_CODEC_AUDIO_PCM;
  config.put_cb = NULL;  // No callback needed for output

  // Speaker-specific configuration (crucial for audio output!)
  config.fps = 25;  // frames per second, suggest 25
  config.mic_volume = 0x2d;  // microphone volume
  config.spk_volume = 0x5d;  // speaker volume (93 in decimal - much louder)
  config.spk_gpio_polarity = TUYA_GPIO_LEVEL_LOW;  // GPIO polarity (LOW = active)
  config.spk_sample = SAMPLE_RATE;  // speaker sample rate
  config.spk_gpio = TUYA_GPIO_NUM_28;  // Speaker enable GPIO pin (from TUYA_T5AI_BOARD config)

  // Initialize audio system
  OPERATE_RET rt = tkl_ai_init(&config, 1);
  if (rt != OPRT_OK) {
    PR_ERR("Audio init failed: %d", rt);
    return rt;
  }

  // Start audio system
  rt = tkl_ai_start(TKL_AUDIO_TYPE_BOARD, TKL_AI_1);
  if (rt != OPRT_OK) {
    PR_ERR("Audio start failed: %d", rt);
    return rt;
  }

  // Set additional volume controls
  rt = tkl_ai_set_vol(TKL_AUDIO_TYPE_BOARD, TKL_AI_1, 80);
  if (rt != OPRT_OK) {
    PR_WARN("Audio input volume failed: %d", rt);
  }

  rt = tkl_ao_set_vol(TKL_AUDIO_TYPE_BOARD, (TKL_AO_CHN_E)TKL_AI_1, NULL, 90);
  if (rt != OPRT_OK) {
    PR_WARN("Audio output volume failed: %d", rt);
  }

  PR_NOTICE("Audio output initialized successfully");
  return OPRT_OK;
}

/**
 * @brief Audio output thread - continuously generates and plays tone with precise timing
 */
static void audio_output_task(void *arg) {
  (void)arg;

  PR_NOTICE("Audio output task started");

  // Audio timing constants - adjusted for 256 sample frames (~16ms)
  const uint32_t FRAME_DURATION_MS = 16;  // ~16ms per frame (~62.5 FPS)
  const uint32_t FRAME_DURATION_TICKS = FRAME_DURATION_MS;  // Assuming 1 tick = 1ms
  const uint32_t TIMING_TOLERANCE_MS = 4;  // Allow 4ms tolerance for timing variations

  uint32_t next_frame_time = tal_system_get_tick_count();

  while (1) {
    if (g_tone_gen.is_playing) {
      uint32_t current_time = tal_system_get_tick_count();

      // Check if it's time to send the next frame (with tolerance)
      int32_t time_diff = (int32_t)(current_time - next_frame_time);

      if (time_diff >= -(int32_t)TIMING_TOLERANCE_MS) {  // Allow some earliness
        // Generate sine wave
        generate_sine_wave(pcm_buffer, AUDIO_FRAME_SIZE,
                          g_tone_gen.frequency,
                          &g_tone_gen.phase);

        // Create audio frame
        TKL_AUDIO_FRAME_INFO_T frame = {0};
        frame.pbuf = (CHAR_T *)pcm_buffer;
        frame.buf_size = sizeof(pcm_buffer);
        frame.used_size = sizeof(pcm_buffer);
        frame.pts = current_time;  // Use current timestamp

        // Send to audio output - use same card/channel as initialization
        OPERATE_RET rt = tkl_ao_put_frame(TKL_AUDIO_TYPE_BOARD, (TKL_AO_CHN_E)TKL_AI_1, NULL, &frame);
        if (rt != OPRT_OK) {
          PR_WARN("Failed to put audio frame: %d (time_diff: %d)", rt, time_diff);
          // Don't increment frame counter on failure to avoid timing drift
        } else {
          // Calculate next frame time precisely
          next_frame_time += FRAME_DURATION_TICKS;

          // Handle timing wraparound or if we fell behind
          if (next_frame_time < current_time) {
            // We've fallen behind, reset timing
            next_frame_time = current_time + FRAME_DURATION_TICKS;
            PR_WARN("Audio timing reset - fell behind by %d ms", time_diff);
          }

          // Only log occasionally to avoid spam
          static int frame_count = 0;
          if (++frame_count % 100 == 0) {  // Log every 100 frames (every 2 seconds)
            PR_DEBUG("Audio frames sent: %d, frequency: %d Hz", frame_count, g_tone_gen.frequency);
          }
        }
      } else {
        // Not time for next frame yet, sleep briefly
        tal_system_sleep(2);  // Sleep a bit longer to reduce CPU usage
      }
    } else {
      // When not playing, sleep longer to save CPU and reset timing
      next_frame_time = tal_system_get_tick_count() + FRAME_DURATION_TICKS;
      tal_system_sleep(100);  // Longer sleep when not playing
    }
  }
}

/**
 * @brief UI timer callback - updates display
 */
static void ui_timer_cb(lv_timer_t *timer) {
  (void)timer;

  // Update frequency display
  lv_label_set_text_fmt(g_tone_gen.freq_label, "%d Hz", g_tone_gen.frequency);

  // Update status
  if (g_tone_gen.is_playing) {
    lv_label_set_text(g_tone_gen.status_label, "Playing");
    lv_obj_set_style_text_color(g_tone_gen.status_label,
                                lv_color_hex(0x00ff00), LV_PART_MAIN);
  } else {
    lv_label_set_text(g_tone_gen.status_label, "Stopped");
    lv_obj_set_style_text_color(g_tone_gen.status_label,
                                lv_color_hex(0x888888), LV_PART_MAIN);
  }

  // Update volume display (in case it was changed externally)
  lv_label_set_text_fmt(g_tone_gen.volume_label, "Vol: %d", g_tone_gen.volume);
  lv_slider_set_value(g_tone_gen.volume_slider, g_tone_gen.volume, LV_ANIM_OFF);
}

/**
 * @brief Toggle play/stop button callback
 */
static void play_toggle_cb(lv_event_t *e) {
  (void)e;

  g_tone_gen.is_playing = !g_tone_gen.is_playing;

  if (g_tone_gen.is_playing) {
    // Clear audio buffer and reset phase for clean start
    tkl_ao_clear_buffer(TKL_AUDIO_TYPE_BOARD, (TKL_AO_CHN_E)TKL_AI_1);
    g_tone_gen.phase = 0.0f; // Reset phase for clean start
    PR_NOTICE("Tone started: %d Hz", g_tone_gen.frequency);
  } else {
    // Clear audio buffer when stopping to prevent crackling
    tkl_ao_clear_buffer(TKL_AUDIO_TYPE_BOARD, (TKL_AO_CHN_E)TKL_AI_1);
    PR_NOTICE("Tone stopped");
  }
  PR_NOTICE("Play button pressed, is_playing: %d", g_tone_gen.is_playing);
}

/**
 * @brief Increase frequency button callback
 */
static void freq_up_cb(lv_event_t *e) {
  (void)e;

  // Increase by 10 Hz
  g_tone_gen.frequency += 10;
  if (g_tone_gen.frequency > 20000) {
    g_tone_gen.frequency = 20000; // Max audible frequency
  }
  PR_NOTICE("Frequency: %d Hz", g_tone_gen.frequency);
}

/**
 * @brief Decrease frequency button callback
 */
static void freq_down_cb(lv_event_t *e) {
  (void)e;

  // Decrease by 10 Hz
  if (g_tone_gen.frequency >= 10) {
    g_tone_gen.frequency -= 10;
  } else {
    g_tone_gen.frequency = 10; // Minimum frequency
  }
  PR_NOTICE("Frequency: %d Hz", g_tone_gen.frequency);
}

/**
 * @brief Volume slider callback
 */
static void volume_slider_cb(lv_event_t *e) {
  (void)e;

  // Get slider value (0-100)
  g_tone_gen.volume = lv_slider_get_value(g_tone_gen.volume_slider);

  // Update volume immediately
  OPERATE_RET rt = tkl_ao_set_vol(TKL_AUDIO_TYPE_BOARD, (TKL_AO_CHN_E)TKL_AI_1, NULL, g_tone_gen.volume);
  if (rt != OPRT_OK) {
    PR_WARN("Failed to set volume to %d: %d", g_tone_gen.volume, rt);
  } else {
    PR_DEBUG("Volume set to %d", g_tone_gen.volume);
  }

  // Update volume label
  lv_label_set_text_fmt(g_tone_gen.volume_label, "Vol: %d", g_tone_gen.volume);
}

/**
 * @brief Create the tone generator UI
 */
static void create_tone_ui(void) {
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
  
  // Title
  lv_obj_t *title = lv_label_create(main_cont);
  lv_label_set_text(title, "Tone Generator");
  lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
  
  // Frequency display (large)
  g_tone_gen.freq_label = lv_label_create(main_cont);
  lv_label_set_text_fmt(g_tone_gen.freq_label, "%d Hz", g_tone_gen.frequency);
  lv_obj_set_width(g_tone_gen.freq_label, 200);
  lv_obj_set_style_text_align(g_tone_gen.freq_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(g_tone_gen.freq_label, lv_color_hex(0x00ff00), 
                              LV_PART_MAIN);
  lv_obj_set_style_text_font(g_tone_gen.freq_label, &lv_font_montserrat_24, 
                             LV_PART_MAIN);
  
  // Status label
  g_tone_gen.status_label = lv_label_create(main_cont);
  lv_label_set_text(g_tone_gen.status_label, "Stopped");
  lv_obj_set_style_text_color(g_tone_gen.status_label, lv_color_hex(0x888888), 
                              LV_PART_MAIN);
  lv_obj_set_style_text_font(g_tone_gen.status_label, &lv_font_montserrat_16, 
                             LV_PART_MAIN);
  
  // Frequency control buttons container
  lv_obj_t *freq_cont = lv_obj_create(main_cont);
  lv_obj_set_size(freq_cont, LV_PCT(80), 60);
  lv_obj_set_style_bg_opa(freq_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(freq_cont, 0, LV_PART_MAIN);
  lv_obj_set_flex_flow(freq_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(freq_cont, LV_FLEX_ALIGN_CENTER, 
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(freq_cont, 20, LV_PART_MAIN);
  
  // Down button
  lv_obj_t *down_btn = lv_button_create(freq_cont);
  lv_obj_set_size(down_btn, 60, 50);
  lv_obj_set_style_bg_color(down_btn, lv_color_hex(0x333333), LV_PART_MAIN);
  lv_obj_add_event_cb(down_btn, freq_down_cb, LV_EVENT_CLICKED, NULL);
  
  lv_obj_t *down_lbl = lv_label_create(down_btn);
  lv_label_set_text(down_lbl, "-10 Hz");
  lv_obj_center(down_lbl);
  lv_obj_set_style_text_color(down_lbl, lv_color_hex(0xffffff), LV_PART_MAIN);
  
  // Up button
  lv_obj_t *up_btn = lv_button_create(freq_cont);
  lv_obj_set_size(up_btn, 60, 50);
  lv_obj_set_style_bg_color(up_btn, lv_color_hex(0x333333), LV_PART_MAIN);
  lv_obj_add_event_cb(up_btn, freq_up_cb, LV_EVENT_CLICKED, NULL);
  
  lv_obj_t *up_lbl = lv_label_create(up_btn);
  lv_label_set_text(up_lbl, "+10 Hz");
  lv_obj_center(up_lbl);
  lv_obj_set_style_text_color(up_lbl, lv_color_hex(0xffffff), LV_PART_MAIN);
  
  // Play/Stop button (prominent)
  lv_obj_t *play_btn = lv_button_create(main_cont);
  lv_obj_set_size(play_btn, 100, 100);
  lv_obj_set_style_radius(play_btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);
  lv_obj_set_style_bg_color(play_btn, lv_color_hex(0x007aff), LV_PART_MAIN);
  lv_obj_add_event_cb(play_btn, play_toggle_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_set_style_shadow_width(play_btn, 0, LV_PART_MAIN);

  lv_obj_t *play_lbl = lv_label_create(play_btn);
  lv_label_set_text(play_lbl, LV_SYMBOL_PLAY);
  lv_obj_center(play_lbl);
  lv_obj_set_style_text_font(play_lbl, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(play_lbl, lv_color_hex(0xffffff), LV_PART_MAIN);

  // Volume control section
  lv_obj_t *volume_cont = lv_obj_create(main_cont);
  lv_obj_set_size(volume_cont, LV_PCT(90), 60);
  lv_obj_set_style_bg_opa(volume_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(volume_cont, 0, LV_PART_MAIN);
  lv_obj_set_flex_flow(volume_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(volume_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(volume_cont, 5, LV_PART_MAIN);

  // Volume label
  g_tone_gen.volume_label = lv_label_create(volume_cont);
  lv_label_set_text_fmt(g_tone_gen.volume_label, "Vol: %d", g_tone_gen.volume);
  lv_obj_set_style_text_color(g_tone_gen.volume_label, lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_style_text_font(g_tone_gen.volume_label, &lv_font_montserrat_16, LV_PART_MAIN);

  // Volume slider
  g_tone_gen.volume_slider = lv_slider_create(volume_cont);
  lv_obj_set_size(g_tone_gen.volume_slider, LV_PCT(100), 25);
  lv_slider_set_range(g_tone_gen.volume_slider, 0, 100);
  lv_slider_set_value(g_tone_gen.volume_slider, g_tone_gen.volume, LV_ANIM_OFF);
  lv_obj_add_event_cb(g_tone_gen.volume_slider, volume_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

  // Style the slider track
  lv_obj_set_style_bg_color(g_tone_gen.volume_slider, lv_color_hex(0x444444), LV_PART_MAIN);
  lv_obj_set_style_bg_color(g_tone_gen.volume_slider, lv_color_hex(0x00ff00), LV_PART_INDICATOR);

  // Style the knob
  lv_obj_set_style_bg_color(g_tone_gen.volume_slider, lv_color_hex(0xffffff), LV_PART_KNOB);
  lv_obj_set_style_border_width(g_tone_gen.volume_slider, 2, LV_PART_KNOB);
  lv_obj_set_style_border_color(g_tone_gen.volume_slider, lv_color_hex(0xcccccc), LV_PART_KNOB);
}

/**
 * @brief user_main - Application logic entry point
 */
void user_main(void) {
  tal_log_init(TAL_LOG_LEVEL_DEBUG, 4096, (TAL_LOG_OUTPUT_CB)tkl_log_output);
  
  PR_NOTICE("========================================");
  PR_NOTICE("=== Tone Generator Starting ===");
  PR_NOTICE("Firmware version: 1.0.0");
  PR_NOTICE("========================================");
  
  // Initialize tone generator manager
  memset(&g_tone_gen, 0, sizeof(g_tone_gen));
  g_tone_gen.frequency = DEFAULT_FREQUENCY;
  g_tone_gen.is_playing = FALSE;
  g_tone_gen.phase = 0.0f;
  g_tone_gen.volume = 90;  // Default volume (0-100)
  
  // 1. Initialize Board Hardware
  PR_NOTICE("Registering Board Hardware...");
  int ret = board_register_hardware();
  if (ret != OPRT_OK) {
    PR_ERR("board_register_hardware failed: %d", ret);
  } else {
    PR_NOTICE("board_register_hardware success");
  }
  
  // 2. Initialize LVGL via Vendor API
#ifdef DISPLAY_NAME
  lv_vendor_init(DISPLAY_NAME);
#else
  lv_vendor_init("display");
#endif
  PR_NOTICE("LVGL Vendor Init complete");
  
  // 3. Create UI
  create_tone_ui();
  PR_NOTICE("UI Created");
  
  // 4. Initialize Audio Output
  ret = app_audio_output_init();
  if (ret != OPRT_OK) {
    PR_ERR("Audio output initialization failed: %d", ret);
  }
  
  // 5. Create Audio Output Thread
  THREAD_CFG_T thrd_param = {2048, 5, "audio_output"};
  tal_thread_create_and_start(&g_tone_gen.audio_thread, NULL, NULL, 
                              audio_output_task, NULL, &thrd_param);
  PR_NOTICE("Audio output thread created");
  
  // 6. Setup UI Timer (update display every 100ms)
  g_tone_gen.ui_timer = lv_timer_create(ui_timer_cb, 100, NULL);
  
  // 7. Start the LVGL background task
  lv_vendor_start(5, 1024 * 8);
  PR_NOTICE("LVGL Task Started");
  
  // Main application loop
  int counter = 0;
  while (1) {
    tal_system_sleep(1000);
    counter++;
    if (counter % 10 == 0) {
      PR_DEBUG("Tone Generator alive, uptime=%ds", counter);
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
