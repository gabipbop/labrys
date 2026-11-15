#pragma once

#include "../Calibration.h"
#include "Utilities.h"

#include <array>

namespace dedale
{
// Mixeur saturant 4 canaux (§3.5) : gain unité à midi, saturation au-delà. Chaque canal sature seul
// (avec une légère asymétrie, pour des harmoniques paires), puis la somme sature à nouveau. Entrées et sortie
// en volts.
class SaturatingMixer
{
public:
    void prepare(double sampleRate, int /*maxBlock*/) noexcept
    {
        dcBlocker.prepare(sampleRate);
        reset();
    }

    void reset() noexcept { dcBlocker.reset(); }

    // Niveaux 0..1 des canaux VCO, RING MOD, MOD VCO, NOISE.
    void setLevels(float vco, float ring, float modVco, float noise) noexcept
    {
        gains = {gainForLevel(vco), gainForLevel(ring), gainForLevel(modVco), gainForLevel(noise)};
    }

    float processSample(float vco, float ring, float modVco, float noise) noexcept
    {
        const auto sum = channel(vco, gains[0]) + channel(ring, gains[1]) + channel(modVco, gains[2]) +
                         channel(noise, gains[3]);
        return 5.0f * dcBlocker.processSample(softLimit(sum, calibration::kMixerSumCeiling));
    }

private:
    static float gainForLevel(float level) noexcept
    {
        return level <= 0.5f ? 2.0f * level
                             : 1.0f + (level - 0.5f) * 2.0f * (calibration::kMixerMaxGain - 1.0f);
    }

    static float channel(float volts, float gain) noexcept
    {
        namespace cal = calibration;
        const auto offset = cal::kMixerAsymmetry;
        return softLimit(gain * volts / 5.0f + offset, cal::kMixerChannelCeiling) -
               softLimit(offset, cal::kMixerChannelCeiling);
    }

    std::array<float, 4> gains{};
    DcBlocker dcBlocker;
};
} // namespace dedale
