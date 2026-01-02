# AI Music Coach

An intelligent music practice assistant that uses the Tuya T5AI development board to analyze musical performances and provide real-time coaching feedback.

## System Architecture

```
[Mobile App] → [T5AI Device] → [Cloud Backend] → [AI Analysis] → [Feedback]
```

## Components

### Mobile App (`mobile-app/`)
React Native app with Expo for iOS and Android:
- Sheet music upload (photo/PDF/MusicXML)
- Bluetooth LE connection to T5AI device
- Real-time mistake timeline visualization
- Audio playback with seekable markers
- AI coaching feedback display

**Setup**: `cd mobile-app && npm install && npm start`

**Testing on Phone**: 
- Install Expo Go app on your phone
- Run `npm start` and scan QR code
- See `mobile-app/TESTING.md` for quick guide
- See `mobile-app/DEPLOYMENT.md` for app store submission

### Cloud Backend (`cloud-backend/`)
Python Flask server for audio analysis:
- Pitch detection (PYIN algorithm)
- Timing analysis (hesitation/rushing detection)
- Dynamics analysis
- AI coaching feedback (Claude API)
- RESTful API endpoints

**Setup**: `cd cloud-backend && python3 -m venv venv && source venv/bin/activate && pip install -r requirements.txt`

**API Endpoints**:
- `GET /health` - Health check
- `POST /api/analyze` - Analyze performance
- `POST /api/upload-sheet-music` - Upload sheet music
- `POST /api/process-musicxml` - Process MusicXML

### T5AI Firmware (`firmware/`)
C++ firmware for Tuya T5AI development board:
- Audio recording (16kHz, stereo)
- Real-time audio streaming to cloud
- Text-to-Speech feedback output
- BLE communication with mobile app
- Wi-Fi connectivity

**Configuration**: Edit `config.hpp` for Wi-Fi, backend URL, and device settings

## Quick Start

### Automated Setup

Run the setup script to install all dependencies:

```bash
./setup.sh
```

### Manual Setup

#### Cloud Backend

```bash
cd cloud-backend
python3 -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install --upgrade pip
pip install -r requirements.txt
cp .env.example .env  # Then edit .env and add your ANTHROPIC_API_KEY
python server.py
```

Or use the convenience script:
```bash
./start-backend.sh
```

#### Mobile App

```bash
cd mobile-app
npm install
npm start
```

Or use the convenience script:
```bash
./start-mobile.sh
```

### Configuration

1. **Backend**: Edit `cloud-backend/.env` and add your `ANTHROPIC_API_KEY`
2. **Mobile App**: Edit `mobile-app/src/config.js` to update `API_BASE_URL` if needed

### Firmware Setup

Edit `firmware/config.hpp` to configure:
- Wi-Fi SSID and password
- Cloud backend URL
- BLE device name
- Audio sample rate and buffer size

Build with ESP-IDF or Tuya IoT Development Platform SDK.

## Features

- **Sheet Music Recognition**: Upload photos/PDFs and extract musical notation
- **Real-time Analysis**: Pitch detection, timing analysis, dynamics detection
- **AI Coaching**: Personalized feedback using Claude API
- **Visual Timeline**: Interactive mistake markers with audio playback
- **TTS Feedback**: Spoken coaching via T5AI onboard speaker

## Development Roadmap

- [x] Project structure
- [ ] Phase 1: MVP (basic pitch detection)
- [ ] Phase 2: Core features (OMR, TTS, dynamics)
- [ ] Phase 3: Advanced features (multi-instrument, progress tracking)

## License

MIT

