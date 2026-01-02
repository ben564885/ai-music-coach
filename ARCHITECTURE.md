# System Architecture - What You Actually Need

## Hardware Components

### 1. **T5AI Development Board** ✅
- Dual microphones (for stereo audio recording)
- Speaker (for TTS feedback output)
- Bluetooth LE 5.4 (for mobile app communication)
- Wi-Fi (for cloud backend communication)
- Button/GPIO (for start/stop recording)
- **Screen Module** (optional but recommended):
  - Real-time note display
  - Fingering charts
  - Wrong note detection
  - Visual feedback
- The board uses:
  - Button press to control recording
  - TTS (text-to-speech) for audio feedback
  - Screen module for visual fingering guidance
  - BLE to receive commands from mobile app
  - Real-time pitch detection for immediate feedback

### 2. **Mobile Phone/Tablet** ✅
- Runs the React Native mobile app
- This is your "screen/touch panel" - the UI runs here!
- Connects to T5AI board via Bluetooth
- Displays recordings, mistake timeline, sheet music

## Software/Cloud Services

### 3. **Supabase** ✅
- PostgreSQL database (stores recordings, sheet music)
- Authentication (email/password + Google OAuth)
- File storage (for audio files and PDFs - optional, currently using local storage)

### 4. **Claude AI (Anthropic)** ✅
- Generates personalized coaching feedback
- Analyzes mistakes and provides actionable suggestions

### 5. **Backend Server** ✅
- Python Flask server (runs on your computer or cloud server)
- Handles audio analysis (pitch detection, timing, dynamics)
- Connects to Supabase database
- Calls Claude API for coaching feedback
- Can run on:
  - Your local computer (for development)
  - Cloud server (AWS, Google Cloud, Heroku, etc.)
  - Raspberry Pi
  - Any machine with Python

## Complete System Flow

```
[User's Phone] ←→ [T5AI Board] ←→ [Your Backend Server] ←→ [Supabase + Claude]
     (UI)         (Hardware)          (Analysis)            (Storage + AI)
```

## What Each Component Does

### T5AI Board
- Records audio via microphones
- Speaks feedback via speaker
- Communicates with phone via Bluetooth
- Uploads audio to backend via Wi-Fi
- **No screen needed** - controlled by button + phone app

### Mobile Phone
- User interface (all screens, buttons, displays)
- Bluetooth connection to T5AI board
- Shows recordings list, mistake timeline, sheet music
- Handles authentication, file uploads

### Backend Server
- Receives audio from T5AI board
- Analyzes audio (pitch, timing, dynamics)
- Compares to sheet music
- Calls Claude API for feedback
- Stores results in Supabase database

### Supabase
- Stores user accounts
- Stores recordings and sheet music metadata
- Handles authentication

### Claude AI
- Generates natural-language coaching feedback
- Provides specific, actionable suggestions

## Screen Module - Now Supported!

**Yes!** The T5AI board now supports a screen module for:

1. **Real-Time Note Detection**: Shows what note you're playing as you play it
2. **Wrong Note Feedback**: Immediately displays correct fingering when wrong note detected
3. **Fingering Charts**: Visual diagrams for correct fingerings on your instrument
4. **Scale Practice**: Shows expected notes in scale with fingerings
5. **Visual Feedback**: Green = correct, Red = wrong note

**Example**: Playing C major scale, hit Bb instead of A → Screen immediately shows:
- "You played Bb"
- "Expected: A"  
- Fingering diagram for A
- TTS speaks: "You played B flat, but the correct note is A. Here's how to finger it."

The screen module enables **standalone practice** - you can practice with just the T5AI board without needing your phone!

## Minimum Requirements Summary

✅ **Hardware:**
- T5AI Development Board
- Screen Module (SPI display - ST7789, ILI9341, or similar) - **Recommended for real-time feedback**
- User's smartphone/tablet (for initial setup and review)

✅ **Cloud Services:**
- Supabase account (free tier works)
- Anthropic/Claude API key

✅ **Infrastructure:**
- Backend server (can be your computer, cloud server, or Raspberry Pi)

**Screen Module Benefits:**
- Real-time note detection and display
- Immediate fingering guidance when wrong notes detected
- Standalone practice (can practice without phone)
- Visual feedback for faster learning

The screen module transforms the T5AI board into a complete standalone practice assistant!

