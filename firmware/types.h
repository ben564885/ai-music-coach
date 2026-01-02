/**
 * Common type definitions for T5AI Music Coach
 */

#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <cstring>

// Instrument types
enum class InstrumentType {
    PIANO = 0,
    VIOLIN = 1,
    GUITAR = 2,
    FLUTE = 3,
    CLARINET = 4,
    TRUMPET = 5,
    SAXOPHONE = 6
};

// Audio configuration
struct audio_config_t {
    uint32_t sample_rate;
    uint8_t bit_depth;
    uint8_t channels;
    uint32_t buffer_size;
};

// Audio chunk
struct audio_chunk_t {
    int16_t* data;
    size_t size;
    uint32_t timestamp_ms;
};

// Music reference data (from sheet music)
struct music_reference_t {
    char title[128];
    uint32_t note_count;
    struct {
        char note_name[8];
        float frequency;
        uint32_t timestamp_ms;
        uint8_t measure;
        uint8_t beat;
    } notes[1000];
    uint32_t tempo_bpm;
    char key_signature[16];
};

// BLE message types
enum BLEMessageType {
    BLE_MSG_SHEET_MUSIC = 0,
    BLE_MSG_START_RECORDING = 1,
    BLE_MSG_STOP_RECORDING = 2,
    BLE_MSG_SET_INSTRUMENT = 3,
    BLE_MSG_SET_SCALE = 4
};

// BLE message
struct ble_message_t {
    BLEMessageType type;
    void* data;
    size_t data_size;
};

// Analysis result from cloud
struct analysis_result_t {
    uint32_t mistake_count;
    char* feedback_text;
    struct {
        char type[32];
        float timestamp;
        char description[128];
    } mistakes[100];
};

// Music data (sheet music)
struct music_data_t {
    char title[128];
    char* reference_data_json;
    size_t data_size;
};

// TTS voice types
enum VoiceType {
    VOICE_FRIENDLY_TEACHER = 0,
    VOICE_PROFESSIONAL = 1,
    VOICE_ENCOURAGING = 2
};

#endif // TYPES_H

