cmake_minimum_required(VERSION 3.22)
project(BocchiTheFuzz VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE скачивается сам при первой конфигурации (нужен интернет)
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(BocchiTheFuzz
    COMPANY_NAME "BocchiAudio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Bchi
    PLUGIN_CODE Bfzz
    FORMATS VST3 Standalone
    PRODUCT_NAME "Bocchi the Fuzz"
    COPY_PLUGIN_AFTER_BUILD FALSE)

# Картинки встраиваются внутрь плагина (класс BinaryData)
juce_add_binary_data(BocchiAssets SOURCES
    Assets/background.jpg
    Assets/logo.png
    Assets/calm_girl.png
    Assets/fuzz_girl.png)

target_sources(BocchiTheFuzz PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(BocchiTheFuzz PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(BocchiTheFuzz
    PRIVATE
        BocchiAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)
