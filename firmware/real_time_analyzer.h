/**
 * Real-Time Audio Analyzer
 * Detects notes as they're being played (not just post-recording)
 */

#ifndef REAL_TIME_ANALYZER_H
#define REAL_TIME_ANALYZER_H

#include <cstdint>
#include <cstring>

// Audio buffer size for real-time analysis
#define RT_BUFFER_SIZE 2048
#define RT_SAMPLE_RATE 16000

// Note detection result
struct NoteDetection {
    char note_name[4];      // e.g., "A", "Bb"
    uint8_t octave;         // e.g., 4
    float frequency;        // Detected frequency in Hz
    float confidence;       // 0.0 to 1.0
    bool is_valid;          // Valid note detected
};

// Expected note (from sheet music or scale)
struct ExpectedNote {
    char note_name[4];
    uint8_t octave;
    float frequency;
    uint32_t timestamp_ms; // When this note should be played
};

// Real-time analyzer class
class RealTimeAnalyzer {
public:
    RealTimeAnalyzer();
    ~RealTimeAnalyzer();
    
    // Initialize analyzer
    bool init();
    
    // Process audio chunk (called continuously during recording)
    NoteDetection process_audio_chunk(const int16_t* audio_data, size_t samples);
    
    // Set expected note (from sheet music or scale)
    void set_expected_note(const ExpectedNote& note);
    
    // Check if played note matches expected
    bool check_note_match(const NoteDetection& detected, const ExpectedNote& expected);
    
    // Get current expected note
    ExpectedNote get_current_expected_note() const;
    
    // Reset for new recording/practice session
    void reset();

private:
    ExpectedNote current_expected_note_;
    bool has_expected_note_;
    
    // Pitch detection using autocorrelation
    float detect_pitch_autocorrelation(const int16_t* audio_data, size_t samples);
    
    // Convert frequency to note name
    void frequency_to_note(float frequency, char* note_name, uint8_t* octave);
    
    // Calculate note frequency
    float note_to_frequency(const char* note_name, uint8_t octave);
    
    // Check if frequency matches note (with tolerance)
    bool frequency_matches_note(float frequency, const char* note_name, uint8_t octave, float tolerance_hz = 10.0f);
};

#endif // REAL_TIME_ANALYZER_H

