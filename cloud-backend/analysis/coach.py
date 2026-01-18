"""
AI Coach Module - Improved Prompts
Generates personalized coaching feedback using Gemini API, Audiveris OMR, and Roboflow OMR
"""

import json
import os
import sys
import subprocess
import tempfile
import shutil
import requests
from pathlib import Path
import google.generativeai as genai
from soundslice_automation import upload_sheet_music
from soundslice_api import SoundSliceAPI


class AICoach:
    """Generates encouraging, actionable music coaching feedback"""
    
    def __init__(self, api_key=None, roboflow_api_key=None):
        if api_key:
            genai.configure(api_key=api_key)
            # Flash for fast metadata and feedback
            self.model_flash = genai.GenerativeModel('gemini-2.5-flash')
            # Pro for high-accuracy note reading - using 'gemini-pro-latest' as 1.5-pro was 404
            self.model_pro = genai.GenerativeModel('gemini-pro-latest')
            # Alias for backward compatibility (defaults to Flash for general tasks)
            self.model = self.model_flash
        else:
            self.model = None
            self.model_flash = None
            self.model_pro = None
        
        # Roboflow configuration
        self.roboflow_api_key = roboflow_api_key or os.getenv('ROBOFLOW_API_KEY')
        self.roboflow_workspace = os.getenv('ROBOFLOW_WORKSPACE', 'ben-d5iad')
        self.roboflow_workflow_id = os.getenv('ROBOFLOW_WORKFLOW_ID', 'detect-and-classify')
        # For workflows, must use serverless endpoint (but it doesn't support classification models)
        # For individual models, can use hosted endpoint
        self.roboflow_api_url = os.getenv('ROBOFLOW_API_URL', 'https://serverless.roboflow.com')
        self.roboflow_use_individual_models = os.getenv('ROBOFLOW_USE_INDIVIDUAL_MODELS', 'false').lower() == 'true'
        self.use_roboflow = bool(self.roboflow_api_key)
    
    def detect_time_signature_gemini(self, image_path):
        """
        Use Gemini Vision to extract ONLY the time signature from an image.
        Returns time signature as string (e.g., "4/4", "3/4", "2/2").
        """
        if not self.model:
            print("DEBUG: Gemini model is None. Cannot detect time signature.")
            return None

        import re
        from PIL import Image
        
        try:
            image = Image.open(image_path)
            
            prompt = """Look at this sheet music and identify ONLY the time signature.

The time signature is the two numbers stacked vertically after the key signature (or after the clef if there's no key signature).

Return ONLY this JSON format:
{
  "timeSignature": "4/4"
}

Time signature guide:
- C symbol (common time) = 4/4
- C with vertical line (cut time) = 2/2
- Otherwise: count the two stacked numbers (top number = beats per measure, bottom = note value)

Examples:
- Two numbers: 4 over 4 = "4/4"
- Two numbers: 3 over 4 = "3/4"
- Two numbers: 2 over 2 = "2/2"
- C symbol = "4/4"
- C with line = "2/2"

Look carefully at the beginning of the staff, right after the clef and key signature.
Return ONLY the time signature, nothing else."""

            response = self.model.generate_content([image, prompt])
            result_text = response.text
            
            json_match = re.search(r'\{.*\}', result_text, re.DOTALL)
            if json_match:
                result = json.loads(json_match.group(0))
                time_sig = result.get('timeSignature')
                if time_sig:
                    print(f"DEBUG: Gemini detected time signature: {time_sig}")
                    return time_sig
            print("DEBUG: Could not extract time signature from Gemini response")
            return None
            
        except Exception as e:
            print(f"Error calling Gemini Vision API for time signature: {e}")
            return None

    def verify_metadata(self, image_path):
        """
        Use Gemini Vision to extract music metadata from an image.
        Corrects OMR errors in Clef, Key Signature, and Time Signature.
        """
        if not self.model: return {}
        import re
        from PIL import Image
        try:
            image = Image.open(image_path)
            prompt = """Analyze the sheet music image provided. Follow these steps carefully:

1. **Visual Scan**: Describe what you see at the very beginning (left side) of the first staff. 
2. **Clef Identification**: Is it a Treble (G-clef), Bass (F-clef), or something else?
3. **Key Signature Analysis**: Count the number of sharps (#) or flats (b) between the clef and the time signature. If there are none, it is "C major". If there are e.g. 2 sharps, it is "D major".
4. **Time Signature Analysis**: Look at the two stacked numbers. If you see a 'C' it's 4/4. If you see 'C' with a line, it's 2/2. Otherwise, read the numbers.

Return a JSON object with this EXACT structure (no other text):
{
  "reasoning": "Briefly describe what symbols you counted (e.g., 'I see a treble clef, 3 flats, and the numbers 6 over 8')",
  "clef": "treble",
  "key_signature": "Eb major",
  "timeSignature": "6/8"
}

CRITICAL: Do not default to C major 4/4 if the image shows something else. Look specifically for the 6/8 or 3/4 markings mentioned by the user."""
            
            response = self.model.generate_content([image, prompt])
            print(f"DEBUG: Gemini RAW response for metadata: {response.text}")
            return self._parse_json_response(response.text) or {}
        except Exception as e:
            print(f"Error calling Gemini Vision API for metadata: {e}")
            return {}

    def transcribe_image(self, image_path):
        """
        Transcribe sheet music using Dual-Model Strategy:
        1. Gemini 2.5 Flash: Extract metadata (Key, Clef, Time)
        2. Gemini 1.5 Pro: precise note transcription using metadata context
        """
        try:
            # Try SoundSlice first
            print(f"DEBUG: Attempting SoundSlice transcription for {image_path}...")
            soundslice_result = self.transcribe_image_soundslice(image_path)
            if soundslice_result:
                return soundslice_result
            
            print("DEBUG: SoundSlice failed or not configured, falling back to Dual-Model Gemini...")
            # Step 1: Metadata with Flash (Fast & Robust for text/symbols)
            print(f"DEBUG: Step 1: Getting expert metadata with Gemini Flash for {image_path}...")
            gemini_metadata = self.verify_metadata(image_path)
            
            # Step 2: Notes with Pro (High Reasoning for spatial notes)
            print(f"DEBUG: Step 2: Extracting notes with Gemini Pro...")
            # Pass metadata to guide the Pro model
            return self.transcribe_image_gemini_wrapped(image_path, metadata=gemini_metadata)

        except Exception as e:
            print(f"DEBUG: Transcription error: {e}")
            import traceback
            traceback.print_exc()
            return None

    def transcribe_image_soundslice(self, image_path):
        """
        Transcribe sheet music using SoundSlice OMR (UI Automation + Data API).
        """
        email = os.getenv("SOUNDSLICE_EMAIL")
        password = os.getenv("SOUNDSLICE_PASSWORD")
        app_id = os.getenv("SOUNDSLICE_APP_ID")
        secret_key = os.getenv("SOUNDSLICE_SECRET_KEY")
        
        if not all([email, password, app_id, secret_key]):
            print("DEBUG: SoundSlice credentials incomplete in .env. Skipping SoundSlice.")
            return None
            
        try:
            # 1. Upload via Playwright
            scorehash = upload_sheet_music(email, password, image_path)
            if not scorehash:
                print("DEBUG: SoundSlice upload failed.")
                return None
            
            # 2. Get MusicXML via API
            api = SoundSliceAPI(app_id, secret_key)
            musicxml_content = api.get_musicxml(scorehash)
            if not musicxml_content:
                print("DEBUG: SoundSlice MusicXML retrieval failed.")
                return None
                
            # 3. Parse MusicXML using music21
            from music21 import converter
            from analysis.audio_analyzer import AudioAnalyzer
            
            score = converter.parse(musicxml_content)
            audio_analyzer = AudioAnalyzer()
            reference_data_raw = audio_analyzer.extract_reference_data(score)
            
            # Convert to our combined format
            converted_json = self._convert_music21_to_music_json(score, reference_data_raw)
            
            return {
                'reference_data': converted_json,
                'audiveris_raw_output': musicxml_content # Using this key for DB compatibility
            }
            
        except Exception as e:
            print(f"DEBUG: SoundSlice transcription error: {e}")
            import traceback
            traceback.print_exc()
            return None

    def transcribe_image_gemini_wrapped(self, image_path, metadata=None):
        """Wrapper for transcribe_image_gemini to return the expected dictionary format"""
        valid_json = self.transcribe_image_gemini(image_path, metadata=metadata)
        if not valid_json: return None
        return {
            'reference_data': valid_json,
            'audiveris_raw_output': json.dumps(valid_json, indent=2)
        }
    
    def transcribe_image_audiveris(self, image_path):
        """
        Use Audiveris OMR system to transcribe sheet music image to JSON.
        Audiveris is an open-source Optical Music Recognition system that outputs MusicXML.
        """
        try:
            from music21 import converter
            from analysis.audio_analyzer import AudioAnalyzer
            
            # Check if Audiveris is available
            # Audiveris can be run via command line: java -jar audiveris.jar -batch -input <input> -output <output>
            # Or if installed as application: audiveris -batch -input <input> -output <output>
            audiveris_cmd = os.getenv('AUDIVERIS_CMD', 'audiveris')
            
            # Try to find Audiveris
            try:
                result = subprocess.run(['which', audiveris_cmd], capture_output=True, text=True)
                if result.returncode != 0:
                    # Try java -jar approach
                    audiveris_jar = os.getenv('AUDIVERIS_JAR', None)
                    if audiveris_jar and Path(audiveris_jar).exists():
                        audiveris_cmd = f"java -jar {audiveris_jar}"
                    else:
                        print("DEBUG: Audiveris not found. Please install Audiveris or set AUDIVERIS_CMD or AUDIVERIS_JAR environment variable.")
                        return None
            except Exception:
                print("DEBUG: Could not check for Audiveris installation.")
                return None
            
            # Create temporary directories for input and output
            with tempfile.TemporaryDirectory() as temp_dir:
                input_dir = Path(temp_dir) / 'input'
                output_dir = Path(temp_dir) / 'output'
                input_dir.mkdir()
                output_dir.mkdir()
                
                # Copy image to input directory
                image_name = Path(image_path).stem
                image_ext = Path(image_path).suffix or '.png'
                temp_image_path = input_dir / f"{image_name}{image_ext}"
                
                # Copy the image file
                shutil.copy2(image_path, temp_image_path)
                
                print(f"DEBUG: Running Audiveris OMR on {image_path}...")
                
                # Run Audiveris in batch mode
                # Audiveris command: audiveris -batch -input <input_dir> -output <output_dir>
                cmd_parts = audiveris_cmd.split()
                cmd_parts.extend(['-batch', '-input', str(input_dir), '-output', str(output_dir)])
                
                try:
                    result = subprocess.run(
                        cmd_parts,
                        capture_output=True,
                        text=True,
                        timeout=300  # 5 minute timeout
                    )
                    
                    if result.returncode != 0:
                        print(f"DEBUG: Audiveris returned error code {result.returncode}")
                        print(f"DEBUG: Audiveris stderr: {result.stderr[:500]}")
                        return None
                    
                    print(f"DEBUG: Audiveris completed successfully")
                    
                except subprocess.TimeoutExpired:
                    print("DEBUG: Audiveris timed out after 5 minutes")
                    return None
                except FileNotFoundError:
                    print("DEBUG: Audiveris command not found. Please install Audiveris.")
                    return None
                
                # Find the output MusicXML file
                # Audiveris typically outputs to <output_dir>/<image_name>.mxl or .musicxml
                musicxml_files = list(output_dir.glob(f"{image_name}*.mxl")) + \
                                list(output_dir.glob(f"{image_name}*.musicxml")) + \
                                list(output_dir.glob(f"{image_name}*.xml"))
                
                if not musicxml_files:
                    # Check subdirectories (Audiveris might create project folders)
                    for subdir in output_dir.iterdir():
                        if subdir.is_dir():
                            musicxml_files = list(subdir.glob("*.mxl")) + \
                                           list(subdir.glob("*.musicxml")) + \
                                           list(subdir.glob("*.xml"))
                            if musicxml_files:
                                break
                
                if not musicxml_files:
                    print("DEBUG: Audiveris did not produce MusicXML output file.")
                    print(f"DEBUG: Output directory contents: {list(output_dir.iterdir())}")
                    return None
                
                musicxml_file = musicxml_files[0]
                print(f"DEBUG: Found MusicXML file: {musicxml_file}")
                
                # Read MusicXML content
                with open(musicxml_file, 'r', encoding='utf-8') as f:
                    musicxml_content = f.read()
                
                print(f"DEBUG: Audiveris raw output length: {len(musicxml_content)} chars")
                
                # Parse MusicXML using music21
                try:
                    score = converter.parse(musicxml_content)
                    
                    # Convert to our JSON format using existing extract_reference_data method
                    audio_analyzer = AudioAnalyzer()
                    reference_data = audio_analyzer.extract_reference_data(score)
                    
                    # Convert to expected format with clef, key, timeSignature, notes
                    # The extract_reference_data returns notes with different structure
                    # We need to convert it to match the expected format
                    converted_json = self._convert_music21_to_music_json(score, reference_data)
                    
                    # Return both converted JSON and raw MusicXML output
                    return {
                        'reference_data': converted_json,
                        'audiveris_raw_output': musicxml_content
                    }
                    
                except Exception as e:
                    print(f"DEBUG: Error parsing MusicXML with music21: {e}")
                    import traceback
                    traceback.print_exc()
                    return None
                
        except Exception as e:
            print(f"Error calling Audiveris OMR for transcription: {e}")
            import traceback
            traceback.print_exc()
            return None
    
    def transcribe_image_oemer(self, image_path, pre_extracted_metadata=None):
        """
        Use oemer OMR system to transcribe sheet music image to JSON.
        If pre_extracted_metadata is provided, it uses it for time signature.
        """
        try:
            from music21 import converter
            from analysis.audio_analyzer import AudioAnalyzer
            
            # Step 1: Get metadata (Time and Key)
            gemini_time_sig = None
            gemini_key_sig = None
            
            if pre_extracted_metadata:
                gemini_time_sig = pre_extracted_metadata.get('timeSignature')
                gemini_key_sig = pre_extracted_metadata.get('key_signature')
                print(f"DEBUG: Using pre-extracted metadata - Time: {gemini_time_sig}, Key: {gemini_key_sig}")
            else:
                # Fallback to specialized time signature detection
                print(f"DEBUG: Detecting time signature with Gemini specialized call for {image_path}...")
                gemini_time_sig = self.detect_time_signature_gemini(image_path)
            
            if not gemini_time_sig:
                print("DEBUG: Could not detect time signature with Gemini, will use oemer's default")
            
            # Step 2: Run oemer OMR
            print(f"DEBUG: Running oemer OMR on {image_path}...")
            
            # Try to use oemer as Python library first, fallback to CLI
            musicxml_content = None
            
            # Try importing oemer as a library (but oemer is primarily CLI-based)
            # Skip library approach since oemer README shows it's meant to be run as CLI: oemer <path>
            musicxml_content = None
            print("DEBUG: oemer is CLI-based, will use command-line interface...")
            
            # If library approach didn't work, use CLI
            if musicxml_content is None:
                # Create temporary directory for oemer output
                with tempfile.TemporaryDirectory() as temp_dir:
                    output_dir = Path(temp_dir)
                    
                    # oemer command: oemer <input_image>
                    # oemer outputs MusicXML to the current working directory by default
                    # We'll change to temp_dir, run oemer, then find the output
                    # Ensure image_path is absolute before changing directories
                    image_path = str(Path(image_path).absolute())
                    original_cwd = os.getcwd()
                    image_name = Path(image_path).stem
                    
                    try:
                        # Change to temp directory and run oemer
                        os.chdir(temp_dir)
                        
                        # Copy image to temp directory for oemer to process
                        temp_image_path = Path(temp_dir) / Path(image_path).name
                        shutil.copy2(image_path, temp_image_path)
                        
                        # Try to find oemer command - check multiple locations
                        oemer_cmd = None
                        use_module = False
                        
                        print(f"DEBUG: Searching for oemer command...")
                        
                        # Strategy 1: Try using oemer as Python module directly (most reliable)
                        # This works as long as oemer is installed in the current environment
                        try:
                            import oemer.ete
                            python_exe = sys.executable
                            oemer_cmd = python_exe
                            use_module = True
                            print(f"DEBUG: Strategy 1 (Python module): Found oemer. Will use: {python_exe} -m oemer.ete")
                        except ImportError:
                            print(f"DEBUG: Strategy 1 (Python module) failed. Checking CLI paths...")
                        
                        # Strategy 2: Check if we're in a venv and oemer is there
                        if not oemer_cmd:
                            venv_bin = os.environ.get('VIRTUAL_ENV')
                            print(f"DEBUG: VIRTUAL_ENV: {venv_bin}")
                            if venv_bin:
                                venv_oemer = Path(venv_bin) / 'bin' / 'oemer'
                                print(f"DEBUG: Checking Strategy 2: {venv_oemer} (exists: {venv_oemer.exists()})")
                                if venv_oemer.exists():
                                    oemer_cmd = str(venv_oemer)
                                    print(f"DEBUG: Found oemer in venv (VIRTUAL_ENV): {oemer_cmd}")
                        
                        # Strategy 3: Try to find it relative to current Python executable
                        # Check if Python is in a venv/bin directory
                        if not oemer_cmd:
                            python_exe = sys.executable
                            python_dir = Path(python_exe).parent
                            # Check if oemer is in the same directory as Python (venv/bin)
                            potential_oemer = python_dir / 'oemer'
                            print(f"DEBUG: Checking Strategy 3: {potential_oemer} (exists: {potential_oemer.exists()})")
                            if potential_oemer.exists():
                                oemer_cmd = str(potential_oemer)
                                print(f"DEBUG: Found oemer next to Python executable: {oemer_cmd}")
                        
                        # Strategy 4: Check common venv locations relative to script
                        if not oemer_cmd:
                            # Try to find venv relative to this file
                            script_dir = Path(__file__).absolute().parent.parent  # Go up from analysis/ to cloud-backend/
                            potential_venv_oemer = script_dir / 'venv' / 'bin' / 'oemer'
                            print(f"DEBUG: Checking Strategy 4: script_dir={script_dir}, checking {potential_venv_oemer} (exists: {potential_venv_oemer.exists()})")
                            if potential_venv_oemer.exists():
                                oemer_cmd = str(potential_venv_oemer)
                                print(f"DEBUG: Found oemer in venv relative to script: {oemer_cmd}")
                        
                        # Strategy 5: Check venv relative to current working directory
                        if not oemer_cmd:
                            cwd = Path(os.getcwd())
                            potential_venv_oemer = cwd / 'venv' / 'bin' / 'oemer'
                            print(f"DEBUG: Checking Strategy 5: cwd={cwd}, checking {potential_venv_oemer} (exists: {potential_venv_oemer.exists()})")
                            if potential_venv_oemer.exists():
                                oemer_cmd = str(potential_venv_oemer)
                                print(f"DEBUG: Found oemer in venv relative to cwd: {oemer_cmd}")
                        
                        # Strategy 6: Try 'oemer' directly (might be in PATH)
                        if not oemer_cmd:
                            try:
                                result = subprocess.run(['which', 'oemer'], capture_output=True, text=True, timeout=2)
                                print(f"DEBUG: Checking Strategy 6: which oemer returned: {result.stdout.strip() if result.returncode == 0 else 'not found'}")
                                if result.returncode == 0:
                                    oemer_cmd = result.stdout.strip()
                                    print(f"DEBUG: Found oemer in PATH: {oemer_cmd}")
                            except Exception as e:
                                print(f"DEBUG: Strategy 6 failed: {e}")
                        
                        if not oemer_cmd:
                            raise FileNotFoundError(
                                "oemer command not found. Please ensure oemer is installed: "
                                "pip install oemer (and make sure your virtual environment is activated)"
                            )
                        
                        # Run oemer - it outputs to current directory
                        if use_module:
                            cmd = [oemer_cmd, '-m', 'oemer.ete', str(temp_image_path)]
                        else:
                            cmd = [oemer_cmd, str(temp_image_path)]
                        
                        result = subprocess.run(
                            cmd,
                            capture_output=True,
                            text=True,
                            timeout=600  # 10 minute timeout (oemer can be slow)
                        )
                        
                        # Change back to original directory
                        os.chdir(original_cwd)
                        
                        if result.returncode != 0:
                            print(f"DEBUG: oemer returned error code {result.returncode}")
                            print(f"DEBUG: oemer stderr: {result.stderr[:500]}")
                            print(f"DEBUG: oemer stdout: {result.stdout[:500]}")
                            return None
                        
                        print(f"DEBUG: oemer completed successfully")
                        
                    except subprocess.TimeoutExpired:
                        os.chdir(original_cwd)
                        print("DEBUG: oemer timed out after 10 minutes")
                        return None
                    except FileNotFoundError:
                        os.chdir(original_cwd)
                        print("DEBUG: oemer command not found. Please install oemer: pip install oemer")
                        print("DEBUG: Make sure oemer is installed in your virtual environment")
                        return None
                    except Exception as e:
                        os.chdir(original_cwd)
                        print(f"DEBUG: Error running oemer: {e}")
                        import traceback
                        traceback.print_exc()
                        return None
                    
                    # Find the output MusicXML file
                    # oemer outputs <image_name>.musicxml or <image_name>.xml in the temp directory
                    musicxml_files = list(output_dir.glob(f"{image_name}*.musicxml")) + \
                                    list(output_dir.glob(f"{image_name}*.xml")) + \
                                    list(output_dir.glob("*.musicxml")) + \
                                    list(output_dir.glob("*.xml"))
                    
                    if not musicxml_files:
                        print("DEBUG: oemer did not produce MusicXML output file.")
                        print(f"DEBUG: Output directory contents: {list(output_dir.iterdir())}")
                        return None
                    
                    musicxml_file = musicxml_files[0]
                    print(f"DEBUG: Found MusicXML file: {musicxml_file}")
                    
                    # Read MusicXML content
                    with open(musicxml_file, 'r', encoding='utf-8') as f:
                        musicxml_content = f.read()
            
            if not musicxml_content:
                print("DEBUG: Failed to get MusicXML content from oemer")
                return None
            
            print(f"DEBUG: oemer raw output length: {len(musicxml_content)} chars")
            
            # Step 3: Apply Gemini metadata to MusicXML if detected
            if gemini_time_sig:
                print(f"DEBUG: Applying Gemini time signature {gemini_time_sig} to oemer output...")
                musicxml_content = self._apply_time_signature_to_musicxml(musicxml_content, gemini_time_sig)
            
            if gemini_key_sig:
                print(f"DEBUG: Applying Gemini key signature {gemini_key_sig} to oemer output...")
                musicxml_content = self._apply_key_signature_to_musicxml(musicxml_content, gemini_key_sig)
            
            # Step 4: Parse MusicXML using music21
            try:
                score = converter.parse(musicxml_content)
                
                # Convert to our JSON format using existing extract_reference_data method
                audio_analyzer = AudioAnalyzer()
                reference_data = audio_analyzer.extract_reference_data(score)
                
                # Convert to expected format with clef, key, timeSignature, notes
                converted_json = self._convert_music21_to_music_json(score, reference_data, pre_extracted_metadata)
                
                # Return both converted JSON and raw MusicXML output
                # Using 'audiveris_raw_output' key for backward compatibility with database schema
                return {
                    'reference_data': converted_json,
                    'audiveris_raw_output': musicxml_content  # Actually oemer output, but using same key for DB compatibility
                }
                
            except Exception as e:
                print(f"DEBUG: Error parsing MusicXML with music21: {e}")
                import traceback
                traceback.print_exc()
                return None
                
        except Exception as e:
            print(f"Error calling oemer OMR for transcription: {e}")
            import traceback
            traceback.print_exc()
            return None
    
    def _apply_key_signature_to_musicxml(self, musicxml_content, key_sig):
        """
        Apply the Gemini-detected key signature to MusicXML content.
        """
        import re
        
        # Mapping of key names to fifths
        KEY_TO_FIFTHS = {
            'C major': 0, 'G major': 1, 'D major': 2, 'A major': 3, 'E major': 4, 'B major': 5, 'F# major': 6, 'C# major': 7,
            'F major': -1, 'Bb major': -2, 'Eb major': -3, 'Ab major': -4, 'Db major': -5, 'Gb major': -6, 'Cb major': -7,
            'A minor': 0, 'E minor': 1, 'B minor': 2, 'F# minor': 3, 'C# minor': 4, 'G# minor': 5, 'D# minor': 6, 'A# minor': 7,
            'D minor': -1, 'G minor': -2, 'C minor': -3, 'F minor': -4, 'Bb minor': -5, 'Eb minor': -6, 'Ab minor': -7
        }
        
        fifths = KEY_TO_FIFTHS.get(key_sig)
        if fifths is None:
            print(f"DEBUG: Unknown key signature for XML injection: {key_sig}")
            return musicxml_content
            
        key_pattern = r'<key[^>]*>.*?</key>'
        new_key = f'<key><fifths>{fifths}</fifths></key>'
        
        if re.search(key_pattern, musicxml_content, re.DOTALL):
            musicxml_content = re.sub(key_pattern, new_key, musicxml_content, flags=re.DOTALL)
            print(f"DEBUG: Replaced existing key signature with {key_sig} ({fifths} fifths)")
        else:
            # Add after clef or at start of attributes
            if '</clef>' in musicxml_content:
                musicxml_content = musicxml_content.replace('</clef>', f'</clef>\n        {new_key}')
            elif '<attributes>' in musicxml_content:
                musicxml_content = musicxml_content.replace('<attributes>', f'<attributes>\n        {new_key}', 1)
            print(f"DEBUG: Added new key signature {key_sig} to MusicXML")
            
        return musicxml_content

    def _apply_time_signature_to_musicxml(self, musicxml_content, time_sig):
        """
        Apply the Gemini-detected time signature to MusicXML content.
        Replaces or adds time signature in the MusicXML.
        """
        import re
        
        # Parse time signature (e.g., "4/4" -> numerator=4, denominator=4)
        parts = time_sig.split('/')
        if len(parts) != 2:
            print(f"DEBUG: Invalid time signature format: {time_sig}")
            return musicxml_content
        
        numerator = parts[0]
        denominator = parts[1]
        
        # Find and replace existing time signature
        # MusicXML time signature format: <time><beats>4</beats><beat-type>4</beat-type></time>
        time_sig_pattern = r'<time[^>]*>.*?</time>'
        new_time_sig = f'<time><beats>{numerator}</beats><beat-type>{denominator}</beat-type></time>'
        
        # Try to replace existing time signature
        if re.search(time_sig_pattern, musicxml_content, re.DOTALL):
            musicxml_content = re.sub(time_sig_pattern, new_time_sig, musicxml_content, flags=re.DOTALL)
            print(f"DEBUG: Replaced existing time signature with {time_sig}")
        else:
            # If no time signature found, add it after the key signature or clef
            # Look for </key> or </clef> and insert time signature after it
            if '</key>' in musicxml_content:
                musicxml_content = musicxml_content.replace('</key>', f'</key>\n        {new_time_sig}')
            elif '</clef>' in musicxml_content:
                musicxml_content = musicxml_content.replace('</clef>', f'</clef>\n        {new_time_sig}')
            else:
                # Fallback: add after first <attributes> tag
                if '<attributes>' in musicxml_content:
                    musicxml_content = musicxml_content.replace(
                        '<attributes>',
                        f'<attributes>\n        {new_time_sig}',
                        1
                    )
            print(f"DEBUG: Added new time signature {time_sig} to MusicXML")
        
        return musicxml_content
    
    def _convert_music21_to_music_json(self, score, reference_data, gemini_metadata=None):
        """Convert music21 score and reference_data to our expected JSON format"""
        from music21 import clef, key, meter
        
        # Use Gemini metadata as primary source if available
        clef_name = "treble"
        key_name = "C major"
        time_name = "4/4"
        
        if gemini_metadata:
            clef_name = gemini_metadata.get('clef', clef_name)
            key_name = gemini_metadata.get('key_signature', gemini_metadata.get('key', key_name))
            time_name = gemini_metadata.get('timeSignature', time_name)
        else:
            # Fallback to score extraction
            clefs = score.flat.getElementsByClass(clef.Clef)
            if clefs:
                clef_obj = clefs[0]
                if isinstance(clef_obj, clef.TrebleClef): clef_name = "treble"
                elif isinstance(clef_obj, clef.BassClef): clef_name = "bass"
                
            keys = score.flat.getElementsByClass(key.KeySignature)
            if keys:
                key_name = str(keys[0])
                
            time_sigs = score.flat.getElementsByClass(meter.TimeSignature)
            if time_sigs:
                ts = time_sigs[0]
                time_name = f"{ts.numerator}/{ts.denominator}"

        result = {
            "clef": clef_name,
            "key_signature": key_name,
            "timeSignature": time_name,
            "notes": []
        }
        
        # Convert notes to pitch/duration/startBeat/timestamp
        current_beat = 1.0
        for note_data in reference_data.get('notes', []):
            pitch = note_data.get('note', 'C4')
            duration_quarters = note_data.get('duration', 1.0)
            
            result["notes"].append({
                "pitch": pitch,
                "note": pitch, # For AudioAnalyzer compatibility
                "duration": duration_quarters,
                "startBeat": current_beat,
                "timestamp": note_data.get('timestamp', 0) # For AudioAnalyzer compatibility
            })
            current_beat += duration_quarters
        
        return result

    def transcribe_image_gemini(self, image_path, metadata=None):
        """Use Gemini 1.5 Pro to transcribe notes with metadata context"""
        # Use Pro model for notes if available, otherwise fallback to standard model
        model = self.model_pro if self.model_pro else self.model
        if not model: return None
        
        from PIL import Image
        try:
            image = Image.open(image_path)
            if not metadata:
                metadata = self.verify_metadata(image_path)
            
            clef = metadata.get('clef', 'treble')
            key_sig = metadata.get('key_signature', metadata.get('key', 'C major'))
            time_sig = metadata.get('timeSignature', '4/4')
            
            prompt = f"""You are a Professional Music Transcriptionist using Chain-of-Thought reasoning.
Your task is to transcribe sheet music into machine-readable JSON.

CONTEXT:
- Clef: {clef}
- Key Signature: {key_sig}
- Time Signature: {time_sig}

INSTRUCTIONS:
1. Analyze the image MEASURE BY MEASURE, from left to right.
2. For EACH measure, first describe what you see (notes, rhythms, accidentals) in the "reasoning" field.
3. Then, extract the exact notes into the "notes" list.
4. Pitch: Use scientific pitch notation (e.g., C4, F#5, Bb3).
5. Duration: Quarter=1.0, Eighth=0.5, Half=2.0, Whole=4.0, Sixteenth=0.25 (Dotted = x 1.5).

REQUIRED JSON STRUCTURE:
{{
  "reasoning": "In measure 1, I see a treble clef. The first note is a quarter note G4...",
  "notes": [
    {{"pitch": "G4", "duration": 1.0, "startBeat": 1.0}},
    {{"pitch": "A4", "duration": 0.5, "startBeat": 2.0}}
  ]
}}

STAFF POSITIONS ({clef}):
{"Lines: G2 B2 D3 F3 A3; Spaces: A2 C3 E3 G3" if "bass" in clef.lower() else "Lines: E4 G4 B4 D5 F5; Spaces: F4 A4 C5 E5"}

CRITICAL: Return ONLY valid JSON. Verify your reasoning against the visual evidence before outputting notes."""
            
            # Use JSON mode for guaranteed structured output
            generation_config = {"response_mime_type": "application/json"}
            response = model.generate_content(
                [image, prompt],
                generation_config=generation_config
            )
            # print(f"DEBUG: GEMINI 1.5 PRO RAW RESPONSE: {response.text}")
            notes_data = self._parse_json_response(response.text)
            
            if notes_data:
                # Handle case where Gemini returns a list directly instead of {"notes": [...]}
                if isinstance(notes_data, list):
                    print(f"DEBUG: Gemini returned a LIST of {len(notes_data)} items")
                    extracted_notes = notes_data
                elif isinstance(notes_data, dict):
                    print(f"DEBUG: Parsed JSON keys: {list(notes_data.keys())}")
                    extracted_notes = notes_data.get('notes', [])
                else:
                    print(f"DEBUG: Unexpected JSON type: {type(notes_data)}")
                    extracted_notes = []
            else:
                print("DEBUG: _parse_json_response returned None/Empty")
                extracted_notes = []

            result = {
                "clef": clef,
                "key_signature": key_sig,
                "timeSignature": time_sig,
                "notes": []
            }
            
            for n in extracted_notes:
                if isinstance(n, dict):
                    n["note"] = n.get("pitch", "C4")
                    result["notes"].append(n)
            
            print(f"DEBUG: Final result contains {len(result['notes'])} notes")
            return result
        except Exception as e:
            print(f"DEBUG: Gemini extraction error: {e}")
            return None
    
    def transcribe_image_roboflow(self, image_path):
        """
        Use Roboflow workflow to transcribe sheet music image to JSON.
        
        NOTE: Workflows only work on serverless endpoint, but serverless doesn't support
        classification models. If your workflow contains classification models, you may need to:
        1. Use individual model inference instead of workflows
        2. Modify your workflow to remove classification models
        3. Rely on Gemini fallback (which is working)
        """
        if not self.roboflow_api_key:
            print("DEBUG: Roboflow API key not configured.")
            return None
        
        try:
            from inference_sdk import InferenceHTTPClient
            
            print(f"DEBUG: Initializing Roboflow client for image {image_path}...")
            
            # Workflows only work on serverless endpoint
            # Use configured endpoint or default to serverless
            api_url = self.roboflow_api_url or "https://serverless.roboflow.com"
            
            print(f"DEBUG: Using endpoint: {api_url}")
            client = InferenceHTTPClient(
                api_url=api_url,
                api_key=self.roboflow_api_key
            )
            
            print(f"DEBUG: Running Roboflow workflow '{self.roboflow_workflow_id}'...")
            
            # Run the workflow
            result = client.run_workflow(
                workspace_name=self.roboflow_workspace,
                workflow_id=self.roboflow_workflow_id,
                images={
                    "image": image_path  # Path to your image file
                },
                use_cache=True  # Speeds up repeated requests
            )
            
            print(f"DEBUG: Received response from Roboflow. Type: {type(result)}")
            print(f"DEBUG: Response keys: {result.keys() if isinstance(result, dict) else 'Not a dict'}")
            
            # Convert Roboflow result to our expected format
            # The exact format depends on what your workflow returns
            # This is a template - you may need to adjust based on actual response
            return self._convert_roboflow_to_music_json(result)
            
        except ImportError:
            print("DEBUG: inference-sdk not installed. Install with: pip install inference-sdk")
            return None
        except Exception as e:
            error_msg = str(e)
            if "Model type not supported" in error_msg:
                print(f"DEBUG: Your workflow contains unsupported model types for serverless endpoint.")
                print(f"DEBUG: Consider using individual models or modifying your workflow.")
            print(f"Error calling Roboflow API for transcription: {e}")
            import traceback
            traceback.print_exc()
            return None
    
    def _convert_roboflow_to_music_json(self, roboflow_result):
        """
        Convert Roboflow workflow result to our expected music JSON format.
        Adjust this based on your actual Roboflow workflow output.
        """
        # This is a placeholder - adjust based on your actual Roboflow output format
        # Common Roboflow outputs include predictions, classifications, etc.
        
        if not isinstance(roboflow_result, dict):
            print(f"DEBUG: Unexpected Roboflow result type: {type(roboflow_result)}")
            return None
        
        # Try to extract structured data from Roboflow response
        # Your workflow might return different keys - adjust accordingly
        result = {
            "clef": "treble",  # Default, should be extracted from Roboflow
            "key_signature": "C major",  # Standardized
            "timeSignature": "4/4",  # Default, should be extracted from Roboflow
            "notes": []
        }
        
        # Example: if Roboflow returns predictions with note information
        # Adjust this parsing based on your actual workflow output
        if 'predictions' in roboflow_result:
            # Process predictions to extract notes
            # This is a template - customize based on your workflow
            pass
        elif 'results' in roboflow_result:
            # Alternative key name
            pass
        
        # For now, return a basic structure
        # You'll need to map your Roboflow output to this format
        print(f"DEBUG: Converted Roboflow result. Keys: {result.keys()}")
        return result
    
    def _parse_json_response(self, text):
        """Helper to extract and parse JSON from Gemini response"""
        import re
        import json
        
        # 1. Try direct JSON parsing first (Since we use response_mime_type="application/json")
        try:
            return json.loads(text)
        except json.JSONDecodeError:
            pass
            
        # 2. Try to find markdown code block
        json_block = re.search(r'```json\s*(.*?)\s*```', text, re.DOTALL)
        if json_block:
            json_str = json_block.group(1)
        else:
            # 3. Fallback to finding first { and last }
            json_match = re.search(r'\{.*\}', text, re.DOTALL)
            if json_match:
                json_str = json_match.group(0)
            else:
                json_str = text

        try:
            return json.loads(json_str)
        except json.JSONDecodeError as e:
            print(f"DEBUG: JSON parse error in _parse_json_response: {e}")
            # print(f"DEBUG: Failed text was: {text}") # Uncomment if deep debugging needed
            return None
    
    def analyze_performance_with_audio(self, audio_url, reference_data):
        """
        Analyze a recorded performance against sheet music using Gemini.
        
        Args:
            audio_url: URL to the recorded audio file
            reference_data: MusicXML string or parsed reference data from sheet music
            
        Returns:
            String containing AI-generated feedback
        """
        if not self.model:
            return "AI analysis not available - Gemini API key not configured."
        
        import requests
        import tempfile
        import os
        
        try:
            # Download the audio file
            print(f"DEBUG: Downloading audio from {audio_url}")
            response = requests.get(audio_url, timeout=30)
            response.raise_for_status()
            
            # Save to temporary file
            with tempfile.NamedTemporaryFile(suffix='.wav', delete=False) as temp_audio:
                temp_audio.write(response.content)
                temp_audio_path = temp_audio.name
            
            try:
                # Upload audio file to Gemini
                audio_file = genai.upload_file(temp_audio_path, mime_type='audio/wav')
                
                # Prepare the prompt
                if isinstance(reference_data, str):
                    # It's MusicXML string
                    sheet_info = f"Reference sheet music (MusicXML):\n{reference_data[:2000]}..."
                else:
                    # It's a dict
                    sheet_info = f"Reference sheet music:\n{json.dumps(reference_data, indent=2)[:2000]}..."
                
                prompt = f"""You are an expert music coach analyzing a student's performance.

I'm providing you with:
1. An audio recording of the student playing
2. The reference sheet music they were supposed to play

{sheet_info}

Please analyze the performance and provide feedback:

1. **Overall Assessment** (1-2 sentences): How well did they follow the sheet music?

2. **What went well** (2-3 bullet points): Positive aspects of the performance

3. **Areas to improve** (2-3 bullet points): Specific things to work on

4. **Practice suggestions** (1-2 actionable tips)

Keep your response concise (under 200 words) and encouraging. Focus on the most important feedback.
"""
                
                # Generate analysis
                response = self.model.generate_content([audio_file, prompt])
                feedback = response.text
                
                return feedback
                
            finally:
                # Clean up temp file
                if os.path.exists(temp_audio_path):
                    os.remove(temp_audio_path)
                    
        except requests.exceptions.RequestException as e:
            print(f"Error downloading audio: {e}")
            return f"Could not download audio file: {str(e)}"
        except Exception as e:
            print(f"Error in analyze_performance_with_audio: {e}")
            import traceback
            traceback.print_exc()
            return f"Analysis error: {str(e)}"

    def analyze_midi_vs_sheet(self, midi_url, musicxml_reference):
        """
        Analyze a MIDI performance against MusicXML sheet music using Gemini 2.5 Flash.
        
        Args:
            midi_url: URL to the MIDI file in Supabase storage (midis bucket)
            musicxml_reference: MusicXML string from sheet_music.audiveris_raw_output
            
        Returns:
            JSON string containing structured feedback with score (0-100) and 6 feedback points.
            Format: {"score": 85, "feedback_points": [{"text": "...", "is_important": true}, ...]}
        """
        if not self.model_flash:
            default_feedback = {
                "score": 50,
                "strength": "Good effort attempting the piece!",
                "improvement": "AI analysis not available - please configure API key.",
                "feedback_points": [
                    {"text": "Good effort attempting the piece.", "is_important": True},
                    {"text": "AI analysis not available - Gemini API key not configured.", "is_important": True},
                    {"text": "Please configure the API key to get detailed feedback.", "is_important": False},
                    {"text": "Continue practicing regularly.", "is_important": False},
                    {"text": "Focus on accuracy and timing.", "is_important": False},
                    {"text": "Work on maintaining consistent tempo.", "is_important": False}
                ]
            }
            return json.dumps(default_feedback)
        
        try:
            # Download the MIDI file
            print(f"DEBUG: Downloading MIDI from {midi_url}")
            response = requests.get(midi_url, timeout=30)
            response.raise_for_status()
            
            # Save to temporary file
            with tempfile.NamedTemporaryFile(suffix='.mid', delete=False) as temp_midi:
                temp_midi.write(response.content)
                temp_midi_path = temp_midi.name
            
            print(f"DEBUG: MIDI file downloaded, size: {len(response.content)} bytes")
            
            try:
                # Upload MIDI file to Gemini
                midi_file = genai.upload_file(temp_midi_path, mime_type='audio/midi')
                
                # Truncate MusicXML if too long (keep first 8000 chars for context)
                musicxml_truncated = musicxml_reference[:8000] if musicxml_reference else "No reference available"
                if len(musicxml_reference or '') > 8000:
                    musicxml_truncated += "\n... [truncated]"
                
                prompt = f"""You are a music teacher analyzing a student's MIDI performance against sheet music.

REFERENCE SHEET MUSIC (MusicXML):
{musicxml_truncated}

TASK: Compare the student's MIDI performance (attached) against the reference sheet music above.

You MUST respond with a JSON object in this EXACT format:

{{
  "score": 85,
  "strength": "Your rhythm and timing were excellent throughout the piece, showing good musical understanding.",
  "improvement": "Work on the F# in measure 3 - you played F natural. Focus on reading accidentals carefully.",
  "feedback_points": [
    {{"text": "Excellent rhythm and timing throughout.", "is_important": true}},
    {{"text": "Work on the F# in measure 3.", "is_important": true}},
    {{"text": "Good dynamics in the middle section.", "is_important": true}},
    {{"text": "Nice phrasing in measures 5-8.", "is_important": false}},
    {{"text": "Consider working on finger technique.", "is_important": false}},
    {{"text": "Overall tempo was consistent.", "is_important": false}}
  ]
}}

REQUIREMENTS:
1. score: Integer from 0-100 (81-100: excellent, 61-80: good, 41-60: decent, 21-40: needs work, 0-20: major difficulties)
2. strength: ONE sentence describing the best aspect of their performance (what they did well)
3. improvement: ONE sentence describing the most important thing to work on (specific and actionable)
4. feedback_points: EXACTLY 6 feedback points for detailed view

The "strength" and "improvement" fields are CRITICAL - they will be displayed on a small screen device.
Keep them concise (under 100 characters each) but specific.

Return ONLY valid JSON, no other text."""

                # Use Gemini 2.5 Flash with JSON mode for structured output
                generation_config = {"response_mime_type": "application/json"}
                response = self.model_flash.generate_content(
                    [midi_file, prompt],
                    generation_config=generation_config
                )
                feedback_json = response.text.strip()
                
                # Parse and validate the JSON response
                try:
                    feedback_data = json.loads(feedback_json)
                    
                    # Validate structure
                    if 'score' not in feedback_data:
                        raise ValueError("Missing required score field")
                    
                    score = int(feedback_data['score'])
                    if not (0 <= score <= 100):
                        print(f"DEBUG: Score {score} out of range, clamping to 0-100")
                        score = max(0, min(100, score))
                        feedback_data['score'] = score
                    
                    # Ensure strength and improvement are present
                    if 'strength' not in feedback_data or not feedback_data['strength']:
                        feedback_data['strength'] = "Good effort on this performance!"
                    if 'improvement' not in feedback_data or not feedback_data['improvement']:
                        feedback_data['improvement'] = "Continue practicing to improve accuracy."
                    
                    # Handle feedback_points if present
                    feedback_points = feedback_data.get('feedback_points', [])
                    if len(feedback_points) < 6:
                        print(f"DEBUG: Expected 6 feedback points, got {len(feedback_points)}, padding...")
                        while len(feedback_points) < 6:
                            feedback_points.append({"text": "Continue practicing regularly.", "is_important": False})
                        feedback_data['feedback_points'] = feedback_points
                    elif len(feedback_points) > 6:
                        feedback_points = feedback_points[:6]
                        feedback_data['feedback_points'] = feedback_points
                    
                    # Ensure exactly 3 are marked as important
                    important_count = sum(1 for fp in feedback_points if fp.get('is_important', False))
                    if important_count != 3:
                        print(f"DEBUG: Expected 3 important points, got {important_count}, adjusting...")
                        for i, fp in enumerate(feedback_points):
                            fp['is_important'] = (i < 3)
                    
                    print(f"DEBUG: Gemini feedback generated - Score: {score}/100, strength: {feedback_data['strength'][:50]}...")
                    return json.dumps(feedback_data)
                    
                except json.JSONDecodeError as e:
                    print(f"DEBUG: Failed to parse JSON response: {e}")
                    print(f"DEBUG: Raw response: {feedback_json[:500]}")
                    # Return default structured response
                    default_feedback = {
                        "score": 50,
                        "strength": "Good effort on this piece!",
                        "improvement": "Continue practicing to improve accuracy.",
                        "feedback_points": [
                            {"text": "Good effort on this piece.", "is_important": True},
                            {"text": "Continue practicing to improve accuracy.", "is_important": True},
                            {"text": "Work on maintaining consistent tempo.", "is_important": True},
                            {"text": "Focus on reading the sheet music carefully.", "is_important": False},
                            {"text": "Practice difficult sections slowly.", "is_important": False},
                            {"text": "Keep up the regular practice routine.", "is_important": False}
                        ]
                    }
                    return json.dumps(default_feedback)
                
            finally:
                # Cleanup temp file
                if os.path.exists(temp_midi_path):
                    os.unlink(temp_midi_path)
                    
        except requests.exceptions.RequestException as e:
            print(f"Error downloading MIDI: {e}")
            error_feedback = {
                "score": 0,
                "strength": "Recording uploaded successfully!",
                "improvement": "Could not download MIDI file. Please try again.",
                "feedback_points": [
                    {"text": "Recording uploaded successfully.", "is_important": True},
                    {"text": f"Could not download MIDI file for analysis: {str(e)[:80]}", "is_important": True},
                    {"text": "Please try uploading again.", "is_important": True},
                    {"text": "Check your internet connection.", "is_important": False},
                    {"text": "Ensure the recording was processed correctly.", "is_important": False},
                    {"text": "Contact support if the issue persists.", "is_important": False}
                ]
            }
            return json.dumps(error_feedback)
        except Exception as e:
            print(f"Error in analyze_midi_vs_sheet: {e}")
            import traceback
            traceback.print_exc()
            error_feedback = {
                "score": 0,
                "strength": "Recording uploaded successfully!",
                "improvement": f"Analysis error occurred. Please try again.",
                "feedback_points": [
                    {"text": "Recording uploaded successfully.", "is_important": True},
                    {"text": f"Analysis error: {str(e)[:80]}", "is_important": True},
                    {"text": "Please try again later.", "is_important": True},
                    {"text": "The system encountered an unexpected error.", "is_important": False},
                    {"text": "Your recording is saved and can be analyzed later.", "is_important": False},
                    {"text": "Contact support if the issue persists.", "is_important": False}
                ]
            }
            return json.dumps(error_feedback)

    def generate_feedback(self, mistakes, reference_data, metadata):
        """
        Generate coaching feedback based on detected mistakes
        
        Args:
            mistakes: List of detected mistakes
            reference_data: Reference music data
            metadata: Performance metadata
        
        Returns:
            Dictionary with feedback text and structured suggestions
        """
        if not self.model:
            return self._generate_fallback_feedback(mistakes)
        
        # Prepare context for AI
        context = self._prepare_context(mistakes, reference_data, metadata)
        
        try:
            response = self.model.generate_content(context)
            feedback_text = response.text
            
            return {
                'text': feedback_text,
                'mistakes_count': len(mistakes),
                'suggestions': self._extract_suggestions(mistakes)
            }
        
        except Exception as e:
            print(f"Error calling Gemini API: {e}")
            return self._generate_fallback_feedback(mistakes)
    
    def _prepare_context(self, mistakes, reference_data, metadata):
        """Prepare context prompt for AI"""
        mistakes_summary = self._summarize_mistakes(mistakes)
        
        context = f"""You are an encouraging music teacher. A student just played a piece.

MISTAKES:
{json.dumps(mistakes_summary, indent=2)}

PIECE INFO:
- Tempo: {metadata.get('tempo', 'Unknown')} BPM
- Key: {reference_data.get('key_signature', 'Unknown')}
- Total Notes: {len(reference_data.get('notes', []))}

Give feedback (max 300 words) that:
1. Starts positive
2. Groups similar mistakes
3. Mentions specific measures
4. Suggests marking the sheet music
5. Ends encouragingly

Be conversational and supportive."""

        return context
    
    def _summarize_mistakes(self, mistakes):
        """Summarize mistakes by type for AI context"""
        summary = {
            'note_accuracy': [],
            'timing': [],
            'dynamics': []
        }
        
        for mistake in mistakes:
            mistake_type = mistake.get('type', 'unknown')
            
            if mistake_type == 'note_accuracy':
                summary['note_accuracy'].append({
                    'measure': mistake.get('measure', '?'),
                    'expected': mistake.get('expected'),
                    'played': mistake.get('played'),
                    'timestamp': mistake.get('timestamp')
                })
            elif mistake_type in ['hesitation', 'rushing', 'tempo_deviation']:
                summary['timing'].append({
                    'type': mistake_type,
                    'timestamp': mistake.get('timestamp'),
                    'details': {k: v for k, v in mistake.items() if k not in ['type', 'timestamp']}
                })
            elif mistake_type == 'dynamics':
                summary['dynamics'].append({
                    'measure': mistake.get('measure', '?'),
                    'marking': mistake.get('marking'),
                    'timestamp': mistake.get('timestamp')
                })
        
        return summary
    
    def _extract_suggestions(self, mistakes):
        """Extract actionable suggestions from mistakes"""
        suggestions = []
        
        for mistake in mistakes:
            if mistake['type'] == 'note_accuracy':
                suggestions.append({
                    'action': 'mark_note',
                    'measure': mistake.get('measure'),
                    'message': f"Circle the {mistake.get('expected')} in measure {mistake.get('measure')}"
                })
            elif mistake['type'] == 'hesitation':
                suggestions.append({
                    'action': 'mark_timing',
                    'timestamp': mistake.get('timestamp'),
                    'message': f"Mark 'prep early' at {self._format_time(mistake.get('timestamp'))}"
                })
            elif mistake['type'] == 'dynamics':
                suggestions.append({
                    'action': 'mark_dynamic',
                    'measure': mistake.get('measure'),
                    'message': f"Emphasize the {mistake.get('marking')} marking in measure {mistake.get('measure')}"
                })
        
        return suggestions
    
    def _format_time(self, seconds):
        """Format seconds to MM:SS"""
        mins = int(seconds // 60)
        secs = int(seconds % 60)
        return f"{mins}:{secs:02d}"
    
    def _generate_fallback_feedback(self, mistakes):
        """Generate feedback without AI API (fallback)"""
        if not mistakes:
            return {
                'text': "Excellent performance! You played everything correctly. Keep up the great work!",
                'mistakes_count': 0,
                'suggestions': []
            }
        
        feedback_parts = []
        
        note_mistakes = [m for m in mistakes if m['type'] == 'note_accuracy']
        timing_mistakes = [m for m in mistakes if m['type'] in ['hesitation', 'rushing']]
        dynamics_mistakes = [m for m in mistakes if m['type'] == 'dynamics']
        
        if note_mistakes:
            feedback_parts.append(f"Found {len(note_mistakes)} note accuracy issues. Review the marked measures.")
        
        if timing_mistakes:
            feedback_parts.append(f"Detected {len(timing_mistakes)} timing issues. Practice with a metronome.")
        
        if dynamics_mistakes:
            feedback_parts.append(f"Pay attention to {len(dynamics_mistakes)} dynamic markings.")
        
        return {
            'text': " ".join(feedback_parts),
            'mistakes_count': len(mistakes),
            'suggestions': self._extract_suggestions(mistakes)
        }
    
    def chat(self, user_message):
        """
        General chat method for voice assistant
        Returns AI response as string
        """
        if not self.model:
            return "I'm sorry, the AI service is not configured."
        
        try:
            prompt = f"""You are a helpful music practice coach assistant. 
The user is asking: {user_message}

Provide a helpful, encouraging response. Keep it concise (1-2 sentences) for voice responses."""
            
            response = self.model_flash.generate_content(prompt)
            return response.text.strip()
        except Exception as e:
            print(f"Chat error: {e}")
            return "I'm sorry, I encountered an error. Please try again."