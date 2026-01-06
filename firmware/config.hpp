/**
 * Updated Configuration Header for T5AI Music Coach
 * C++ namespace configuration compatible with TuyaOpen SDK
 */

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "tuya_config.h"
#include <cstdint>

namespace MusicCoach {
namespace Config {

// =============================================================================
// Re-export config values for C++ code
// =============================================================================

// WiFi Configuration
constexpr const char *WifiSsid = WIFI_SSID;
constexpr const char *WifiPassword = WIFI_PASSWORD;
constexpr uint32_t WifiConnectTimeoutMs = WIFI_CONNECT_TIMEOUT_MS;

// Cloud Backend Configuration
constexpr const char *CloudBackendUrl = CLOUD_BACKEND_URL;
constexpr const char *CloudUploadEndpoint = CLOUD_UPLOAD_ENDPOINT;
constexpr const char *CloudAnalysisEndpoint = CLOUD_ANALYSIS_ENDPOINT;

// BLE Configuration
constexpr const char *BleDeviceName = BLE_DEVICE_NAME;

// Audio Configuration
constexpr uint32_t AudioSampleRate = AUDIO_SAMPLE_RATE;
constexpr uint8_t AudioBitDepth = AUDIO_BIT_DEPTH;
constexpr uint8_t AudioChannels = AUDIO_CHANNELS;
constexpr uint32_t AudioBufferSize = AUDIO_BUFFER_SIZE;

// TTS Configuration
constexpr float TtsVoiceSpeed = TTS_VOICE_SPEED;
constexpr float TtsVoiceVolume = TTS_VOICE_VOLUME;

// Display Configuration
constexpr bool DisplayEnabled = DISPLAY_ENABLE;
constexpr uint16_t DisplayWidth = DISPLAY_WIDTH;
constexpr uint16_t DisplayHeight = DISPLAY_HEIGHT;

// Real-Time Analysis Configuration
constexpr bool RtAnalysisEnabled = RT_ANALYSIS_ENABLE;
constexpr float RtPitchToleranceHz = RT_PITCH_TOLERANCE_HZ;
constexpr float RtConfidenceThreshold = RT_CONFIDENCE_THRESHOLD;

// Task Configuration
constexpr uint8_t TaskPriorityAudio = TASK_PRIORITY_AUDIO;
constexpr uint8_t TaskPriorityMain = TASK_PRIORITY_MAIN;
constexpr uint8_t TaskPriorityDisplay = TASK_PRIORITY_DISPLAY;
constexpr uint8_t TaskPriorityNetwork = TASK_PRIORITY_NETWORK;

constexpr uint32_t TaskStackAudio = TASK_STACK_AUDIO;
constexpr uint32_t TaskStackMain = TASK_STACK_MAIN;
constexpr uint32_t TaskStackDisplay = TASK_STACK_DISPLAY;
constexpr uint32_t TaskStackNetwork = TASK_STACK_NETWORK;

// Memory Limits
constexpr uint32_t MaxReferenceNotes = MAX_REFERENCE_NOTES;
constexpr uint32_t MaxFeedbackTextLen = MAX_FEEDBACK_TEXT_LEN;
constexpr uint32_t MaxRecordingSeconds = MAX_RECORDING_SECONDS;

} // namespace Config
} // namespace MusicCoach

#endif // CONFIG_HPP
