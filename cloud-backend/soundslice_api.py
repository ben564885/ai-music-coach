import requests
import os
import time

class SoundSliceAPI:
    def __init__(self, app_id=None, secret_key=None):
        self.app_id = app_id or os.getenv("SOUNDSLICE_APP_ID")
        self.secret_key = secret_key or os.getenv("SOUNDSLICE_SECRET_KEY")
        self.base_url = "https://www.soundslice.com/api/v1"

    def get_slice_info(self, scorehash):
        """
        Get slice metadata to check processing status.
        """
        if not self.app_id or not self.secret_key:
            raise Exception("SOUNDSLICE_APP_ID or SOUNDSLICE_SECRET_KEY not set.")

        url = f"{self.base_url}/slices/{scorehash}/"
        auth = (self.app_id, self.secret_key)
        
        response = requests.get(url, auth=auth)
        if response.status_code == 200:
            return response.json()
        return None

    def get_musicxml(self, scorehash, wait_for_stability=True, initial_wait_seconds=30):
        """
        Fetches the MusicXML for a given scorehash using HTTP Basic Auth.
        
        Args:
            scorehash: The SoundSlice slice ID
            wait_for_stability: If True, waits for the MusicXML to stabilize (stop changing)
            initial_wait_seconds: How long to wait before first fetch (gives OMR time to process)
        """
        if not self.app_id or not self.secret_key:
            raise Exception("SOUNDSLICE_APP_ID or SOUNDSLICE_SECRET_KEY not set.")

        url = f"{self.base_url}/slices/{scorehash}/musicxml/"
        auth = (self.app_id, self.secret_key)
        
        # Initial wait to let SoundSlice OMR process the image
        if initial_wait_seconds > 0:
            print(f"Waiting {initial_wait_seconds}s for SoundSlice OMR to process...")
            time.sleep(initial_wait_seconds)
        
        # Poll for MusicXML availability
        max_retries = 12  # Up to 2 minutes of polling
        last_content = None
        stable_count = 0
        REQUIRED_STABLE_CHECKS = 2  # Require 2 consecutive identical responses
        
        for i in range(max_retries):
            print(f"Attempt {i+1}/{max_retries}: Fetching MusicXML for {scorehash}...")
            response = requests.get(url, auth=auth)
            
            if response.status_code == 200:
                content = response.text
                content_length = len(content)
                
                # Count notes in the content as a rough measure of completeness
                note_count = content.count('<note>')
                print(f"  -> Got MusicXML: {content_length} chars, ~{note_count} notes")
                
                if wait_for_stability:
                    if last_content == content:
                        stable_count += 1
                        print(f"  -> Content stable ({stable_count}/{REQUIRED_STABLE_CHECKS})")
                        if stable_count >= REQUIRED_STABLE_CHECKS:
                            print(f"  -> MusicXML is stable! Returning final content.")
                            return content
                    else:
                        stable_count = 0
                        print(f"  -> Content changed, resetting stability counter")
                    
                    last_content = content
                    time.sleep(15)  # Wait 15 seconds between stability checks
                else:
                    # No stability check, return immediately
                    return content
                    
            elif response.status_code == 404:
                print("  -> Slice not found or MusicXML not yet available. Waiting...")
                time.sleep(10)
            else:
                print(f"  -> Error: {response.status_code} - {response.text}")
                time.sleep(10)
        
        # If we got content but didn't stabilize, return what we have
        if last_content:
            print("Timeout waiting for stable MusicXML, returning last received content.")
            return last_content
            
        print("Timeout waiting for MusicXML.")
        return None

if __name__ == "__main__":
    from dotenv import load_dotenv
    load_dotenv()
    
    api = SoundSliceAPI()
    # Test with a known scorehash if available
    # print(api.get_musicxml("YOUR_SCOREHASH"))
