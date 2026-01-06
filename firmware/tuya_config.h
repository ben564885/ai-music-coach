/**
 * TuyaOpen SDK Configuration for AI Music Coach
 *
 * This file contains all Tuya IoT Platform credentials and hardware
 * configuration. IMPORTANT: Replace placeholder values with your actual Tuya
 * IoT credentials.
 */

#ifndef TUYA_CONFIG_H
#define TUYA_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* =============================================================================
 * Tuya IoT Platform Credentials
 * Obtain these from: https://iot.tuya.com/
 * =============================================================================
 */

// Product ID from Tuya IoT Platform
#ifndef TUYA_PRODUCT_ID
#define TUYA_PRODUCT_ID "your_product_id_here"
#endif

// Device UUID (unique per device)
#ifndef TUYA_OPENSDK_UUID
#define TUYA_OPENSDK_UUID "your_uuid_here"
#endif

// Authorization Key (unique per device)
#ifndef TUYA_OPENSDK_AUTHKEY
#define TUYA_OPENSDK_AUTHKEY "your_authkey_here"
#endif

/* =============================================================================
 * WiFi Configuration
 * =============================================================================
 */

#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"

// WiFi connection timeout (milliseconds)
#define WIFI_CONNECT_TIMEOUT_MS 30000

/* =============================================================================
 * Cloud Backend Configuration
 * URL for your Flask backend server for audio analysis
 * =============================================================================
 */

#define CLOUD_BACKEND_URL "http://your-backend-url:5000"
#define CLOUD_UPLOAD_ENDPOINT "/api/upload_audio"
#define CLOUD_ANALYSIS_ENDPOINT "/api/get_analysis"

// HTTP request timeout (milliseconds)
#define HTTP_REQUEST_TIMEOUT_MS 10000

/* =============================================================================
 * BLE Configuration
 * =============================================================================
 */

#define BLE_DEVICE_NAME "T5AI-MusicCoach"

// Custom service UUID for music coach communication
#define BLE_SERVICE_UUID "0000FFE0-0000-1000-8000-00805F9B34FB"
#define BLE_CHAR_TX_UUID "0000FFE1-0000-1000-8000-00805F9B34FB"
#define BLE_CHAR_RX_UUID "0000FFE2-0000-1000-8000-00805F9B34FB"

// Maximum BLE packet size
#define BLE_MAX_PACKET_SIZE 512

/* =============================================================================
 * Audio Configuration
 * T5AI board has dual microphones and speaker
 * =============================================================================
 */

#define AUDIO_SAMPLE_RATE 16000 // 16 kHz for voice
#define AUDIO_BIT_DEPTH 16      // 16-bit samples
#define AUDIO_CHANNELS 2        // Stereo (dual mics)
#define AUDIO_BUFFER_SIZE 4096  // Buffer size in samples

// Audio recording chunk duration (milliseconds)
#define AUDIO_CHUNK_DURATION_MS 100

// TTS Configuration
#define TTS_VOICE_SPEED 1.0f  // Normal speed
#define TTS_VOICE_VOLUME 0.8f // 80% volume

/* =============================================================================
 * Display Configuration (SPI LCD)
 * Adjust pins based on your T5AI board connection
 * =============================================================================
 */

#define DISPLAY_ENABLE 1 // Set to 0 to disable display

#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 320
#define DISPLAY_DRIVER "ST7789" // or "ILI9341"

// SPI Pin Configuration for T5AI board
#define DISPLAY_SPI_MOSI 23
#define DISPLAY_SPI_CLK 18
#define DISPLAY_SPI_CS 5
#define DISPLAY_DC_PIN 2
#define DISPLAY_RST_PIN 4
#define DISPLAY_BL_PIN 15 // Backlight

// SPI speed
#define DISPLAY_SPI_FREQ_HZ 40000000 // 40 MHz

/* =============================================================================
 * Real-Time Analysis Configuration
 * =============================================================================
 */

#define RT_ANALYSIS_ENABLE 1         // Enable real-time note detection
#define RT_PITCH_TOLERANCE_HZ 10.0f  // Tolerance for note matching
#define RT_CONFIDENCE_THRESHOLD 0.7f // Minimum confidence for detection

/* =============================================================================
 * GPIO Pin Definitions
 * =============================================================================
 */

#define GPIO_RECORD_BUTTON 0 // Boot button used for recording
#define GPIO_LED_STATUS 8    // Status LED

/* =============================================================================
 * Task Priorities (FreeRTOS)
 * =============================================================================
 */

#define TASK_PRIORITY_AUDIO 6   // High priority for audio
#define TASK_PRIORITY_MAIN 5    // Main application task
#define TASK_PRIORITY_DISPLAY 4 // Display updates
#define TASK_PRIORITY_NETWORK 4 // Network operations

// Task stack sizes
#define TASK_STACK_AUDIO 4096
#define TASK_STACK_MAIN 8192
#define TASK_STACK_DISPLAY 4096
#define TASK_STACK_NETWORK 8192

/* =============================================================================
 * Memory Limits
 * =============================================================================
 */

#define MAX_REFERENCE_NOTES 1000   // Max notes in sheet music
#define MAX_FEEDBACK_TEXT_LEN 2048 // Max TTS feedback length
#define MAX_RECORDING_SECONDS 300  // 5 minutes max recording

#ifdef __cplusplus
}
#endif

#endif // TUYA_CONFIG_H
