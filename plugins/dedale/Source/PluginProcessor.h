#pragma once

#include "Engine.h"
#include "Sequencer/Clock.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

// Enveloppe JUCE autour du moteur (dedale::Engine) : paramètres, horloge interne/hôte, MIDI,
// suréchantillonnage. Le panneau arrive en phase 5.
class DedaleProcessor final : public juce::AudioProcessor
{
public:
    DedaleProcessor();

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

    // Fixe la seed du séquenceur et tire le motif de départ (provisoire : il n'y a pas de boutons avant la
    // phase 5).
    void seedSequencer(std::uint32_t seed) noexcept;

    // Accès direct au séquenceur et à la patch bay, hors temps réel seulement (rendu offline) : jamais
    // pendant processBlock. L'UI passera par une FIFO en phase 5.
    dedale::GenerativeSequencer& sequencer() noexcept { return engine.sequencer(); }
    dedale::PatchBay& patchBay() noexcept { return engine.patchBay(); }

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

    // Dernières valeurs poussées au séquenceur : chain et quantizer ne sont réappliqués que s'ils changent,
    // pour ne pas écraser un rappel de buffer.
    int lastQuantMode = -1;
    int lastChain = -1;
    bool wasRunning = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DedaleProcessor)
};
