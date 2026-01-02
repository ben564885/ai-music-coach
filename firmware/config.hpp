/**
 * Configuration header for T5AI Music Coach (C++)
 */

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdint>

namespace MusicCoach {
namespace Config {

// Wi-Fi Configuration
constexpr const char* WIFI_SSID = "YourWiFiSSID";
constexpr const char* WIFI_PASSWORD = "YourWiFiPassword";

// Cloud Backend Configuration
constexpr const char* CLOUD_BACKEND_URL = "http://your-backend-url:5000";

// BLE Configuration
constexpr const char* T5AI_DEVICE_NAME = "T5AI-MusicCoach";
constexpr const char* BLE_SERVICE_UUID = "0000FFE0-0000-1000-8000-00805F9B34FB";
constexpr const char* BLE_CHARACTERISTIC_UUID = "0000FFE1-0000-1000-8000-00805F9B34FB";

// Audio Configuration
constexpr uint32_t AUDIO_SAMPLE_RATE = 16000;  // 16 kHz
constexpr uint8_t AUDIO_BIT_DEPTH = 16;
constexpr uint8_t AUDIO_CHANNELS = 2;  // Stereo
constexpr uint32_t AUDIO_BUFFER_SIZE = 4096;

// TTS Configuration
constexpr float TTS_VOICE_SPEED = 0.9f;
constexpr float TTS_VOICE_VOLUME = 0.8f;

// Memory Configuration
constexpr uint32_t MAX_REFERENCE_NOTES = 1000;
constexpr uint32_t MAX_FEEDBACK_TEXT_LEN = 2000;

// Display Configuration
constexpr bool ENABLE_DISPLAY_MODULE = true;
constexpr uint8_t DISPLAY_SPI_MOSI = 23;
constexpr uint8_t DISPLAY_SPI_CLK = 18;
constexpr uint8_t DISPLAY_CS = 5;
constexpr uint8_t DISPLAY_DC = 2;
constexpr uint8_t DISPLAY_RST = 4;

// Real-Time Analysis Configuration
constexpr bool ENABLE_REAL_TIME_ANALYSIS = true;
constexpr float RT_PITCH_TOLERANCE_HZ = 10.0f;
constexpr float RT_CONFIDENCE_THRESHOLD = 0.7f;

} // namespace Config
} // namespace MusicCoach

// C-compatible defines for legacy code
#define WIFI_SSID MusicCoach::Config::WIFI_SSID
#define WIFI_PASSWORD MusicCoach::Config::WIFI_PASSWORD
#define CLOUD_BACKEND_URL MusicCoach::Config::CLOUD_BACKEND_URL
#define T5AI_DEVICE_NAME MusicCoach::Config::T5AI_DEVICE_NAME
#define BLE_SERVICE_UUID MusicCoach::Config::BLE_SERVICE_UUID
#define BLE_CHARACTERISTIC_UUID MusicCoach::Config::BLE_CHARACTERISTIC_UUID
#define AUDIO_SAMPLE_RATE MusicCoach::Config::AUDIO_SAMPLE_RATE
#define AUDIO_BIT_DEPTH MusicCoach::Config::AUDIO_BIT_DEPTH
#define AUDIO_CHANNELS MusicCoach::Config::AUDIO_CHANNELS
#define AUDIO_BUFFER_SIZE MusicCoach::Config::AUDIO_BUFFER_SIZE
#define TTS_VOICE_SPEED MusicCoach::Config::TTS_VOICE_SPEED
#define TTS_VOICE_VOLUME MusicCoach::Config::TTS_VOICE_VOLUME
#define MAX_REFERENCE_NOTES MusicCoach::Config::MAX_REFERENCE_NOTES
#define MAX_FEEDBACK_TEXT_LEN MusicCoach::Config::MAX_FEEDBACK_TEXT_LEN

#endif // CONFIG_HPP

