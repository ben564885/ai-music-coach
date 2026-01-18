"""
AI Music Coach Cloud Backend
Handles audio analysis, mistake detection, and AI coaching feedback generation
"""

from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS
import os
from dotenv import load_dotenv
import librosa
import numpy as np
from music21 import converter, note, tempo, dynamics
import json
from datetime import datetime
import whisper
import tempfile
import base64
import io
from gtts import gTTS
from analysis.audio_analyzer import AudioAnalyzer
from analysis.coach import AICoach
from auth.auth_utils import require_auth
from database.repository import RecordingRepository, SheetMusicRepository, DeviceRepository, AnalysisRepository, WiFiNetworkRepository
from database.models import Recording, SheetMusic, Analysis
from database.models import Recording, SheetMusic

load_dotenv()

app = Flask(__name__)
CORS(app)

# Increase max content length to 50MB for audio uploads
app.config['MAX_CONTENT_LENGTH'] = 50 * 1024 * 1024 

# Configuration
UPLOAD_FOLDER = 'uploads'
RECORDINGS_FOLDER = 'recordings'
os.makedirs(UPLOAD_FOLDER, exist_ok=True)
os.makedirs(RECORDINGS_FOLDER, exist_ok=True)

# Initialize components
audio_analyzer = AudioAnalyzer()
ai_coach = AICoach(
    api_key=os.getenv('GEMINI_API_KEY'),
    roboflow_api_key=os.getenv('ROBOFLOW_API_KEY')
)

# Whisper model (loaded lazily on first use)
whisper_model = None
WAKE_WORD = "Hey Tuya"

def get_whisper_model():
    """Lazy load Whisper model on first use"""
    global whisper_model
    if whisper_model is None:
        print("Loading Whisper model (first time - this may take a minute)...")
        whisper_model = whisper.load_model("base")  # Use "base" for faster inference, "small" or "medium" for better accuracy
        print("Whisper model loaded")
    return whisper_model


@app.route('/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({'status': 'healthy', 'timestamp': datetime.now().isoformat()})




@app.route('/api/auth/verify', methods=['POST'])
@require_auth
def verify_auth():
    """Verify authentication token"""
    return jsonify({
        'success': True,
        'user_id': request.user_id,
        'email': request.user_email
    })


# =============================================================================
# DEVICE MANAGEMENT ENDPOINTS
# =============================================================================

@app.route('/api/devices', methods=['GET'])
@require_auth
def get_devices():
    """Get all devices linked to the authenticated user"""
    try:
        devices = DeviceRepository.get_devices_by_user(request.user_id)
        return jsonify({
            'success': True,
            'devices': [d.to_dict() for d in devices]
        })
    except Exception as e:
        return jsonify({'error': str(e)}), 500


@app.route('/api/devices/link', methods=['POST'])
@require_auth
def link_device():
    """
    Link a device to the authenticated user's account.
    
    Expected JSON body:
    {
        "device_id": "uuid2395651a4cae9262",  // Tuya device UUID
        "name": "My PracticePod"               // Optional friendly name
    }
    """
    try:
        data = request.json
        device_id = data.get('device_id')
        name = data.get('name', 'PracticePod')
        
        if not device_id:
            return jsonify({'error': 'device_id is required'}), 400
        
        # Sanitize device_id
        import re
        device_id = re.sub(r'[^a-zA-Z0-9_-]', '', device_id)
        if not device_id:
            return jsonify({'error': 'Invalid device_id'}), 400
        
        device = DeviceRepository.link_device(device_id, request.user_id, name)
        
        return jsonify({
            'success': True,
            'device': device.to_dict(),
            'message': f'Device {device_id} linked to your account'
        })
    except Exception as e:
        print(f"Device link error: {e}")
        return jsonify({'error': str(e)}), 500


@app.route('/api/devices/<device_id>', methods=['DELETE'])
@require_auth
def unlink_device(device_id):
    """Unlink a device from the authenticated user's account"""
    try:
        success = DeviceRepository.unlink_device(device_id, request.user_id)
        
        if success:
            return jsonify({
                'success': True,
                'message': f'Device {device_id} unlinked from your account'
            })
        else:
            return jsonify({'error': 'Device not found or not owned by you'}), 404
    except Exception as e:
        return jsonify({'error': str(e)}), 500


@app.route('/api/recordings', methods=['GET'])
@require_auth
def get_recordings():
    """Get all recordings for the authenticated user"""
    try:
        recordings = RecordingRepository.get_by_user(request.user_id)
        return jsonify({
            'success': True,
            'recordings': [r.to_dict() for r in recordings]
        })
    except Exception as e:
        return jsonify({'error': str(e)}), 500


@app.route('/api/recordings/<recording_id>', methods=['GET'])
@require_auth
def get_recording(recording_id):
    """Get a specific recording"""
    try:
        recording = RecordingRepository.get_by_id(recording_id)
        if not recording:
            return jsonify({'error': 'Recording not found'}), 404
        
        # Verify ownership
        if recording.user_id != request.user_id:
            return jsonify({'error': 'Unauthorized'}), 403
        
        return jsonify({
            'success': True,
            'recording': recording.to_dict()
        })
    except Exception as e:
        return jsonify({'error': str(e)}), 500


@app.route('/api/recordings/<recording_id>', methods=['DELETE'])
@require_auth
def delete_recording(recording_id):
    """Delete a recording"""
    try:
        recording = RecordingRepository.get_by_id(recording_id)
        if not recording:
            return jsonify({'error': 'Recording not found'}), 404
        
        # Verify ownership
        if recording.user_id != request.user_id:
            return jsonify({'error': 'Unauthorized'}), 403
        
        RecordingRepository.delete(recording_id)
        return jsonify({'success': True})
    except Exception as e:
        return jsonify({'error': str(e)}), 500


@app.route('/api/recordings/<recording_id>', methods=['PATCH'])
@require_auth
def update_recording(recording_id):
    """Update a recording (e.g., rename)"""
    try:
        recording = RecordingRepository.get_by_id(recording_id)
        if not recording:
            return jsonify({'error': 'Recording not found'}), 404
        
        # Verify ownership
        if recording.user_id != request.user_id:
            return jsonify({'error': 'Unauthorized'}), 403
        
        data = request.json or {}
        updates = {}
        
        # Only allow updating specific fields
        if 'title' in data:
            updates['title'] = data['title']
        
        if not updates:
            return jsonify({'error': 'No valid fields to update'}), 400
        
        updated = RecordingRepository.update(recording_id, updates)
        return jsonify({
            'success': True,
            'recording': updated.to_dict() if updated else None
        })
    except Exception as e:
        return jsonify({'error': str(e)}), 500


@app.route('/api/firmware/upload', methods=['POST'])
def upload_firmware_recording():
    """
    Handle raw audio upload from firmware.
    
    Looks up the device_id in the devices table to find the associated user_id,
    then saves the recording to that user's account.
    """
    print(f"--- Incoming Firmware Upload ---")
    print(f"Headers: {dict(request.headers)}")
    print(f"Content Length: {request.content_length}")
    
    try:
        device_id = request.headers.get('X-User-ID', 'unknown')
        
        # Sanitize device_id - only allow alphanumeric, dash, underscore
        import re
        device_id = re.sub(r'[^a-zA-Z0-9_-]', '', device_id)
        if not device_id:
            device_id = 'firmware_device'
        device_id = device_id[:50]
        
        print(f"Sanitized device_id: {device_id}")
        
        # =================================================================
        # DEVICE -> USER LOOKUP
        # Look up the user_id associated with this device
        # =================================================================
        user_id = DeviceRepository.get_user_by_device(device_id)
        
        if user_id:
            print(f"Device '{device_id}' is linked to user '{user_id}'")
            # Update last upload timestamp
            DeviceRepository.update_last_upload(device_id)
        else:
            print(f"Device '{device_id}' is NOT linked to any user")
            # Device not registered - still accept upload but won't save to DB
        
        # Generate title with date/time format
        now = datetime.now()
        formatted_time = now.strftime('%b %d, %I:%M %p')  # e.g., "Jan 10, 3:45 PM"
        title = request.headers.get('X-Title') or f'{formatted_time}'
        
        # Generate unique filename
        timestamp = now.timestamp()
        filename = f"fw_{device_id}_{timestamp}.wav"
        
        # Save raw body locally (backup)
        audio_path = os.path.join(RECORDINGS_FOLDER, filename)
        with open(audio_path, 'wb') as f:
            f.write(request.data)
        
        # Upload to Supabase Storage
        # If user_id exists, organize by user; otherwise by device
        storage_path = f"users/{user_id}/{filename}" if user_id else f"unlinked/{device_id}/{filename}"
        
        try:
            from database.supabase_client import upload_to_storage
            audio_url = upload_to_storage(
                bucket_name='recordings',
                file_path=storage_path,
                file_data=request.data,
                content_type='audio/wav'
            )
            print(f"Uploaded to Supabase Storage: {audio_url}")
        except Exception as storage_error:
            print(f"Supabase storage upload failed, using local path: {storage_error}")
            audio_url = f"/recordings/{filename}"
        
        # =================================================================
        # SAVE TO DATABASE (only if device is linked to a user)
        # =================================================================
        if user_id:
            try:
                recording = Recording(
                    user_id=user_id,  # Use the looked-up user_id!
                    title=title,
                    audio_url=audio_url,
                    mistakes=[],
                    feedback="Recording uploaded from PracticePod."
                )
                saved_recording = RecordingRepository.create(recording)
                print(f"Recording saved to database with ID: {saved_recording.id}")
                
                # =================================================================
                # CONVERT AUDIO TO MIDI USING SAMPLAB (background process)
                # =================================================================
                try:
                    print(f"Starting MIDI conversion for recording {saved_recording.id}...")
                    from samplab_automation import convert_audio_to_midi
                    
                    # Convert audio to MIDI using Samplab
                    midi_content = convert_audio_to_midi(audio_path)
                    
                    if midi_content:
                        # Upload MIDI file to Supabase Storage
                        from database.supabase_client import upload_to_storage
                        midi_filename = f"{saved_recording.id}.mid"
                        midi_storage_path = f"users/{user_id}/{midi_filename}"
                        
                        try:
                            midi_url = upload_to_storage(
                                bucket_name='midis',
                                file_path=midi_storage_path,
                                file_data=midi_content,
                                content_type='audio/midi'
                            )
                            # Update recording with MIDI file URL
                            RecordingRepository.update(saved_recording.id, {'midi_raw': midi_url})
                            print(f"MIDI conversion successful for recording {saved_recording.id} ({len(midi_content)} bytes, URL: {midi_url})")
                        except Exception as storage_error:
                            print(f"Failed to upload MIDI to storage for recording {saved_recording.id}: {storage_error}")
                            # Don't fail the whole process if storage upload fails
                    else:
                        print(f"MIDI conversion returned None for recording {saved_recording.id}")
                except Exception as midi_error:
                    # Don't fail the upload if MIDI conversion fails
                    print(f"MIDI conversion failed for recording {saved_recording.id}: {midi_error}")
                    import traceback
                    traceback.print_exc()
                
                return jsonify({
                    'success': True,
                    'recording': saved_recording.to_dict(),
                    'message': 'Recording saved to your account!'
                })
            except Exception as db_error:
                print(f"Database insert failed: {db_error}")
                import traceback
                traceback.print_exc()
                # Continue - storage upload succeeded
        
        # Device not linked - return success but note it wasn't saved to a user
        return jsonify({
            'success': True,
            'audio_url': audio_url,
            'filename': filename,
            'device_linked': False,
            'message': 'Uploaded to storage. Link your device in the app to save recordings to your account.'
        })
        
    except Exception as e:
        print(f"Firmware upload error: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e)}), 500


# =============================================================================
# DEVICE API ENDPOINTS (for firmware to call)
# =============================================================================

@app.route('/api/device/status', methods=['GET'])
def get_device_status():
    """
    Check if a device is linked and get its status.
    Useful for debugging firmware connectivity.
    """
    device_id = request.headers.get('X-User-ID', '')
    print(f"[/api/device/status] Device status check for: '{device_id}'")
    
    if not device_id:
        return jsonify({
            'linked': False,
            'error': 'No X-User-ID header provided',
            'help': 'The firmware needs to send the device UUID in the X-User-ID header'
        })
    
    user_id = DeviceRepository.get_user_by_device(device_id)
    
    if user_id:
        # Get some user info for debugging
        recordings_count = len(RecordingRepository.get_by_user(user_id))
        return jsonify({
            'linked': True,
            'device_id': device_id,
            'user_id': user_id,
            'recordings_count': recordings_count,
            'message': f'Device is linked! User has {recordings_count} recordings.'
        })
    else:
        return jsonify({
            'linked': False,
            'device_id': device_id,
            'message': f'Device "{device_id}" is not linked to any user account.',
            'help': 'Open the mobile app and link this device from the device pairing screen.'
        })


@app.route('/api/device/wifi-network', methods=['GET'])
def get_device_wifi_network():
    """
    Get the active WiFi network for a device (called by firmware).
    Uses X-User-ID header to identify the device.
    Returns the active WiFi network credentials if available.
    """
    try:
        device_id = request.headers.get('X-User-ID', '')
        print(f"[/api/device/wifi-network] Device: '{device_id}'")
        
        if not device_id:
            return jsonify({
                'has_network': False,
                'error': 'X-User-ID header required'
            }), 400
        
        # Get active WiFi network for this device
        wifi_network = WiFiNetworkRepository.get_active_network(device_id)
        
        if wifi_network:
            return jsonify({
                'has_network': True,
                'ssid': wifi_network.ssid,
                'password': wifi_network.password
            })
        else:
            return jsonify({
                'has_network': False,
                'message': 'No saved WiFi network found for this device'
            })
            
    except Exception as e:
        print(f"Error fetching WiFi network: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e)}), 500


@app.route('/api/device/wifi-network', methods=['POST'])
def save_device_wifi_network():
    """
    Save WiFi network credentials for a device (called by firmware).
    Uses X-User-ID header to identify the device.
    Expected JSON body: {"ssid": "NetworkName", "password": "password123"}
    """
    try:
        device_id = request.headers.get('X-User-ID', '')
        print(f"[/api/device/wifi-network] POST request from device: '{device_id}'")
        print(f"[/api/device/wifi-network] Request headers: {dict(request.headers)}")
        print(f"[/api/device/wifi-network] Request data: {request.data}")
        
        if not device_id:
            print("[/api/device/wifi-network] ERROR: No X-User-ID header")
            return jsonify({'error': 'X-User-ID header required'}), 400
        
        data = request.json
        if not data:
            print("[/api/device/wifi-network] ERROR: No JSON body")
            return jsonify({'error': 'JSON body required'}), 400
        
        print(f"[/api/device/wifi-network] Parsed JSON: {data}")
        
        ssid = data.get('ssid', '').strip()
        password = data.get('password', '').strip()
        
        print(f"[/api/device/wifi-network] SSID: '{ssid}', Password length: {len(password)}")
        
        if not ssid:
            print("[/api/device/wifi-network] ERROR: Empty SSID")
            return jsonify({'error': 'ssid is required'}), 400
        
        # Save WiFi network (this will deactivate other networks and activate this one)
        print(f"[/api/device/wifi-network] Calling WiFiNetworkRepository.save_network...")
        wifi_network = WiFiNetworkRepository.save_network(device_id, ssid, password)
        
        print(f"[/api/device/wifi-network] Successfully saved network '{ssid}' for device '{device_id}'")
        print(f"[/api/device/wifi-network] WiFi network ID: {wifi_network.id}")
        
        return jsonify({
            'success': True,
            'message': f'WiFi network "{ssid}" saved successfully',
            'ssid': wifi_network.ssid
        })
        
    except Exception as e:
        print(f"Error saving WiFi network: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e)}), 500


@app.route('/api/device/recordings', methods=['GET'])
def get_device_recordings():
    """
    Get recordings for a device (called by firmware).
    Uses X-User-ID header to identify the device (same as upload endpoint).
    """
    try:
        device_id = request.headers.get('X-User-ID', '')
        print(f"[/api/device/recordings] Received request with device_id: '{device_id}'")
        print(f"[/api/device/recordings] All headers: {dict(request.headers)}")
        
        if not device_id:
            print("[/api/device/recordings] ERROR: No X-User-ID header")
            return jsonify({'error': 'X-User-ID header required'}), 400
        
        # Look up user for this device
        user_id = DeviceRepository.get_user_by_device(device_id)
        print(f"[/api/device/recordings] Device '{device_id}' -> user_id: {user_id}")
        
        if not user_id:
            print(f"[/api/device/recordings] Device '{device_id}' not linked to any user")
            return jsonify({
                'recordings': [],
                'message': f'Device {device_id} not linked to any user'
            })
        
        # Get recordings for this user
        recordings = RecordingRepository.get_by_user(user_id)
        print(f"[/api/device/recordings] Found {len(recordings)} recordings for user {user_id}")
        
        # Return simplified list (limit to most recent 10)
        recordings_list = []
        for rec in recordings[:10]:
            recordings_list.append({
                'id': rec.id,
                'title': rec.title,
                'created_at': rec.created_at.isoformat() if rec.created_at else None,
                'audio_url': rec.audio_url
            })
        
        return jsonify({
            'recordings': recordings_list,
            'count': len(recordings_list)
        })
        
    except Exception as e:
        print(f"Error fetching device recordings: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e)}), 500


@app.route('/api/device/sheet-music', methods=['GET'])
def get_device_sheet_music():
    """
    Get sheet music for a device (called by firmware).
    Uses X-User-ID header to identify the device.
    """
    try:
        device_id = request.headers.get('X-User-ID', '')
        print(f"[/api/device/sheet-music] Received request with device_id: '{device_id}'")
        
        if not device_id:
            return jsonify({'error': 'X-User-ID header required'}), 400
        
        # Look up user for this device
        user_id = DeviceRepository.get_user_by_device(device_id)
        print(f"[/api/device/sheet-music] Device '{device_id}' -> user_id: {user_id}")
        
        if not user_id:
            return jsonify({
                'sheet_music': [],
                'message': f'Device {device_id} not linked to any user'
            })
        
        # Get sheet music for this user
        sheet_music_list = SheetMusicRepository.get_by_user(user_id)
        print(f"[/api/device/sheet-music] Found {len(sheet_music_list)} sheet music for user {user_id}")
        
        # Return simplified list (limit to most recent 10)
        # IMPORTANT: Return LIGHTWEIGHT data for firmware - full reference_data can exceed HTTP buffer
        uploads_list = []
        for sm in sheet_music_list[:10]:
            # Ensure id and title are never None - firmware requires both
            if not sm.id or not sm.title:
                print(f"[/api/device/sheet-music] Skipping sheet music with missing id or title: id={sm.id}, title={sm.title}")
                continue
            
            # Extract only essential fields from reference_data (firmware has limited buffer)
            ref_data = sm.reference_data or {}
            lite_ref_data = {
                'clef': ref_data.get('clef', ''),
                'time_signature': ref_data.get('time_signature') or ref_data.get('timeSignature', '4/4'),
                'key_signature': ref_data.get('key_signature') or ref_data.get('key', 'C'),
                'note_count': len(ref_data.get('notes', [])) if ref_data.get('notes') else 0
            }
            
            # Don't send full audiveris output - just a flag
            has_analysis = bool(sm.audiveris_raw_output and len(sm.audiveris_raw_output) > 0)
                
            uploads_list.append({
                'id': sm.id,
                'title': sm.title,
                'created_at': sm.created_at.isoformat() if sm.created_at else None,
                'file_url': sm.file_url if sm.file_url else None,
                'reference_data': lite_ref_data,
                'has_analysis': has_analysis
            })
        
        print(f"[/api/device/sheet-music] Returning {len(uploads_list)} items:")
        for i, item in enumerate(uploads_list):
            print(f"  [{i}] id={item.get('id')}, title={item.get('title')}, has_file_url={bool(item.get('file_url'))}")
        
        response_data = {
            'sheet_music': uploads_list,
            'count': len(uploads_list)
        }
        
        # Log the actual JSON structure
        import json
        print(f"[/api/device/sheet-music] JSON response sample: {json.dumps(response_data, indent=2)[:500]}")
        
        return jsonify(response_data)
        
    except Exception as e:
        print(f"Error fetching device sheet music: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e)}), 500


@app.route('/api/device/analyze', methods=['POST'])
def analyze_device_recording():
    """
    Analyze a recording for a device (called by firmware).
    Uses X-User-ID header to identify the device (same as upload endpoint).
    
    Request body:
    {
        "recording_id": "uuid",
        "sheet_music_id": "uuid"  // specific sheet music to compare against
    }
    """
    try:
        device_id = request.headers.get('X-User-ID', '')
        if not device_id:
            return jsonify({'error': 'X-User-ID header required'}), 400
        
        # Look up user for this device
        user_id = DeviceRepository.get_user_by_device(device_id)
        if not user_id:
            return jsonify({'error': 'Device not linked to any user'}), 401
        
        data = request.json or {}
        recording_id = data.get('recording_id')
        sheet_music_id = data.get('sheet_music_id')
        
        if not recording_id:
            return jsonify({'error': 'recording_id required'}), 400
        
        if not sheet_music_id:
            return jsonify({'error': 'sheet_music_id required'}), 400
        
        # Get the recording
        recording = RecordingRepository.get_by_id(recording_id)
        if not recording:
            return jsonify({'error': 'Recording not found'}), 404
        
        if recording.user_id != user_id:
            return jsonify({'error': 'Unauthorized'}), 403
        
        # Get sheet music by ID
        sheet_music = SheetMusicRepository.get_by_id(sheet_music_id)
        if not sheet_music:
            return jsonify({'error': 'Sheet music not found'}), 404
        
        if sheet_music.user_id != user_id:
            return jsonify({'error': 'Unauthorized'}), 403
        
        if not sheet_music.audiveris_raw_output:
            return jsonify({
                'feedback': 'Sheet music has no analysis data. Please re-upload in the app.',
                'score': 0,
                'strength': 'Sheet music needs analysis',
                'improvement': 'Please re-upload the sheet music in the app to analyze it.'
            })
        
        # Construct MIDI URL from recording ID
        # Format: {SUPABASE_URL}/storage/v1/object/public/midis/users/{user_id}/{recording_id}.mid
        supabase_url = os.getenv('SUPABASE_URL')
        midi_url = f"{supabase_url}/storage/v1/object/public/midis/users/{user_id}/{recording_id}.mid"
        
        print(f"[/api/device/analyze] Recording: {recording_id}, Sheet: {sheet_music_id}")
        print(f"[/api/device/analyze] MIDI URL: {midi_url}")

        # Create a pending analysis record first (so firmware can poll for it)
        import json as json_lib
        import uuid
        import threading
        
        analysis_id = str(uuid.uuid4())
        
        # Create initial pending analysis
        try:
            analysis = Analysis(
                id=analysis_id,
                user_id=user_id,
                recording_id=recording_id,
                sheet_music_id=sheet_music_id,
                score=0,
                strength="Processing...",
                improvement="Please wait...",
                feedback_points=[],
                full_feedback="",
                recording_title=recording.title,
                sheet_music_title=sheet_music.title,
                status="pending"
            )
            AnalysisRepository.create(analysis)
            print(f"[/api/device/analyze] Created pending analysis: {analysis_id}")
        except Exception as db_err:
            print(f"[/api/device/analyze] Failed to create pending analysis: {db_err}")
            return jsonify({'error': 'Failed to start analysis'}), 500

        # Run Gemini analysis in background thread
        def run_analysis_background():
            try:
                print(f"[BACKGROUND] Starting Gemini analysis for {analysis_id}...")
                feedback = ai_coach.analyze_midi_vs_sheet(
                    midi_url=midi_url,
                    musicxml_reference=sheet_music.audiveris_raw_output
                )
                
                score = 0
                strength = "Good effort overall!"
                improvement = "Keep practicing to improve!"
                feedback_points = []
                
                try:
                    feedback_data = json_lib.loads(feedback)
                    score = int(feedback_data.get('score', 0))
                    strength = feedback_data.get('strength', '') or "Good effort overall!"
                    improvement = feedback_data.get('improvement', '') or "Keep practicing to improve!"
                    feedback_points = feedback_data.get('feedback_points', [])
                    print(f"[BACKGROUND] Parsed - score: {score}, strength: {strength[:50]}...")
                except (json_lib.JSONDecodeError, KeyError, ValueError) as e:
                    print(f"[BACKGROUND] Failed to parse JSON: {e}")
                    if feedback and len(feedback) > 0:
                        improvement = feedback[:500]

                # Update analysis with results
                AnalysisRepository.update_status(
                    analysis_id=analysis_id,
                    status="complete",
                    score=score,
                    strength=strength,
                    improvement=improvement,
                    feedback_points=feedback_points,
                    full_feedback=feedback
                )
                print(f"[BACKGROUND] Analysis {analysis_id} completed with score {score}")
                
            except Exception as e:
                print(f"[BACKGROUND] Gemini analysis error: {e}")
                import traceback
                traceback.print_exc()
                AnalysisRepository.update_status(
                    analysis_id=analysis_id,
                    status="error",
                    score=0,
                    strength="Analysis failed",
                    improvement=f"Error: {str(e)[:200]}",
                    feedback_points=[],
                    full_feedback=""
                )

        thread = threading.Thread(target=run_analysis_background)
        thread.daemon = True
        thread.start()

        # Return immediately with analysis_id for polling
        print(f"[/api/device/analyze] Returning analysis_id: {analysis_id}")
        return jsonify({
            'analysis_id': analysis_id,
            'status': 'pending',
            'message': 'Analysis started'
        })
        
    except Exception as e:
        print(f"Error analyzing device recording: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e)}), 500


@app.route('/api/device/analyze/<analysis_id>', methods=['GET'])
def poll_device_analysis(analysis_id):
    """
    Poll for analysis results. Firmware calls this repeatedly until status is 'complete'.
    """
    try:
        import re
        
        # Clean string for firmware display
        def clean_for_firmware(text, max_len=80):
            if not text:
                return "Good effort"
            clean = re.sub(r'[^a-zA-Z0-9 .,]', ' ', text)
            clean = re.sub(r' +', ' ', clean).strip()
            if len(clean) > max_len:
                clean = clean[:max_len]
            return clean
        
        analysis = AnalysisRepository.get_by_id(analysis_id)
        if not analysis:
            return jsonify({'error': 'Analysis not found', 'status': 'error'}), 404
        
        status = getattr(analysis, 'status', 'complete')  # Default to complete for old records
        
        if status == 'pending':
            print(f"[/api/device/analyze/{analysis_id}] Status: pending")
            return jsonify({
                'status': 'pending',
                'message': 'Analysis in progress'
            })
        elif status == 'error':
            print(f"[/api/device/analyze/{analysis_id}] Status: error")
            return jsonify({
                'status': 'error',
                'score': 0,
                'strength': clean_for_firmware(analysis.strength, 80),
                'improvement': clean_for_firmware(analysis.improvement, 80)
            })
        else:
            # Complete
            print(f"[/api/device/analyze/{analysis_id}] Status: complete, score={analysis.score}")
            return jsonify({
                'status': 'complete',
                'score': analysis.score,
                'strength': clean_for_firmware(analysis.strength, 80),
                'improvement': clean_for_firmware(analysis.improvement, 80)
            })
            
    except Exception as e:
        print(f"Error polling analysis: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e), 'status': 'error'}), 500


@app.route('/api/analyses', methods=['GET'])
def get_user_analyses():
    """
    Get all analyses for the authenticated user.
    Requires Authorization header with Supabase JWT token.
    
    Query params:
    - limit: max number of results (default 50)
    """
    try:
        # Get user ID from auth token
        auth_header = request.headers.get('Authorization', '')
        if not auth_header.startswith('Bearer '):
            return jsonify({'error': 'Authorization header required'}), 401
        
        token = auth_header.replace('Bearer ', '')
        
        # Verify token and get user
        from auth.auth_utils import get_user_from_token
        user = get_user_from_token(token)
        if not user:
            return jsonify({'error': 'Invalid or expired token'}), 401
        
        user_id = user.id
        limit = request.args.get('limit', 50, type=int)
        
        # Get analyses from database
        analyses = AnalysisRepository.get_by_user(user_id, limit=limit)
        
        # Convert to JSON-serializable format
        result = []
        for a in analyses:
            result.append({
                'id': a.id,
                'recording_id': a.recording_id,
                'sheet_music_id': a.sheet_music_id,
                'score': a.score,
                'strength': a.strength,
                'improvement': a.improvement,
                'full_feedback': a.full_feedback,
                'recording_title': a.recording_title,
                'sheet_music_title': a.sheet_music_title,
                'created_at': a.created_at.isoformat() if a.created_at else None
            })
        
        return jsonify({'analyses': result})
        
    except Exception as e:
        print(f"Error fetching analyses: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e)}), 500


@app.route('/api/analyses/<analysis_id>', methods=['GET'])
def get_analysis_by_id(analysis_id):
    """
    Get a specific analysis by ID.
    Requires Authorization header with Supabase JWT token.
    """
    try:
        # Get user ID from auth token
        auth_header = request.headers.get('Authorization', '')
        if not auth_header.startswith('Bearer '):
            return jsonify({'error': 'Authorization header required'}), 401
        
        token = auth_header.replace('Bearer ', '')
        
        # Verify token and get user
        from auth.auth_utils import get_user_from_token
        user = get_user_from_token(token)
        if not user:
            return jsonify({'error': 'Invalid or expired token'}), 401
        
        user_id = user.id
        
        # Get analysis from database
        analysis = AnalysisRepository.get_by_id(analysis_id)
        
        if not analysis:
            return jsonify({'error': 'Analysis not found'}), 404
        
        # Verify user owns this analysis
        if analysis.user_id != user_id:
            return jsonify({'error': 'Unauthorized'}), 403
        
        return jsonify({
            'id': analysis.id,
            'recording_id': analysis.recording_id,
            'sheet_music_id': analysis.sheet_music_id,
            'score': analysis.score,
            'strength': analysis.strength,
            'improvement': analysis.improvement,
            'full_feedback': analysis.full_feedback,
            'recording_title': analysis.recording_title,
            'sheet_music_title': analysis.sheet_music_title,
            'created_at': analysis.created_at.isoformat() if analysis.created_at else None
        })
        
    except Exception as e:
        print(f"Error fetching analysis: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e)}), 500


@app.route('/api/device/audio-test', methods=['GET'])
def test_device_audio():
    """Test endpoint to verify firmware HTTP client can receive responses"""
    from flask import Response
    # Return a tiny valid WAV file (just the header, ~44 bytes)
    wav_header = bytes([
        0x52, 0x49, 0x46, 0x46,  # "RIFF"
        0x24, 0x00, 0x00, 0x00,  # File size - 8 (36 bytes)
        0x57, 0x41, 0x56, 0x45,  # "WAVE"
        0x66, 0x6D, 0x74, 0x20,  # "fmt "
        0x10, 0x00, 0x00, 0x00,  # Subchunk1Size (16)
        0x01, 0x00,              # AudioFormat (1 = PCM)
        0x01, 0x00,              # NumChannels (1 = mono)
        0x80, 0x3E, 0x00, 0x00,  # SampleRate (16000)
        0x00, 0x7D, 0x00, 0x00,  # ByteRate (32000)
        0x02, 0x00,              # BlockAlign (2)
        0x10, 0x00,              # BitsPerSample (16)
        0x64, 0x61, 0x74, 0x61,  # "data"
        0x00, 0x00, 0x00, 0x00,  # Subchunk2Size (0)
    ])
    print(f"[/api/device/audio-test] Returning {len(wav_header)} bytes")
    return Response(
        wav_header,
        mimetype='audio/wav',
        headers={
            'Content-Type': 'audio/wav',
            'Content-Length': str(len(wav_header))
        }
    )


@app.route('/api/device/audio/<recording_id>', methods=['GET'])
def proxy_device_audio(recording_id):
    """
    Proxy endpoint to serve audio files to firmware devices.
    Supports HTTP Range requests for chunked downloading.
    Firmware can download in small chunks (e.g., 8KB) to avoid buffer limits.
    """
    import requests
    from flask import Response
    
    try:
        device_id = request.headers.get('X-User-ID', '')
        print(f"[/api/device/audio/{recording_id}] Device: {device_id}")
        
        # Get recording from database
        recording = RecordingRepository.get_by_id(recording_id)
        if not recording:
            print(f"[/api/device/audio] Recording not found: {recording_id}")
            return jsonify({'error': 'Recording not found'}), 404
        
        if not recording.audio_url:
            print(f"[/api/device/audio] No audio_url for recording: {recording_id}")
            return jsonify({'error': 'Audio still processing, try again in a moment'}), 202
        
        print(f"[/api/device/audio] Fetching from: {recording.audio_url}")
        
        # Fetch full audio file from Supabase
        audio_response = requests.get(recording.audio_url, timeout=120)
        if audio_response.status_code != 200:
            print(f"[/api/device/audio] Supabase returned: {audio_response.status_code}")
            return jsonify({'error': f'Failed to fetch audio: {audio_response.status_code}'}), 502
        
        audio_data = audio_response.content
        total_size = len(audio_data)
        
        # Check for Range header (chunked download support)
        range_header = request.headers.get('Range')
        if range_header:
            # Parse Range: bytes=start-end
            try:
                range_match = range_header.replace('bytes=', '').split('-')
                start = int(range_match[0]) if range_match[0] else 0
                end = int(range_match[1]) if range_match[1] else total_size - 1
                
                # Clamp to valid range
                start = max(0, min(start, total_size - 1))
                end = max(start, min(end, total_size - 1))
                
                chunk = audio_data[start:end + 1]
                content_range = f'bytes {start}-{end}/{total_size}'
                
                print(f"[/api/device/audio] Range request: {start}-{end}, returning {len(chunk)} bytes")
                
                return Response(
                    chunk,
                    status=206,  # Partial Content
                    mimetype='audio/wav',
                    headers={
                        'Content-Type': 'audio/wav',
                        'Content-Length': str(len(chunk)),
                        'Content-Range': content_range,
                        'Accept-Ranges': 'bytes'
                    }
                )
            except Exception as e:
                print(f"[/api/device/audio] Invalid Range header: {range_header}, error: {e}")
                # Fall through to full response
        
        print(f"[/api/device/audio] Returning full file: {total_size} bytes")
        
        # Return full file with Accept-Ranges header
        return Response(
            audio_data,
            mimetype='audio/wav',
            headers={
                'Content-Type': 'audio/wav',
                'Content-Length': str(total_size),
                'Accept-Ranges': 'bytes'
            }
        )
        
    except requests.exceptions.Timeout:
        print(f"[/api/device/audio] Timeout fetching audio for: {recording_id}")
        return jsonify({'error': 'Timeout fetching audio'}), 504
    except Exception as e:
        print(f"Error proxying audio: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e)}), 500


@app.route('/api/device/audio-info/<recording_id>', methods=['GET'])
def get_audio_info(recording_id):
    """
    Get audio file size for firmware to plan chunked download.
    Returns just the metadata without the audio data.
    """
    import requests
    
    try:
        recording = RecordingRepository.get_by_id(recording_id)
        if not recording:
            return jsonify({'error': 'Recording not found'}), 404
        
        if not recording.audio_url:
            return jsonify({'error': 'Audio still processing'}), 202
        
        # Do a HEAD request to get file size without downloading
        head_response = requests.head(recording.audio_url, timeout=10)
        if head_response.status_code != 200:
            # Fall back to GET if HEAD not supported
            get_response = requests.get(recording.audio_url, timeout=30)
            file_size = len(get_response.content)
        else:
            file_size = int(head_response.headers.get('Content-Length', 0))
        
        print(f"[/api/device/audio-info/{recording_id}] Size: {file_size} bytes")
        
        return jsonify({
            'id': recording_id,
            'size': file_size,
            'title': recording.title
        })
        
    except Exception as e:
        print(f"Error getting audio info: {e}")
        return jsonify({'error': str(e)}), 500


@app.route('/api/analyze', methods=['POST'])
@require_auth
def analyze_performance():
    """
    Analyze a musical performance against reference sheet music
    
    Expected request:
    - audio_file: audio recording (WAV/MP3)
    - reference_data: MusicXML or MIDI data
    - metadata: tempo, key signature, etc.
    - title: (optional) recording title
    - sheet_music_id: (optional) ID of associated sheet music
    """
    try:
        if 'audio' not in request.files:
            return jsonify({'error': 'No audio file provided'}), 400
        
        audio_file = request.files['audio']
        reference_data = request.form.get('reference_data', '{}')
        metadata = json.loads(request.form.get('metadata', '{}'))
        title = request.form.get('title', 'Untitled Recording')
        sheet_music_id = request.form.get('sheet_music_id')
        
        # Save uploaded audio
        audio_path = os.path.join(RECORDINGS_FOLDER, f"{datetime.now().timestamp()}.wav")
        audio_file.save(audio_path)
        
        # Parse reference data
        reference_music = json.loads(reference_data) if isinstance(reference_data, str) else reference_data
        
        # Analyze performance
        mistakes = audio_analyzer.analyze(audio_path, reference_music, metadata)
        
        # Generate AI coaching feedback
        feedback = ai_coach.generate_feedback(mistakes, reference_music, metadata)
        
        # TODO: Upload audio to Supabase Storage and get URL
        # For now, use local path
        audio_url = f"/recordings/{os.path.basename(audio_path)}"
        
        # Save to database
        recording = Recording(
            user_id=request.user_id,
            title=title,
            audio_url=audio_url,
            sheet_music_id=sheet_music_id,
            mistakes=mistakes,
            feedback=feedback.get('text', '') if isinstance(feedback, dict) else str(feedback)
        )
        
        saved_recording = RecordingRepository.create(recording)
        
        return jsonify({
            'success': True,
            'recording': saved_recording.to_dict(),
            'mistakes': mistakes,
            'feedback': feedback
        })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500


@app.route('/api/sheet-music', methods=['GET'])
@require_auth
def get_sheet_music():
    """Get all sheet music for the authenticated user"""
    try:
        sheet_music_list = SheetMusicRepository.get_by_user(request.user_id)
        return jsonify({
            'success': True,
            'sheet_music': [sm.to_dict() for sm in sheet_music_list]
        })
    except Exception as e:
        return jsonify({'error': str(e)}), 500


@app.route('/api/upload-sheet-music', methods=['POST'])
@require_auth
def upload_sheet_music():
    """
    Upload and process sheet music (photo/PDF)
    Returns structured music data in the requested JSON format
    """
    try:
        if 'file' not in request.files:
            return jsonify({'error': 'No file provided'}), 400
        
        file = request.files['file']
        filename = file.filename
        title = request.form.get('title', 'Untitled')
        
        # Save uploaded file
        file_path = os.path.join(UPLOAD_FOLDER, filename)
        file.save(file_path)
        
        # Run Full AI Transcription (Dual Model Gemini)
        print(f"Transcribing {file_path} with Gemini Dual-Model...")
        
        transcription_result = ai_coach.transcribe_image(file_path)
        
        if not transcription_result:
            return jsonify({'error': 'Transcription failed'}), 500
            
        reference_data = transcription_result.get('reference_data', {})
        audiveris_raw_output = transcription_result.get('audiveris_raw_output', '')
        
        print(f"DEBUG: Saved output - reference_data: {len(reference_data.get('notes', []))} notes")
        
        # Create DB Record with Audiveris output as reference_data JSONB and raw output in separate column
        sheet_music = SheetMusic(
            user_id=request.user_id,
            title=title,
            file_url=file_path, # Local path for now as per reference code
            reference_data=reference_data,  # This will be saved as JSONB in the database
            audiveris_raw_output=audiveris_raw_output  # Raw Audiveris output in separate TEXT column
        )
        
        saved_sheet_music = SheetMusicRepository.create(sheet_music)
        print(f"DEBUG: Sheet music saved with ID: {saved_sheet_music.id}, reference_data contains {len(saved_sheet_music.reference_data.get('notes', [])) if saved_sheet_music.reference_data else 0} notes")
        
        return jsonify({
            'success': True,
            'sheet_music': saved_sheet_music.to_dict(),
            'transcription': reference_data,
            'audiveris_raw_output': audiveris_raw_output
        })
        
    except Exception as e:
        print(f"OMR Error: {e}")
        return jsonify({'error': str(e)}), 500


@app.route('/api/analyze-with-audio', methods=['POST'])
@require_auth
def analyze_with_audio():
    """
    Analyze a recording against sheet music using Gemini 2.5 Flash.
    
    Expected JSON body:
    {
        "recording_id": "uuid",
        "audio_url": "https://...",
        "sheet_music_id": "uuid",
        "audiveris_raw_output": "MusicXML string",
        "sheet_music_title": "Title of the piece"
    }
    
    Returns AI-generated feedback comparing the performance to the reference.
    """
    import google.generativeai as genai
    import requests
    import tempfile
    
    try:
        data = request.json
        recording_id = data.get('recording_id')
        audio_url = data.get('audio_url')
        sheet_music_id = data.get('sheet_music_id')
        audiveris_raw_output = data.get('audiveris_raw_output', '')
        sheet_music_title = data.get('sheet_music_title', 'Unknown Piece')
        
        if not audio_url:
            return jsonify({'error': 'audio_url is required'}), 400
        
        print(f"Analyzing recording {recording_id} against sheet music {sheet_music_id}")
        print(f"Audio URL: {audio_url}")
        print(f"Audiveris output length: {len(audiveris_raw_output)} chars")
        
        # Download the audio file
        audio_data = None
        audio_path = None
        
        # Check if it's a local path or Supabase URL
        if audio_url.startswith('/recordings/'):
            # Local file
            local_path = os.path.join(RECORDINGS_FOLDER, audio_url.split('/')[-1])
            if os.path.exists(local_path):
                with open(local_path, 'rb') as f:
                    audio_data = f.read()
                audio_path = local_path
        elif audio_url.startswith('http'):
            # Remote URL - download it
            try:
                response = requests.get(audio_url, timeout=30)
                response.raise_for_status()
                audio_data = response.content
                
                # Save to temp file for Gemini
                with tempfile.NamedTemporaryFile(suffix='.wav', delete=False) as tmp:
                    tmp.write(audio_data)
                    audio_path = tmp.name
            except Exception as e:
                print(f"Failed to download audio: {e}")
                return jsonify({'error': f'Failed to download audio: {e}'}), 400
        
        if not audio_data or not audio_path:
            return jsonify({'error': 'Could not load audio file'}), 400
        
        # Initialize Gemini
        api_key = os.getenv('GEMINI_API_KEY')
        if not api_key:
            return jsonify({'error': 'Gemini API key not configured'}), 500
        
        genai.configure(api_key=api_key)
        model = genai.GenerativeModel('gemini-2.5-flash')
        
        # Upload the audio to Gemini
        print("Uploading audio to Gemini...")
        audio_file = genai.upload_file(audio_path, mime_type='audio/wav')
        
        # Prepare the analysis prompt - generates context for voice AI conversation
        prompt = f"""You are an expert music teacher with perfect pitch. Analyze this student's audio recording and compare it EXACTLY against the sheet music.

## Reference Sheet Music
**Piece:** "{sheet_music_title}"

**MusicXML Score (contains every note the student SHOULD play):**
```xml
{audiveris_raw_output[:15000] if audiveris_raw_output else 'No reference data available'}
```

## YOUR TASK

Listen to the audio recording carefully. Compare what you HEAR vs what the sheet music SHOWS.

Generate a conversational analysis that will be spoken aloud to the student. The student will then be able to ask you follow-up questions about their performance.

## REQUIREMENTS

1. **Start with a warm greeting** - Address the student directly (use "you")

2. **Give an overall impression** - How did the piece sound overall? (1-2 sentences)

3. **List SPECIFIC pitch errors** - Be extremely specific:
   - "In measure 2, beat 1, you played F natural instead of F sharp - you missed the sharp"
   - "In measure 5, you played a B when it should have been C"
   - "Your E notes in measures 3 through 4 were consistently a bit flat"

4. **List SPECIFIC duration/rhythm issues**:
   - "In measure 3, you cut the half note D short - only held it for about 1 beat instead of 2"
   - "You rushed through the eighth notes in measure 10"

5. **Highlight what they did WELL** - Be specific and encouraging

6. **Give the TOP 3 priorities to fix** - Ranked by importance

7. **End with encouragement** and mention you're happy to discuss any specific measure or passage

8. **Give a score out of 10**

## OUTPUT FORMAT

Write your response as if you're speaking directly to the student. This will be spoken aloud by a voice AI, so:
- Use natural, conversational language
- Don't use markdown formatting or bullet points
- Organize thoughts clearly but conversationally  
- Keep it under 400 words so it's not too long to listen to

Example tone: "Hey! I just listened to your performance of {sheet_music_title}. Overall, nice work! I noticed a few things though. In measure 2, you played an F natural when it should have been F sharp - easy to miss that accidental. Also in measure 5..."

End with: "Feel free to ask me about any specific measure or if you want tips on how to practice a tricky section!"
"""
        
        print("Generating analysis with Gemini...")
        response = model.generate_content([audio_file, prompt])
        
        feedback_text = response.text
        print(f"Generated feedback: {len(feedback_text)} chars")
        
        # Clean up temp file if we created one
        if audio_path and audio_path.startswith(tempfile.gettempdir()):
            try:
                os.unlink(audio_path)
            except:
                pass
        
        # Optionally update the recording with feedback
        if recording_id:
            try:
                from database.supabase_client import get_supabase_client
                supabase = get_supabase_client()
                supabase.table('recordings').update({
                    'feedback': feedback_text,
                    'sheet_music_id': sheet_music_id
                }).eq('id', recording_id).execute()
                print(f"Updated recording {recording_id} with feedback")
            except Exception as e:
                print(f"Failed to update recording: {e}")
        
        return jsonify({
            'success': True,
            'feedback': feedback_text,
            'recording_id': recording_id,
            'sheet_music_id': sheet_music_id
        })
        
    except Exception as e:
        print(f"Analysis error: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e)}), 500


@app.route('/api/process-musicxml', methods=['POST'])
@require_auth
def process_musicxml():
    """
    Process MusicXML data and extract structured reference information
    """
    try:
        data = request.json
        musicxml_content = data.get('musicxml', '')
        
        if not musicxml_content:
            return jsonify({'error': 'No MusicXML content provided'}), 400
        
        # Parse MusicXML using music21
        score = converter.parse(musicxml_content)
        
        # Extract structured data
        reference_data = audio_analyzer.extract_reference_data(score)
        
        return jsonify({
            'success': True,
            'reference_data': reference_data
        })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500


@app.route('/api/voice/wake-word', methods=['POST'])
def detect_wake_word():
    """
    Detect wake word "Hey Tuya" in audio using Whisper
    Returns: {"detected": true/false, "transcribed_text": "..."}
    """
    try:
        if not request.data:
            return jsonify({'error': 'No audio data provided'}), 400

        # Save audio to temporary file
        with tempfile.NamedTemporaryFile(delete=False, suffix='.wav') as tmp_file:
            tmp_file.write(request.data)
            tmp_path = tmp_file.name

        try:
            # Transcribe with Whisper
            model = get_whisper_model()
            result = model.transcribe(tmp_path, language="en")
            transcribed_text = result["text"].strip().lower()
            
            # Check for wake word (flexible matching)
            wake_word_lower = WAKE_WORD.lower()
            detected = wake_word_lower in transcribed_text or \
                      "hey tuya" in transcribed_text or \
                      "hey toya" in transcribed_text
            
            print(f"Wake word check - Transcribed: '{transcribed_text}' -> Detected: {detected}")
            
            return jsonify({
                'detected': detected,
                'transcribed_text': transcribed_text
            })
        finally:
            try:
                os.unlink(tmp_path)
            except:
                pass

    except Exception as e:
        print(f"Wake word detection error: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e), 'detected': False}), 500


@app.route('/api/voice/chat', methods=['POST'])
def voice_chat():
    """
    Process voice input, transcribe, get AI response
    Returns: {"user_text": "...", "assistant_text": "..."}
    """
    try:
        if not request.data:
            return jsonify({'error': 'No audio data provided'}), 400

        # Save audio to temporary file
        with tempfile.NamedTemporaryFile(delete=False, suffix='.wav') as tmp_file:
            tmp_file.write(request.data)
            tmp_path = tmp_file.name

        try:
            # Transcribe with Whisper
            model = get_whisper_model()
            result = model.transcribe(tmp_path, language="en")
            user_text = result["text"].strip()
            
            print(f"User said: '{user_text}'")
            
            # Remove wake word if present
            user_text_clean = user_text.replace(WAKE_WORD.lower(), "").replace("hey tuya", "").strip()
            if not user_text_clean:
                user_text_clean = user_text
            
            # Get AI response
            assistant_text = ai_coach.chat(user_text_clean)
            
            print(f"Assistant: '{assistant_text}'")
            
            # Generate TTS audio using gTTS (Google Text-to-Speech - free)
            tts_audio_data = None
            try:
                tts = gTTS(text=assistant_text[:500], lang='en', slow=False)  # Limit to 500 chars
                audio_buffer = io.BytesIO()
                tts.write_to_fp(audio_buffer)
                tts_audio_data = audio_buffer.getvalue()
                print(f"Generated TTS audio: {len(tts_audio_data)} bytes")
            except Exception as e:
                print(f"TTS generation failed: {e}")
                import traceback
                traceback.print_exc()
                tts_audio_data = None
            
            # Return response with TTS audio as base64 if available
            result = {
                'user_text': user_text_clean,
                'assistant_text': assistant_text
            }
            
            if tts_audio_data:
                result['tts_audio'] = base64.b64encode(tts_audio_data).decode('utf-8')
                result['tts_format'] = 'mp3'  # gTTS returns MP3
            
            return jsonify(result)
        finally:
            try:
                os.unlink(tmp_path)
            except:
                pass

    except Exception as e:
        print(f"Voice chat error: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e)}), 500


@app.route('/uploads/<filename>')
def serve_upload(filename):
    """Serve uploaded files"""
    try:
        return send_from_directory(UPLOAD_FOLDER, filename)
    except Exception as e:
        return jsonify({'error': str(e)}), 404


if __name__ == '__main__':
    port = int(os.getenv('PORT', 5001))
    app.run(host='0.0.0.0', port=port, debug=True)
