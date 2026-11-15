#pragma once

#include <algorithm>
#include <cmath>

namespace dedale
{
// MOD VCO : triangle anti-aliasé par PolyBLAMP (§3.2), du LFO à l'audio. Sortie ±5 V.
class ModVco
{
public:
    void prepare(double newSampleRate, int /*maxBlock*/) noexcept
    {
        sampleRate = static_cast<float>(newSampleRate);
        reset();
    }

    void reset() noexcept { phase = 0.0f; }

    // syncHigh : la phase reste à zéro tant que le gate est haut (pas seulement sur le front).
    float processSample(float frequencyHz, bool syncHigh) noexcept
    {
        const auto dt = std::clamp(frequencyHz / sampleRate, 0.0f, 0.45f);
        phase += dt;
        if (phase >= 1.0f)
            phase -= 1.0f;
        if (syncHigh)
            phase = 0.0f;

        // Triangle naïf : +1 à la phase 0, −1 à la phase 0,5. Chaque sommet est un coin (saut de pente de
        // ±8 par cycle), arrondi par le résidu BLAMP.
        auto triangle = 4.0f * std::abs(phase - 0.5f) - 1.0f;
        const auto shifted = phase < 0.5f ? phase + 0.5f : phase - 0.5f;
        triangle += 4.0f * dt * (blamp(shifted, dt) - blamp(phase, dt));
        return 5.0f * triangle;
    }

private:
    // Résidu PolyBLAMP sur les deux échantillons qui encadrent un coin situé à la phase 0.
    static float blamp(float t, float dt) noexcept
    {
        if (dt <= 0.0f)
            return 0.0f;
        if (t < dt)
        {
            const auto x = t / dt - 1.0f;
            return -x * x * x / 3.0f;
        }
        if (t > 1.0f - dt)
        {
            const auto x = (t - 1.0f) / dt + 1.0f;
            return x * x * x / 3.0f;
        }
        return 0.0f;
    }

    float sampleRate = 48000.0f;
    float phase = 0.0f;
};
} // namespace dedale
