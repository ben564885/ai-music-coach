/**
 * Real-Time Audio Analyzer Implementation
 * Pitch detection using autocorrelation for real-time note feedback
 */

#include "real_time_analyzer.h"
#include "tuya_config.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// Note frequencies for standard tuning (A4 = 440 Hz)
static const char *NOTE_NAMES[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                   "F#", "G",  "G#", "A",  "A#", "B"};
static const int NUM_NOTES = 12;

// A4 = 440 Hz (MIDI note 69)
static const float A4_FREQ = 440.0f;
static const int A4_MIDI = 69;

RealTimeAnalyzer::RealTimeAnalyzer() : has_expected_note_(false) {
  memset(&current_expected_note_, 0, sizeof(ExpectedNote));
}

RealTimeAnalyzer::~RealTimeAnalyzer() {}

bool RealTimeAnalyzer::init() {
  reset();
  return true;
}

void RealTimeAnalyzer::reset() {
  has_expected_note_ = false;
  memset(&current_expected_note_, 0, sizeof(ExpectedNote));
}

/**
 * Detect pitch using autocorrelation method
 */
float RealTimeAnalyzer::detect_pitch_autocorrelation(const int16_t *audio_data,
                                                     size_t samples) {
  if (!audio_data || samples < 64) {
    return 0.0f;
  }

  // Find the peak autocorrelation to determine period
  const int min_period = RT_SAMPLE_RATE / 2000; // Max freq: 2000 Hz
  const int max_period = RT_SAMPLE_RATE / 60;   // Min freq: 60 Hz

  if ((size_t)max_period > samples / 2) {
    return 0.0f;
  }

  // Normalize audio to float
  float *buffer = new float[samples];
  float max_val = 0.0f;
  for (size_t i = 0; i < samples; i++) {
    buffer[i] = (float)audio_data[i];
    max_val = std::max(max_val, std::abs(buffer[i]));
  }

  // Check for silence
  if (max_val < 100.0f) {
    delete[] buffer;
    return 0.0f;
  }

  // Normalize
  for (size_t i = 0; i < samples; i++) {
    buffer[i] /= max_val;
  }

  // Calculate autocorrelation for each lag
  float best_correlation = 0.0f;
  int best_period = 0;

  for (int period = min_period; period <= max_period; period++) {
    float correlation = 0.0f;
    float energy1 = 0.0f;
    float energy2 = 0.0f;

    size_t compare_length = samples - period;
    for (size_t i = 0; i < compare_length; i++) {
      correlation += buffer[i] * buffer[i + period];
      energy1 += buffer[i] * buffer[i];
      energy2 += buffer[i + period] * buffer[i + period];
    }

    // Normalized correlation
    float denominator = std::sqrt(energy1 * energy2);
    if (denominator > 0.0f) {
      correlation /= denominator;
    }

    if (correlation > best_correlation) {
      best_correlation = correlation;
      best_period = period;
    }
  }

  delete[] buffer;

  // Check if correlation is strong enough
  if (best_correlation < 0.5f || best_period == 0) {
    return 0.0f;
  }

  // Convert period to frequency
  float frequency = (float)RT_SAMPLE_RATE / (float)best_period;

  return frequency;
}

/**
 * Convert frequency to note name and octave
 */
void RealTimeAnalyzer::frequency_to_note(float frequency, char *note_name,
                                         uint8_t *octave) {
  if (frequency <= 0.0f || !note_name || !octave) {
    return;
  }

  // Calculate MIDI note number
  float midi_note_f = 12.0f * std::log2(frequency / A4_FREQ) + A4_MIDI;
  int midi_note = (int)std::round(midi_note_f);

  // Extract note and octave
  int note_index = midi_note % 12;
  if (note_index < 0)
    note_index += 12;

  *octave = (midi_note / 12) - 1;
  strcpy(note_name, NOTE_NAMES[note_index]);
}

/**
 * Convert note name and octave to frequency
 */
float RealTimeAnalyzer::note_to_frequency(const char *note_name,
                                          uint8_t octave) {
  if (!note_name)
    return 0.0f;

  // Find note index
  int note_index = -1;
  for (int i = 0; i < NUM_NOTES; i++) {
    if (strcmp(note_name, NOTE_NAMES[i]) == 0) {
      note_index = i;
      break;
    }
  }

  // Handle flat notes
  if (note_index < 0) {
    if (strcmp(note_name, "Db") == 0)
      note_index = 1;
    else if (strcmp(note_name, "Eb") == 0)
      note_index = 3;
    else if (strcmp(note_name, "Gb") == 0)
      note_index = 6;
    else if (strcmp(note_name, "Ab") == 0)
      note_index = 8;
    else if (strcmp(note_name, "Bb") == 0)
      note_index = 10;
  }

  if (note_index < 0)
    return 0.0f;

  // Calculate MIDI note
  int midi_note = (octave + 1) * 12 + note_index;

  // Calculate frequency
  return A4_FREQ * std::pow(2.0f, (midi_note - A4_MIDI) / 12.0f);
}

/**
 * Check if frequency matches a note within tolerance
 */
bool RealTimeAnalyzer::frequency_matches_note(float frequency,
                                              const char *note_name,
                                              uint8_t octave,
                                              float tolerance_hz) {
  float target_freq = note_to_frequency(note_name, octave);
  if (target_freq <= 0.0f)
    return false;

  return std::abs(frequency - target_freq) <= tolerance_hz;
}

NoteDetection RealTimeAnalyzer::process_audio_chunk(const int16_t *audio_data,
                                                    size_t samples) {
  NoteDetection result = {};
  result.is_valid = false;
  result.confidence = 0.0f;

  // Detect pitch
  float frequency = detect_pitch_autocorrelation(audio_data, samples);

  if (frequency > 50.0f && frequency < 2000.0f) {
    result.frequency = frequency;
    result.is_valid = true;

    // Convert to note
    frequency_to_note(frequency, result.note_name, &result.octave);

    // Calculate confidence based on how close to exact note frequency
    float exact_freq = note_to_frequency(result.note_name, result.octave);
    float deviation = std::abs(frequency - exact_freq);
    result.confidence = 1.0f - std::min(1.0f, deviation / 50.0f);
  }

  return result;
}

void RealTimeAnalyzer::set_expected_note(const ExpectedNote &note) {
  current_expected_note_ = note;
  has_expected_note_ = true;
}

bool RealTimeAnalyzer::check_note_match(const NoteDetection &detected,
                                        const ExpectedNote &expected) {
  if (!detected.is_valid)
    return false;

  // Check if note names match (including enharmonic equivalents)
  bool name_match = (strcmp(detected.note_name, expected.note_name) == 0);

  // Also check enharmonic equivalents
  if (!name_match) {
    // Common enharmonic pairs
    if ((strcmp(detected.note_name, "C#") == 0 &&
         strcmp(expected.note_name, "Db") == 0) ||
        (strcmp(detected.note_name, "Db") == 0 &&
         strcmp(expected.note_name, "C#") == 0) ||
        (strcmp(detected.note_name, "D#") == 0 &&
         strcmp(expected.note_name, "Eb") == 0) ||
        (strcmp(detected.note_name, "Eb") == 0 &&
         strcmp(expected.note_name, "D#") == 0) ||
        (strcmp(detected.note_name, "F#") == 0 &&
         strcmp(expected.note_name, "Gb") == 0) ||
        (strcmp(detected.note_name, "Gb") == 0 &&
         strcmp(expected.note_name, "F#") == 0) ||
        (strcmp(detected.note_name, "G#") == 0 &&
         strcmp(expected.note_name, "Ab") == 0) ||
        (strcmp(detected.note_name, "Ab") == 0 &&
         strcmp(expected.note_name, "G#") == 0) ||
        (strcmp(detected.note_name, "A#") == 0 &&
         strcmp(expected.note_name, "Bb") == 0) ||
        (strcmp(detected.note_name, "Bb") == 0 &&
         strcmp(expected.note_name, "A#") == 0)) {
      name_match = true;
    }
  }

  // Check octave match and name match
  bool octave_match = (detected.octave == expected.octave);

  // Also use frequency-based matching with tolerance
  bool freq_match =
      frequency_matches_note(detected.frequency, expected.note_name,
                             expected.octave, RT_PITCH_TOLERANCE_HZ);

  return (name_match && octave_match) || freq_match;
}

ExpectedNote RealTimeAnalyzer::get_current_expected_note() const {
  if (has_expected_note_) {
    return current_expected_note_;
  }

  ExpectedNote empty = {};
  return empty;
}
