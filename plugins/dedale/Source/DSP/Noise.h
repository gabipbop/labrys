#pragma once

#include "../Calibration.h"
#include "../Sequencer/Rng.h"
#include "Svf.h"

namespace dedale
{
// Bruit à tonalité variable (§3.4) : bruit blanc dans un passe-bande large dont le centre va du grave à
// l'aigu. Le niveau RMS reste à peu près constant sur toute la course.
class Noise
{
public:
    void prepare(double newSampleRate, int /*maxBlock*/) noexcept
    {
        sampleRate = static_cast<float>(newSampleRate);
        tone = -1.0f;
        setTone(0.5f);
        reset();
    }

    void reset() noexcept
    {
        filter.reset();
        rng.seed(0x4e4f4953u); // bruit reproductible d'un rendu à l'autre
    }

    void setTone(float newTone) noexcept
    {
        if (newTone == tone)
            return;
        tone = newTone;
        namespace cal = calibration;
        const auto centre =
            cal::kNoiseMinHz * std::pow(cal::kNoiseMaxHz / cal::kNoiseMinHz, std::clamp(tone, 0.0f, 1.0f));
        const auto damping = 1.0f / cal::kNoiseQ;
        filter.setCoefficients(centre, damping, sampleRate);
        // Puissance d'un bruit blanc uniforme (variance 1/3) dans un passe-bande de gain de crête 1 :
        // variance × π × fc × damping / sr. On normalise vers le niveau RMS visé.
        const auto bandRms = std::sqrt(kPi * centre * damping / (3.0f * sampleRate));
        gain = cal::kNoiseRmsVolts * damping / bandRms;
    }

    float processSample() noexcept
    {
        filter.processSample(rng.uniform() * 2.0f - 1.0f);
        return gain * filter.band;
    }

private:
    Svf filter;
    Rng rng;
    float sampleRate = 48000.0f;
    float tone = -1.0f;
    float gain = 1.0f;
};
} // namespace dedale
