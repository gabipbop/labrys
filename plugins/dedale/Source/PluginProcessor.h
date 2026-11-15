#pragma once

#include "DSP/DebugBeep.h"
#include "Sequencer/Clock.h"
#include "Sequencer/GenerativeSequencer.h"

#include <juce_audio_processors/juce_audio_processors.h>

// Phase 2 : le séquenceur tourne sur l'horloge interne ou hôte, et chaque trigger joue un bip de debug.
// La voix arrive en phase 3, la patch bay en phase 4, le panneau en phase 5.
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

    // Fixe la seed du séquenceur et tire le motif de départ (provisoire : en phase 2 il n'y a pas de
    // boutons).
    void seedSequencer(std::uint32_t seed) noexcept;

    // Accès direct au séquenceur, hors temps réel seulement (rendu offline) : jamais pendant processBlock.
    dedale::GenerativeSequencer& sequencer() noexcept { return seq; }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void pullParameters() noexcept;
    bool startHostClock() noexcept;

    dedale::GenerativeSequencer seq;
    dedale::InternalClock internalClock;
    dedale::HostClock hostClock;
    std::array<dedale::DebugBeep, dedale::kNumLanes> beeps;

    std::atomic<float>* tempoParam = nullptr;
    std::atomic<float>* syncModeParam = nullptr;
    std::atomic<float>* clockDivParam = nullptr;
    std::array<std::atomic<float>*, dedale::kNumLanes> corruptParams{};
    std::array<std::atomic<float>*, dedale::kNumLanes> rangeParams{};
    std::atomic<float>* runParam = nullptr;
    std::atomic<float>* chainParam = nullptr;
    std::atomic<float>* quantModeParam = nullptr;

    // Dernières valeurs poussées au séquenceur : chain et quantizer ne sont réappliqués que s'ils changent,
    // pour ne pas écraser un rappel de buffer.
    int lastQuantMode = -1;
    int lastChain = -1;
    bool wasRunning = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DedaleProcessor)
};
