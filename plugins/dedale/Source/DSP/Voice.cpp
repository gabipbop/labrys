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
}

void Voice::setControls(const VoiceParams& p, const VoiceCv& cv) noexcept
{
    const auto egPitch = cv.eg1 * cal::kEgPitchOctavesPerVolt;
    vcoHz =
        std::clamp(p.vcoFreq * std::exp2(p.vcoEg1Amt * egPitch + sequencerAmountGain(p.vcoSeq1Amt) * cv.seq1),
                   1.0f, 20000.0f);
    modVcoHz = std::clamp(
        p.mvcoFreq * std::exp2(p.mvcoEg1Amt * egPitch + sequencerAmountGain(p.mvcoSeq2Amt) * cv.seq2), 0.01f,
        20000.0f);
    fmIndex = p.fmAmt * cal::kTzfmMaxIndex;

    mixer.setLevels(p.mixVco, p.mixRing, p.mixMvco, p.mixNoise);
    noise.setTone(p.noiseTone);

    // FOLD : knob + EG1 (via EG1/CV AMT) + SEQ1, chacun ramené à la course du knob.
    fold = std::clamp(p.fold + p.vcwEg1Amt * cv.eg1 / cal::kEgMaxVolts + p.vcwSeq1Amt * cv.seq1 / 5.0f, 0.0f,
                      1.0f);
    bias = p.bias;

    const auto cutoffVolts = p.vcfEg1Amt * cv.eg1 + p.vcfSeq2Amt * cv.seq2;
    filter.setCutoff(p.cutoff *
                         std::exp2(std::clamp(cutoffVolts * cal::kCutoffOctavesPerVolt, -10.0f, 10.0f)),
                     p.resonance);
    filterMode = p.filterMode;
    order = p.order;

    // Les deux VCA suivent EG2, puis BLEND équipuissance et VOLUME.
    const auto vca = vcaGain(cv.eg2) * p.volume * cal::kOutputHeadroom;
    gainVcw = vca * std::cos(0.5f * kPi * p.blend);
    gainVcf = vca * std::sin(0.5f * kPi * p.blend);
}

float Voice::processSample() noexcept
{
    const auto modulator = modVco.processSample(modVcoHz, false);
    const auto carrier = vco.processSample(vcoHz, modulator / 5.0f, fmIndex);
    const auto mix =
        mixer.processSample(carrier, carrier * modulator / 5.0f, modulator, noise.processSample());

    auto folded = 0.0f;
    auto filtered = 0.0f;
    switch (order)
    {
    case Order::parallel:
        folded = folder.processSample(mix, fold, bias);
        filtered = filter.processSample(mix, filterMode);
        break;
    case Order::vcwToVcf:
        folded = folder.processSample(mix, fold, bias);
        filtered = filter.processSample(folded, filterMode);
        break;
    case Order::vcfToVcw:
        filtered = filter.processSample(mix, filterMode);
        folded = folder.processSample(filtered, fold, bias);
        break;
    }

    // DC-block avant l'écrêtage de sécurité : la sortie reste bornée à ±1 (ADR-015).
    const auto volts = outputDcBlocker.processSample(folded * gainVcw + filtered * gainVcf);
    return safetyClip(volts) * 0.2f;
}
} // namespace dedale
