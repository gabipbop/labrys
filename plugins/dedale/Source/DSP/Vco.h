#pragma once

#include "Utilities.h"

namespace dedale
{
// VCO sinus à FM through-zero linéaire (§3.1) : la fréquence instantanée peut devenir négative,
// la phase recule alors. Sortie ±5 V.
class Vco
{
public:
    void prepare(double newSampleRate, int /*maxBlock*/) noexcept
    {
        sampleRate = static_cast<float>(newSampleRate);
        reset();
    }

    void reset() noexcept { phase = 0.0f; }

    // modulator : signal FM normalisé (±1) ; index : profondeur (0 = pas de FM).
    float processSample(float frequencyHz, float modulator, float index) noexcept
    {
        phase += frequencyHz * (1.0f + index * modulator) / sampleRate;
        phase -= std::floor(phase); // repli dans les deux sens
        return 5.0f * std::sin(2.0f * kPi * phase);
    }

private:
    float sampleRate = 48000.0f;
    float phase = 0.0f;
};
} // namespace dedale
