#include "Clock.h"

#include "../Calibration.h"

#include <cmath>

namespace dedale
{
void InternalClock::prepare(double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    reset();
}

bool InternalClock::processSample(double stepsPerMinute) noexcept
{
    const auto tick = phase >= 1.0;
    if (tick)
        phase -= 1.0;
    phase += stepsPerMinute / 60.0 / sampleRate;
    return tick;
}

void HostClock::beginBlock(double ppqAtBlockStart, double ppqPerSample) noexcept
{
    // Saut de position (boucle, déplacement du curseur, démarrage) : on repart de la position annoncée,
    // comme si l'échantillon précédent était juste avant. Un départ pile sur un temps donne donc un tick.
    const auto jumped = !continuous || std::abs(ppqAtBlockStart - ppq) > 2.0 * ppqPerSample;
    if (jumped)
        previousPpq = ppqAtBlockStart - ppqPerSample;
    ppq = ppqAtBlockStart;
    increment = ppqPerSample;
    continuous = true;
}

bool HostClock::processSample(int division) noexcept
{
    const auto stepLength = calibration::kHostQuartersPerStepAtDivision1 * (division < 1 ? 1 : division);
    const auto tick = std::floor(ppq / stepLength) != std::floor(previousPpq / stepLength);
    previousPpq = ppq;
    ppq += increment;
    return tick;
}

void MidiClock::prepare(double newSampleRate) noexcept
{
    timeoutSamples = static_cast<long>(calibration::kMidiClockTimeoutSeconds * newSampleRate);
    samplesSincePulse = timeoutSamples;
    pulses = 0;
}

bool MidiClock::onPulse(int division) noexcept
{
    samplesSincePulse = 0;
    const auto pulsesPerStep = calibration::kMidiPulsesPerStepAtDivision1 * (division < 1 ? 1 : division);
    const auto tick = pulses % pulsesPerStep == 0;
    pulses = (pulses + 1) % pulsesPerStep;
    return tick;
}
} // namespace dedale
