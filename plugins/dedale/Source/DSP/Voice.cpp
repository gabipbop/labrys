#include "Voice.h"

namespace dedale
{
namespace cal = calibration;

namespace
{
// Écrêtage de sécurité : transparent sous le genou, borné à ±5 V au-delà.
float safetyClip(float volts) noexcept
{
    const auto knee = cal::kOutputClipKneeVolts;
    const auto magnitude = std::abs(volts);
    if (magnitude <= knee)
        return volts;
    return std::copysign(knee + softLimit(magnitude - knee, 5.0f - knee), volts);
}
} // namespace

void Voice::prepare(double oversampledRate, int maxBlock) noexcept
{
    vco.prepare(oversampledRate, maxBlock);
    modVco.prepare(oversampledRate, maxBlock);
    noise.prepare(oversampledRate, maxBlock);
    mixer.prepare(oversampledRate, maxBlock);
    folder.prepare(oversampledRate, maxBlock);
    filter.prepare(oversampledRate, maxBlock);
    outputDcBlocker.prepare(oversampledRate);
    setControlRate(oversampledRate / 4.0);
}

void Voice::setControlRate(double controlRate) noexcept
{
    // Bruit blanc uniforme (écart-type 1/√3) lissé par un passe-bas du premier ordre : on en déduit l'échelle
    // qui donne l'écart-type voulu, en cents.
    driftCoefficient = 1.0f - std::exp(-2.0f * kPi * cal::kDriftHz / static_cast<float>(controlRate));
    const auto smoothedDeviation = std::sqrt(driftCoefficient / (2.0f - driftCoefficient) / 3.0f);
    driftScale = cal::kDriftCents / 1200.0f / smoothedDeviation;
}

void Voice::reset() noexcept
{
    vco.reset();
    modVco.reset();
    noise.reset();
    mixer.reset();
    folder.reset();
    filter.reset();
    outputDcBlocker.reset();
    syncGate.reset();
    driftRng.seed(0x44524946u);
    drift = {};
}

void Voice::setControls(const VoiceParams& p, const VoiceCv& cv, const PatchBay& bay) noexcept
{
    const auto egPitch = cv.eg1 * cal::kEgPitchOctavesPerVolt;
    for (auto& d : drift)
        d = p.analogDrift ? d + driftCoefficient * (driftRng.uniform() * 2.0f - 1.0f - d) : 0.0f;
    zeroColour = p.vcwZeroColour;
    vcoHz =
        std::clamp(p.vcoFreq * std::exp2(p.vcoEg1Amt * egPitch + sequencerAmountGain(p.vcoSeq1Amt) * cv.seq1 +
                                         drift[0] * driftScale),
                   1.0f, 20000.0f);
    modVcoHz = std::clamp(p.mvcoFreq *
                              std::exp2(p.mvcoEg1Amt * egPitch +
                                        sequencerAmountGain(p.mvcoSeq2Amt) * cv.seq2 + drift[1] * driftScale),
                          0.01f, 20000.0f);
    fmIndex = p.fmAmt * cal::kTzfmMaxIndex;

    mixer.setLevels(p.mixVco, p.mixRing, p.mixMvco, p.mixNoise);
    noise.setTone(p.noiseTone);

    // FOLD : knob + SEQ1, puis EG1 (ou la CV patchée) via EG1/CV AMT, chacun ramené à la course du knob.
    foldBase = p.fold + p.vcwSeq1Amt * cv.seq1 / 5.0f;
    foldAmount = p.vcwEg1Amt;
    fold = std::clamp(foldBase + foldAmount * cv.eg1 / cal::kEgMaxVolts, 0.0f, 1.0f);
    bias = p.bias;

    // CUTOFF : knob + SEQ2, puis EG1 (ou la CV patchée) via EG1/CV AMT, en 1 V/octave.
    cutoffBaseHz =
        p.cutoff * std::exp2(std::clamp(p.vcfSeq2Amt * cv.seq2 * cal::kCutoffOctavesPerVolt, -10.0f, 10.0f));
    cutoffAmount = p.vcfEg1Amt;
    resonance = p.resonance;
    if (!bay.isPatched(In::cutoff))
        filter.setCutoff(
            cutoffBaseHz *
                std::exp2(std::clamp(cutoffAmount * cv.eg1 * cal::kCutoffOctavesPerVolt, -10.0f, 10.0f)),
            resonance);
    filterMode = p.filterMode;
    order = p.order;

    // Les deux VCA suivent EG2, puis BLEND équipuissance et VOLUME.
    blend = p.blend;
    eg2 = cv.eg2;
    outputGain = p.volume * cal::kOutputHeadroom;
    umix1Level = p.umix1Level;
    setPathGains(blend, eg2, eg2);
}

void Voice::setPathGains(float blendPosition, float vcwVcaVolts, float vcfVcaVolts) noexcept
{
    const auto angle = 0.5f * kPi * std::clamp(blendPosition, 0.0f, 1.0f);
    gainVcw = vcaGain(vcwVcaVolts) * std::cos(angle) * outputGain;
    gainVcf = vcaGain(vcfVcaVolts) * std::sin(angle) * outputGain;
}

float Voice::processSample(PatchBay& bay) noexcept
{
    // --- Sources ---
    auto modulatorHz = modVcoHz;
    if (bay.isPatched(In::mvcoPitch))
        modulatorHz =
            std::clamp(modulatorHz * std::exp2(std::clamp(bay.read(In::mvcoPitch, 0.0f), -10.0f, 10.0f)),
                       0.01f, 20000.0f);
    syncGate.process(bay.read(In::mvcoSync, 0.0f));
    const auto modulator = modVco.processSample(modulatorHz, syncGate.isHigh());
    bay.write(Out::mvco, modulator);

    auto carrierHz = vcoHz;
    if (bay.isPatched(In::vcoPitch))
        carrierHz = std::clamp(carrierHz * std::exp2(std::clamp(bay.read(In::vcoPitch, 0.0f), -10.0f, 10.0f)),
                               1.0f, 20000.0f);
    const auto carrier = vco.processSample(carrierHz, modulator / 5.0f, fmIndex);
    const auto ring = carrier * modulator / 5.0f;

    const auto hiss = noise.processSample();
    bay.write(Out::noise, hiss);
    const auto mix = mixer.processSample(carrier, ring, modulator, hiss);
    bay.write(Out::mixer, mix);

    // U MIX : mixeur utilitaire DC-coupled, borné aux rails comme le serait le circuit.
    const auto utility = bay.read(In::umix1, ring) * umix1Level + bay.read(In::umix2, 0.0f);
    bay.write(Out::umix, std::clamp(utility, -cal::kRailVolts, cal::kRailVolts));

    // --- VCW et VCF ---
    auto foldNow = fold;
    if (bay.isPatched(In::vcwFold))
        foldNow = std::clamp(foldBase + foldAmount * bay.read(In::vcwFold, 0.0f) / 5.0f, 0.0f, 1.0f);
    if (bay.isPatched(In::cutoff))
        filter.setCutoff(cutoffBaseHz * std::exp2(std::clamp(cutoffAmount * bay.read(In::cutoff, 0.0f) *
                                                                 cal::kCutoffOctavesPerVolt,
                                                             -10.0f, 10.0f)),
                         resonance);

    auto folded = 0.0f;
    auto filtered = 0.0f;
    switch (order)
    {
    case Order::parallel:
        folded = folder.processSample(bay.read(In::vcwIn, mix), foldNow, bias, zeroColour);
        filtered = filter.processSample(bay.read(In::vcfIn, mix), filterMode);
        break;
    case Order::vcwToVcf:
        folded = folder.processSample(bay.read(In::vcwIn, mix), foldNow, bias, zeroColour);
        filtered = filter.processSample(bay.read(In::vcfIn, folded), filterMode);
        break;
    case Order::vcfToVcw:
        filtered = filter.processSample(bay.read(In::vcfIn, mix), filterMode);
        folded = folder.processSample(bay.read(In::vcwIn, filtered), foldNow, bias, zeroColour);
        break;
    }

    // --- VCA, BLEND, sortie ---
    if (bay.isPatched(In::blend) || bay.isPatched(In::vcwVcaCv) || bay.isPatched(In::vcfVcaCv))
        setPathGains(blend + bay.read(In::blend, 0.0f) / 5.0f, bay.read(In::vcwVcaCv, eg2),
                     bay.read(In::vcfVcaCv, eg2));

    // DC-block avant l'écrêtage de sécurité : la sortie reste bornée à ±1 (ADR-015).
    const auto volts = safetyClip(outputDcBlocker.processSample(folded * gainVcw + filtered * gainVcf));
    bay.write(Out::vca, volts);
    return volts * 0.2f;
}
} // namespace dedale
