cmake_minimum_required(VERSION 3.22)

project(VirtualAMP VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# JUCE
# ============================================================

add_subdirectory(
    "C:/Users/Croissant/Desktop/This is DEV/JUCE/JUCE-9.0.2"
    "${CMAKE_BINARY_DIR}/JUCE"
)

# ============================================================
# Application
# ============================================================

juce_add_gui_app(VirtualAMP
    PRODUCT_NAME "VirtualAMP"
    COMPANY_NAME "Croissant"
    VERSION "0.1.0"
)

# ============================================================
# Source
# ============================================================

target_sources(VirtualAMP
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/AudioEngine.cpp
)

# ============================================================
# JUCE configuration
# ============================================================

target_compile_definitions(VirtualAMP
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

# ============================================================
# JUCE modules
# ============================================================

target_link_libraries(VirtualAMP
    PRIVATE
        juce::juce_gui_extra
        juce::juce_audio_utils
        juce::juce_dsp

    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ============================================================
# Generate JuceHeader.h
# ============================================================

juce_generate_juce_header(VirtualAMP)