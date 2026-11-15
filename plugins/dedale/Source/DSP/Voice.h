#pragma once

#include "../Calibration.h"
#include "ModVco.h"
#include "Noise.h"
#include "SaturatingMixer.h"
#include "Utilities.h"
#include "VcFilter.h"
#include "Vco.h"
#include "Wavefolder.h"

#include <algorithm>

namespace dedale
{
enum class Order
{
    parallel,
    vcwToVcf,
    vcfToVcw
};

// Réglages de la voix, en unités physiques (valeurs par défaut du §10).
struct VoiceParams
{
    float vcoFreq = 110.0f;  // Hz
    float vcoEg1Amt = 0.0f;  // −1..+1
    float vcoSeq1Amt = 1.0f; // 0..1, cran QTZ en butée
    float mvcoFreq = 55.0f;  // Hz
    float mvcoEg1Amt = 0.0f;
    float mvcoSeq2Amt = 0.0f;
    float fmAmt = 0.0f;
    float mixVco = 0.5f;
    float mixRing = 0.0f;
    float mixMvco = 0.0f;
    float mixNoise = 0.0f;
    float noiseTone = 0.5f;
    float fold = 0.3f;
    float bias = 0.0f;
    float vcwEg1Amt = 0.3f;
    float vcwSeq1Amt = 0.0f;
    float cutoff = 800.0f; // Hz
    float vcfEg1Amt = 0.3f;
    float vcfSeq2Amt = 0.0f;
    float resonance = 0.2f;
    float filterMode = 0.0f; // 0 = LP, 1 = BP
    Order order = Order::parallel;
    float blend = 0.5f; // 0 = VCW, 1 = VCF
    float volume = 0.7f;
};

// Tensions de modulation, en volts.
struct VoiceCv
{
    float seq1 = 0.0f; // ±5 V
    float seq2 = 0.0f;
    float eg1 = 0.0f; // 0..+8 V
    float eg2 = 0.0f;
};

// Atténuateur SEQ AMT : gain exactement 1 dans les derniers pour-cent de course (cran QTZ, §3.1).
inline float sequencerAmountGain(float amount) noexcept
{
    return amount >= calibration::kQtzZoneStart ? 1.0f : std::max(amount, 0.0f) / calibration::kQtzZoneStart;
}

// VCA : 0 V = silence total, +8 V = gain 1, réponse légèrement exponentielle.
inline float vcaGain(float controlVolts) noexcept
{
    const auto x = std::clamp(controlVolts / calibration::kEgMaxVolts, 0.0f, 1.0f);
    return std::pow(x, calibration::kVcaExponent);
}

// Voix de DÉDALE (§2) avec ses normalisations par défaut câblées en dur :
// SEQ1 → VCO et FOLD, SEQ2 → MOD VCO et CUTOFF, EG1 → hauteurs, FOLD et CUTOFF, EG2 → les deux VCA.
// setControls() est appelé à la fréquence de base, processSample() à la fréquence suréchantillonnée.
class Voice
{
public:
    void prepare(double oversampledRate, int maxBlock) noexcept;
    void reset() noexcept;
    void setControls(const VoiceParams& params, const VoiceCv& cv) noexcept;
    float processSample() noexcept; // sortie ±1.0

private:
    Vco vco;
    ModVco modVco;
    Noise noise;
    SaturatingMixer mixer;
    Wavefolder folder;
    VcFilter filter;
    DcBlocker outputDcBlocker;

    float vcoHz = 110.0f;
    float modVcoHz = 55.0f;
    float fmIndex = 0.0f;
    float fold = 0.0f;
    float bias = 0.0f;
    float filterMode = 0.0f;
    float gainVcw = 0.0f;
    float gainVcf = 0.0f;
    Order order = Order::parallel;
};
} // namespace dedale
