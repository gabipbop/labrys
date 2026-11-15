#pragma once

#include "../Calibration.h"
#include "Utilities.h"

#include <algorithm>

namespace dedale
{
// Enveloppe decay-only (§7) : 0 → +8 V, décroissance exponentielle. Anti-clic : micro-attaque qui repart
// du niveau courant vers la nouvelle crête, puis léger lissage de la sortie.
class DecayEnv
{
public:
    void prepare(double newSampleRate, int /*maxBlock*/) noexcept
    {
        namespace cal = calibration;
        sampleRate = static_cast<float>(newSampleRate);
        attackSamples = std::max(1, static_cast<int>(cal::kEgAttackSeconds * sampleRate));
        smoothing = 1.0f - std::exp(-2.0f * kPi * cal::kEgSmoothingHz / sampleRate);
        reset();
    }

    void reset() noexcept
    {
        level = output = 0.0f;
        attackRemaining = 0;
        decaySeconds = -1.0f;
    }

    // velocity 0..1 = hauteur de la crête. Une vélocité nulle ne déclenche rien.
    void trigger(float velocity) noexcept
    {
        if (velocity <= 0.0f)
            return;
        attackRemaining = attackSamples;
        attackStep =
            (calibration::kEgMaxVolts * std::min(velocity, 1.0f) - level) / static_cast<float>(attackSamples);
    }

    // decay : temps pour retomber à −60 dB, en secondes.
    float processSample(float decay) noexcept
    {
        if (attackRemaining > 0)
        {
            level += attackStep;
            --attackRemaining;
        }
        else
        {
            if (decay != decaySeconds)
            {
                decaySeconds = decay;
                coefficient = std::pow(0.001f, 1.0f / (std::max(decay, 0.001f) * sampleRate));
            }
            level *= coefficient;
        }
        output += (level - output) * smoothing;
        return output;
    }

private:
    float sampleRate = 48000.0f;
    float smoothing = 1.0f;
    float level = 0.0f;
    float output = 0.0f;
    float attackStep = 0.0f;
    float coefficient = 0.0f;
    float decaySeconds = -1.0f;
    int attackSamples = 1;
    int attackRemaining = 0;
};
} // namespace dedale
