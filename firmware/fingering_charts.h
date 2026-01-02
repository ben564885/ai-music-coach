/**
 * Fingering Charts for Different Instruments
 * Provides visual/textual fingering instructions for notes
 */

#ifndef FINGERING_CHARTS_H
#define FINGERING_CHARTS_H

#include "display_manager.h"

// Fingering chart data structure
struct FingeringChart {
    const char* note_name;
    uint8_t octave;
    InstrumentType instrument;
    const char* fingering_text;      // Text description
    const char* fingering_diagram;    // ASCII/visual diagram
    const char* hand_position;       // Hand/finger position
};

class FingeringCharts {
public:
    // Get fingering for a note on an instrument
    static const FingeringChart* get_fingering(
        const char* note_name,
        uint8_t octave,
        InstrumentType instrument
    );
    
    // Get scale fingering (e.g., C major scale)
    static const FingeringChart* get_scale_fingering(
        const char* scale_name,
        InstrumentType instrument
    );

private:
    // Piano fingerings
    static const FingeringChart piano_fingerings[];
    
    // Violin fingerings
    static const FingeringChart violin_fingerings[];
    
    // Guitar fingerings
    static const FingeringChart guitar_fingerings[];
    
    // Flute fingerings
    static const FingeringChart flute_fingerings[];
    
    // Find fingering in array
    static const FingeringChart* find_fingering(
        const FingeringChart* charts,
        size_t count,
        const char* note_name,
        uint8_t octave
    );
};

// Example fingering data (simplified - would be more comprehensive)
const FingeringChart FingeringCharts::piano_fingerings[] = {
    {"A", 4, InstrumentType::PIANO, 
     "Right hand: Use thumb (1) on A", 
     "  [1]  [2]  [3]  [4]  [5]\n   A    B    C    D    E",
     "Place thumb on A, fingers curved"},
    
    {"Bb", 4, InstrumentType::PIANO,
     "Right hand: Use thumb (1) on Bb, or index finger (2)",
     "  [1]  [2]  [3]  [4]  [5]\n  Bb    B    C    D    E",
     "Black key - use thumb or index finger"},
    
    // Add more notes...
};

const FingeringChart FingeringCharts::violin_fingerings[] = {
    {"A", 4, InstrumentType::VIOLIN,
     "Open A string (no fingers)",
     "  O  [1]  [2]  [3]  [4]\n   A   B   C#   D   E",
     "Play open A string"},
    
    {"A", 5, InstrumentType::VIOLIN,
     "First finger on E string",
     "  O  [1]  [2]  [3]  [4]\n   E   F#   G   A   B",
     "Place first finger on E string"},
    
    // Add more...
};

const FingeringChart FingeringCharts::guitar_fingerings[] = {
    {"A", 4, InstrumentType::GUITAR,
     "Open A string (5th string)",
     "E|--0--|\nB|-----|\nG|-----|\nD|-----|\nA|--0--|\nE|-----|",
     "Play open 5th string"},
    
    {"A", 4, InstrumentType::GUITAR,
     "5th fret on low E string",
     "E|--5--|\nB|-----|\nG|-----|\nD|-----|\nA|-----|\nE|-----|",
     "5th fret, low E string, use index finger"},
    
    // Add more...
};

#endif // FINGERING_CHARTS_H

