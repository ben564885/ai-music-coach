# CMakeLists.txt for T5AI Music Coach Firmware (C++)
cmake_minimum_required(VERSION 3.16)

project(ai_music_coach_firmware CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ESP-IDF specific includes (if using ESP-IDF)
# This is a template - adjust based on your actual build system

# Source files
set(SOURCES
    main.cpp
    # Add other source files here:
    # wifi_manager.cpp
    # audio_recorder.cpp
    # tts_engine.cpp
    # ble_manager.cpp
    # cloud_client.cpp
)

# Header files
set(HEADERS
    config.hpp
    # Add other header files here
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    # Add ESP-IDF includes here
)

# Compiler flags
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    # ESP-IDF specific flags
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
# target_link_libraries(${PROJECT_NAME}
#     # Add ESP-IDF components here
# )

