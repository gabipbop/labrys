#pragma once

#include "Engine.h"
#include "Sequencer/ButtonComboHandler.h"
#include "Sequencer/Clock.h"
#include "StateIO.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

// Enveloppe JUCE autour du moteur (dedale::Engine) : paramètres, horloge interne/hôte, MIDI,
// suréchantillonnage. Le panneau arrive en phase 5.
class DedaleProcessor final : public juce::AudioProcessor, private juce::Timer
{
public:
    DedaleProcessor();
    ~DedaleProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Fixe la seed du séquenceur et tire le motif de départ.
    void seedSequencer(std::uint32_t seed) noexcept;

    // Accès direct au séquenceur, hors temps réel seulement (rendu offline) : jamais pendant processBlock.
    dedale::GenerativeSequencer& sequencer() noexcept { return engine.sequencer(); }

    // --- État hors paramètres (thread de l'interface) ---
    // Séquence : copie cohérente de l'état du séquenceur, et restauration prise en compte au bloc audio
    // suivant.
    dedale::SequencerState sequencerState() const;
    void setSequencerState(const dedale::SequencerState& state);
    dedale::GlobalSettings settings() const noexcept;
    void setSettings(const dedale::GlobalSettings& settings);
    // Tout l'état du plugin : paramètres, câbles, réglages globaux, et la séquence si demandé (projet : oui ;
    // preset : en option).
    juce::ValueTree captureState(bool withSequence);
    void applyState(const juce::ValueTree& state, bool withSequence);
    // Recale les paramètres chain et quant_mode quand le séquenceur les a changés lui-même (rappel du
    // buffer).
    void syncParametersFromSequencer();

    // --- Interface → audio (à appeler depuis le thread de l'interface) ---
    // Action d'un bouton du panneau : les réglages (quantizer, division) passent par les paramètres, le reste
    // par une FIFO lue au début du bloc audio suivant.
    void perform(const dedale::Action& action);
    // Câbles : la table ci-dessous fait foi, le thread audio la recopie dans la patch bay à chaque bloc.
    void connectCable(dedale::Out output, dedale::In input) noexcept;
    void disconnectCable(dedale::In input) noexcept;
    int cableSource(dedale::In input) const noexcept; // sortie reliée, ou −1

    // --- Audio → interface ---
    struct Display
    {
        std::array<std::atomic<std::uint8_t>, dedale::kNumLanes> bits{}; // bit i = pas i actif
        std::array<std::atomic<int>, dedale::kNumLanes> playHead{};
        std::array<std::atomic<int>, dedale::kNumLanes> writeHead{};
        std::array<std::atomic<int>, dedale::kNumLanes> length{};
        std::atomic<int> quantMode{0};
        std::atomic<bool> chained{false};
        std::atomic<bool> running{false};
        std::atomic<bool> hasBuffer{false};
    };
    Display display;
    // Copie dans destination les derniers échantillons de sortie disponibles pour l'oscilloscope.
    int pullScope(float* destination, int maxSamples) noexcept;

    juce::AudioProcessorValueTreeState apvts;

    // Paramètres continus, lissés (~20 ms) avant d'atteindre le DSP. L'ordre suit le tableau kContinuous du
    // .cpp.
    enum Continuous
    {
        vcoFreq,
        vcoEg1Amt,
        vcoSeq1Amt,
        mvcoFreq,
        mvcoEg1Amt,
        mvcoSeq2Amt,
        mvcoFmAmt,
        mixVco,
        mixRing,
        mixMvco,
        mixNoise,
        noiseTone,
        vcwFold,
        vcwBias,
        vcwEg1Amt,
        vcwSeq1Amt,
        vcfCutoff,
        vcfEg1Amt,
        vcfSeq2Amt,
        vcfRes,
        vcfMode,
        blend,
        eg1Decay,
        eg2Decay,
        egTrigMix,
        volume,
        umix1Lvl,
        numContinuous
    };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void pullParameters() noexcept;
    void applyUiActions() noexcept;
    void applyCables() noexcept;
    void publishDisplay() noexcept;
    void exchangeSequencerState() noexcept;
    void rebuildOversampling();
    void timerCallback() override { syncParametersFromSequencer(); }
    bool startHostClock() noexcept;
    void renderChunk(float* output, int numSamples, juce::MidiBufferIterator& event,
                     const juce::MidiBufferIterator& lastEvent, int offset, bool useHost) noexcept;

    dedale::Engine engine;
    dedale::InternalClock internalClock;
    dedale::HostClock hostClock;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::AudioBuffer<float>
        scratch; // un canal, à la fréquence de base : entrée (vide) du suréchantillonneur
    int maxChunk = 0;

    std::array<std::atomic<float>*, numContinuous> continuousParams{};
    std::array<float, numContinuous> targets{};
    std::array<float, numContinuous> smoothed{};
    float smoothingCoefficient = 1.0f;

    std::atomic<float>* tempoParam = nullptr;
    std::atomic<float>* syncModeParam = nullptr;
    std::atomic<float>* clockDivParam = nullptr;
    std::array<std::atomic<float>*, dedale::kNumLanes> corruptParams{};
    std::array<std::atomic<float>*, dedale::kNumLanes> rangeParams{};
    std::atomic<float>* runParam = nullptr;
    std::atomic<float>* chainParam = nullptr;
    std::atomic<float>* quantModeParam = nullptr;
    std::atomic<float>* orderParam = nullptr;

    // Dernières valeurs lues des paramètres chain et quantizer : ils ne sont appliqués que lorsqu'ils
    // changent, pour ne pas écraser un rappel de buffer.
    int lastQuantParam = -1;
    int lastChainParam = -1;
    bool wasRunning = false;

    // FIFO des actions de boutons (un producteur : l'interface ; un consommateur : l'audio).
    static constexpr int kActionCapacity = 64;
    juce::AbstractFifo actionFifo{kActionCapacity};
    std::array<dedale::Action, kActionCapacity> actionQueue{};
    bool pendingTrigger = false;

    // Échange de l'état du séquenceur avec l'interface. Le thread audio n'attend jamais ce verrou (tryLock).
    juce::SpinLock stateLock;
    dedale::SequencerState publishedState;
    dedale::SequencerState pendingState;
    bool hasPendingState = false;
    bool modesChanged = false; // le séquenceur a changé chain ou quantizer : à répercuter sur les paramètres
    std::atomic<bool> modesDirty{false};

    std::atomic<std::uint32_t> settingsBits{dedale::pack(dedale::GlobalSettings{})};
    dedale::GlobalSettings blockSettings; // réglages lus au début du bloc en cours
    double currentSampleRate = 0.0;

    std::array<std::atomic<std::int8_t>, dedale::kNumInputs> cables;
    std::array<std::int8_t, dedale::kNumInputs> appliedCables{};

    static constexpr int kScopeCapacity = 4096;
    juce::AbstractFifo scopeFifo{kScopeCapacity};
    std::array<float, kScopeCapacity> scopeData{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DedaleProcessor)
};
