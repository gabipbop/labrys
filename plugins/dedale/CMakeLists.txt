# DÉDALE : plugin (VST3, AU, Standalone) + tests.
# Noms et codes : libs/ariane/cmake/SuiteIdentity.cmake

# Cœur pur (aucun JUCE) : séquenceur, quantizer, horloges. Partagé par le plugin, les tests et les outils.
add_library(dedale_core STATIC
    Source/Sequencer/Clock.cpp
    Source/Sequencer/GenerativeSequencer.cpp
    Source/Sequencer/Quantizer.cpp)
target_include_directories(dedale_core PUBLIC Source)
target_link_libraries(dedale_core PUBLIC labrys::ariane PRIVATE ariane_warnings)

juce_add_plugin(Dedale
    COMPANY_NAME "${LABRYS_VENDOR_NAME}"
    COMPANY_COPYRIGHT "${LABRYS_COMPANY_NAME}"
    PRODUCT_NAME "${DEDALE_FILE_NAME}"
    PLUGIN_NAME "${DEDALE_HOST_NAME}"
    BUNDLE_ID "${DEDALE_BUNDLE_ID}"
    PLUGIN_MANUFACTURER_CODE "${LABRYS_MANUFACTURER_CODE}"
    PLUGIN_CODE "${DEDALE_PLUGIN_CODE}"
    VERSION "${PROJECT_VERSION}"
    FORMATS VST3 AU Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    COPY_PLUGIN_AFTER_BUILD ${LABRYS_COPY_PLUGINS})

target_sources(Dedale PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Dedale PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    LABRYS_DISPLAY_NAME="${DEDALE_DISPLAY_NAME}")

target_link_libraries(Dedale
    PRIVATE
        dedale_core
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

# Warnings stricts sur notre code seulement (pas sur les sources des modules JUCE compilées dans la cible).
set_source_files_properties(Source/PluginProcessor.cpp Source/PluginEditor.cpp
    TARGET_DIRECTORY Dedale
    PROPERTIES COMPILE_OPTIONS "$<TARGET_PROPERTY:ariane_warnings,INTERFACE_COMPILE_OPTIONS>")

if(LABRYS_BUILD_TESTS)
    add_executable(DedaleTests
        Tests/ClockTests.cpp
        Tests/QuantizerTests.cpp
        Tests/SequencerTests.cpp)
    target_link_libraries(DedaleTests PRIVATE dedale_core ariane_warnings Catch2::Catch2WithMain)
    catch_discover_tests(DedaleTests)
endif()
