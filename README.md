# AI Music Coach

An intelligent music practice assistant that analyzes musical performances and provides real-time coaching feedback by comparing audio recordings against reference notation retrieved via SoundSlice.

## System Architecture

```
[Mobile App (Flutter)] → [T5AI Device] → [Cloud Backend (Flask)] → [SoundSlice (Playwright Agent)] → [Analysis] → [Feedback]
```

## Components

### Mobile App (`mobile_app_flutter/`)
Cross-platform Flutter app for iOS and Android:
- Sheet music management and upload (photo/PDF)
- Bluetooth LE connection to T5AI device for audio control
- Real-time mistakes and performance visualization
- Supabase Authentication (Email/Password & Google Sign-InSync)
- Profile management and practice statistics

**Setup**: 
1. `cd mobile_app_flutter`
2. `flutter pub get`
3. `cp .env.example .env` (Add your Supabase and Google Client IDs)
4. `flutter run`

### Cloud Backend (`cloud-backend/`)
Python Flask server for audio analysis and OMR coordination:
- **Sheet Music Transcription (OMR)**: 
    - **Playwright Agent**: Automates SoundSlice's web interface for high-accuracy OMR processing.
    - **SoundSlice Data API**: Fetches structured MusicXML for analysis.
- **Audio Analysis**: 
    - **Pitch Tracking**: PYIN algorithm for accurate frequency identification.
    - **Timing Analysis**: Compares performer onsets with reference beats from notation.
- **Coaching Engine**: Generates actionable feedback based on performance discrepancies.

**Setup**: 
1. `cd cloud-backend`
2. `python3 -m venv venv`
3. `source venv/bin/activate`
4. `pip install -r requirements.txt`
5. `cp .env.example .env` (Add your SoundSlice and other credentials)
6. `python server.py`

### T5AI Firmware (`firmware/`)
C++ firmware for the Tuya T5AI development board:
- Audio recording and streaming to the cloud backend via Wi-Fi.
- Text-to-Speech (TTS) integration for spoken coaching.
- BLE communication with the mobile app for device lifecycle management.

**Configuration**: Edit `config.hpp` to set Wi-Fi credentials, backend URL, and BLE settings.

## Environment Variables

The project requires `.env` files for operation. Templates are provided at:
- `cloud-backend/.env.example`
- `mobile_app_flutter/.env.example`

## License

MIT
