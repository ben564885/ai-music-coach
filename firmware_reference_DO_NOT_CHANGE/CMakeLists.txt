##
# @file CMakeLists.txt
# @brief CMakeLists.txt for PracticePod T5AI Firmware
##

# APP_PATH
set(APP_PATH ${CMAKE_CURRENT_LIST_DIR})

# APP_NAME
get_filename_component(APP_NAME ${APP_PATH} NAME)

# Source files
set(APP_SRCS ${APP_PATH}/main.c ${APP_PATH}/wav_encode.c)

# Include directories
set(APP_INC
    ${APP_PATH}
)

# Compiler options
set(APP_OPTIONS "-W")
list(APPEND APP_OPTIONS "-Wall" "-DLV_LVGL_H_INCLUDE_SIMPLE")

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
