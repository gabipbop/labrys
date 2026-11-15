#pragma once

#include "../Calibration.h"
#include "../Patch/PatchBay.h"
#include "../Sequencer/Rng.h"
#include "ModVco.h"
#include "Noise.h"
#include "SaturatingMixer.h"
#include "Utilities.h"
#include "VcFilter.h"
#include "Vco.h"
#include "Wavefolder.h"

#include <algorithm>
#include <array>

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
    float umix1Level = 0.0f;
    bool analogDrift = false; // réglage global : dérive lente de la hauteur des deux oscillateurs
    bool vcwZeroColour = calibration::kVcwZeroColour; // réglage global : coloration du wavefolder à FOLD = 0
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

// Voix de DÉDALE (§2). Ses entrées passent par la patch bay : sans câble, chacune voit sa normalisation
// (EG1 → FOLD et CUTOFF, EG2 → les deux VCA, mixeur → VCW IN et VCF IN selon ORDER, ring mod → U MIX 1).
// Les routes à atténuateur du panneau restent internes : SEQ1 → VCO et FOLD, SEQ2 → MOD VCO et CUTOFF,
// EG1 → hauteurs. setControls() est appelé à la fréquence de base, processSample() à la fréquence
// suréchantillonnée ; une entrée patchée est relue à chaque échantillon (modulation à l'audio-rate).
class Voice
{
public:
    void prepare(double oversampledRate, int maxBlock) noexcept;
    // Cadence d'appel de setControls (par défaut : un quart de la fréquence suréchantillonnée).
    void setControlRate(double controlRate) noexcept;
    void reset() noexcept;
    void setControls(const VoiceParams& params, const VoiceCv& cv, const PatchBay& bay) noexcept;
    float
    processSample(PatchBay& bay) noexcept; // sortie ±1.0 ; écrit les jacks M VCO, NOISE, MIXER, U MIX, VCA

private:
    void setPathGains(float blend, float vcwVcaVolts, float vcfVcaVolts) noexcept;

    Vco vco;
    ModVco modVco;
    Noise noise;
    SaturatingMixer mixer;
    Wavefolder folder;
    VcFilter filter;
    DcBlocker outputDcBlocker;
    GateDetector syncGate;
    Rng driftRng{0x44524946u};
    std::array<float, 2> drift{}; // bruit lent, un par oscillateur
    float driftCoefficient = 0.0f;
    float driftScale = 0.0f; // vers des octaves
    bool zeroColour = true;

    // Valeurs sans câble, calculées à la fréquence de base.
    float vcoHz = 110.0f;
    float modVcoHz = 55.0f;
    float fmIndex = 0.0f;
    float fold = 0.0f;
    float bias = 0.0f;
    float filterMode = 0.0f;
    float gainVcw = 0.0f;
    float gainVcf = 0.0f;
    Order order = Order::parallel;

    // De quoi recalculer une entrée patchée à chaque échantillon.
    float foldBase = 0.0f;       // knob + SEQ1
    float foldAmount = 0.0f;     // EG1/CV AMT du VCW
    float cutoffBaseHz = 800.0f; // knob + SEQ2
    float cutoffAmount = 0.0f;   // EG1/CV AMT du VCF
    float resonance = 0.0f;
    float blend = 0.5f;
    float eg2 = 0.0f;
    float outputGain = 0.0f; // VOLUME × marge de sortie
    float umix1Level = 0.0f;
};
} // namespace dedale
