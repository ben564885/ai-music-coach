# Screen Module Setup Guide

## What You Need

### Screen Module Options

**Recommended: ST7789 (240x320 SPI)**
- Common and well-supported
- Good resolution for fingering charts
- SPI interface (fast updates)
- ~$5-10

**Alternative: ILI9341 (240x320 SPI)**
- Similar to ST7789
- Also well-supported
- ~$5-10

**Budget Option: SSD1306 (128x64 I2C)**
- Smaller display
- Lower resolution but cheaper (~$3-5)
- I2C interface (slower but simpler wiring)

## Wiring (ST7789 Example)

Connect to T5AI board:
```
ST7789 Pin → T5AI Pin
VCC       → 3.3V
GND       → GND
MOSI      → GPIO 23 (configurable)
SCLK      → GPIO 18 (configurable)
CS        → GPIO 5 (configurable)
DC        → GPIO 2 (configurable)
RST       → GPIO 4 (configurable)
```

Update `firmware/config.hpp` with your pin assignments.

## What the Screen Shows

### During Practice (Real-Time)

1. **Current Note Display**
   ```
   ┌─────────────┐
   │  Playing: A │
   │  Octave: 4  │
   │  440 Hz     │
   │     ✓       │  (green = correct)
   └─────────────┘
   ```

2. **Wrong Note Detected**
   ```
   ┌─────────────┐
   │ You played: │
   │     Bb      │
   │             │
   │ Expected: A │
   │             │
   │ [Fingering] │
   │   Diagram   │
   └─────────────┘
   ```

3. **Scale Practice Mode**
   ```
   ┌─────────────┐
   │ C Major     │
   │             │
   │ Current: C  │
   │ Next: D     │
   │             │
   │ [C Fingering│
   │   Diagram]  │
   └─────────────┘
   ```

## Example: C Major Scale Practice

**User plays C Major Scale:**

1. Plays **C** ✅
   - Screen: "C" with fingering diagram
   - TTS: (silent, correct note)

2. Plays **D** ✅
   - Screen: "D" with fingering diagram
   - TTS: (silent, correct note)

3. Plays **Bb** ❌ (should be A)
   - Screen immediately shows:
     - "You played: Bb"
     - "Expected: A"
     - Fingering diagram for A
   - TTS: "You played B flat, but the correct note is A. Here's how to finger it."

4. Plays **A** ✅
   - Screen: "A" with fingering diagram
   - TTS: (silent, correct note)

## Implementation Status

✅ **Code Structure**: Complete
- Display manager header
- Real-time analyzer header
- Fingering charts structure
- Integration in main.cpp

⏳ **Needs Implementation** (when you get the board):
- Display driver (ST7789/ILI9341 driver using Tuya SDK)
- Real-time pitch detection algorithm
- Fingering chart rendering
- Screen update loop

## Benefits

1. **Immediate Feedback**: Know mistakes as you play
2. **Learn Fingerings**: See correct fingerings instantly
3. **Standalone Practice**: Can practice with just T5AI + screen (no phone needed)
4. **Visual Learning**: See fingerings, not just hear them
5. **Faster Improvement**: Immediate correction accelerates learning

## Configuration

In mobile app:
- Select instrument (piano, violin, guitar, etc.)
- Toggle real-time feedback on/off
- Instrument selection sent to T5AI via BLE

In firmware `config.hpp`:
```cpp
constexpr bool ENABLE_DISPLAY_MODULE = true;
constexpr bool ENABLE_REAL_TIME_ANALYSIS = true;
```

## Next Steps

When you get the T5AI board:
1. Connect screen module (ST7789 recommended)
2. Implement display driver using Tuya SDK
3. Implement real-time pitch detection
4. Add fingering chart rendering
5. Test with C major scale!

The code structure is ready - just need to implement the hardware-specific drivers when you have the board!

