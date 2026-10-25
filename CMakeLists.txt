cmake_minimum_required(VERSION 3.22)

project(SOCLE VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

# French labels are written in UTF-8 in the sources
if(MSVC)
    add_compile_options(/utf-8)
endif()

# AU only exists on macOS; VST3 everywhere; Standalone for quick tests without a DAW
if(APPLE)
    set(SOCLE_FORMATS VST3 AU Standalone)
else()
    set(SOCLE_FORMATS VST3 Standalone)
endif()

# same manufacturer as MONOLITHE: hosts list the two plugins together
juce_add_plugin(Socle
    COMPANY_NAME "Monolithe"
    BUNDLE_ID "fr.monolithe.socle"
    PLUGIN_MANUFACTURER_CODE Mnlt
    PLUGIN_CODE Scl1
    FORMATS ${SOCLE_FORMATS}
    PRODUCT_NAME "SOCLE"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    COPY_PLUGIN_AFTER_BUILD FALSE
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE)

target_sources(Socle
    PRIVATE
        source/PluginProcessor.cpp
        source/PluginEditor.cpp)

target_include_directories(Socle PRIVATE source)

target_compile_definitions(Socle
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Socle
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Render: bass lines through the whole plugin, one .wav per setting, to tune the sound without a DAW
juce_add_console_app(SocleRender PRODUCT_NAME "socle_render")
target_sources(SocleRender PRIVATE tools/render.cpp)
# reuse the plugin's compiled code: same headers and settings, without compiling JUCE twice
target_include_directories(SocleRender PRIVATE $<TARGET_PROPERTY:Socle,INCLUDE_DIRECTORIES>)
target_compile_definitions(SocleRender PRIVATE $<TARGET_PROPERTY:Socle,COMPILE_DEFINITIONS>)
target_link_libraries(SocleRender
    PRIVATE
        Socle
    PUBLIC
        juce::juce_recommended_config_flags)
