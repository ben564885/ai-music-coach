# Complete Project Architecture - Software & Firmware

## 🎯 System Overview

An AI-powered music practice assistant that uses a Tuya T5AI development board to record performances, analyze them against sheet music, and provide real-time coaching feedback through multiple channels (TTS, screen display, mobile app).

```
┌─────────────────┐      BLE       ┌──────────────┐      Wi-Fi      ┌──────────────┐
│   Mobile App    │◄──────────────►│  T5AI Board  │◄──────────────►│ Cloud Backend│
│   (Flutter)    │                 │  (C++ FW)    │                 │  (Python)    │
└─────────────────┘                 └──────────────┘                 └──────────────┘
       │                                    │                                  │
       │                                    │                                  │
       ▼                                    ▼                                  ▼
┌─────────────────┐                 ┌──────────────┐                 ┌──────────────┐
│   Supabase      │                 │ Screen Module│                 │ Claude AI    │
│   (Database)    │                 │ (Optional)   │                 │ (Feedback)   │
└─────────────────┘                 └──────────────┘                 └──────────────┘
```

---

## 📱 **1. MOBILE APP (Flutter)**

### Technology Stack
- **Framework**: Flutter (Dart)
- **State Management**: BLoC pattern
- **Backend Communication**: HTTP REST API
- **Device Communication**: Bluetooth LE (`flutter_blue_plus`)
- **Authentication**: Supabase Auth (email/password + Google OAuth)
- **Storage**: Supabase PostgreSQL database

### Key Components

#### **1.1 Authentication System**
- **Location**: `lib/blocs/auth/`, `lib/repositories/auth_repository.dart`
- **Flow**:
  1. User signs up/logs in via email/password or Google OAuth
  2. Supabase generates JWT token
  3. Token stored locally and sent with all API requests
  4. Token validated on backend via `@require_auth` decorator

#### **1.2 Device Connection (BLE)**
- **Location**: `lib/services/ble_service.dart`
- **Flow**:
  1. User taps "Connect Device" → Starts BLE scan
  2. Scans for devices named "T5AI-MusicCoach"
  3. User selects device → Establishes BLE connection
  4. Connection state streamed to UI
  5. Device saved to user preferences for auto-connect

#### **1.3 Sheet Music Upload**
- **Location**: `lib/screens/sheet_music_upload_screen.dart`, `sheet_music_process_screen.dart`
- **Flow**:
  1. User uploads sheet music via:
     - Camera photo
     - PDF file picker
     - MusicXML file
  2. File sent to backend `/api/upload-sheet-music`
  3. Backend uses Gemini Vision API to transcribe image → JSON
  4. JSON reference data returned to mobile app
  5. Mobile app sends reference data to T5AI board via BLE
  6. Sheet music stored in Supabase database

#### **1.4 Recording Management**
- **Location**: `lib/screens/recordings_screen.dart`
- **Features**:
  - List all user recordings
  - View mistake timeline with color-coded markers
  - Audio playback with seekable markers
  - View AI coaching feedback
  - Delete recordings
  - Filter/search recordings

#### **1.5 Real-Time Feedback Display**
- **Location**: `lib/screens/recording_detail_screen.dart`
- **Features**:
  - Interactive timeline showing mistakes
  - Color coding: Red (wrong notes), Orange (timing), Blue (dynamics)
  - Clickable markers → jump to audio timestamp
  - Full AI feedback text display
  - Statistics (total mistakes, breakdown by type)

### Data Flow: Mobile App → T5AI Board

**BLE Message Types** (defined in `firmware/types.h`):
- `BLE_MSG_SHEET_MUSIC`: Send sheet music reference data
- `BLE_MSG_START_RECORDING`: Start recording command
- `BLE_MSG_STOP_RECORDING`: Stop recording command
- `BLE_MSG_SET_INSTRUMENT`: Set instrument type (piano, violin, etc.)
- `BLE_MSG_SET_SCALE`: Set scale for practice mode

**Example**: Sending sheet music data
```dart
// Mobile app sends JSON reference data via BLE
bleService.writeData(
  serviceUuid: "0000ff00-0000-1000-8000-00805f9b34fb",
  characteristicUuid: "0000ff01-0000-1000-8000-00805f9b34fb",
  data: utf8.encode(jsonEncode(referenceData))
);
```

---

## 🔧 **2. FIRMWARE (C++ on T5AI Board)**

### Technology Stack
- **Platform**: Tuya T5AI Development Board (ESP32-based)
- **Framework**: ESP-IDF / Tuya IoT SDK
- **Language**: C++17
- **RTOS**: FreeRTOS (multi-tasking)

### Hardware Capabilities
- **Dual microphones**: Stereo audio recording (16kHz, 16-bit)
- **Speaker**: TTS audio output
- **Bluetooth LE 5.4**: Communication with mobile app
- **Wi-Fi**: Cloud backend communication
- **GPIO Button**: Start/stop recording
- **Screen Module** (optional): SPI display (ST7789/ILI9341) for visual feedback

### Key Components

#### **2.1 Initialization (`main.cpp`)**
```cpp
app_init() {
  - Initialize NVS (non-volatile storage)
  - Connect to Wi-Fi
  - Initialize BLE with device name "T5AI-MusicCoach"
  - Initialize audio recorder (dual mics)
  - Initialize TTS engine
  - Initialize cloud client (HTTP)
  - Initialize display manager (if screen module present)
  - Initialize real-time analyzer
}
```

#### **2.2 Application State**
- **Location**: `firmware/main.cpp` - `AppState` class
- **State Variables**:
  - `is_recording`: Recording status
  - `is_connected`: BLE connection status
  - `reference_data`: Sheet music data from mobile app
  - `feedback_text`: AI coaching feedback
  - `current_instrument`: Piano, violin, guitar, etc.
  - `real_time_feedback_enabled`: Enable/disable real-time note detection

#### **2.3 Recording Flow**

**Start Recording**:
1. User presses button OR receives `BLE_MSG_START_RECORDING`
2. Check prerequisites:
   - BLE connected? → TTS: "Please connect mobile app first"
   - Sheet music loaded? → TTS: "Please upload sheet music"
3. Initialize audio config (16kHz, stereo, 16-bit)
4. Start audio recorder
5. TTS: "Recording started. Begin playing when ready."
6. Start audio streaming task

**During Recording**:
- **Audio Streaming Task** (`audio_streaming_task`):
  1. Read audio chunk every 100ms
  2. Stream chunk to cloud backend via Wi-Fi (`cloud_client_stream_audio`)
  3. Save chunk to local buffer
  4. **Real-time analysis** (if enabled):
     - Process chunk through `RealTimeAnalyzer`
     - Detect current note being played
     - Compare to expected note from sheet music
     - If wrong note detected:
       - Display on screen: "You played X, Expected Y"
       - Show fingering chart for correct note
       - TTS: "You played X, but correct note is Y. Here's how to finger it."
     - If correct note:
       - Display note name on screen
       - Green indicator

**Stop Recording**:
1. User presses button again OR receives `BLE_MSG_STOP_RECORDING`
2. Stop audio recorder
3. TTS: "Recording complete. Analyzing your performance..."
4. Upload complete audio file to cloud backend
5. Wait for analysis results

#### **2.4 BLE Communication**

**Receiving Messages** (from mobile app):
```cpp
ble_message_t* msg = ble_manager_receive_message();
switch (msg->type) {
  case BLE_MSG_SHEET_MUSIC:
    // Store reference data
    app_state.reference_data = parse_music_data(msg->data);
    tts_engine_speak("Sheet music received. Ready to record.");
    break;
    
  case BLE_MSG_START_RECORDING:
    handle_record_button(); // Start recording
    break;
    
  case BLE_MSG_SET_INSTRUMENT:
    app_state.current_instrument = parse_instrument(msg->data);
    break;
}
```

**Sending Messages** (to mobile app):
- Analysis results after cloud processing
- Connection status updates
- Error messages

#### **2.5 Real-Time Note Detection**

**Location**: `firmware/real_time_analyzer.h`

**Process**:
1. Audio chunk received (100ms window)
2. Fast pitch detection using autocorrelation
3. Convert frequency → note name (C, D, E, etc.) + octave
4. Get expected note from `reference_data` at current timestamp
5. Compare detected vs expected:
   - **Match**: Display note on screen (green)
   - **Mismatch**: 
     - Display wrong note feedback (red)
     - Show correct fingering chart
     - Speak TTS feedback

**Confidence Threshold**: Only triggers feedback if confidence > 70%

#### **2.6 Display Module** (Optional Screen)

**Location**: `firmware/display_manager.h`

**Capabilities**:
- **Current Note Display**: Shows note being played in real-time
- **Wrong Note Feedback**: "You played X, Expected Y" + fingering chart
- **Scale Practice Mode**: Shows current note in scale + next expected note
- **Recording Status**: Visual indicator (red dot when recording)
- **Connection Status**: Shows BLE connection state

**Fingering Charts**:
- Stored in firmware (`firmware/fingering_charts.h`)
- Instrument-specific (piano, violin, guitar, flute, clarinet, trumpet, saxophone)
- Visual diagrams + text descriptions
- Can be updated via BLE

#### **2.7 Cloud Communication**

**Upload Audio**:
- HTTP POST to `/api/analyze`
- Multipart form data: audio file + reference data + metadata
- Wait for analysis results
- Receive JSON response with mistakes + feedback

**Stream Audio** (real-time):
- HTTP POST chunks every 100ms during recording
- Allows backend to start analysis before recording finishes

---

## ☁️ **3. CLOUD BACKEND (Python Flask)**

### Technology Stack
- **Framework**: Flask (Python)
- **Audio Processing**: librosa, numpy, scipy
- **Music Analysis**: music21
- **AI**: Google Gemini API (Gemini 2.5 Flash)
- **Database**: Supabase PostgreSQL
- **Authentication**: JWT tokens (Supabase)

### Key Components

#### **3.1 Authentication**
- **Location**: `cloud-backend/auth/auth_utils.py`
- **Decorator**: `@require_auth`
- **Flow**:
  1. Extract JWT token from `Authorization: Bearer <token>` header
  2. Verify token with Supabase
  3. Extract `user_id` and `user_email`
  4. Attach to `request` object
  5. All protected endpoints require valid token

#### **3.2 Audio Analysis**
- **Location**: `cloud-backend/analysis/audio_analyzer.py`

**Analysis Pipeline**:
1. **Load Audio**: librosa loads WAV/MP3 file (resampled to 16kHz)
2. **Pitch Detection**: PYIN algorithm extracts pitch track
3. **Onset Detection**: Detects note onsets (when notes start)
4. **Beat Tracking**: Detects tempo and beat positions
5. **RMS Analysis**: Extracts dynamics (volume levels)

**Mistake Detection**:
- **Note Accuracy** (`_analyze_note_accuracy`):
  - Compare detected pitch to expected note frequency
  - Tolerance: ±10Hz (configurable)
  - Records: timestamp, expected note, played note, frequency deviation
  
- **Timing Analysis** (`_analyze_timing`):
  - Compare detected onsets to expected timing
  - Detects hesitations (paused 50%+ longer than expected)
  - Detects rushing (played 20%+ faster than expected)
  - Detects tempo deviations
  
- **Dynamics Analysis** (`_analyze_dynamics`):
  - Compare RMS levels to sheet music markings (p, mp, mf, f, ff)
  - Detects when played too soft/loud compared to markings

**Output**: List of mistakes, each with:
```python
{
  "type": "wrong_note" | "timing" | "dynamics",
  "timestamp": 1.23,  # seconds
  "description": "Played B-natural instead of B-flat",
  "measure": 8,
  "beat": 2,
  "severity": "minor" | "major"
}
```

#### **3.3 AI Coaching**
- **Location**: `cloud-backend/analysis/coach.py`

**Feedback Generation** (`generate_feedback`):
1. Receives list of mistakes + reference sheet music
2. Groups related mistakes together
3. Calls Gemini API with prompt:
   - List of mistakes with measure numbers
   - Sheet music context
   - Request for encouraging, actionable feedback
4. Gemini generates natural-language feedback:
   - Encouraging tone
   - Specific measure references
   - Actionable suggestions
   - Sheet music marking recommendations

**Sheet Music Transcription** (`transcribe_image`):
- Uses Gemini Vision API
- Uploads sheet music image (photo/PDF)
- Gemini extracts:
  - Notes with timestamps
  - Tempo
  - Key signature
  - Time signature
  - Dynamics markings
- Returns structured JSON reference data

#### **3.4 API Endpoints**

**Authentication**:
- `POST /api/auth/verify` - Verify JWT token

**Recordings**:
- `GET /api/recordings` - Get all user recordings
- `GET /api/recordings/<id>` - Get specific recording
- `DELETE /api/recordings/<id>` - Delete recording

**Analysis**:
- `POST /api/analyze` - Analyze performance
  - Input: audio file, reference data, metadata
  - Output: mistakes list, AI feedback, recording ID

**Sheet Music**:
- `GET /api/sheet-music` - Get all user sheet music
- `POST /api/upload-sheet-music` - Upload and transcribe sheet music
- `POST /api/process-musicxml` - Process MusicXML data

#### **3.5 Database Integration**
- **Location**: `cloud-backend/database/repository.py`
- **Models**: `Recording`, `SheetMusic` (see `database/models.py`)
- **Operations**:
  - Create/read/update/delete recordings
  - Create/read sheet music
  - User-specific queries (Row Level Security)

---

## 🔄 **4. COMPLETE SYSTEM FLOW**

### **Scenario: User Practices Piano Piece**

#### **Step 1: Setup & Connection**
1. User opens mobile app → Logs in (Supabase Auth)
2. User taps "Connect Device" → BLE scan starts
3. T5AI board appears as "T5AI-MusicCoach"
4. User connects → BLE connection established
5. Mobile app shows "Connected" status

#### **Step 2: Upload Sheet Music**
1. User taps "Upload Sheet Music" → Takes photo of sheet music
2. Mobile app uploads image to `/api/upload-sheet-music`
3. Backend uses Gemini Vision → Transcribes to JSON:
   ```json
   {
     "notes": [
       {"note": "C4", "timestamp": 0.0, "measure": 1, "beat": 1},
       {"note": "D4", "timestamp": 0.5, "measure": 1, "beat": 2},
       ...
     ],
     "tempo": 120,
     "key": "C major",
     "timeSignature": "4/4"
   }
   ```
4. Backend saves to Supabase database
5. Mobile app receives JSON → Sends to T5AI board via BLE
6. T5AI board stores reference data → TTS: "Sheet music received. Ready to record."

#### **Step 3: Recording Performance**
1. User presses button on T5AI board
2. T5AI board starts recording (16kHz, stereo)
3. TTS: "Recording started. Begin playing when ready."
4. **Real-time streaming**:
   - Every 100ms: Audio chunk → Cloud backend
   - Backend starts preliminary analysis
5. **Real-time feedback** (if enabled):
   - T5AI board detects notes in real-time
   - Compares to expected notes
   - If wrong note: Screen shows fingering + TTS speaks feedback
6. User plays piano piece
7. User presses button again → Recording stops
8. TTS: "Recording complete. Analyzing your performance..."

#### **Step 4: Cloud Analysis**
1. T5AI board uploads complete audio file to `/api/analyze`
2. Backend receives audio + reference data
3. **Audio Analysis**:
   - Pitch detection (PYIN algorithm)
   - Timing analysis
   - Dynamics analysis
   - Mistake detection
4. **AI Coaching**:
   - Mistakes sent to Gemini API
   - Gemini generates personalized feedback
5. Backend saves recording to database
6. Backend sends results to T5AI board

#### **Step 5: Feedback Delivery**
1. **T5AI Board**:
   - Receives analysis results via Wi-Fi
   - TTS speaks AI feedback: "Great start! In measure 8, you're playing a B-natural, but the sheet shows a B-flat..."
   - Sends results to mobile app via BLE

2. **Mobile App**:
   - Receives results via BLE
   - Displays mistake timeline:
     - Red markers: Wrong notes
     - Orange markers: Timing issues
     - Blue markers: Dynamics issues
   - Shows full AI feedback text
   - User can tap markers → Jump to audio timestamp

#### **Step 6: Review & Practice**
1. User reviews mistakes on mobile app
2. User listens to recording with mistake markers
3. User marks up sheet music (manual annotation)
4. User practices again → Repeat from Step 3

---

## 🎯 **5. KEY TECHNICAL DECISIONS**

### **Why BLE for Mobile ↔ T5AI?**
- Low power consumption
- Direct device-to-device communication (no cloud needed)
- Low latency for real-time commands
- Standard protocol (works on iOS and Android)

### **Why Wi-Fi for T5AI ↔ Backend?**
- Large audio file uploads
- Real-time audio streaming (100ms chunks)
- Reliable for large data transfers
- Lower latency than BLE for cloud communication

### **Why Real-Time Analysis on T5AI?**
- Immediate feedback (no waiting for cloud)
- Works offline (no Wi-Fi needed for basic feedback)
- Reduces cloud processing load
- Better user experience (instant correction)

### **Why Cloud Analysis?**
- More sophisticated algorithms (PYIN pitch detection)
- AI-powered feedback generation
- Historical data storage
- Cross-device access (review on phone later)

### **Why Gemini Vision for Sheet Music?**
- Handles photos, PDFs, handwritten music
- More accurate than traditional OMR
- Extracts metadata (key, tempo, time signature)
- No need for specialized OMR libraries

---

## 📊 **6. DATA STRUCTURES**

### **Reference Data (Sheet Music)**
```json
{
  "title": "Piano Sonata No. 1",
  "notes": [
    {
      "note": "C4",
      "frequency": 261.63,
      "timestamp": 0.0,
      "measure": 1,
      "beat": 1,
      "duration": 0.5
    },
    ...
  ],
  "tempo": 120,
  "keySignature": "C major",
  "timeSignature": "4/4",
  "dynamics": [
    {"timestamp": 2.0, "marking": "f"},
    {"timestamp": 4.0, "marking": "p"}
  ]
}
```

### **Mistake Object**
```json
{
  "type": "wrong_note",
  "timestamp": 1.23,
  "description": "Played B-natural instead of B-flat",
  "measure": 8,
  "beat": 2,
  "expected": "Bb4",
  "played": "B4",
  "frequency_deviation": 15.5,
  "severity": "minor"
}
```

### **Analysis Result**
```json
{
  "recording_id": "uuid-here",
  "mistakes": [...],
  "feedback": "Great start! In measure 8...",
  "statistics": {
    "total_mistakes": 4,
    "wrong_notes": 2,
    "timing_issues": 1,
    "dynamics_issues": 1
  }
}
```

---

## 🔐 **7. SECURITY & AUTHENTICATION**

### **Authentication Flow**
1. User logs in → Supabase generates JWT token
2. Token stored in mobile app (secure storage)
3. All API requests include: `Authorization: Bearer <token>`
4. Backend verifies token with Supabase
5. Row Level Security (RLS) in Supabase ensures users only see their data

### **Data Privacy**
- All recordings are user-specific
- Sheet music is user-specific
- No cross-user data access
- Audio files stored securely (Supabase Storage or local)

---

## 🚀 **8. DEPLOYMENT**

### **Mobile App**
- **Platform**: iOS (App Store) + Android (Google Play)
- **Build**: Flutter build commands
- **CI/CD**: GitHub Actions or similar

### **Backend**
- **Options**:
  - Local development server
  - Cloud server (AWS, Google Cloud, Heroku, Railway)
  - Raspberry Pi
- **Requirements**: Python 3.9+, Flask, dependencies from `requirements.txt`

### **Firmware**
- **Build**: ESP-IDF or Tuya IoT Development Platform
- **Flash**: USB cable to T5AI board
- **Configuration**: Edit `firmware/config.hpp` (Wi-Fi, backend URL)

---

## 📝 **9. CONFIGURATION FILES**

### **Firmware Config** (`firmware/config.hpp`)
```cpp
#define WIFI_SSID "YourWiFi"
#define WIFI_PASSWORD "password"
#define CLOUD_BACKEND_URL "https://your-backend.com"
#define T5AI_DEVICE_NAME "T5AI-MusicCoach"
#define AUDIO_SAMPLE_RATE 16000
#define ENABLE_DISPLAY_MODULE true
#define ENABLE_REAL_TIME_ANALYSIS true
```

### **Backend Config** (`.env`)
```
GEMINI_API_KEY=your-key-here
SUPABASE_URL=https://your-project.supabase.co
SUPABASE_KEY=your-anon-key
PORT=5001
```

### **Mobile App Config** (`.env`)
```
SUPABASE_URL=https://your-project.supabase.co
SUPABASE_ANON_KEY=your-anon-key
BACKEND_URL=https://your-backend.com
```

---

This architecture provides a complete, production-ready music practice assistant with real-time feedback, AI coaching, and multi-modal user experience! 🎵

