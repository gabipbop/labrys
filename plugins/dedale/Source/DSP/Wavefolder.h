#pragma once

#include "../Calibration.h"
#include "Utilities.h"

#include <algorithm>

namespace dedale
{
// Wavefolder (VCW, §4) : pré-gain + bias, deux étages de pli à coins adoucis, saturation finale, DC-block.
// Entrée et sortie en volts (±5 V). L'anti-aliasing vient du suréchantillonnage de la voix.
class Wavefolder
{
public:
    void prepare(double sampleRate, int /*maxBlock*/) noexcept
    {
        dcBlocker.prepare(sampleRate);
        reset();
    }

    void reset() noexcept { dcBlocker.reset(); }

    // fold : 0..1 ; bias : −1..+1.
    float processSample(float inputVolts, float fold, float bias) noexcept
    {
        namespace cal = calibration;
        auto x = inputVolts / 5.0f;
        if (cal::kVcwZeroColour) // coloration résiduelle, même à FOLD = 0
            x = std::tanh(cal::kVcwZeroColourDrive * x) / cal::kVcwZeroColourDrive;

        const auto driven = x * (cal::kVcwInputGain + fold * cal::kVcwFoldGain) + bias * cal::kVcwBiasRange;
        auto y = 0.0f;
        if (cal::kFolderModel == cal::FolderModel::sine)
        {
            y = std::sin(0.5f * kPi * driven);
        }
        else
        {
            y = foldStage(driven);
            y = foldStage(y * (1.0f + fold * cal::kVcwStage2Gain) + fold * cal::kVcwAsymmetry);
        }

        y = std::tanh(cal::kVcwOutputDrive * y) / std::tanh(cal::kVcwOutputDrive);
        return 5.0f * dcBlocker.processSample(y);
    }

private:
    // Pli triangulaire (linéaire entre −1 et +1, replié au-delà) dont les coins sont arrondis :
    // kVcwCornerSharpness → 1 donne le triangle brut, plus bas tend vers un sinus.
    static float foldStage(float u) noexcept
    {
        const auto sharpness = calibration::kVcwCornerSharpness;
        return std::asin(sharpness * std::sin(0.5f * kPi * u)) / std::asin(sharpness);
    }

    DcBlocker dcBlocker;
};
} // namespace dedale
