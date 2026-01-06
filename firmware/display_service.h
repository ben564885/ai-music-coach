/**
 * Display Service for T5AI Music Coach
 * Uses LVGL for graphical rendering on SPI display
 */

#ifndef DISPLAY_SERVICE_H
#define DISPLAY_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * Instrument types for fingering charts
 */
typedef enum {
  INSTRUMENT_PIANO = 0,
  INSTRUMENT_VIOLIN,
  INSTRUMENT_GUITAR,
  INSTRUMENT_FLUTE,
  INSTRUMENT_CLARINET,
  INSTRUMENT_TRUMPET,
  INSTRUMENT_SAXOPHONE,
  INSTRUMENT_COUNT
} instrument_type_t;

/**
 * Note display info
 */
typedef struct {
  const char *note_name; // e.g., "A", "Bb", "C#"
  uint8_t octave;        // e.g., 4
  float frequency;       // Hz
  bool is_correct;       // Matches expected note
} display_note_info_t;

/**
 * Initialize display service
 * @return 0 on success, negative on error
 */
int display_service_init(void);

/**
 * Deinitialize display service
 */
void display_service_deinit(void);

/**
 * Set current instrument for fingering charts
 * @param instrument Instrument type
 */
void display_service_set_instrument(instrument_type_t instrument);

/**
 * Display current note being played
 * Shows note name, frequency, and correct/incorrect indicator
 * @param note Note information
 */
void display_service_show_note(const display_note_info_t *note);

/**
 * Display wrong note with expected correction
 * Shows played note, expected note, and fingering chart
 * @param played Played note
 * @param expected Expected note
 */
void display_service_show_wrong_note(const display_note_info_t *played,
                                     const display_note_info_t *expected);

/**
 * Display fingering chart for a note
 * @param note_name Note name
 * @param octave Octave
 */
void display_service_show_fingering(const char *note_name, uint8_t octave);

/**
 * Display recording status
 * @param is_recording true if recording
 * @param duration_sec Recording duration in seconds
 */
void display_service_show_recording(bool is_recording, uint32_t duration_sec);

/**
 * Display connection status
 * @param wifi_connected WiFi connection status
 * @param ble_connected BLE connection status
 */
void display_service_show_connection(bool wifi_connected, bool ble_connected);

/**
 * Display idle/ready screen
 */
void display_service_show_idle(void);

/**
 * Display message on screen
 * @param title Title text
 * @param message Message text
 */
void display_service_show_message(const char *title, const char *message);

/**
 * Clear display
 */
void display_service_clear(void);

/**
 * Update display (call periodically in main loop)
 * Handles LVGL tick and refresh
 */
void display_service_update(void);

/**
 * Set display brightness
 * @param brightness 0-100
 */
void display_service_set_brightness(uint8_t brightness);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_SERVICE_H
