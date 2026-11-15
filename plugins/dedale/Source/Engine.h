#pragma once

#include "DSP/DecayEnv.h"
#include "DSP/Voice.h"
#include "Patch/PatchBay.h"
#include "Sequencer/GenerativeSequencer.h"

#include <array>

namespace dedale
{
struct EngineParams
{
    VoiceParams voice;
    float eg1Decay = 0.3f; // secondes
    float eg2Decay = 0.4f;
    float egTrigMix = 0.0f; // 0 = SEQ1 seul, 1 = SEQ2 seul
};

// Moteur de DÉDALE, sans JUCE : séquenceur, enveloppes, voix et patch bay.
// processControl() tourne à la fréquence de base (séquenceur, enveloppes, entrées trigger/gate),
// processAudio() à la fréquence suréchantillonnée (sources, VCW, VCF, entrées audio et CV de la voix).
class Engine
{
public:
    explicit Engine(std::uint32_t seed = 1) noexcept : seq(seed) {}

    void prepare(double baseSampleRate, int oversamplingFactor, int maxBlock) noexcept;
    void reset() noexcept;

    GenerativeSequencer& sequencer() noexcept { return seq; }
    PatchBay& patchBay() noexcept { return bay; }
    // Bouton BIT FLIP du panneau : s'ajoute au jack BIT FLIP de la même lane.
    // Réglage global : le jack RESET rappelle aussi le buffer.
    void setResetRecallsBuffer(bool shouldRecall) noexcept { resetRecallsBuffer = shouldRecall; }
    void setBitFlipButton(int lane, bool down) noexcept { flipButtons[lane == 0 ? 0u : 1u] = down; }

    // masterClock : tick de l'horloge interne/hôte. manualTrigger : bouton TRIGGER ou Note On à l'arrêt.
    void processControl(const EngineParams& params, bool masterClock, bool manualTrigger) noexcept;
    float processAudio() noexcept { return voice.processSample(bay); } // sortie ±1.0

private:
    bool risingEdge(In input, GateDetector& detector) noexcept;
    float pulse(int& remaining) noexcept;

    GenerativeSequencer seq;
    PatchBay bay;
    DecayEnv eg1;
    DecayEnv eg2;
    Voice voice;

    std::array<GateDetector, kNumLanes> clockGates;
    std::array<GateDetector, kNumLanes> flipGates;
    std::array<bool, kNumLanes> flipButtons{};
    GateDetector resetGate;
    GateDetector triggerGate;
    GateDetector eg2TrigGate;

    std::array<int, kNumLanes> trigPulse{}; // échantillons restants des impulsions SEQ TRIG
    int clockPulse = 0;
    bool resetRecallsBuffer = false;
    int pulseSamples = 48;
};
} // namespace dedale
