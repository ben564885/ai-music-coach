##
# @file CMakeLists.txt
# @brief CMakeLists.txt for PracticePod T5AI Firmware
##

# APP_PATH
set(APP_PATH ${CMAKE_CURRENT_LIST_DIR})

# APP_NAME
get_filename_component(APP_NAME ${APP_PATH} NAME)

# Source files
set(APP_SRCS
    ${APP_PATH}/main.c
    ${APP_PATH}/wav_encode.c
    ${APP_PATH}/coach_voice.c
    ${APP_PATH}/voice_assistant.c
    ${APP_PATH}/voice_debug_overlay.c
)

# Upstream TuyaOpen (hackathon2026) sources (cloned into repo root)
set(TUYAOPEN_HACKATHON_PATH ${APP_PATH}/../TuyaOpen_hackathon2026)
set(AI_AUDIO_PATH ${TUYAOPEN_HACKATHON_PATH}/apps/tuya.ai/ai_components/ai_audio)
set(AI_SERVICE_PATH ${TUYAOPEN_HACKATHON_PATH}/src/tuya_ai_service)

list(APPEND APP_SRCS
    ${AI_AUDIO_PATH}/src/ai_audio_agent.c
    ${AI_AUDIO_PATH}/src/ai_audio_cloud_asr.c
    ${AI_AUDIO_PATH}/src/ai_audio_input.c
    ${AI_AUDIO_PATH}/src/ai_audio_main.c
    ${AI_AUDIO_PATH}/src/ai_audio_player.c
)

# Tuya AI service stack (required by hackathon2026 ai_audio_agent.c)
list(APPEND APP_SRCS
    ${AI_SERVICE_PATH}/svc_ai_basic/src/tuya_ai_protocol.c
    ${AI_SERVICE_PATH}/svc_ai_basic/src/tuya_ai_mqtt.c
    ${AI_SERVICE_PATH}/svc_ai_basic/src/tuya_ai_http.c
    ${AI_SERVICE_PATH}/svc_ai_basic/src/tuya_ai_event.c
    ${AI_SERVICE_PATH}/svc_ai_basic/src/tuya_ai_client.c
    ${AI_SERVICE_PATH}/svc_ai_basic/src/tuya_ai_biz.c

    ${AI_SERVICE_PATH}/svc_ai_agent/src/tuya_ai_output.c
    ${AI_SERVICE_PATH}/svc_ai_agent/src/tuya_ai_input.c
    ${AI_SERVICE_PATH}/svc_ai_agent/src/tuya_ai_agent.c

    ${AI_SERVICE_PATH}/svc_ai_monitor/src/tuya_ai_monitor.c

    ${AI_SERVICE_PATH}/svc_ai_codec/src/tuya_ai_encoder.c
    ${AI_SERVICE_PATH}/svc_ai_codec/src/tuya_ai_encoder_opus.c
    ${AI_SERVICE_PATH}/svc_ai_codec/src/tuya_ai_encoder_speex.c

    ${AI_SERVICE_PATH}/port/src/tuya_svc_netmgr_linkage.c
    ${AI_SERVICE_PATH}/port/src/tuya_svc_netmgr.c
    ${AI_SERVICE_PATH}/port/src/tuya_svc_devos.c
    ${AI_SERVICE_PATH}/port/src/tuya_devos_utils.c
    ${AI_SERVICE_PATH}/port/src/smart_frame.c
    ${AI_SERVICE_PATH}/port/src/mqc_app.c
    ${AI_SERVICE_PATH}/port/src/httpc.c
    ${AI_SERVICE_PATH}/port/src/http_manager.c
    ${AI_SERVICE_PATH}/port/src/http_inf.c
    ${AI_SERVICE_PATH}/port/src/gw_intf.c
)

# Include directories
set(APP_INC
    ${APP_PATH}
    ${AI_AUDIO_PATH}/include
    ${AI_AUDIO_PATH}/minimp3
    ${AI_SERVICE_PATH}/svc_ai_basic/include
    ${AI_SERVICE_PATH}/svc_ai_agent/include
    ${AI_SERVICE_PATH}/svc_ai_monitor/include
    ${AI_SERVICE_PATH}/svc_ai_codec/include
    ${AI_SERVICE_PATH}/port/include
)

# Compiler options
set(APP_OPTIONS "-W")
list(APPEND APP_OPTIONS
    "-Wall"
    "-DLV_LVGL_H_INCLUDE_SIMPLE"

    # TuyaOpen hackathon2026 AI service defaults (normally set via Kconfig)
    "-DAI_BIZ_MAX_NUM=5"
    "-DAI_HEARTBEAT_INTERVAL=120"

    # Using manual/button mode for voice - no wake word needed

    # Tuya toolchains commonly build with -Werror; allow these upstream files to compile.
    "-Wno-error=unused-parameter"
    "-Wno-unused-parameter"
    "-Wno-error=sign-compare"
    "-Wno-sign-compare"

    # Some hackathon2026 sources use TAL_PR_DEBUG but our build exposes PR_DEBUG.
    "-DTAL_PR_DEBUG=PR_DEBUG"
)

# Create library target
add_library(${EXAMPLE_LIB})

# Add source files
target_sources(${EXAMPLE_LIB}
    PRIVATE
        ${APP_SRCS}
)

# Add include directories
target_include_directories(${EXAMPLE_LIB}
    PRIVATE
        ${APP_INC}
)

# Add compile options
target_compile_options(${EXAMPLE_LIB}
    PRIVATE
        ${APP_OPTIONS}
)
