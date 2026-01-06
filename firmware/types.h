/**
 * Common type definitions for T5AI Music Coach
 * Updated for TuyaOpen SDK compatibility
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Instrument types
typedef enum {
  INST_PIANO = 0,
  INST_VIOLIN = 1,
  INST_GUITAR = 2,
  INST_FLUTE = 3,
  INST_CLARINET = 4,
  INST_TRUMPET = 5,
  INST_SAXOPHONE = 6,
  INST_COUNT
} instrument_type_enum_t;

// Audio configuration
typedef struct {
  uint32_t sample_rate;
  uint8_t bit_depth;
  uint8_t channels;
  uint32_t buffer_size;
} firmware_audio_config_t;

// Audio chunk
typedef struct {
  int16_t *data;
  size_t size;
  uint32_t timestamp_ms;
} firmware_audio_chunk_t;

// Analysis result from cloud
typedef struct {
  uint32_t mistake_count;
  char *feedback_text;
  struct {
    char type[32];
    float timestamp;
    char description[128];
  } mistakes[100];
} firmware_analysis_result_t;

// Music data (sheet music)
typedef struct {
  char title[128];
  char *reference_data_json;
  size_t data_size;
} firmware_music_data_t;

#ifdef __cplusplus
}
#endif

#endif // TYPES_H
