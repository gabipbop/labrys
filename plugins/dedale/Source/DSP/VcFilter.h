#pragma once

#include "../Calibration.h"
#include "Svf.h"

#include <numbers>

namespace dedale
{
// VCF (§5) : SVF 2 pôles, morph continu passe-bas → passe-bande, résonance retenue (pas d'auto-oscillation),
// saturation douce des intégrateurs. Entrée et sortie en volts.
class VcFilter
{
public:
    void prepare(double newSampleRate, int /*maxBlock*/) noexcept
    {
        sampleRate = static_cast<float>(newSampleRate);
        setCutoff(1000.0f, 0.0f);
        reset();
    }

    void reset() noexcept { svf.reset(); }

    // resonance 0..1 : de Butterworth (−3 dB au cutoff) à kVcfMinDamping.
    void setCutoff(float cutoffHz, float resonance) noexcept
    {
        const auto damping =
            kFlatDamping + std::clamp(resonance, 0.0f, 1.0f) * (calibration::kVcfMinDamping - kFlatDamping);
        svf.setCoefficients(std::min(cutoffHz, 20000.0f), damping, sampleRate);
        bandGain = std::sqrt(kFlatDamping * damping); // passe-bande à gain unité sans résonance
    }

    // mode : 0 = passe-bas, 1 = passe-bande.
    float processSample(float input, float mode) noexcept
    {
        svf.processSample(input, calibration::kVcfStateCeilingVolts);
        return (1.0f - mode) * svf.low + mode * bandGain * svf.band;
    }

private:
    static constexpr float kFlatDamping = std::numbers::sqrt2_v<float>;

    Svf svf;
    float sampleRate = 48000.0f;
    float bandGain = 1.0f;
};
} // namespace dedale
