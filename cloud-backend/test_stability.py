"""Test the improved SoundSlice API stability checking"""
import os
from dotenv import load_dotenv
from soundslice_api import SoundSliceAPI

load_dotenv()

def test_stability():
    app_id = os.getenv("SOUNDSLICE_APP_ID")
    secret_key = os.getenv("SOUNDSLICE_SECRET_KEY")
    
    if not app_id or not secret_key:
        print("ERROR: SOUNDSLICE_APP_ID or SOUNDSLICE_SECRET_KEY not set in .env")
        return
    
    # Use the scorehash for the user's uploaded image
    # This is the same slice they see on soundslice.com
    scorehash = "Cdqbc"  # From our earlier debug
    
    print("=" * 60)
    print(f"Testing MusicXML fetch with stability check for: {scorehash}")
    print("=" * 60)
    
    api = SoundSliceAPI(app_id, secret_key)
    
    # Use shorter initial wait since this slice already exists
    musicxml = api.get_musicxml(scorehash, wait_for_stability=True, initial_wait_seconds=5)
    
    if musicxml:
        print(f"\n\nFinal MusicXML: {len(musicxml)} characters")
        print("\n--- First 1500 chars ---")
        print(musicxml[:1500])
        
        # Count key metrics
        note_count = musicxml.count('<note>')
        measure_count = musicxml.count('<measure')
        print(f"\nMetrics: {note_count} notes, {measure_count} measures")
    else:
        print("Failed to get MusicXML!")

if __name__ == "__main__":
    test_stability()
