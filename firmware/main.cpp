/**
 * T5AI Music Coach Firmware
 * Main application entry point (C++)
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_manager.h"
#include "audio_recorder.h"
#include "tts_engine.h"
#include "ble_manager.h"
#include "cloud_client.h"
#include "display_manager.h"
#include "real_time_analyzer.h"
#include "types.h"
#include "config.h"

namespace MusicCoach {

static constexpr const char* TAG = "MUSIC_COACH";

// Application state class
class AppState {
public:
    bool is_recording = false;
    bool is_connected = false;
    std::unique_ptr<music_reference_t> reference_data = nullptr;
    std::string feedback_text;
    InstrumentType current_instrument = InstrumentType::PIANO;  // Default instrument
    bool real_time_feedback_enabled = true;  // Enable real-time note detection

    AppState() = default;
    ~AppState() = default;
    
    // Disable copy constructor and assignment
    AppState(const AppState&) = delete;
    AppState& operator=(const AppState&) = delete;
};

static AppState app_state;

/**
 * Initialize all subsystems
 */
static void app_init() {
    ESP_LOGI(TAG, "Initializing AI Music Coach...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize Wi-Fi
    wifi_manager_init();
    wifi_manager_connect(WIFI_SSID, WIFI_PASSWORD);
    
    // Initialize BLE
    ble_manager_init();
    ble_manager_set_device_name(T5AI_DEVICE_NAME);
    
    // Initialize audio recorder
    audio_recorder_init();
    
    // Initialize TTS engine
    tts_engine_init();
    tts_engine_set_voice(VOICE_FRIENDLY_TEACHER);
    tts_engine_set_speed(0.9f);
    tts_engine_set_volume(0.8f);
    
    // Initialize cloud client
    cloud_client_init(CLOUD_BACKEND_URL);
    
    // Initialize display manager (screen module)
    DisplayManager display_manager;
    if (display_manager.init()) {
        display_manager.set_instrument(app_state.current_instrument);
        display_manager.display_connection_status(false);
        ESP_LOGI(TAG, "Display initialized");
    } else {
        ESP_LOGW(TAG, "Display initialization failed - continuing without screen");
    }
    
    // Initialize real-time analyzer
    RealTimeAnalyzer rt_analyzer;
    if (rt_analyzer.init()) {
        ESP_LOGI(TAG, "Real-time analyzer initialized");
    }
    
    ESP_LOGI(TAG, "Initialization complete");
}

/**
 * Handle button press to start/stop recording
 */
static void handle_record_button() {
    if (!app_state.is_connected) {
        ESP_LOGW(TAG, "Not connected to mobile app");
        tts_engine_speak("Please connect the mobile app first");
        return;
    }
    
    if (!app_state.reference_data) {
        ESP_LOGW(TAG, "No sheet music data loaded");
        tts_engine_speak("Please upload sheet music from the mobile app");
        return;
    }
    
    if (app_state.is_recording) {
        // Stop recording
        audio_recorder_stop();
        app_state.is_recording = false;
        ESP_LOGI(TAG, "Recording stopped");
        tts_engine_speak("Recording complete. Analyzing your performance...");
        
        // Upload audio to cloud for analysis
        cloud_client_upload_audio();
        
    } else {
        // Start recording
        audio_config_t config{};
        config.sample_rate = AUDIO_SAMPLE_RATE;
        config.bit_depth = 16;
        config.channels = 2;
        config.buffer_size = AUDIO_BUFFER_SIZE;
        
        audio_recorder_start(&config);
        app_state.is_recording = true;
        ESP_LOGI(TAG, "Recording started");
        tts_engine_speak("Recording started. Begin playing when ready.");
    }
}

/**
 * Handle received sheet music data from BLE
 */
static void handle_sheet_music_received(music_data_t* data) {
    ESP_LOGI(TAG, "Received sheet music data");
    
    // Store reference data
    // Note: Assuming music_data_t can be cast to music_reference_t
    // Adjust based on actual type definitions
    app_state.reference_data = std::unique_ptr<music_reference_t>(
        reinterpret_cast<music_reference_t*>(data)
    );
    
    // Acknowledge receipt
    tts_engine_speak("Sheet music received. Ready to record.");
}

/**
 * Handle analysis results from cloud
 */
static void handle_analysis_results(analysis_result_t* result) {
    ESP_LOGI(TAG, "Received analysis results: %d mistakes found", result->mistake_count);
    
    // Store feedback text
    if (result->feedback_text) {
        app_state.feedback_text = std::string(result->feedback_text);
    }
    
    // Speak feedback
    tts_engine_speak(result->feedback_text);
    
    // Send results to mobile app via BLE
    ble_manager_send_analysis_results(result);
}

// Global instances (would be better as singleton, but keeping simple)
static DisplayManager* g_display_manager = nullptr;
static RealTimeAnalyzer* g_rt_analyzer = nullptr;

/**
 * Audio streaming task
 * Streams audio chunks to cloud in real-time during recording
 * Also performs real-time note detection for immediate feedback
 */
static void audio_streaming_task(void* pvParameters) {
    audio_chunk_t chunk{};
    
    while (true) {
        if (app_state.is_recording) {
            if (audio_recorder_read_chunk(&chunk) == ESP_OK) {
                // Stream to cloud
                cloud_client_stream_audio(&chunk);
                
                // Also save to local buffer
                audio_recorder_save_chunk(&chunk);
                
                // Real-time note detection and display
                if (app_state.real_time_feedback_enabled && g_rt_analyzer && g_display_manager) {
                    NoteDetection detected = g_rt_analyzer->process_audio_chunk(
                        chunk.data, chunk.size / sizeof(int16_t)
                    );
                    
                    if (detected.is_valid && detected.confidence > 0.7f) {
                        // Check if we have an expected note
                        ExpectedNote expected = g_rt_analyzer->get_current_expected_note();
                        
                        if (expected.frequency > 0) {
                            // Check if note matches expected
                            bool matches = g_rt_analyzer->check_note_match(detected, expected);
                            
                            if (!matches) {
                                // Wrong note detected - show correct fingering
                                NoteInfo note_info{};
                                strncpy(note_info.note_name, detected.note_name, 4);
                                note_info.octave = detected.octave;
                                note_info.frequency = detected.frequency;
                                note_info.is_correct = false;
                                
                                g_display_manager->display_wrong_note(
                                    detected.note_name,
                                    expected.note_name,
                                    expected.octave
                                );
                                
                                // Speak feedback
                                char feedback_msg[128];
                                snprintf(feedback_msg, sizeof(feedback_msg),
                                    "You played %s, but the correct note is %s%d. Here's how to finger it.",
                                    detected.note_name, expected.note_name, expected.octave);
                                tts_engine_speak(feedback_msg);
                            } else {
                                // Correct note - show confirmation
                                NoteInfo note_info{};
                                strncpy(note_info.note_name, detected.note_name, 4);
                                note_info.octave = detected.octave;
                                note_info.frequency = detected.frequency;
                                note_info.is_correct = true;
                                
                                g_display_manager->display_current_note(note_info);
                            }
                        } else {
                            // No expected note set - just show what's being played
                            NoteInfo note_info{};
                            strncpy(note_info.note_name, detected.note_name, 4);
                            note_info.octave = detected.octave;
                            note_info.frequency = detected.frequency;
                            note_info.is_correct = true;
                            
                            g_display_manager->display_current_note(note_info);
                        }
                    }
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // 100ms delay
    }
}

/**
 * Main application task
 */
static void app_task(void* pvParameters) {
    while (true) {
        // Check for button press (GPIO interrupt or polling)
        // For now, simulate with a delay
        // In production, use GPIO interrupt handler
        
        // Check for BLE messages
        std::unique_ptr<ble_message_t> msg(ble_manager_receive_message());
        if (msg) {
            switch (msg->type) {
                case BLE_MSG_SHEET_MUSIC:
                    handle_sheet_music_received(static_cast<music_data_t*>(msg->data));
                    break;
                case BLE_MSG_START_RECORDING:
                    handle_record_button();
                    break;
                case BLE_MSG_STOP_RECORDING:
                    if (app_state.is_recording) {
                        handle_record_button();
                    }
                    break;
                default:
                    break;
            }
        }
        
        // Check for cloud responses
        std::unique_ptr<analysis_result_t> result(cloud_client_get_results());
        if (result) {
            handle_analysis_results(result.get());
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

} // namespace MusicCoach

/**
 * Main entry point
 */
extern "C" void app_main() {
    using namespace MusicCoach;
    
    // Initialize application
    app_init();
    
    // Create tasks
    xTaskCreate(audio_streaming_task, "audio_stream", 4096, nullptr, 5, nullptr);
    xTaskCreate(app_task, "app_main", 8192, nullptr, 5, nullptr);
    
    ESP_LOGI(TAG, "Application started");
}

