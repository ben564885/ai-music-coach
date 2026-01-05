"""Check all slices and compare their content signatures"""
import os
from dotenv import load_dotenv
import requests

load_dotenv()

def list_all_slices():
    app_id = os.getenv("SOUNDSLICE_APP_ID")
    secret_key = os.getenv("SOUNDSLICE_SECRET_KEY")
    
    base_url = "https://www.soundslice.com/api/v1"
    auth = (app_id, secret_key)
    
    response = requests.get(f"{base_url}/slices/", auth=auth)
    
    if response.status_code == 200:
        slices = response.json()
        print(f"Found {len(slices)} slices:\n")
        
        for s in slices:
            scorehash = s.get('scorehash')
            name = s.get('name', 'N/A')
            
            print(f"Scorehash: {scorehash}")
            print(f"  Name: {name}")
            
            # Fetch first bit of MusicXML to check key signature
            musicxml_url = f"{base_url}/slices/{scorehash}/musicxml/"
            musicxml_response = requests.get(musicxml_url, auth=auth)
            
            if musicxml_response.status_code == 200:
                content = musicxml_response.text[:3000]
                
                # Extract key signature
                import re
                fifths_match = re.search(r'<fifths>(-?\d+)</fifths>', content)
                time_match = re.search(r'<beats>(\d+)</beats>\s*<beat-type>(\d+)</beat-type>', content)
                first_note = re.search(r'<step>([A-G])</step>.*?<octave>(\d)</octave>', content, re.DOTALL)
                
                key = f"{fifths_match.group(1)} fifths" if fifths_match else "unknown"
                time = f"{time_match.group(1)}/{time_match.group(2)}" if time_match else "unknown"
                note = f"{first_note.group(1)}{first_note.group(2)}" if first_note else "unknown"
                
                print(f"  Key: {key}, Time: {time}, First note: {note}")
            else:
                print(f"  MusicXML: Not available ({musicxml_response.status_code})")
            
            print()
    else:
        print(f"Error: {response.status_code}")

if __name__ == "__main__":
    list_all_slices()
