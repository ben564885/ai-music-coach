# AI Music Coach

An intelligent music practice assistant that analyzes musical performances and provides real-time coaching feedback. The system combines embedded hardware, mobile app, and cloud analysis to help musicians improve their technique.

## What Makes Us Different

• **Solved the OMR Problem**: No reliable OMR APIs exist, so we automated SoundSlice's web interface with Playwright to extract MusicXML from sheet music images

• **Complete Hardware Solution**: Custom firmware on T5AI board with embedded display, real-time feedback, TTS coaching, and standalone practice mode

• **AI-Powered Coaching**: Advanced pitch tracking (PYIN algorithm) + Gemini AI generates personalized, actionable feedback beyond simple wrong-note detection

• **Standalone Practice**: Embedded screen shows real-time notes, fingering charts, and visual feedback—no phone distractions needed

## System Architecture

```
[T5AI Device] ←BLE→ [Mobile App (Flutter)] ←HTTPS→ [Cloud Backend (Flask)] ←→ [Supabase + Gemini AI]
    (Hardware)              (UI)                      (Analysis)              (Storage + AI)
```

## Components

### T5AI Firmware (`firmware_copy/`)
C firmware for the Tuya T5AI development board:
- Dual microphone audio recording → WAV encoding → HTTP upload
- LVGL graphics on embedded display (ST7789/ILI9341)
- Real-time note display, fingering charts, and visual feedback
- Text-to-Speech (TTS) coaching via speaker
- BLE 5.4 communication with mobile app
- Wi-Fi HTTP API client for cloud backend

**Setup**: 
1. `cd firmware_copy`
2. Edit `main.c` to set your Wi-Fi credentials and backend host
3. `tos.py build` (requires Tuya TuyaOpen SDK)
4. `tos.py flash` to upload to T5AI device

### Mobile App (`mobile_app_flutter/`)
Cross-platform Flutter app for iOS and Android:
- Sheet music upload and management (photo/PDF)
- BLE connection to T5AI device for pairing and control
- Recording playback and mistake timeline visualization
- Supabase authentication (Email/Password + Google OAuth)
- Profile management and practice history

**Setup**: 
1. `cd mobile_app_flutter`
2. `flutter pub get`
3. Update `lib/utils/api_config.dart` with your backend URL
4. `flutter run`

### Cloud Backend (`cloud-backend/`)
Python Flask server for audio analysis and OMR:
- **OMR Processing**: 
    - Playwright automation of SoundSlice web interface
    - SoundSlice API for MusicXML retrieval
    - oemer and Roboflow fallback methods
- **Audio Analysis**: 
    - librosa: PYIN pitch tracking, onset detection, beat tracking
    - music21: MusicXML parsing and score comparison
    - scipy/numpy: Signal processing and statistics
- **AI Coaching**: 
    - Gemini API for natural language feedback generation
    - Whisper for speech recognition (voice assistant)
    - gTTS for text-to-speech synthesis
- **Database**: Supabase PostgreSQL + Repository pattern

**Setup**: 
1. `cd cloud-backend`
2. `python3 -m venv venv`
3. `source venv/bin/activate`
4. `pip install -r requirements.txt`
5. Set environment variables (see below)
6. `python server.py`

## Environment Variables

### Cloud Backend
Create `cloud-backend/.env` with:
```bash
# Supabase
SUPABASE_URL=https://your-project.supabase.co
SUPABASE_KEY=your-anon-key

# SoundSlice (for OMR automation)
SOUNDSLICE_EMAIL=your-email@example.com
SOUNDSLICE_PASSWORD=your-password
SOUNDSLICE_APP_ID=your-app-id
SOUNDSLICE_PASSWORD_TOKEN=your-password-token

# AI Services
GEMINI_API_KEY=your-gemini-api-key
ROBOFLOW_API_KEY=your-roboflow-key (optional)

# Server
FLASK_ENV=development
PORT=5001
```

### Mobile App
Update `mobile_app_flutter/lib/utils/api_config.dart` with your backend URL (e.g., `http://192.168.1.100:5001`)

### Firmware
Edit `firmware_copy/main.c`:
- `CLOUD_BACKEND_HOST` - Your backend IP address
- `USER_SSID` / `USER_PASSWORD` - Your Wi-Fi credentials
- `DEVICE_UUID` - Your Tuya device UUID

## Documentation

- **[ARCHITECTURE.md](ARCHITECTURE.md)** - System overview and component details
- **[DATA_FLOW.md](DATA_FLOW.md)** - Complete data flow charts
- **[TECH_STACK_FLOWCHART.md](TECH_STACK_FLOWCHART.md)** - Visual tech stack diagram
- **[WHAT_MAKES_US_DIFFERENT.md](WHAT_MAKES_US_DIFFERENT.md)** - Key differentiators
- **[DEPLOYMENT.md](cloud-backend/DEPLOYMENT.md)** - Cloud deployment guide
- **[HOW_TO_USE_VOICE_ASSISTANT.md](HOW_TO_USE_VOICE_ASSISTANT.md)** - Voice chat setup

## Quick Start

1. **Backend**: Set up Supabase account, configure `.env`, run Flask server
2. **Firmware**: Flash T5AI device with correct Wi-Fi and backend IP
3. **Mobile**: Install Flutter app, sign in with Google, pair device via BLE
4. **Practice**: Upload sheet music, record on device, get AI coaching feedback!

## License

MIT
