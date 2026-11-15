#pragma once

// Scénarios de rendu hors temps réel, partagés par DedaleRender (WAV d'écoute) et par les golden tests.

#include "../Source/PluginProcessor.h"

#include <cmath>
#include <functional>
#include <vector>

namespace render
{
inline constexpr double kSampleRate = 48000.0;
inline constexpr int kBlockSize = 512;
inline constexpr std::uint32_t kDefaultSeed = 7;
inline constexpr double kGoldenSeconds = 4.0; // les golden tests ne rendent que le début de chaque scénario

// Transport hôte simulé : lecture continue à tempo fixe depuis la position 0.
class FixedTempoPlayHead final : public juce::AudioPlayHead
{
public:
    explicit FixedTempoPlayHead(double bpmToUse) : bpm(bpmToUse) {}

    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        info.setIsPlaying(true);
        info.setBpm(bpm);
        info.setPpqPosition(static_cast<double>(samples) / kSampleRate * bpm / 60.0);
        info.setTimeInSamples(samples);
        return info;
    }

    void advance(int numSamples) { samples += numSamples; }

private:
    double bpm;
    juce::int64 samples = 0;
};

struct Scenario
{
    const char* name;
    double seconds;
    double hostBpm; // 0 = horloge interne
    std::function<void(DedaleProcessor&)> setup;
    std::function<void(DedaleProcessor&, double)> atBlockStart = {}; // reçoit le temps en secondes
    bool noteOnAtStart = false; // séquenceur à l'arrêt, une seule note tenue par le decay
};

inline void set(DedaleProcessor& processor, const char* id, float value)
{
    auto* parameter = processor.apvts.getParameter(id);
    jassert(parameter != nullptr);
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

inline void muteLane(DedaleProcessor& processor, int lane)
{
    for (int step = 0; step < dedale::kMaxSteps; ++step)
        processor.sequencer().setStep(lane, step, false, 0.0f);
}

// 4 pas par seconde partout : croches à 120 BPM (hôte, division 2) ou 240 pas/min (interne).
inline void hostEighths(DedaleProcessor& processor)
{
    set(processor, "run", 1.0f);
    set(processor, "sync_mode", 1.0f);
    set(processor, "clock_div", 2.0f);
}

inline void internalClock(DedaleProcessor& processor)
{
    set(processor, "run", 1.0f);
    set(processor, "sync_mode", 0.0f);
    set(processor, "tempo", 240.0f);
}

// Hauteur fixe et un seul chemin, pour n'entendre que le réglage balayé.
inline void fixedPitch(DedaleProcessor& processor, float blend)
{
    internalClock(processor);
    set(processor, "vco_seq1_amt", 0.0f);
    set(processor, "vcw_eg1_amt", 0.0f);
    set(processor, "vcf_eg1_amt", 0.0f);
    set(processor, "eg2_decay", 0.8f);
    set(processor, "blend", blend);
}

inline std::vector<Scenario> scenarios()
{
    const auto corrupt = [](float amount)
    {
        return [amount](DedaleProcessor& p)
        {
            hostEighths(p);
            muteLane(p, 1);
            set(p, "seq1_corrupt", amount);
        };
    };
    // Balayage linéaire d'un paramètre sur toute la durée du rendu.
    const auto sweep = [](const char* id, float from, float to, double seconds)
    {
        return [=](DedaleProcessor& p, double time)
        { set(p, id, from + (to - from) * static_cast<float>(std::min(1.0, time / seconds))); };
    };

    return {
        // --- Séquenceur (phase 2) ---
        {"seq_corrupt_0", 16.0, 120.0, corrupt(0.0f)},
        {"seq_corrupt_40", 24.0, 120.0, corrupt(0.4f)},
        {"seq_corrupt_75", 24.0, 120.0, corrupt(0.75f)},
        {"seq_polymeter_8_5", 20.0, 0.0,
         [](DedaleProcessor& p)
         {
             internalClock(p);
             for (int i = 0; i < 3; ++i)
                 p.sequencer().pressLength(1); // SEQ2 sur 5 pas
             set(p, "eg_trig_mix", 0.5f);      // les deux flux de triggers à égalité
             set(p, "mvco_freq", 220.0f);      // SEQ2 joue le MOD VCO, une octave au-dessus
             set(p, "mvco_seq2_amt", 1.0f);
             set(p, "mix_mvco", 0.5f);
         }},
        {"seq_chain_16", 16.0, 0.0,
         [](DedaleProcessor& p)
         {
             internalClock(p);
             set(p, "chain", 1.0f);
         }},
        {"seq_quant_modes", 30.0, 0.0,
         [](DedaleProcessor& p)
         {
             internalClock(p);
             muteLane(p, 1);
             set(p, "seq1_range", 0.5f);
         },
         [](DedaleProcessor& p, double time)
         {
             // Toutes les 6 s (3 boucles de 8 pas) : non quantifié, chromatique, majeur, pentatonique, quads.
             constexpr std::array<float, 5> modes{1.0f, 2.0f, 3.0f, 4.0f, 16.0f};
             set(p, "quant_mode", modes[std::min<std::size_t>(static_cast<std::size_t>(time / 6.0), 4)]);
         }},

        // --- Voix (phase 3) ---
        {"init_patch", 10.0, 0.0, [](DedaleProcessor& p) { internalClock(p); }},
        {"tzfm_sweep", 12.0, 0.0,
         [](DedaleProcessor& p)
         {
             fixedPitch(p, 0.0f);
             set(p, "vcw_fold", 0.0f);
             set(p, "mvco_freq", 110.0f); // rapport 1:1 avec le VCO
         },
         sweep("mvco_fm_amt", 0.0f, 1.0f, 12.0)},
        {"fold_sweep", 12.0, 0.0, [](DedaleProcessor& p) { fixedPitch(p, 0.0f); },
         sweep("vcw_fold", 0.0f, 1.0f, 12.0)},
        {"fold_bias_sweep", 12.0, 0.0,
         [](DedaleProcessor& p)
         {
             fixedPitch(p, 0.0f);
             set(p, "vcw_fold", 0.6f);
         },
         sweep("vcw_bias", -1.0f, 1.0f, 12.0)},
        {"vcf_res_sweep", 12.0, 0.0,
         [](DedaleProcessor& p)
         {
             fixedPitch(p, 1.0f);
             set(p, "mix_noise", 0.4f);
             set(p, "vcf_cutoff", 400.0f);
             set(p, "vcf_eg1_amt", 0.5f); // l'enveloppe ouvre le filtre à chaque note
         },
         sweep("vcf_res", 0.0f, 1.0f, 12.0)},
        {"mixer_drive", 12.0, 0.0,
         [](DedaleProcessor& p)
         {
             fixedPitch(p, 1.0f);
             set(p, "vcf_cutoff", 1500.0f);
         },
         sweep("mix_vco", 0.5f, 1.0f, 12.0)},
        {"order_compare", 12.0, 0.0,
         [](DedaleProcessor& p)
         {
             internalClock(p);
             set(p, "vcw_fold", 0.7f);
             set(p, "vcf_cutoff", 600.0f);
             set(p, "vcf_res", 0.5f);
         },
         [](DedaleProcessor& p, double time)
         {
             set(p, "order", static_cast<float>(std::min(2, static_cast<int>(time / 4.0)))); // 4 s par mode
         }},
        // --- Patch bay (phase 4) ---
        {"patch_bitflip_lfo", 20.0, 0.0,
         [](DedaleProcessor& p)
         {
             internalClock(p);
             set(p, "mvco_freq", 0.7f); // M VCO en LFO lent : le rythme de SEQ1 se réécrit en continu
             p.connectCable(dedale::Out::mvco, dedale::In::bitFlip1);
         }},
        {"patch_clock2_polymeter", 20.0, 0.0,
         [](DedaleProcessor& p)
         {
             internalClock(p);
             set(p, "mvco_freq", 3.0f); // SEQ2 à 3 pas par seconde contre 4 pour SEQ1
             set(p, "eg_trig_mix", 0.5f);
             set(p, "vcf_seq2_amt", 0.5f); // SEQ2 s'entend aussi sur le filtre
             p.connectCable(dedale::Out::mvco, dedale::In::clock2);
         }},
        {"patch_fm_1voct", 12.0, 0.0,
         [](DedaleProcessor& p)
         {
             fixedPitch(p, 0.0f);
             set(p, "vcw_fold", 0.0f);
             set(p, "mvco_freq", 165.0f);
             // U MIX sert d'atténuateur : M VCO → U MIX 1 (LVL) → VCO 1V/OCT, FM exponentielle de plus en
             // plus profonde.
             p.connectCable(dedale::Out::mvco, dedale::In::umix1);
             p.connectCable(dedale::Out::umix, dedale::In::vcoPitch);
         },
         sweep("umix1_lvl", 0.0f, 0.3f, 12.0)},
        {"patch_noise_cutoff", 12.0, 0.0,
         [](DedaleProcessor& p)
         {
             fixedPitch(p, 1.0f);
             set(p, "vcf_cutoff", 500.0f);
             p.connectCable(dedale::Out::noise, dedale::In::cutoff); // remplace EG1 sur le cutoff
         },
         sweep("vcf_eg1_amt", 0.0f, 0.5f, 12.0)},
        {"patch_eg1_vcw_vca", 12.0, 0.0,
         [](DedaleProcessor& p)
         {
             internalClock(p);
             set(p, "eg1_decay", 0.06f); // chemin VCW : claquement court (EG1)
             set(p, "eg2_decay", 1.5f);  // chemin VCF : longue queue (EG2)
             set(p, "vcw_fold", 0.7f);
             p.connectCable(dedale::Out::eg1, dedale::In::vcwVcaCv);
         }},
        {"patch_feedback", 12.0, 0.0,
         [](DedaleProcessor& p)
         {
             internalClock(p);
             set(p, "vcf_res", 0.7f);
             set(p, "vcf_cutoff", 1200.0f);
             p.connectCable(dedale::Out::vca, dedale::In::vcfIn); // la sortie revient dans le filtre
         }},

        // Mesure d'aliasing : note tenue à 2,5 kHz, FOLD à fond, chemin VCW seul.
        {"alias_fold_high",
         3.0,
         0.0,
         [](DedaleProcessor& p)
         {
             set(p, "vco_freq", 2500.0f);
             set(p, "vco_seq1_amt", 0.0f);
             set(p, "vcw_fold", 1.0f);
             set(p, "vcw_eg1_amt", 0.0f);
             set(p, "blend", 0.0f);
             set(p, "eg2_decay", 10.0f);
         },
         {},
         true},
    };
}

// Rend un scénario (sortie mono, les deux canaux du plugin sont identiques). maxSeconds > 0 tronque le rendu.
inline std::vector<float> renderScenario(const Scenario& scenario, std::uint32_t seed,
                                         double maxSeconds = 0.0)
{
    DedaleProcessor processor;
    processor.seedSequencer(seed);
    scenario.setup(processor);

    FixedTempoPlayHead playHead(scenario.hostBpm);
    if (scenario.hostBpm > 0.0)
        processor.setPlayHead(&playHead);

    const auto seconds = maxSeconds > 0.0 ? std::min(maxSeconds, scenario.seconds) : scenario.seconds;
    const auto totalSamples = static_cast<int>(seconds * kSampleRate);
    std::vector<float> output(static_cast<std::size_t>(totalSamples));
    juce::AudioBuffer<float> block(2, kBlockSize);
    juce::MidiBuffer midi;

    processor.setPlayConfigDetails(0, 2, kSampleRate, kBlockSize);
    processor.prepareToPlay(kSampleRate, kBlockSize);
    for (int start = 0; start < totalSamples; start += kBlockSize)
    {
        if (scenario.atBlockStart)
            scenario.atBlockStart(processor, start / kSampleRate);
        midi.clear();
        if (scenario.noteOnAtStart && start == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);

        const auto length = std::min(kBlockSize, totalSamples - start);
        block.setSize(2, length, false, false, true);
        processor.processBlock(block, midi);
        playHead.advance(length);
        std::copy_n(block.getReadPointer(0), length, output.begin() + start);
    }
    processor.releaseResources();
    return output;
}

// Métriques figées par les golden tests (§15.6).
struct Metrics
{
    double peakDb = 0.0;
    double rmsDb = 0.0;
    double dc = 0.0;
    double crossingsPerSecond = 0.0; // passages à zéro : indicateur grossier de brillance
    bool finite = true;
};

inline Metrics measure(const std::vector<float>& x)
{
    Metrics m;
    auto peak = 0.0;
    auto sumSquares = 0.0;
    auto sum = 0.0;
    auto crossings = 0;
    for (std::size_t i = 0; i < x.size(); ++i)
    {
        const auto v = static_cast<double>(x[i]);
        m.finite = m.finite && std::isfinite(v);
        peak = std::max(peak, std::abs(v));
        sumSquares += v * v;
        sum += v;
        if (i > 0 && (x[i - 1] < 0.0f) != (x[i] < 0.0f))
            ++crossings;
    }
    const auto n = static_cast<double>(std::max<std::size_t>(x.size(), 1));
    m.peakDb = 20.0 * std::log10(std::max(peak, 1e-9));
    m.rmsDb = 20.0 * std::log10(std::max(std::sqrt(sumSquares / n), 1e-9));
    m.dc = sum / n;
    m.crossingsPerSecond = crossings * kSampleRate / n;
    return m;
}
} // namespace render
