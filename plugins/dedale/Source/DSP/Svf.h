#pragma once

#include "Utilities.h"

#include <algorithm>

namespace dedale
{
// Cœur de filtre à variable d'état TPT (forme de Simper/Cytomic), 2 pôles.
// damping = 1/Q. Sorties : passe-bas et passe-bande (gain de crête 1/damping).
struct Svf
{
    void setCoefficients(float cutoffHz, float damping, float sampleRate) noexcept
    {
        const auto g = std::tan(kPi * std::clamp(cutoffHz, 10.0f, 0.45f * sampleRate) / sampleRate);
        a1 = 1.0f / (1.0f + g * (g + damping));
        a2 = g * a1;
        a3 = g * a2;
    }

    void reset() noexcept { ic1 = ic2 = low = band = 0.0f; }

    // stateCeiling > 0 : les intégrateurs saturent doucement à ce niveau (grain quand on pousse fort).
    void processSample(float input, float stateCeiling = 0.0f) noexcept
    {
        const auto v3 = input - ic2;
        band = a1 * ic1 + a2 * v3;
        low = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * band - ic1;
        ic2 = 2.0f * low - ic2;
        if (stateCeiling > 0.0f)
        {
            ic1 = softLimit(ic1, stateCeiling);
            ic2 = softLimit(ic2, stateCeiling);
        }
    }

    float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float ic1 = 0.0f, ic2 = 0.0f;
    float low = 0.0f, band = 0.0f;
};
} // namespace dedale
