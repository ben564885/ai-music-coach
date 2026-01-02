# What Your AI Music Coach Will Do (With T5AI Board)

## 🎵 Complete User Flow

### 1. **Setup & Connection**
- **Mobile App**: User signs up/logs in with email/password or Google
- **Mobile App**: User taps "Connect Device" → Scans for T5AI board via Bluetooth
- **T5AI Board**: Appears as "T5AI-MusicCoach" in Bluetooth scan
- **Connection**: Establishes BLE connection between phone and board
- **Status**: Shows "Connected" with green indicator

### 2. **Sheet Music Upload**
- **Mobile App**: User uploads sheet music via:
  - Camera photo of sheet music
  - PDF file upload
  - MusicXML/MIDI file upload
- **Backend**: Processes and stores sheet music in database
- **Mobile App**: Sends sheet music data to T5AI board via Bluetooth
- **T5AI Board**: Receives sheet music data, stores it locally
- **T5AI Board**: Speaks "Sheet music received. Ready to record."

### 3. **Recording Performance**
- **T5AI Board**: User presses button on board to start recording
- **T5AI Board**: Speaks "Recording started. Begin playing when ready."
- **T5AI Board**: Records audio using dual microphones (16kHz, stereo)
- **T5AI Board**: Streams audio chunks to cloud backend in real-time via Wi-Fi
- **T5AI Board**: Also saves audio locally as backup
- **User**: Plays their instrument (piano, violin, guitar, etc.)
- **T5AI Board**: User presses button again to stop recording
- **T5AI Board**: Speaks "Recording complete. Analyzing your performance..."

### 4. **AI Analysis**
- **Backend**: Receives audio file from T5AI board
- **Backend**: Analyzes audio using:
  - **Pitch Detection** (PYIN algorithm): Detects what notes were actually played
  - **Timing Analysis**: Detects hesitations, rushing, tempo deviations
  - **Dynamics Analysis**: Compares volume levels to sheet music markings (piano, forte, etc.)
- **Backend**: Compares performance to uploaded sheet music:
  - Wrong notes (e.g., played B-natural instead of B-flat)
  - Timing issues (paused too long, rushed through section)
  - Dynamics issues (played softly when forte was marked)
- **Backend**: Generates AI coaching feedback using Claude API:
  - Encouraging, personalized feedback
  - Specific measure numbers
  - Actionable suggestions
  - Sheet music marking recommendations

### 5. **Feedback Delivery**
- **Backend**: Sends analysis results back to T5AI board
- **T5AI Board**: Receives feedback via Wi-Fi
- **T5AI Board**: Speaks the AI coaching feedback through onboard speaker
  - Example: "Great start! In measure 8, you're playing a B-natural, but the sheet shows a B-flat. Circle that flat sign..."
- **Backend**: Also sends results to mobile app
- **Mobile App**: Displays detailed mistake timeline with:
  - Color-coded mistake markers (red=wrong note, orange=timing, blue=dynamics)
  - Timestamp for each mistake
  - Clickable markers to jump to that moment in audio
  - Full AI feedback text
  - Statistics (total mistakes, breakdown by type)

### 6. **Review & Practice**
- **Mobile App**: User sees all recordings in a beautiful list
- **Mobile App**: Can search/filter recordings
- **Mobile App**: Tap a recording to see:
  - Full mistake timeline
  - Audio playback with seekable markers
  - AI coaching feedback
  - Associated sheet music
- **Mobile App**: Can delete recordings
- **Database**: All recordings stored securely, user-specific

## 🎯 Key Features

### Real-Time Analysis
- Audio streams to cloud during recording (not just after)
- Analysis starts before recording finishes
- Fast feedback turnaround

### Intelligent Mistake Detection
- **Note Accuracy**: Detects wrong notes with ±10Hz tolerance
- **Timing Issues**: 
  - Hesitations (paused 50%+ longer than expected)
  - Rushing (played 20%+ faster than expected)
  - Overall tempo deviations
- **Dynamics**: Compares actual volume to sheet music markings (p, mp, mf, f, ff)

### AI-Powered Coaching
- Uses Claude AI to generate natural, encouraging feedback
- References specific measures and beats
- Suggests marking sheet music
- Groups related mistakes together
- Provides actionable practice tips

### Multi-Modal Feedback
- **T5AI Board**: Spoken feedback via TTS (immediate)
- **Mobile App**: Visual timeline with detailed breakdown (for review)

### User Management
- Each user has their own recordings
- Secure authentication
- Data privacy (users only see their own data)

## 📊 Example Scenario

**User plays piano piece:**

1. **Upload**: Takes photo of sheet music → Uploads to app
2. **Connect**: Connects to T5AI board via Bluetooth
3. **Record**: Presses button on board → Plays piece → Presses button again
4. **Analysis**: System detects:
   - Wrong note in measure 8 (B-natural instead of B-flat)
   - Hesitation before measure 12 (paused 200ms too long)
   - Tempo sped up from 120 BPM to 132 BPM in middle section
   - Missed forte marking in measure 16 (played softly)
5. **Feedback**: 
   - **T5AI speaks**: "Great start! In measure 8, you're playing a B-natural, but the sheet shows a B-flat. Circle that flat sign right now..."
   - **Mobile app shows**: Interactive timeline with 4 mistake markers, each clickable to hear that moment
6. **Review**: User reviews mistakes on phone, marks up sheet music, practices again

## 🔧 Technical Capabilities

- **Audio Quality**: 16kHz, 16-bit, stereo recording
- **Pitch Detection**: PYIN algorithm (robust to noise)
- **Real-Time Streaming**: Audio chunks sent every 100ms during recording
- **Bluetooth LE**: Low-power, reliable connection
- **Wi-Fi**: Fast upload to cloud backend
- **Database**: PostgreSQL with Row Level Security
- **Authentication**: Secure JWT tokens
- **Storage**: All recordings and sheet music stored in database

## 🎓 Educational Value

- **Immediate Feedback**: Know mistakes right after playing
- **Specific Guidance**: Exact measure numbers, not vague comments
- **Visual Learning**: See mistakes on timeline, hear them in context
- **Progress Tracking**: All recordings saved for comparison over time
- **Practice History**: Review past performances to see improvement

This is a complete, production-ready music practice assistant! 🎉

