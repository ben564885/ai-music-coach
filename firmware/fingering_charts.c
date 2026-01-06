/**
 * Fingering Charts Data Implementation
 * Contains static data for various instruments
 */

#include "fingering_charts.h"
#include <stddef.h>

// Piano fingerings
static const FingeringChart s_piano_fingerings[] = {
    {"A", 4, 0, "Right hand: Use thumb (1) on A",
     " [1] [2] [3] [4] [5]\n  A   B   C   D   E",
     "Place thumb on A, fingers curved"},
    {"Bb", 4, 0, "Right hand: Use index finger (2) on Bb",
     " [1] [2] [3] [4] [5]\n Bb   B   C   D   E",
     "Black key - use index finger"},
    {"C", 4, 0, "Right hand: Use thumb (1) on C",
     " [1] [2] [3] [4] [5]\n  C   D   E   F   G", "Place thumb on Middle C"},
    // Add more piano notes...
};

// Violin fingerings
static const FingeringChart s_violin_fingerings[] = {
    {"A", 4, 1, "Open A string", " O [1] [2] [3] [4]\n  A  B  C#  D  E",
     "Play open A string"},
    {"D", 4, 1, "Open D string", " O [1] [2] [3] [4]\n  D  E  F#  G  A",
     "Play open D string"},
    // Add more violin notes...
};

// Guitar fingerings
static const FingeringChart s_guitar_fingerings[] = {
    {"A", 2, 2, "Open 5th string",
     "E|---|\nB|---|\nG|---|\nD|---|\nA|-0-|\nE|---|", "Play open 5th string"},
    {"E", 2, 2, "Open 6th string",
     "E|---|\nB|---|\nG|---|\nD|---|\nA|---|\nE|-0-|", "Play open 6th string"},
};

const FingeringChart *FingeringCharts_get_fingering(const char *note_name,
                                                    uint8_t octave,
                                                    uint8_t instrument) {
  const FingeringChart *chart_list = NULL;
  size_t chart_count = 0;

  switch (instrument) {
  case 0: // Piano
    chart_list = s_piano_fingerings;
    chart_count = sizeof(s_piano_fingerings) / sizeof(FingeringChart);
    break;
  case 1: // Violin
    chart_list = s_violin_fingerings;
    chart_count = sizeof(s_violin_fingerings) / sizeof(FingeringChart);
    break;
  case 2: // Guitar
    chart_list = s_guitar_fingerings;
    chart_count = sizeof(s_guitar_fingerings) / sizeof(FingeringChart);
    break;
  default:
    return NULL;
  }

  for (size_t i = 0; i < chart_count; i++) {
    if (strcmp(chart_list[i].note_name, note_name) == 0 &&
        chart_list[i].octave == octave) {
      return &chart_list[i];
    }
  }

  return NULL;
}
