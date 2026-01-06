# CMakeLists.txt for T5AI Music Coach Firmware
# TuyaOpen SDK Project Configuration
#
# Build with: tos.py build
# Configure: tos.py config choice (select T5AI-Board)

cmake_minimum_required(VERSION 3.16)

# Project name
project(ai_music_coach VERSION 1.0.0 LANGUAGES C CXX)

# Set C/C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SRCS
    # Main application
    main.cpp
    
    # HAL implementations
    wifi_manager.c
    ble_manager.c
    audio_service.c
    cloud_client.c
    display_service.c
    fingering_charts.c
)

# Header files (for IDE support)
set(HEADERS
    tuya_config.h
    wifi_manager.h
    ble_manager.h
    audio_service.h
    cloud_client.h
    display_service.h
    real_time_analyzer.h
    fingering_charts.h
    types.h
)

# Include directories
set(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# TuyaOpen SDK component dependencies
set(COMPONENTS
    # Core TAL components
    tal_api
    tal_log
    tal_system
    tal_thread
    tal_memory
    tal_mutex
    tal_wifi
    
    # TKL components
    tkl_bluetooth
    tkl_gpio
    
    # TDD components (drivers)
    tdd_display
    tdl_audio
    
    # Libraries
    http_client
    cJSON
    lvgl
)

# Optional: Load from .env file for local development
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.env")
    message(STATUS "Loading environment variables from ${CMAKE_CURRENT_SOURCE_DIR}/.env")
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/.env" DOTENV_LINES)
    foreach(LINE ${DOTENV_LINES})
        # Ignore comments and empty lines
        if(NOT LINE MATCHES "^#" AND LINE MATCHES "^([^=]+)=(.*)$")
            string(STRIP ${CMAKE_MATCH_1} VAR_NAME)
            string(STRIP ${CMAKE_MATCH_2} VAR_VAL)
            set(ENV{${VAR_NAME}} "${VAR_VAL}")
            message(STATUS "Found env var: ${VAR_NAME}")
        endif()
    endforeach()
endif()

# Map environment variables to compiler definitions
set(ENV_VARS_TO_DEFINE
    TUYA_PRODUCT_ID
    TUYA_OPENSDK_UUID
    TUYA_OPENSDK_AUTHKEY
    WIFI_SSID
    WIFI_PASSWORD
    CLOUD_BACKEND_URL
)

foreach(VAR ${ENV_VARS_TO_DEFINE})
    if(DEFINED ENV{${VAR}})
        add_definitions(-D${VAR}="$ENV{${VAR}}")
        message(STATUS "Defining ${VAR} from environment")
    endif()
endforeach()

# For TuyaOpen SDK integration
# The actual build is done by tos.py which handles SDK integration
# This CMakeLists.txt is for reference and IDE support

# If building standalone (for syntax checking)
if(NOT TUYAOPEN_SDK)
    # Create interface library for header files
    add_library(${PROJECT_NAME}_headers INTERFACE)
    target_include_directories(${PROJECT_NAME}_headers INTERFACE ${INCLUDE_DIRS})
    
    # Note: Actual build requires TuyaOpen SDK environment
    message(STATUS "Building in standalone mode - use 'tos.py build' for full build")
endif()

# TuyaOpen project registration
# This macro is provided by TuyaOpen SDK build system
if(COMMAND tuya_component_register)
    tuya_component_register(
        SRCS ${SRCS}
        INCLUDE_DIRS ${INCLUDE_DIRS}
        REQUIRES ${COMPONENTS}
    )
endif()
