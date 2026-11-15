# DÉDALE : plugin (VST3, AU, Standalone) + tests.
# Noms et codes : libs/ariane/cmake/SuiteIdentity.cmake

# Cœur pur (aucun JUCE) : séquenceur, quantizer, horloges, DSP de la voix, patch bay, moteur. Partagé par le plugin, les tests et les outils.
add_library(dedale_core STATIC
    Source/Engine.cpp
    Source/DSP/Voice.cpp
    Source/Sequencer/ButtonComboHandler.cpp
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

set(DEDALE_PLUGIN_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/UI/Panel.cpp
    Source/UI/PatchBayView.cpp
    Source/UI/Widgets.cpp)
target_sources(Dedale PRIVATE ${DEDALE_PLUGIN_SOURCES})

# Police de l'interface (Barlow Condensed, licence OFL : Resources/fonts/OFL-*.txt), embarquée dans le binaire.
juce_add_binary_data(DedaleAssets SOURCES
    Resources/fonts/BarlowCondensed-SemiBold.ttf
    Resources/fonts/BarlowCondensed-Bold.ttf
    Resources/fonts/SairaCondensed-SemiBold.ttf
    Resources/fonts/SairaCondensed-Bold.ttf
    Resources/fonts/SairaStencilOne-Regular.ttf
    Resources/fonts/ShareTechMono-Regular.ttf
    Resources/fonts/Quantico-Bold.ttf
    Resources/fonts/ChakraPetch-SemiBold.ttf
    Resources/fonts/ChakraPetch-Bold.ttf)

target_compile_definitions(Dedale PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    LABRYS_DISPLAY_NAME="${DEDALE_DISPLAY_NAME}")

target_link_libraries(Dedale
    PRIVATE
        dedale_core
        DedaleAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

# Warnings stricts sur notre code seulement (pas sur les sources des modules JUCE compilées dans la cible).
set_source_files_properties(${DEDALE_PLUGIN_SOURCES}
    TARGET_DIRECTORY Dedale
    PROPERTIES COMPILE_OPTIONS "$<TARGET_PROPERTY:ariane_warnings,INTERFACE_COMPILE_OPTIONS>")

# Outils et tests qui instancient le processor hors temps réel : ils lient le code partagé du plugin (cible
# statique Dedale) et reprennent ses includes et définitions JUCE, sans recompiler les modules.
function(dedale_link_processor target)
    target_link_libraries(${target} PRIVATE Dedale dedale_core ariane_warnings)
    target_include_directories(${target} SYSTEM PRIVATE $<TARGET_PROPERTY:Dedale,INCLUDE_DIRECTORIES>)
    target_compile_definitions(${target} PRIVATE $<TARGET_PROPERTY:Dedale,COMPILE_DEFINITIONS>)
endfunction()

# DedaleRender : rendu des scénarios d'écoute (WAV dans renders/, §15.6) et des métriques de référence.
add_executable(DedaleRender Tools/DedaleRender.cpp)
dedale_link_processor(DedaleRender)

if(LABRYS_BUILD_TESTS)
    add_executable(DedaleTests
        Tests/ButtonComboTests.cpp
        Tests/ClockTests.cpp
        Tests/DspTests.cpp
        Tests/PatchTests.cpp
        Tests/QuantizerTests.cpp
        Tests/SequencerTests.cpp)
    target_link_libraries(DedaleTests PRIVATE dedale_core ariane_warnings Catch2::Catch2WithMain)
    catch_discover_tests(DedaleTests)

    # Golden tests : rendus complets du plugin comparés à des métriques versionnées.
    add_executable(DedaleGoldenTests Tests/GoldenTests.cpp)
    dedale_link_processor(DedaleGoldenTests)
    target_link_libraries(DedaleGoldenTests PRIVATE Catch2::Catch2)
    target_compile_definitions(DedaleGoldenTests PRIVATE
        DEDALE_GOLDEN_FILE="${CMAKE_CURRENT_SOURCE_DIR}/Tests/golden/metrics.json")
    add_test(NAME "Golden : rendus de référence" COMMAND DedaleGoldenTests)
endif()
