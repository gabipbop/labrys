// DedaleBench : mesure le coût de processBlock (blocs de 512 échantillons à 48 kHz, suréchantillonnage ×4) et
// échoue si le plugin dépasse son budget de CPU (§15.8).
// Usage : DedaleBench [budget en % d'un cœur = 5]

#include "Scenarios.h"

#include <chrono>
#include <iostream>

namespace
{
// Pire cas réaliste : toutes les sources, FM, résonance, et quatre modulations à l'audio-rate par câble.
void heavyPatch(DedaleProcessor& p)
{
    render::internalClock(p);
    for (const auto* id : {"mix_vco", "mix_ring", "mix_mvco", "mix_noise"})
        render::set(p, id, 0.8f);
    render::set(p, "mvco_freq", 440.0f);
    render::set(p, "mvco_fm_amt", 0.7f);
    render::set(p, "vcw_fold", 0.9f);
    render::set(p, "vcf_res", 0.9f);
    render::set(p, "eg2_decay", 5.0f);
    render::set(p, "seq1_corrupt", 0.6f);
    p.connectCable(dedale::Out::noise, dedale::In::cutoff);
    p.connectCable(dedale::Out::mvco, dedale::In::vcoPitch);
    p.connectCable(dedale::Out::mixer, dedale::In::vcwFold);
    p.connectCable(dedale::Out::vca, dedale::In::blend);
    p.connectCable(dedale::Out::mvco, dedale::In::bitFlip1);
}

// Pourcentage d'un cœur utilisé pour rendre en temps réel : meilleur de trois passes, pour écarter le bruit.
double cpuPercent(const std::function<void(DedaleProcessor&)>& setup)
{
    constexpr double seconds = 20.0;
    auto best = 1.0e9;
    for (int pass = 0; pass < 3; ++pass)
    {
        DedaleProcessor processor;
        processor.seedSequencer(render::kDefaultSeed);
        setup(processor);
        juce::AudioBuffer<float> block(2, render::kBlockSize);
        juce::MidiBuffer midi;
        processor.setPlayConfigDetails(0, 2, render::kSampleRate, render::kBlockSize);
        processor.prepareToPlay(render::kSampleRate, render::kBlockSize);

        const auto blocks = static_cast<int>(seconds * render::kSampleRate / render::kBlockSize);
        const auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < blocks; ++i)
            processor.processBlock(block, midi);
        const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        best = std::min(best, 100.0 * elapsed / seconds);
    }
    return best;
}
} // namespace

int main(int argc, char* argv[])
{
    const juce::ScopedJuceInitialiser_GUI juceInitialiser;
    const auto budget = argc > 1 ? juce::String(argv[1]).getDoubleValue() : 5.0;

    const auto init = cpuPercent([](DedaleProcessor& p) { render::internalClock(p); });
    const auto heavy = cpuPercent(heavyPatch);
    std::cout << "Patch init  : " << juce::String(init, 2) << " % d'un coeur\n"
              << "Patch lourd : " << juce::String(heavy, 2) << " % d'un coeur\n"
              << "Budget      : " << juce::String(budget, 2) << " %\n";
    if (init > budget || heavy > budget)
    {
        std::cerr << "Budget de CPU depasse.\n";
        return 1;
    }
    return 0;
}
