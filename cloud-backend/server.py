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
from analysis.audio_analyzer import AudioAnalyzer
from analysis.coach import AICoach
from auth.auth_utils import require_auth
from database.repository import RecordingRepository, SheetMusicRepository
from database.models import Recording, SheetMusic
from database.models import Recording, SheetMusic

load_dotenv()

app = Flask(__name__)
CORS(app)

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
