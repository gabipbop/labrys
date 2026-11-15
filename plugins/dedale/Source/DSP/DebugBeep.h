#pragma once

#include "../Calibration.h"

#include <algorithm>
#include <cmath>

namespace dedale
{
// Bip sinus de debug (phase 2) : un bip par trigger, hauteur = CV à 1 V/octave. Remplacé par la voix en
// phase 3.
class DebugBeep
{
public:
    void prepare(double newSampleRate, int /*maxBlock*/) noexcept
    {
        sampleRate = static_cast<float>(newSampleRate);
        decay = std::pow(0.001f,
                         1.0f / (calibration::kBeepDecaySeconds * sampleRate)); // −60 dB en kBeepDecaySeconds
        attack = 1.0f - std::exp(-1.0f / (0.001f * sampleRate)); // montée d'environ 1 ms, pour éviter le clic
        reset();
    }

    void reset() noexcept
    {
        phase = 0.0f;
        envelope = 0.0f;
        smoothed = 0.0f;
    }

    void trigger() noexcept { envelope = 1.0f; }

    float processSample(float cvVolts) noexcept
    {
        const auto frequency =
            std::clamp(calibration::kBeepFrequencyAtZeroVolt * std::exp2(cvVolts), 20.0f, 0.45f * sampleRate);
        phase += frequency / sampleRate;
        phase -= std::floor(phase);
        smoothed += (envelope - smoothed) * attack;
        envelope *= decay;
        return calibration::kBeepLevel * smoothed * std::sin(6.2831853f * phase);
    }

private:
    float sampleRate = 48000.0f;
    float decay = 0.0f;
    float attack = 1.0f;
    float phase = 0.0f;
    float envelope = 0.0f;
    float smoothed = 0.0f;
};
} // namespace dedale
