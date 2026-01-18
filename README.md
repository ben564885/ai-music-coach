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
2. **Configure credentials** (REQUIRED):
   ```bash
   cp config.h.example config.h
   # Edit config.h with your actual credentials:
   # - PRODUCT_KEY: From Tuya IoT Platform (iot.tuya.com)
   # - DEVICE_UUID: From Tuya IoT Platform
   # - AUTH_KEY: From Tuya IoT Platform (device Auth Key)
   # - CLOUD_BACKEND_HOST: Your backend server IP (e.g., "192.168.1.100")
   ```
3. `tos.py build` (requires Tuya TuyaOpen SDK)
4. `tos.py flash` to upload to T5AI device

**Security Note**: `config.h` contains sensitive credentials and is excluded from git. Never commit it.

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
3. **Configure backend URL** (REQUIRED):
   ```bash
   cp .env.example .env
   # Edit .env and set:
   # BACKEND_URL=http://your-backend-ip:5001
   # EXPO_PUBLIC_SUPABASE_URL=your-supabase-url
   # EXPO_PUBLIC_SUPABASE_ANON_KEY=your-supabase-anon-key
   ```
4. `flutter run`

**Security Note**: `.env` contains sensitive credentials and is excluded from git. Never commit it.

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
3. `source venv/bin/activate` (or `venv\Scripts\activate` on Windows)
4. `pip install -r requirements.txt`
5. **Install Playwright browsers** (required for Samplab and SoundSlice automation):
   ```bash
   playwright install chromium
   ```
6. Set environment variables (see below)
7. `python server.py`

## Environment Variables

### Cloud Backend
Create `cloud-backend/.env` with the following required API keys and credentials:

#### Required Services

**1. Supabase (Database & Storage)**
- Sign up at [supabase.com](https://supabase.com)
- Create a new project
- Get your project URL and service role key from Settings → API
```bash
SUPABASE_URL=https://your-project.supabase.co
SUPABASE_SERVICE_KEY=your-service-role-key
SUPABASE_JWT_SECRET=your-jwt-secret  # Optional, for auth fallback
```

**2. Google Gemini AI (AI Coaching)**
- Sign up at [Google AI Studio](https://makersuite.google.com/app/apikey)
- Create a new API key
- Free tier includes generous usage limits
```bash
GEMINI_API_KEY=your-gemini-api-key
# OR (legacy name, both work)
GOOGLE_API_KEY=your-gemini-api-key
```

#### Optional Services

**3. SoundSlice (OMR - Optical Music Recognition)**
- Sign up at [soundslice.com](https://www.soundslice.com)
- Create an account and log in
- Get API credentials from your account settings
- Used for high-quality sheet music transcription (fallback to Gemini if not configured)
```bash
SOUNDSLICE_EMAIL=your-email@example.com
SOUNDSLICE_PASSWORD=your-password
SOUNDSLICE_APP_ID=your-app-id
SOUNDSLICE_SECRET_KEY=your-secret-key
```

**4. Roboflow (Alternative OMR)**
- Sign up at [roboflow.com](https://roboflow.com)
- Create a workspace and get your API key
- Optional fallback for OMR if SoundSlice fails
```bash
ROBOFLOW_API_KEY=your-roboflow-key
ROBOFLOW_WORKSPACE=your-workspace-name  # Optional, defaults to 'ben-d5iad'
ROBOFLOW_WORKFLOW_ID=your-workflow-id   # Optional, defaults to 'detect-and-classify'
```

**5. Samplab (Audio-to-MIDI Conversion)**
- Sign up at [samplab.com](https://samplab.com)
- Create a free account
- Used to convert recorded audio performances to MIDI format for analysis
```bash
SAMPLAB_EMAIL=your-email@example.com
SAMPLAB_PASSWORD=your-password
```

#### Server Configuration
```bash
FLASK_ENV=development
PORT=5001
```

**Note**: 
- **Minimum required**: Supabase and Gemini API keys
- **For sheet music upload**: SoundSlice credentials (recommended) or Roboflow API key (fallback)
- **For audio recording analysis**: Samplab credentials (required to convert recordings to MIDI for comparison with sheet music)

### Mobile App
Update `mobile_app_flutter/lib/utils/api_config.dart` with your backend URL (e.g., `http://192.168.1.100:5001`)

### Firmware
**SECURITY**: Credentials are stored in `firmware_copy/config.h` (not in git).

1. Copy the template:
   ```bash
   cd firmware_copy
   cp config.h.example config.h
   ```

2. Edit `config.h` with your credentials:
   - `PRODUCT_KEY` - From Tuya IoT Platform (iot.tuya.com)
   - `DEVICE_UUID` - From Tuya IoT Platform
   - `AUTH_KEY` - From Tuya IoT Platform (device Auth Key)
   - `CLOUD_BACKEND_HOST` - Your backend server IP address

3. **Never commit `config.h` to git** - it's in `.gitignore`

## Documentation

- **[ARCHITECTURE.md](ARCHITECTURE.md)** - System overview and component details
- **[DATA_FLOW.md](DATA_FLOW.md)** - Complete data flow charts
- **[TECH_STACK_FLOWCHART.md](TECH_STACK_FLOWCHART.md)** - Visual tech stack diagram
- **[WHAT_MAKES_US_DIFFERENT.md](WHAT_MAKES_US_DIFFERENT.md)** - Key differentiators
- **[DEPLOYMENT.md](cloud-backend/DEPLOYMENT.md)** - Cloud deployment guide
- **[HOW_TO_USE_VOICE_ASSISTANT.md](HOW_TO_USE_VOICE_ASSISTANT.md)** - Voice chat setup

## Quick Start

1. **Backend**: Set up Supabase account, configure `.env`, run Flask server
2. **Firmware**: Copy `config.h.example` to `config.h`, fill in credentials, build and flash
3. **Mobile**: Copy `.env.example` to `.env`, configure backend URL and Supabase keys, run app
4. **Practice**: Upload sheet music, record on device, get AI coaching feedback!

## Security

**IMPORTANT**: This project contains sensitive credentials that must be configured:

- **Firmware**: `firmware_copy/config.h` - Contains Tuya device credentials and backend IP
- **Backend**: `cloud-backend/.env` - Contains API keys and database credentials
- **Mobile App**: `mobile_app_flutter/.env` - Contains backend URL and Supabase keys

**Security Best Practices**:
- ✅ Never commit `.env` or `config.h` files to git (they're in `.gitignore`)
- ✅ Use `.env.example` and `config.h.example` as templates
- ✅ Rotate API keys if they're accidentally exposed
- ✅ Use different credentials for development and production
- ✅ Keep your Tuya Auth Key secret - it authenticates your device

## License

MIT
