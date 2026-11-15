#include "Engine.h"

#include "Calibration.h"

#include <algorithm>

namespace dedale
{
namespace cal = calibration;

void Engine::prepare(double baseSampleRate, int oversamplingFactor, int maxBlock) noexcept
{
    eg1.prepare(baseSampleRate, maxBlock);
    eg2.prepare(baseSampleRate, maxBlock);
    voice.prepare(baseSampleRate * oversamplingFactor, maxBlock * oversamplingFactor);
    voice.setControlRate(baseSampleRate);
    pulseSamples =
        std::max(1, static_cast<int>(cal::kTriggerPulseSeconds * static_cast<float>(baseSampleRate)));
    reset();
}

void Engine::reset() noexcept
{
    eg1.reset();
    eg2.reset();
    voice.reset();
    bay.resetSignals();
    for (auto* gates : {&clockGates, &flipGates})
        for (auto& gate : *gates)
            gate.reset();
    resetGate.reset();
    triggerGate.reset();
    eg2TrigGate.reset();
    trigPulse = {};
    clockPulse = 0;
}

// Front montant sur une entrée trigger. Sans câble : jamais, et le détecteur repart de zéro.
bool Engine::risingEdge(In input, GateDetector& detector) noexcept
{
    if (!bay.isPatched(input))
    {
        detector.reset();
        return false;
    }
    return detector.process(bay.read(input, 0.0f));
}

float Engine::pulse(int& remaining) noexcept
{
    if (remaining <= 0)
        return 0.0f;
    --remaining;
    return 5.0f;
}

void Engine::processControl(const EngineParams& params, bool masterClock, bool manualTrigger) noexcept
{
    // --- Horloges : CLOCK 1 normalisé sur l'horloge maître, CLOCK 2 sur CLOCK 1 ---
    const auto clock1 = bay.isPatched(In::clock1) ? risingEdge(In::clock1, clockGates[0]) : masterClock;
    const auto clock2 = bay.isPatched(In::clock2) ? risingEdge(In::clock2, clockGates[1]) : clock1;

    if (risingEdge(In::reset, resetGate))
    {
        seq.reset();
        if (resetRecallsBuffer)
            seq.recallBuffer();
    }

    // BIT FLIP : flippe sur le front montant, puis à chaque passage du write head tant que l'entrée reste
    // haute.
    risingEdge(In::bitFlip1, flipGates[0]);
    risingEdge(In::bitFlip2, flipGates[1]);
    seq.setBitFlipGate(0, flipGates[0].isHigh() || flipButtons[0]);
    seq.setBitFlipGate(1, flipGates[1].isHigh() || flipButtons[1]);

    const auto trigger1 = clock1 && seq.tick(0);
    const auto trigger2 = clock2 && seq.tick(1);

    // --- Enveloppes ---
    // EG TRIG MIX : vélocité de chaque flux de triggers ; sur un même pas, la plus forte gagne (§7).
    const auto mix = params.egTrigMix;
    const auto fullTrigger = manualTrigger || risingEdge(In::trigger, triggerGate);
    const auto velocity = fullTrigger ? 1.0f
                                      : std::max(trigger1 ? std::min(1.0f, 2.0f * (1.0f - mix)) : 0.0f,
                                                 trigger2 ? std::min(1.0f, 2.0f * mix) : 0.0f);
    eg1.trigger(velocity);
    const auto eg2Isolated = cal::kEg2TrigJackIsolatesEg2 && bay.isPatched(In::eg2Trig);
    if (!eg2Isolated)
        eg2.trigger(velocity);
    if (risingEdge(In::eg2Trig, eg2TrigGate))
        eg2.trigger(1.0f);

    // --- Sorties à la fréquence de base ---
    const VoiceCv cv{seq.cv(0), seq.cv(1), eg1.processSample(params.eg1Decay),
                     eg2.processSample(params.eg2Decay)};
    if (trigger1)
        trigPulse[0] = pulseSamples;
    if (trigger2)
        trigPulse[1] = pulseSamples;
    if (masterClock)
        clockPulse = pulseSamples;

    bay.write(Out::eg1, cv.eg1);
    bay.write(Out::eg2, cv.eg2);
    bay.write(Out::seq1Cv, cv.seq1);
    bay.write(Out::seq2Cv, cv.seq2);
    bay.write(Out::seq1Trig, pulse(trigPulse[0]));
    bay.write(Out::seq2Trig, pulse(trigPulse[1]));
    bay.write(Out::clock, pulse(clockPulse));

    voice.setControls(params.voice, cv, bay);
}
} // namespace dedale
