/**
 * Display Manager for T5AI Screen Module
 * Handles real-time note detection display and fingering charts
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <cstdint>
#include <string>

// Display dimensions (adjust based on your screen module)
#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 320

// Instrument types
enum class InstrumentType {
    PIANO,
    VIOLIN,
    GUITAR,
    FLUTE,
    CLARINET,
    TRUMPET,
    SAXOPHONE
};

// Note information
struct NoteInfo {
    char note_name[4];      // e.g., "A", "Bb", "C#"
    uint8_t octave;         // e.g., 4
    float frequency;        // Hz
    bool is_correct;        // Matches expected note
    const char* fingering;  // Fingering chart data
};

// Display manager class
class DisplayManager {
public:
    DisplayManager();
    ~DisplayManager();
    
    // Initialize display
    bool init();
    
    // Set instrument type
    void set_instrument(InstrumentType instrument);
    
    // Display current note being played (real-time)
    void display_current_note(const NoteInfo& note);
    
    // Display expected note with fingering
    void display_expected_note(const char* note_name, uint8_t octave);
    
    // Display wrong note warning with correct fingering
    void display_wrong_note(const char* played_note, const char* expected_note, uint8_t octave);
    
    // Display scale/practice mode
    void display_scale_mode(const char* scale_name);
    
    // Display recording status
    void display_recording_status(bool is_recording);
    
    // Display connection status
    void display_connection_status(bool connected);
    
    // Clear display
    void clear();
    
    // Update display (call in main loop)
    void update();

private:
    InstrumentType current_instrument_;
    bool display_initialized_;
    
    // Get fingering chart for note
    const char* get_fingering_chart(const char* note_name, uint8_t octave);
    
    // Draw fingering diagram
    void draw_fingering_diagram(const char* fingering_data);
    
    // Draw note name
    void draw_note_name(const char* note_name, uint8_t octave, bool is_correct);
    
    // Draw frequency display
    void draw_frequency(float freq);
};

#endif // DISPLAY_MANAGER_H

