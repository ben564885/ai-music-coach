"""
Audio Analysis Module
Handles pitch detection, timing analysis, and dynamics detection
"""

import librosa
import numpy as np
from music21 import converter, note, tempo
from scipy.signal import find_peaks
import json


class AudioAnalyzer:
    """Analyzes audio recordings and compares to reference sheet music"""
    
    def __init__(self, sample_rate=16000):
        self.sample_rate = sample_rate
        self.pitch_tolerance_hz = 10  # Hz deviation tolerance
        self.timing_tolerance = 0.2  # 20% timing tolerance
    
    def analyze(self, audio_path, reference_data, metadata):
        """
        Main analysis function
        
        Args:
            audio_path: Path to audio file
            reference_data: Dictionary with expected notes, timing, dynamics
            metadata: Tempo, key signature, etc.
        
        Returns:
            List of detected mistakes
        """
        # Load audio
        y, sr = librosa.load(audio_path, sr=self.sample_rate)
        
        mistakes = []
        
        # Extract audio features
        pitches, magnitudes = self._extract_pitches(y, sr)
        onset_frames = librosa.onset.onset_detect(y=y, sr=sr)
        tempo_detected, beats = librosa.beat.beat_track(y=y, sr=sr)
        rms = librosa.feature.rms(y=y)[0]
        
        # Analyze note accuracy
        note_mistakes = self._analyze_note_accuracy(
            pitches, magnitudes, reference_data.get('notes', []), sr
        )
        mistakes.extend(note_mistakes)
        
        # Analyze timing
        timing_mistakes = self._analyze_timing(
            onset_frames, reference_data.get('timing', []), 
            tempo_detected, metadata.get('tempo', 120), sr
        )
        mistakes.extend(timing_mistakes)
        
        # Analyze dynamics
        dynamics_mistakes = self._analyze_dynamics(
            rms, reference_data.get('dynamics', []), sr
        )
        mistakes.extend(dynamics_mistakes)
        
        # Sort mistakes by timestamp
        mistakes.sort(key=lambda x: x['timestamp'])
        
        return mistakes
    
    def _extract_pitches(self, y, sr):
        """Extract pitch information from audio"""
        # Use PYIN algorithm for robust pitch tracking
        pitches, magnitudes = librosa.piptrack(y=y, sr=sr, threshold=0.1)
        
        # Get the most prominent pitch at each time frame
        pitch_track = []
        magnitude_track = []
        
        for t in range(pitches.shape[1]):
            index = magnitudes[:, t].argmax()
            pitch_value = pitches[index, t]
            magnitude_value = magnitudes[index, t]
            
            if pitch_value > 0:  # Valid pitch
                pitch_track.append(pitch_value)
                magnitude_track.append(magnitude_value)
            else:
                pitch_track.append(0)
                magnitude_track.append(0)
        
        return np.array(pitch_track), np.array(magnitude_track)
    
    def _analyze_note_accuracy(self, pitches, magnitudes, expected_notes, sr):
        """Compare played notes to expected notes"""
        mistakes = []
        
        for expected_note in expected_notes:
            timestamp = expected_note.get('timestamp', 0)
            expected_freq = self._note_to_frequency(expected_note.get('note', 'C4'))
            tolerance = expected_note.get('tolerance_hz', self.pitch_tolerance_hz)
            
            # Find pitch at expected timestamp
            frame_idx = int(timestamp * sr / 512)  # librosa uses 512 hop length
            if frame_idx < len(pitches):
                actual_freq = pitches[frame_idx]
                
                if actual_freq > 0 and abs(actual_freq - expected_freq) > tolerance:
                    played_note = self._frequency_to_note(actual_freq)
                    mistakes.append({
                        'type': 'note_accuracy',
                        'timestamp': timestamp,
                        'expected': expected_note.get('note', 'Unknown'),
                        'played': played_note,
                        'expected_freq': expected_freq,
                        'actual_freq': actual_freq,
                        'deviation_hz': abs(actual_freq - expected_freq),
                        'measure': expected_note.get('measure', 0),
                        'beat': expected_note.get('beat', 0)
                    })
        
        return mistakes
    
    def _analyze_timing(self, onset_frames, expected_timing, detected_tempo, expected_tempo, sr):
        """Analyze timing issues (hesitation, rushing)"""
        mistakes = []
        
        if len(onset_frames) < 2:
            return mistakes
        
        # Convert onset frames to timestamps
        onset_times = librosa.frames_to_time(onset_frames, sr=sr)
        inter_onset_intervals = np.diff(onset_times)
        
        # Calculate expected intervals
        expected_ioi = 60.0 / expected_tempo  # Default quarter note duration
        
        for i, (actual_ioi, onset_time) in enumerate(zip(inter_onset_intervals, onset_times[:-1])):
            # Check for hesitation (too slow)
            if actual_ioi > expected_ioi * 1.5:
                mistakes.append({
                    'type': 'hesitation',
                    'timestamp': onset_time,
                    'gap_ms': (actual_ioi - expected_ioi) * 1000,
                    'expected_interval_ms': expected_ioi * 1000,
                    'actual_interval_ms': actual_ioi * 1000
                })
            # Check for rushing (too fast)
            elif actual_ioi < expected_ioi * 0.8:
                local_bpm = 60.0 / actual_ioi
                mistakes.append({
                    'type': 'rushing',
                    'timestamp': onset_time,
                    'bpm_detected': local_bpm,
                    'bpm_expected': expected_tempo,
                    'speed_increase_percent': ((expected_tempo - local_bpm) / expected_tempo) * 100
                })
        
        # Check overall tempo deviation
        tempo_deviation = abs(detected_tempo - expected_tempo) / expected_tempo
        if tempo_deviation > 0.1:  # More than 10% deviation
            mistakes.append({
                'type': 'tempo_deviation',
                'timestamp': 0,
                'detected_tempo': detected_tempo,
                'expected_tempo': expected_tempo,
                'deviation_percent': tempo_deviation * 100
            })
        
        return mistakes
    
    def _analyze_dynamics(self, rms, expected_dynamics, sr):
        """Analyze dynamics (loudness) compared to markings"""
        mistakes = []
        
        # Convert RMS to dB
        rms_db = librosa.amplitude_to_db(rms, ref=np.max)
        
        dynamic_levels = {
            'ppp': -40,  # pianississimo
            'pp': -30,   # pianissimo
            'p': -20,    # piano
            'mp': -10,   # mezzo-piano
            'mf': 0,     # mezzo-forte
            'f': 10,     # forte
            'ff': 20,    # fortissimo
            'fff': 30    # fortississimo
        }
        
        for dynamic_mark in expected_dynamics:
            timestamp = dynamic_mark.get('timestamp', 0)
            marking = dynamic_mark.get('marking', 'mf').lower()
            expected_db = dynamic_levels.get(marking, 0)
            
            # Find RMS at timestamp
            frame_idx = int(timestamp * sr / 512)
            if frame_idx < len(rms_db):
                actual_db = rms_db[frame_idx]
                
                if abs(actual_db - expected_db) > 10:  # 10 dB tolerance
                    mistakes.append({
                        'type': 'dynamics',
                        'timestamp': timestamp,
                        'marking': marking,
                        'actual_db': actual_db,
                        'expected_db': expected_db,
                        'deviation_db': abs(actual_db - expected_db),
                        'measure': dynamic_mark.get('measure', 0)
                    })
        
        return mistakes
    
    def extract_reference_data(self, score):
        """Extract structured reference data from music21 score"""
        reference_data = {
            'notes': [],
            'timing': [],
            'dynamics': [],
            'tempo': 120,
            'key_signature': None
        }
        
        # Extract tempo
        tempo_marks = score.flat.getElementsByClass(tempo.MetronomeMark)
        if tempo_marks:
            reference_data['tempo'] = tempo_marks[0].number
        
        # Extract key signature
        key_sigs = score.flat.getElementsByClass('KeySignature')
        if key_sigs:
            reference_data['key_signature'] = str(key_sigs[0])
        
        # Extract notes with timing
        offset = 0
        for element in score.flat:
            if isinstance(element, note.Note):
                note_name = element.nameWithOctave
                freq = element.pitch.frequency
                
                reference_data['notes'].append({
                    'note': note_name,
                    'frequency': freq,
                    'timestamp': offset,
                    'duration': element.duration.quarterLength,
                    'measure': element.measureNumber if hasattr(element, 'measureNumber') else 0
                })
                
                offset += element.duration.quarterLength * (60.0 / reference_data['tempo'])
            
            elif isinstance(element, note.Rest):
                offset += element.duration.quarterLength * (60.0 / reference_data['tempo'])
            
            # Extract dynamics
            if hasattr(element, 'dynamics'):
                reference_data['dynamics'].append({
                    'marking': str(element.dynamics),
                    'timestamp': offset,
                    'measure': element.measureNumber if hasattr(element, 'measureNumber') else 0
                })
        
        return reference_data
    
    def _note_to_frequency(self, note_name):
        """Convert note name (e.g., 'C4', 'Bb3') to frequency in Hz"""
        try:
            n = note.Note(note_name)
            return n.pitch.frequency
        except Exception:
            # Fallback: approximate calculation
            # A4 = 440 Hz
            note_map = {'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5,
                       'F#': 6, 'G': 7, 'G#': 8, 'A': 9, 'A#': 10, 'B': 11}
            
            try:
                if len(note_name) >= 2:
                    note_letter = note_name[0]
                    if len(note_name) > 2 and note_name[1] == '#':
                        note_letter = note_name[:2]
                        octave = int(note_name[2])
                    else:
                        octave = int(note_name[1])
                    
                    semitones = note_map.get(note_letter, 0) + (octave - 4) * 12
                    return 440 * (2 ** (semitones / 12))
            except Exception:
                pass
            
            return 440  # Default to A4
    
    def _frequency_to_note(self, frequency):
        """Convert frequency to note name"""
        try:
            # A4 = 440 Hz
            semitones = 12 * np.log2(frequency / 440.0)
            octave = 4 + int(semitones / 12)
            note_index = int(round(semitones % 12))
            
            note_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
            return f"{note_names[note_index]}{octave}"
        except Exception:
            return "Unknown"

