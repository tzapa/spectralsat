cmake_minimum_required(VERSION 3.22)
project(SpectralSat VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── JUCE ──────────────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ── Plugin target ─────────────────────────────────────────────────────────────
juce_add_plugin(SpectralSat
    COMPANY_NAME            "YourStudio"
    PLUGIN_MANUFACTURER_CODE Ystu
    PLUGIN_CODE             Ssat
    FORMATS                 VST3 AU
    PRODUCT_NAME            "SpectralSat"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES         "Fx|Distortion"
)

juce_generate_juce_header(SpectralSat)

target_sources(SpectralSat PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/MultibandSaturator.cpp
)

target_compile_definitions(SpectralSat PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(SpectralSat PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
