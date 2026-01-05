"""Debug: Parse the MusicXML and show what gets stored as reference_data"""
import os
from dotenv import load_dotenv
from music21 import converter
from analysis.audio_analyzer import AudioAnalyzer
from analysis.coach import AICoach

load_dotenv()

def debug_parsing():
    # Read the MusicXML we just saved
    with open("debug_musicxml_output.xml", "r") as f:
        musicxml_content = f.read()
    
    print("=" * 60)
    print("Parsing MusicXML with music21...")
    print("=" * 60)
    
    score = converter.parse(musicxml_content)
    
    print(f"\nScore parts: {len(score.parts)}")
    
    # Extract reference data using the same function as the app
    audio_analyzer = AudioAnalyzer()
    reference_data_raw = audio_analyzer.extract_reference_data(score)
    
    print(f"\nRaw reference_data keys: {reference_data_raw.keys()}")
    print(f"Number of notes extracted: {len(reference_data_raw.get('notes', []))}")
    
    print("\n--- First 10 notes from extract_reference_data ---")
    for i, note in enumerate(reference_data_raw.get('notes', [])[:10]):
        print(f"  [{i+1}] {note}")
    
    # Now convert using AICoach method
    print("\n" + "=" * 60)
    print("Converting with _convert_music21_to_music_json...")
    print("=" * 60)
    
    coach = AICoach()
    converted = coach._convert_music21_to_music_json(score, reference_data_raw)
    
    print(f"\nConverted JSON keys: {converted.keys()}")
    print(f"Clef: {converted.get('clef')}")
    print(f"Key Signature: {converted.get('key_signature')}")
    print(f"Time Signature: {converted.get('timeSignature')}")
    print(f"Number of notes: {len(converted.get('notes', []))}")
    
    print("\n--- First 10 notes from converted JSON ---")
    for i, note in enumerate(converted.get('notes', [])[:10]):
        print(f"  [{i+1}] pitch={note.get('pitch')}, duration={note.get('duration')}, startBeat={note.get('startBeat')}")
    
    # Compare with what's in MusicXML directly
    print("\n" + "=" * 60)
    print("Expected notes from MusicXML (manual reading):")
    print("=" * 60)
    print("Measure 1: F5 quarter")
    print("Measure 2: Bb5 quarter, Bb5 eighth, C6...")
    
if __name__ == "__main__":
    debug_parsing()
