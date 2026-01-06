/**
 * Fingering Charts Implementation for T5AI Music Coach
 * Provides lookup for instrument-specific fingerings
 */

#ifndef FINGERING_CHARTS_H
#define FINGERING_CHARTS_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Matching enum from display_service.h or types.h
typedef enum {
  CHART_PIANO = 0,
  CHART_VIOLIN,
  CHART_GUITAR,
  CHART_FLUTE,
  CHART_CLARINET,
  CHART_TRUMPET,
  CHART_SAXOPHONE
} fingering_instrument_t;

// Fingering chart data structure
typedef struct {
  const char *note_name;
  uint8_t octave;
  uint8_t instrument;
  const char *fingering_text;    // Text description
  const char *fingering_diagram; // ASCII/visual diagram
  const char *hand_position;     // Hand/finger position
} FingeringChart;

/**
 * Get fingering for a note on an instrument
 * @param note_name Note name (e.g., "A", "Bb")
 * @param octave Octave (e.g., 4)
 * @param instrument Instrument type
 * @return Pointer to chart data or NULL if not found
 */
const FingeringChart *FingeringCharts_get_fingering(const char *note_name,
                                                    uint8_t octave,
                                                    uint8_t instrument);

#ifdef __cplusplus
}
#endif

#endif // FINGERING_CHARTS_H
