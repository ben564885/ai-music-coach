"""Debug script to check what SoundSlice API is returning"""
import os
from dotenv import load_dotenv
from soundslice_api import SoundSliceAPI

load_dotenv()

def debug_latest_slice():
    """Check the MusicXML from the most recent slice"""
    app_id = os.getenv("SOUNDSLICE_APP_ID")
    secret_key = os.getenv("SOUNDSLICE_SECRET_KEY")
    
    if not app_id or not secret_key:
        print("ERROR: SOUNDSLICE_APP_ID or SOUNDSLICE_SECRET_KEY not set in .env")
        return
    
    # List all slices accessible via API
    import requests
    
    base_url = "https://www.soundslice.com/api/v1"
    auth = (app_id, secret_key)
    
    print("=" * 60)
    print("Step 1: Listing all slices accessible via API...")
    print("=" * 60)
    
    response = requests.get(f"{base_url}/slices/", auth=auth)
    print(f"Status Code: {response.status_code}")
    
    if response.status_code == 200:
        slices = response.json()
        print(f"Found {len(slices)} slices accessible via API")
        
        if slices:
            for i, s in enumerate(slices[:5]):  # Show first 5
                print(f"\n  [{i+1}] Scorehash: {s.get('scorehash')}")
                print(f"      Name: {s.get('name', 'N/A')}")
                print(f"      Artist: {s.get('artist', 'N/A')}")
                print(f"      Created: {s.get('created', 'N/A')}")
                print(f"      Has notation: {s.get('has_notation', 'N/A')}")
            
            # Get MusicXML for the first (most recent) slice
            latest = slices[0]
            scorehash = latest.get('scorehash')
            
            print("\n" + "=" * 60)
            print(f"Step 2: Fetching MusicXML for scorehash: {scorehash}")
            print("=" * 60)
            
            api = SoundSliceAPI(app_id, secret_key)
            musicxml = api.get_musicxml(scorehash)
            
            if musicxml:
                print(f"\nMusicXML retrieved! Length: {len(musicxml)} characters")
                print("\n--- First 2000 chars of MusicXML ---")
                print(musicxml[:2000])
                
                # Save to file for inspection
                with open("debug_musicxml_output.xml", "w") as f:
                    f.write(musicxml)
                print("\n\nFull MusicXML saved to: debug_musicxml_output.xml")
            else:
                print("\nFailed to retrieve MusicXML!")
                print("This could mean:")
                print("  1. OMR processing is not complete yet")
                print("  2. The slice doesn't have notation data")
                print("  3. API doesn't have access to this slice")
        else:
            print("\nNo slices found!")
            print("This means the API credentials don't have access to any slices.")
            print("Slices created via web UI are NOT accessible via API unless")
            print("they were created BY the API or given specific permissions.")
    else:
        print(f"Error listing slices: {response.text}")

if __name__ == "__main__":
    debug_latest_slice()
