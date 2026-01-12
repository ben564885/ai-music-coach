# AI Music Coach - Data Flow Chart

## Complete Data Flow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           USER INTERACTION FLOW                              │
└─────────────────────────────────────────────────────────────────────────────┘

1. RECORDING FLOW
   ┌──────────────┐      ┌──────────────┐      ┌──────────────┐      ┌──────────────┐
   │   T5AI       │      │   Firmware   │      │   Flask      │      │   Supabase   │
   │   Device     │─────▶│   (C/C++)    │─────▶│   Backend    │─────▶│   Database   │
   │              │      │              │      │   (Python)   │      │              │
   │ • Mic Input  │      │ • WAV Encode │      │ • Store File │      │ • Recording  │
   │ • Button     │      │ • HTTP POST  │      │ • Save Meta  │      │   Metadata   │
   └──────────────┘      └──────────────┘      └──────────────┘      └──────────────┘
        │                       │                       │
        │                       │                       │
        ▼                       ▼                       ▼
   [User presses]          [Upload WAV]            [Store in DB]
   [record button]         [to /api/firmware/      [with user_id,
   [on device]             upload]                 device_id,
                                                    timestamp]

2. SHEET MUSIC UPLOAD FLOW
   ┌──────────────┐      ┌──────────────┐      ┌──────────────┐      ┌──────────────┐
   │   Flutter    │      │   Flask      │      │   SoundSlice │      │   Supabase   │
   │   App        │─────▶│   Backend    │─────▶│   / OMR      │─────▶│   Database   │
   │              │      │              │      │              │      │              │
   │ • Image/PDF  │      │ • Receive    │      │ • OMR Process│      │ • Sheet Music│
   │ • User Picks │      │   Upload     │      │ • MusicXML   │      │   Metadata   │
   │   File       │      │ • Trigger    │      │ • Extract    │      │ • Reference  │
   └──────────────┘      │   OMR        │      │   Notes      │      │   Data       │
                          └──────────────┘      └──────────────┘      └──────────────┘
                                │                       │
                                │                       │
                                ▼                       ▼
                          [POST /api/upload-    [Playwright auto-  [Store reference_data
                          sheet-music]          mates SoundSlice   with notes, clef,
                          [with user_id,        web UI]            time_signature]
                          file, title]          [oemer processes]  [Store file_url]

3. ANALYSIS FLOW
   ┌──────────────┐      ┌──────────────┐      ┌──────────────┐      ┌──────────────┐
   │   Flutter    │      │   Flask      │      │   Audio      │      │   Gemini AI  │
   │   App        │─────▶│   Backend    │─────▶│   Analysis   │─────▶│   API        │
   │              │      │              │      │              │      │              │
   │ • User       │      │ • Get        │      │ • librosa    │      │ • Generate   │
   │   Selects    │      │   Recording  │      │   Pitch      │      │   Feedback   │
   │   Recording  │      │ • Get Sheet  │      │ • music21    │      │ • Coaching   │
   │   + Sheet    │      │   Music       │      │   Compare    │      │   Tips       │
   └──────────────┘      └──────────────┘      └──────────────┘      └──────────────┘
        │                       │                       │                       │
        │                       │                       │                       │
        ▼                       ▼                       ▼                       ▼
   [POST /api/device/    [Load WAV from    [Extract pitches,  [Send analysis
    analyze]             storage]           compare to sheet   results + context
   {recording_id,        [Load reference_  music notes]       to Gemini]
    sheet_music_id}]      data from DB]     [Calculate timing  [Get natural
                          [Call coach.py]   errors, wrong      language feedback]
                                            notes, dynamics]

4. RESULTS STORAGE & RETRIEVAL
   ┌──────────────┐      ┌──────────────┐      ┌──────────────┐
   │   Flask      │      │   Supabase   │      │   Flutter    │
   │   Backend    │─────▶│   Database   │◀─────│   App        │
   │              │      │              │      │              │
   │ • Store      │      │ • Analysis   │      │ • Fetch      │
   │   Analysis   │      │   Results    │      │   Results    │
   │ • Save       │      │ • Mistakes   │      │ • Display    │
   │   Feedback   │      │ • Feedback   │      │   Timeline   │
   └──────────────┘      └──────────────┘      └──────────────┘
        │                       │                       │
        │                       │                       │
        ▼                       ▼                       ▼
   [POST to analyses    [Store in analyses  [GET /api/analyses]
    table]              table with:         [Display on home
   [Save:               - recording_id      screen]
    - mistakes          - sheet_music_id    [Show mistake
    - feedback           - mistakes JSON     timeline]
    - score              - feedback text     [Show AI feedback]
    - timestamp]         - score             [Show score]

5. FIRMWARE DATA FETCH FLOW
   ┌──────────────┐      ┌──────────────┐      ┌──────────────┐
   │   T5AI       │      │   Flask      │      │   Supabase   │
   │   Firmware   │─────▶│   Backend    │─────▶│   Database   │
   │              │      │              │      │              │
   │ • Request    │      │ • Lookup     │      │ • Query by   │
   │   Recordings │      │   User by    │      │   user_id    │
   │ • Request    │      │   Device ID  │      │ • Return     │
   │   Sheet Music│      │ • Query DB   │      │   Results    │
   └──────────────┘      └──────────────┘      └──────────────┘
        │                       │                       │
        │                       │                       │
        ▼                       ▼                       ▼
   [GET /api/device/     [Device UUID →    [SELECT recordings
    recordings]          user_id lookup]     WHERE user_id = ?]
   [GET /api/device/     [SELECT sheet_     [SELECT sheet_music
    sheet-music]         music WHERE        WHERE user_id = ?]
   [X-User-ID header]    user_id = ?]       [Return lite JSON
                          [Return lite      with only essential
                          JSON response]    fields for firmware]

┌─────────────────────────────────────────────────────────────────────────────┐
│                         DETAILED STEP-BY-STEP FLOWS                         │
└─────────────────────────────────────────────────────────────────────────────┘

A. RECORDING PROCESS
   Step 1: User presses button on T5AI device
   Step 2: Firmware starts recording via microphones
   Step 3: Firmware encodes audio to WAV format
   Step 4: Firmware uploads WAV via HTTP POST to Flask backend
   Step 5: Flask receives file, saves to local storage
   Step 6: Flask stores metadata in Supabase (user_id, device_id, timestamp, file_path)
   Step 7: Flask returns recording_id to firmware
   Step 8: Firmware displays success message

B. SHEET MUSIC PROCESSING
   Step 1: User selects image/PDF in Flutter app
   Step 2: Flutter uploads file to Flask backend
   Step 3: Flask saves file to local storage
   Step 4: Flask triggers OMR processing:
           a. If SoundSlice URL: Use Playwright to automate web UI
           b. If image: Use oemer or Roboflow for OMR
   Step 5: OMR extracts notes, clef, time signature, key signature
   Step 6: Flask stores reference_data in Supabase
   Step 7: Flask returns sheet_music_id to Flutter app
   Step 8: Flutter app displays success and shows sheet music in list

C. ANALYSIS PROCESS
   Step 1: User selects recording + sheet music in Flutter app
   Step 2: Flutter sends POST to /api/device/analyze
   Step 3: Flask loads WAV file from storage
   Step 4: Flask loads reference_data from Supabase
   Step 5: Flask calls coach.py analyze() function:
           a. librosa extracts pitches, onsets, dynamics
           b. music21 parses reference_data notes
           c. Compare actual vs expected notes
           d. Calculate timing errors, wrong notes, dynamics
   Step 6: Flask calls Gemini API with analysis results
   Step 7: Gemini generates natural language feedback
   Step 8: Flask stores analysis in Supabase:
           - mistakes JSON
           - feedback text
           - score
   Step 9: Flask returns analysis_id to Flutter app
   Step 10: Flutter app displays results screen

D. FIRMWARE DISPLAY FLOW
   Step 1: User navigates to "Recordings" or "Uploads" screen on device
   Step 2: Firmware sends GET request with X-User-ID header
   Step 3: Flask looks up user_id from device UUID
   Step 4: Flask queries Supabase for user's data
   Step 5: Flask returns lightweight JSON (no full note arrays)
   Step 6: Firmware parses JSON and displays list
   Step 7: User can select items to view details or analyze

┌─────────────────────────────────────────────────────────────────────────────┐
│                           DATA FORMATS & STRUCTURES                         │
└─────────────────────────────────────────────────────────────────────────────┘

RECORDING DATA:
{
  "id": "uuid",
  "user_id": "uuid",
  "device_id": "uuid",
  "file_path": "recordings/file.wav",
  "created_at": "2026-01-11T...",
  "duration": 45.2
}

SHEET MUSIC DATA (Full - for Flutter):
{
  "id": "uuid",
  "user_id": "uuid",
  "title": "Twinkle Twinkle",
  "file_url": "uploads/image.jpg",
  "reference_data": {
    "clef": "treble",
    "time_signature": "4/4",
    "key_signature": "C",
    "notes": [
      {"note": "C-4", "pitch": "C-4", "duration": 1.0, "startBeat": 1.0}
    ]
  }
}

SHEET MUSIC DATA (Lite - for Firmware):
{
  "id": "uuid",
  "title": "Twinkle Twinkle",
  "file_url": "uploads/image.jpg",
  "reference_data": {
    "clef": "treble",
    "time_signature": "4/4",
    "key_signature": "C",
    "note_count": 8
  },
  "has_analysis": true
}

ANALYSIS DATA:
{
  "id": "uuid",
  "recording_id": "uuid",
  "sheet_music_id": "uuid",
  "mistakes": [
    {"timestamp": 2.5, "expected": "C-4", "actual": "C#-4", "type": "wrong_note"},
    {"timestamp": 5.0, "expected": "G-4", "actual": null, "type": "missing_note"}
  ],
  "feedback": "You played C# instead of C at 2.5 seconds...",
  "score": 85,
  "created_at": "2026-01-11T..."
}

┌─────────────────────────────────────────────────────────────────────────────┐
│                              ERROR HANDLING                                 │
└─────────────────────────────────────────────────────────────────────────────┘

• Network errors: Firmware shows "No WiFi" status
• HTTP errors: Firmware displays error code (e.g., "ERR: HTTP 500")
• JSON parse errors: Firmware shows "0 parsed, arr=5, err=2" (parse fail)
• Missing data: Server skips items with null id/title
• Buffer overflow: Server sends lite JSON to prevent truncation

